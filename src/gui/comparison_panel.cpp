#include "comparison_panel.hpp"
#include "log_model.hpp"

#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>

namespace loginsight::gui {
namespace {
const QStringList columns{"", "Baseline", "Current", "Change"};

QString delta(std::size_t before, std::size_t after) {
  return (after > before ? "+" : "") +
         QString::number(static_cast<qlonglong>(after) - static_cast<qlonglong>(before));
}

void configure(QTableView& view) {
  view.setEditTriggers(QAbstractItemView::NoEditTriggers);
  view.setSelectionBehavior(QAbstractItemView::SelectRows);
  view.setAlternatingRowColors(true);
  view.verticalHeader()->hide();
  view.horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}
} // namespace

class SourceComparison final : public QAbstractTableModel {
public:
  explicit SourceComparison(QObject* parent) : QAbstractTableModel(parent) {}

  void updateCounts(const std::map<QString, std::size_t>& baseline,
                    const std::map<QString, std::size_t>& current) {
    beginResetModel();
    rows_.clear();
    auto before = baseline.begin();
    auto after = current.begin();
    while (before != baseline.end() || after != current.end()) {
      if (after == current.end() || (before != baseline.end() && before->first < after->first)) {
        rows_.push_back({before->first, before->second, 0});
        ++before;
      } else if (before == baseline.end() || after->first < before->first) {
        rows_.push_back({after->first, 0, after->second});
        ++after;
      } else {
        rows_.push_back({before->first, before->second, after->second});
        ++before;
        ++after;
      }
    }
    endResetModel();
  }

  int rowCount(const QModelIndex& parent = {}) const override {
    return parent.isValid() ? 0 : static_cast<int>(rows_.size());
  }
  int columnCount(const QModelIndex& parent = {}) const override {
    return parent.isValid() ? 0 : 4;
  }
  QVariant data(const QModelIndex& index, int role) const override {
    if (!index.isValid() || index.row() >= rowCount() || role != Qt::DisplayRole)
      return {};
    const auto& row = rows_[static_cast<std::size_t>(index.row())];
    switch (index.column()) {
    case 0:
      return row.name;
    case 1:
      return QString::number(row.before);
    case 2:
      return QString::number(row.after);
    case 3:
      return delta(row.before, row.after);
    default:
      return {};
    }
  }
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole || section < 0 || section >= 4)
      return {};
    return section == 0 ? QString{"Source"} : columns[section];
  }

private:
  struct Row {
    QString name;
    std::size_t before;
    std::size_t after;
  };
  std::vector<Row> rows_;
};

ComparisonPanel::ComparisonPanel() {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(18, 14, 18, 14);
  auto* title = new QLabel{"Compare sessions"};
  title->setStyleSheet("font-size: 25px; font-weight: 600;");
  layout->addWidget(title);
  summary_ = new QLabel;
  summary_->setTextFormat(Qt::PlainText);
  summary_->setWordWrap(true);
  layout->addWidget(summary_);
  auto* actions = new QHBoxLayout;
  pin_ = new QPushButton{"Use current session as baseline"};
  pin_->setObjectName("pinBaseline");
  clear_ = new QPushButton{"Clear baseline"};
  actions->addWidget(pin_);
  actions->addWidget(clear_);
  actions->addStretch();
  layout->addLayout(actions);
  charts_ = new QWidget;
  auto* tables = new QVBoxLayout(charts_);
  tables->setContentsMargins(0, 0, 0, 0);
  tables->addWidget(new QLabel{"By severity"});
  severities_ = new QTableWidget{7, 4};
  severities_->setObjectName("severityComparison");
  configure(*severities_);
  severities_->setHorizontalHeaderLabels(columns);
  severities_->horizontalHeaderItem(0)->setText("Severity");
  tables->addWidget(severities_, 1);
  tables->addWidget(new QLabel{"By source"});
  auto* sourceTable = new QTableView;
  sourceTable->setObjectName("sourceComparison");
  configure(*sourceTable);
  sources_ = new SourceComparison(sourceTable);
  sourceTable->setModel(sources_);
  tables->addWidget(sourceTable, 1);
  layout->addWidget(charts_, 1);

  connect(pin_, &QPushButton::clicked, this, [this] {
    baseline_ = current_;
    baselineSources_ = currentSources_;
    baselineName_ = currentName_;
    refresh();
  });
  connect(clear_, &QPushButton::clicked, this, [this] {
    baselineSources_.clear();
    baselineName_.clear();
    refresh();
  });
  refresh();
}

void ComparisonPanel::setDataset(const Dataset& dataset) {
  current_ = dataset.counts;
  currentSources_ = dataset.sourceCounts;
  const auto file = std::find_if(dataset.files.begin(), dataset.files.end(),
                                 [](const auto& result) { return result.completed(); });
  currentName_ = file == dataset.files.end()
                     ? QString{}
                     : QString::fromStdString(file->path.filename().string());
  if (dataset.importedFiles > 1)
    currentName_ += QString{" and %1 more"}.arg(dataset.importedFiles - 1);
  refresh();
}

void ComparisonPanel::refresh() {
  const bool ready = !baselineName_.isEmpty();
  pin_->setEnabled(!currentName_.isEmpty());
  clear_->setEnabled(ready);
  charts_->setVisible(ready);
  summary_->setText(
      ready ? QString{"Baseline: %1 · Current: %2"}.arg(baselineName_, currentName_)
            : QString{"Import a batch, pin it as the baseline, then import another batch."});
  if (!ready)
    return;
  const QStringList levels{"Trace", "Debug", "Info", "Warning", "Error", "Critical", "Unknown"};
  for (int row = 0; row < levels.size(); ++row) {
    const auto i = static_cast<std::size_t>(row);
    severities_->setItem(row, 0, new QTableWidgetItem{levels[row]});
    severities_->setItem(row, 1, new QTableWidgetItem{QString::number(baseline_[i])});
    severities_->setItem(row, 2, new QTableWidgetItem{QString::number(current_[i])});
    severities_->setItem(row, 3, new QTableWidgetItem{delta(baseline_[i], current_[i])});
  }
  sources_->updateCounts(baselineSources_, currentSources_);
}

} // namespace loginsight::gui
