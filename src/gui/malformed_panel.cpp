#include "malformed_panel.hpp"
#include "log_model.hpp"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSplitter>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>
#include <utility>

namespace loginsight::gui {

class MalformedModel final : public QAbstractTableModel {
public:
  explicit MalformedModel(QObject* parent) : QAbstractTableModel{parent} {}

  void reset(std::shared_ptr<const Dataset> dataset) {
    beginResetModel();
    rows_.clear();
    all_.clear();
    dataset_ = std::move(dataset);
    if (dataset_) {
      for (const auto& file : dataset_->files) {
        if (!file.completed() || file.parsed.errors.empty())
          continue;
        const auto path = QString::fromStdString(file.path.string());
        for (const auto& error : file.parsed.errors)
          all_.push_back({path, &error});
      }
    }
    endResetModel();
  }

  void filter(const QString& file, const QString& reason) {
    beginResetModel();
    rows_.clear();
    for (const auto& row : all_) {
      if ((file.isEmpty() || file == row.path) &&
          (reason.isEmpty() ||
           QString::fromStdString(row.error->message).contains(reason, Qt::CaseInsensitive)))
        rows_.push_back(&row);
    }
    endResetModel();
  }

  int rowCount(const QModelIndex& parent = {}) const override {
    return parent.isValid() ? 0 : static_cast<int>(rows_.size());
  }

  int columnCount(const QModelIndex& parent = {}) const override {
    return parent.isValid() ? 0 : 3;
  }

  QVariant data(const QModelIndex& index, int role) const override {
    if (!index.isValid() || index.row() >= rowCount() ||
        (role != Qt::DisplayRole && role != Qt::ToolTipRole))
      return {};
    const auto& row = *rows_[static_cast<std::size_t>(index.row())];
    const auto& error = *row.error;
    if (index.column() == 1)
      return QVariant::fromValue(static_cast<qulonglong>(error.lineNumber));
    const auto value = index.column() == 0 ? row.path : QString::fromStdString(error.message);
    return role == Qt::ToolTipRole ? "<qt>" + value.toHtmlEscaped() + "</qt>" : value;
  }

  QVariant headerData(int section, Qt::Orientation orientation, int role) const override {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole || section < 0 || section >= 3)
      return {};
    const QStringList labels{"File", "Line", "Reason"};
    return labels[section];
  }

  const parsers::ParseError* record(int row) const {
    return row < 0 || row >= rowCount() ? nullptr : rows_[static_cast<std::size_t>(row)]->error;
  }

private:
  struct Row {
    QString path;
    const parsers::ParseError* error;
  };
  std::shared_ptr<const Dataset> dataset_;
  std::vector<Row> all_;
  std::vector<const Row*> rows_;
};

MalformedPanel::MalformedPanel(QWidget* parent) : QWidget{parent} {
  auto* layout = new QVBoxLayout{this};
  layout->setContentsMargins(18, 14, 18, 14);
  auto* heading = new QLabel{"Malformed records"};
  heading->setStyleSheet("font-size: 25px; font-weight: 600;");
  layout->addWidget(heading);
  count_ = new QLabel{"Import logs to inspect rejected records."};
  layout->addWidget(count_);
  auto* filters = new QHBoxLayout;
  files_ = new QComboBox;
  files_->setObjectName("malformedFile");
  files_->setAccessibleName("Filter malformed records by file");
  files_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
  files_->setMinimumContentsLength(16);
  files_->addItem("All files");
  reason_ = new QLineEdit;
  reason_->setObjectName("malformedReason");
  reason_->setAccessibleName("Filter malformed records by reason");
  reason_->setPlaceholderText("Filter reasons…");
  reason_->setClearButtonEnabled(true);
  filters->addWidget(files_);
  filters->addWidget(reason_, 1);
  layout->addLayout(filters);

  auto* split = new QSplitter{Qt::Vertical};
  model_ = new MalformedModel{this};
  table_ = new QTableView;
  table_->setObjectName("malformedTable");
  table_->setAccessibleName("Malformed records");
  table_->setModel(model_);
  table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  table_->setSelectionMode(QAbstractItemView::SingleSelection);
  table_->setAlternatingRowColors(true);
  table_->setWordWrap(false);
  table_->verticalHeader()->hide();
  table_->verticalHeader()->setDefaultSectionSize(28);
  table_->setColumnWidth(0, 245);
  table_->setColumnWidth(1, 65);
  table_->horizontalHeader()->setStretchLastSection(true);
  split->addWidget(table_);

  auto* detail = new QWidget;
  auto* detailLayout = new QVBoxLayout{detail};
  detailLayout->setContentsMargins(0, 5, 0, 0);
  auto* header = new QHBoxLayout;
  header->addWidget(new QLabel{"Original record"});
  header->addStretch();
  copy_ = new QPushButton{"Copy original"};
  copy_->setObjectName("copyMalformed");
  copy_->setEnabled(false);
  header->addWidget(copy_);
  detailLayout->addLayout(header);
  raw_ = new QPlainTextEdit;
  raw_->setObjectName("malformedRaw");
  raw_->setAccessibleName("Original rejected record");
  raw_->setStyleSheet("font-family: monospace; background: #191b24; border: 1px solid #343746;");
  raw_->setReadOnly(true);
  raw_->setPlaceholderText("Select a rejected row to inspect its original text.");
  detailLayout->addWidget(raw_);
  split->addWidget(detail);
  split->setStretchFactor(0, 2);
  split->setStretchFactor(1, 1);
  layout->addWidget(split, 1);

  auto* debounce = new QTimer{this};
  debounce->setSingleShot(true);
  debounce->setInterval(160);
  connect(reason_, &QLineEdit::textChanged, debounce, [debounce] { debounce->start(); });
  connect(debounce, &QTimer::timeout, this, &MalformedPanel::applyFilters);
  connect(files_, &QComboBox::currentIndexChanged, this, [this] { applyFilters(); });
  connect(table_->selectionModel(), &QItemSelectionModel::currentRowChanged, this,
          [this] { showRecord(); });
  connect(copy_, &QPushButton::clicked, this, [this] {
    if (const auto* record = model_->record(table_->currentIndex().row()))
      QApplication::clipboard()->setText(QString::fromStdString(record->rawContent));
  });
}

void MalformedPanel::setDataset(std::shared_ptr<const Dataset> dataset) {
  const QSignalBlocker blockFiles{files_};
  const QSignalBlocker blockReason{reason_};
  files_->clear();
  files_->addItem("All files");
  for (const auto& file : dataset->files) {
    if (file.completed() && !file.parsed.errors.empty()) {
      const auto path = QString::fromStdString(file.path.string());
      files_->addItem(path, path);
    }
  }
  reason_->clear();
  model_->reset(std::move(dataset));
  applyFilters();
}

void MalformedPanel::focusRecords() { table_->setFocus(); }

void MalformedPanel::applyFilters() {
  model_->filter(files_->currentData().toString(), reason_->text().trimmed());
  const auto shown = model_->rowCount();
  count_->setText(shown ? QString{"%1 matching records"}.arg(shown)
                        : QString{"No malformed records match."});
  table_->setCurrentIndex(model_->index(0, 0));
  showRecord();
}

void MalformedPanel::showRecord() {
  const auto* record = model_->record(table_->currentIndex().row());
  raw_->setPlainText(record ? QString::fromStdString(record->rawContent) : QString{});
  copy_->setEnabled(record && !record->rawContent.empty());
}
} // namespace loginsight::gui
