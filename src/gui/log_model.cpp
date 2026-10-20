#include "log_model.hpp"
#include <QFont>
#include <algorithm>
#include <climits>
#include <set>
#include <stdexcept>

namespace loginsight::gui {
std::shared_ptr<const Dataset> load_files(const QStringList& paths) {
  auto dataset = std::make_shared<Dataset>();
  const app::AnalysisService service;
  dataset->files.reserve(static_cast<std::size_t>(paths.size()));
  std::set<QString> sources;
  for (const auto& path : paths) {
    auto file = service.analyze(std::filesystem::path{path.toStdString()});
    if (file.completed()) {
      ++dataset->importedFiles;
      dataset->malformed += file.summary.malformedLineCount;
      const auto& s = file.summary;
      const std::array counts{s.traceCount, s.debugCount,    s.infoCount,   s.warningCount,
                              s.errorCount, s.criticalCount, s.unknownCount};
      for (std::size_t i = 0; i < counts.size(); ++i)
        dataset->counts[i] += counts[i];
    }
    dataset->files.push_back(std::move(file));
  }
  // Pointers are made after all files are in their final storage.
  for (const auto& file : dataset->files) {
    if (!file.completed()) {
      dataset->diagnostics +=
          QString::fromStdString(file.path.string() + ": " + file.diagnostic) + '\n';
      continue;
    }
    dataset->diagnostics +=
        QString::fromStdString(file.path.string()) + QString(" — %1 entries, %2 malformed\n")
                                                         .arg(file.summary.validEntryCount)
                                                         .arg(file.summary.malformedLineCount);
    for (const auto& entry : file.parsed.entries) {
      dataset->entries.push_back(&entry);
      sources.insert(QString::fromStdString(entry.source.name));
    }
    // Keep the overview lightweight; the complete errors remain in JSON export.
    const auto shown = std::min(file.parsed.errors.size(), std::size_t{20});
    for (std::size_t i = 0; i < shown; ++i) {
      const auto& error = file.parsed.errors[i];
      dataset->diagnostics += QString("  Line %1: %2\n")
                                  .arg(error.lineNumber)
                                  .arg(QString::fromStdString(error.message));
    }
    if (shown < file.parsed.errors.size())
      dataset->diagnostics += "  More malformed records are included in JSON export.\n";
  }
  if (dataset->entries.size() > static_cast<std::size_t>(INT_MAX)) {
    throw std::runtime_error("too many entries for the desktop table");
  }
  for (const auto& source : sources)
    dataset->sources.push_back(source);
  return dataset;
}

QColor level_color(core::LogLevel level) {
  switch (level) {
  case core::LogLevel::critical:
    return QColor{"#f078a0"};
  case core::LogLevel::error:
    return QColor{"#ff9393"};
  case core::LogLevel::warning:
    return QColor{"#eac278"};
  case core::LogLevel::info:
    return QColor{"#8bd9bc"};
  case core::LogLevel::debug:
    return QColor{"#a6b9ef"};
  case core::LogLevel::trace:
    return QColor{"#a8a1bb"};
  default:
    return QColor{"#b7bac9"};
  }
}
int LogModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() || !dataset_ ? 0 : static_cast<int>(dataset_->entries.size());
}
int LogModel::columnCount(const QModelIndex& parent) const { return parent.isValid() ? 0 : 4; }
const core::LogEntry* LogModel::entry(int row) const {
  return row < 0 || row >= rowCount() ? nullptr : dataset_->entries[static_cast<std::size_t>(row)];
}
QVariant LogModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.column() < 0 || index.column() >= 4)
    return {};
  const auto* item = entry(index.row());
  if (!item)
    return {};
  if (role == Qt::ForegroundRole && index.column() == 1)
    return level_color(item->level);
  if (role == Qt::UserRole) {
    if (index.column() == 0)
      return QVariant::fromValue<qlonglong>(item->timestamp.time_since_epoch().count());
    if (index.column() == 1)
      return static_cast<int>(item->level);
    return data(index, Qt::DisplayRole);
  }
  if (role != Qt::DisplayRole && role != Qt::ToolTipRole)
    return {};
  switch (index.column()) {
  case 0:
    return QString::fromStdString(app::format_timestamp(item->timestamp));
  case 1:
    return QString::fromUtf8(core::to_string(item->level).data(),
                             static_cast<qsizetype>(core::to_string(item->level).size()))
        .toUpper();
  case 2:
    return QString::fromStdString(item->source.name);
  case 3:
    return QString::fromStdString(item->message);
  default:
    return {};
  }
}
QVariant LogModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (orientation != Qt::Horizontal || role != Qt::DisplayRole || section < 0 || section >= 4)
    return {};
  const QStringList labels{"Timestamp · UTC", "Severity", "Source", "Message"};
  return labels[section];
}
void LogModel::setDataset(std::shared_ptr<const Dataset> dataset) {
  beginResetModel();
  dataset_ = std::move(dataset);
  endResetModel();
}
LogFilter::LogFilter(QObject* parent) : QSortFilterProxyModel(parent) {
  setSortRole(Qt::UserRole);
  setSortCaseSensitivity(Qt::CaseInsensitive);
}
void LogFilter::setSearch(QString value) {
  search_ = std::move(value);
  invalidate();
}
void LogFilter::setSeverity(int value) {
  level_ = value;
  invalidate();
}
void LogFilter::setSource(QString value) {
  source_ = std::move(value);
  invalidate();
}
bool LogFilter::filterAcceptsRow(int row, const QModelIndex& parent) const {
  const auto* model = static_cast<const LogModel*>(sourceModel());
  const auto* item = model->entry(row);
  if (!item || parent.isValid())
    return false;
  return (level_ < 0 || static_cast<int>(item->level) == level_) &&
         (source_.isEmpty() || QString::fromStdString(item->source.name) == source_) &&
         (search_.isEmpty() ||
          QString::fromStdString(item->message).contains(search_, Qt::CaseInsensitive));
}
} // namespace loginsight::gui
