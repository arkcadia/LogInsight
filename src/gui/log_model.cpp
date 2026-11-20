#include "log_model.hpp"
#include <QFont>
#include <algorithm>
#include <climits>
#include <map>
#include <stdexcept>

namespace loginsight::gui {
std::shared_ptr<const Dataset> load_files(const QStringList& paths, std::stop_token stop,
                                          const std::function<void(int)>& progress,
                                          parsers::JsonParserConfig config,
                                          const std::vector<NamedRule>& activeRules,
                                          parsers::CsvColumns csv) {
  parsers::ImportBudget budget{.stop = stop, .progress = {}};
  if (paths.size() > 128)
    throw std::runtime_error("session exceeds the 128 file import limit");
  std::size_t expectedBytes{};
  for (const auto& path : paths) {
    budget.checkCancelled();
    std::error_code error;
    const std::filesystem::path file{path.toStdString()};
    if (!std::filesystem::is_regular_file(file, error))
      continue;
    const auto size = std::filesystem::file_size(file, error);
    if (!error) {
      if (size > parsers::ImportBudget::maxBytes - expectedBytes)
        throw std::runtime_error("session exceeds the 32 MiB import limit");
      expectedBytes += static_cast<std::size_t>(size);
    }
  }
  if (progress && expectedBytes > 0) {
    budget.progress = [&](std::size_t bytes) {
      progress(static_cast<int>(std::min(std::size_t{99}, bytes * 100U / expectedBytes)));
    };
  }
  auto dataset = std::make_shared<Dataset>();
  const app::AnalysisService service{std::move(config), std::move(csv)};
  dataset->files.reserve(static_cast<std::size_t>(paths.size()));
  std::map<QString, std::size_t> sources;
  for (const auto& path : paths) {
    budget.checkCancelled();
    auto file = service.analyze(std::filesystem::path{path.toStdString()}, &budget);
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
      budget.checkCancelled();
      dataset->entries.push_back(&entry);
      ++sources[QString::fromStdString(entry.source.name)];
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
  std::vector<std::pair<QString, std::size_t>> top;
  top.reserve(sources.size());
  for (const auto& [source, count] : sources) {
    dataset->sources.push_back(source);
    top.emplace_back(source, count);
  }
  const auto end = top.begin() + static_cast<std::ptrdiff_t>(std::min(top.size(), std::size_t{5}));
  std::partial_sort(top.begin(), end, top.end(), [](const auto& a, const auto& b) {
    return a.second != b.second ? a.second > b.second : a.first < b.first;
  });
  dataset->topSources.assign(top.begin(), end);
  dataset->bursts = evaluate_rules(dataset->entries, activeRules);
  budget.checkCancelled();
  if (progress)
    progress(100);
  return dataset;
}

std::vector<NamedBurst> evaluate_rules(std::span<const core::LogEntry* const> entries,
                                       const std::vector<NamedRule>& activeRules) {
  std::vector<NamedBurst> result;
  for (const auto& named : activeRules) {
    auto matches = rules::find_error_bursts(entries, named.rule);
    for (auto& match : matches)
      result.push_back({named.name, std::move(match)});
  }
  return result;
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
void LogFilter::setBurst(std::span<const core::LogEntry* const> burst, const QString& description) {
  burst_ = {burst.begin(), burst.end()};
  burstDescription_ = description;
  invalidate();
}
void LogFilter::setCorrelation(std::string key, std::string value) {
  correlationKey_ = std::move(key);
  correlationId_ = std::move(value);
  invalidate();
}
void LogFilter::setBookmarks(const std::set<const core::LogEntry*>& entries, bool enabled) {
  if (enabled)
    bookmarks_ = entries;
  else
    bookmarks_.reset();
  invalidate();
}
core::LogAttributes LogFilter::activeFilters() const {
  core::LogAttributes filters;
  if (!search_.isEmpty())
    filters.emplace("message_contains", search_.toStdString());
  if (!source_.isEmpty())
    filters.emplace("source", source_.toStdString());
  if (level_ == errorAndCritical)
    filters.emplace("severity", "error_or_critical");
  else if (level_ >= 0)
    filters.emplace("severity", core::to_string(static_cast<core::LogLevel>(level_)));
  if (from_)
    filters.emplace("from_utc", app::format_timestamp(*from_));
  if (through_)
    filters.emplace("through_utc", app::format_timestamp(*through_));
  if (bookmarks_)
    filters.emplace("bookmarked_only", "true");
  if (!burst_.empty())
    filters.emplace("alert", burstDescription_.toStdString());
  if (!correlationKey_.empty())
    filters.emplace(correlationKey_, correlationId_);
  return filters;
}
void LogFilter::setTimeRange(std::optional<core::LogTimestamp> from,
                             std::optional<core::LogTimestamp> through) {
  from_ = from;
  through_ = through;
  invalidate();
}
bool LogFilter::filterAcceptsRow(int row, const QModelIndex& parent) const {
  const auto* model = static_cast<const LogModel*>(sourceModel());
  const auto* item = model->entry(row);
  if (!item || parent.isValid())
    return false;
  if (!correlationKey_.empty()) {
    const auto value = item->attributes.find(correlationKey_);
    if (value == item->attributes.end() || value->second != correlationId_)
      return false;
  }
  return (!bookmarks_ || bookmarks_->contains(item)) && (burst_.empty() || burst_.contains(item)) &&
         (!from_ || item->timestamp >= *from_) && (!through_ || item->timestamp <= *through_) &&
         (level_ == errorAndCritical
              ? item->level == core::LogLevel::error || item->level == core::LogLevel::critical
              : level_ < 0 || static_cast<int>(item->level) == level_) &&
         (source_.isEmpty() || QString::fromStdString(item->source.name) == source_) &&
         (search_.isEmpty() ||
          QString::fromStdString(item->message).contains(search_, Qt::CaseInsensitive));
}
} // namespace loginsight::gui
