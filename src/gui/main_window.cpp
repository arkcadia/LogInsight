#include "main_window.hpp"
#include "loginsight/parsers/timestamp_parser.hpp"
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMimeData>
#include <QProgressBar>
#include <QPromise>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTableView>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>
#include <exception>

namespace loginsight::gui {
namespace {
QLabel* label(const QString& text, const char* style = "") {
  auto* value = new QLabel{text};
  value->setStyleSheet(style);
  return value;
}
QWidget* panel() {
  auto* widget = new QWidget;
  widget->setObjectName("panel");
  return widget;
}
QTextEdit* textPanel(const QString& placeholder) {
  auto* edit = new QTextEdit;
  edit->setReadOnly(true);
  edit->setAcceptRichText(false);
  edit->setPlaceholderText(placeholder);
  return edit;
}
} // namespace
MainWindow::MainWindow() {
  setWindowTitle("LogInsight — Log analysis");
  resize(1220, 830);
  setMinimumSize(840, 620);
  setAcceptDrops(true);
  setStyleSheet(R"(
    QMainWindow, QWidget { background: #14151c; color: #e6e6ef; font-family: 'DejaVu Sans'; font-size: 13px; }
    QWidget#panel { background: #1d1f29; border: 1px solid #30323f; border-radius: 8px; }
    QLabel { background: transparent; border: none; }
    QPushButton { background: #272936; border: 1px solid #3a3d4e; border-radius: 5px; padding: 9px 16px; }
    QPushButton:hover { background: #343044; border-color: #a38ae4; }
    QPushButton:disabled { color: #686b7c; border-color: #292b35; }
    QPushButton#primary { background: #7960b5; border-color: #a18bd1; color: white; }
    QListWidget { background: #181a22; border: none; padding: 12px; outline: none; }
    QListWidget::item { padding: 13px; margin: 3px 0; border-radius: 5px; }
    QListWidget::item:selected { background: #30283f; color: #c5acef; }
    QLineEdit, QComboBox, QTextEdit { background: #191b24; border: 1px solid #343746; border-radius: 5px; padding: 7px; }
    QLineEdit:focus, QComboBox:focus { border-color: #a38ae4; }
    QComboBox QAbstractItemView { background: #242633; selection-background-color: #554370; }
    QTableView { background: #191b23; alternate-background-color: #1e202b; border: 1px solid #30323f; gridline-color: #292c39; selection-background-color: #39304d; selection-color: white; }
    QHeaderView::section { background: #242633; color: #b9bbca; border: none; border-bottom: 1px solid #363849; padding: 10px; font-weight: bold; }
    QProgressBar { background: #2a2d39; border: none; border-radius: 3px; max-height: 9px; }
    QProgressBar::chunk { background: #a38ae4; border-radius: 3px; }
    QStatusBar { background: #181a22; color: #b4b7c9; border-top: 1px solid #30323f; }
    QSplitter::handle { background: #30323f; height: 4px; }
    QScrollArea { border: none; }
    QScrollBar:vertical { background: #191b24; width: 10px; margin: 0; }
    QScrollBar::handle:vertical { background: #404354; min-height: 25px; border-radius: 4px; }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
    QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
  )");
  auto* central = new QWidget;
  setCentralWidget(central);
  auto* outer = new QVBoxLayout(central);
  outer->setContentsMargins(0, 0, 0, 0);
  outer->setSpacing(0);
  auto* header = new QHBoxLayout;
  header->setContentsMargins(24, 18, 24, 18);
  header->setSpacing(16);
  auto* brand = label("LogInsight", "font-size: 24px; font-weight: 700;");
  header->addWidget(brand);
  header->addWidget(label("/   LOCAL LOG ANALYSIS", "color: #9997ac; font-size: 11px;"));
  header->addStretch();
  samples_ = new QPushButton{"Load Sample Logs"};
  samples_->setObjectName("loadSamples");
  import_ = new QPushButton{"Import Logs"};
  import_->setObjectName("primary");
  export_ = new QPushButton{"Export JSON"};
  export_->setEnabled(false);
  header->addWidget(samples_);
  header->addWidget(import_);
  mapping_ = new QPushButton{"JSON fields"};
  mapping_->setObjectName("jsonFields");
  header->addWidget(mapping_);
  connect(mapping_, &QPushButton::clicked, this, &MainWindow::configureJson);
  header->addWidget(export_);
  outer->addLayout(header);
  auto* body = new QHBoxLayout;
  body->setSpacing(0);
  auto* sidebar = new QWidget;
  sidebar->setFixedWidth(186);
  auto* side = new QVBoxLayout(sidebar);
  side->setContentsMargins(0, 0, 0, 12);
  auto* navigation = navigation_ = new QListWidget;
  navigation->addItems({"Overview", "Log Explorer"});
  navigation->setCurrentRow(0);
  navigation->setObjectName("navigation");
  side->addWidget(navigation);
  auto* sideNote = label("  PORTFOLIO MVP\n\n  Log investigation\n  Local processing",
                         "color: #8b8da2; font-size: 11px;");
  side->addWidget(sideNote);
  body->addWidget(sidebar);
  pages_ = new QStackedWidget;
  body->addWidget(pages_, 1);
  outer->addLayout(body, 1);
  auto* scroll = new QScrollArea;
  scroll->setWidgetResizable(true);
  auto* overview = new QWidget;
  auto* dashboard = new QVBoxLayout(overview);
  dashboard->setContentsMargins(28, 24, 28, 24);
  dashboard->setSpacing(20);
  dashboard->addWidget(
      label("SESSION OVERVIEW", "color: #b29adf; font-size: 11px; letter-spacing: 2px;"));
  dashboard->addWidget(label("Make sense of your logs.", "font-size: 28px; font-weight: 600;"));
  overviewText_ =
      label("Start with the sample session, or import plain-text or JSON Lines logs. You can "
            "also drop files here.");
  overviewText_->setWordWrap(true);
  overviewText_->setStyleSheet("color: #acafc2;");
  dashboard->addWidget(overviewText_);
  auto* cards = new QGridLayout;
  cards->setSpacing(12);
  const QStringList metricNames{"VALID ENTRIES", "ERRORS + CRITICAL", "WARNINGS", "MALFORMED"};
  for (int i = 0; i < 4; ++i) {
    auto* card = panel();
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->addWidget(label(metricNames[i], "font-size: 10px; color: #acafc2;"));
    metrics_[static_cast<std::size_t>(i)] = label("—", "font-size: 32px; font-weight: 600;");
    layout->addWidget(metrics_[static_cast<std::size_t>(i)]);
    cards->addWidget(card, 0, i);
  }
  dashboard->addLayout(cards);
  auto* distribution = panel();
  auto* dist = new QVBoxLayout(distribution);
  dist->setContentsMargins(20, 18, 20, 18);
  dist->addWidget(label("Severity distribution", "font-size: 16px; font-weight: 600;"));
  dist->addWidget(
      label("Normalized entries · absolute counts", "color: #9a9db1; font-size: 11px;"));
  const QStringList levels{"Trace", "Debug", "Info", "Warning", "Error", "Critical", "Unknown"};
  for (std::size_t i = 0; i < 7; ++i) {
    auto* row = new QHBoxLayout;
    auto* name = label(levels[static_cast<qsizetype>(i)]);
    name->setFixedWidth(70);
    name->setStyleSheet("color: " + level_color(static_cast<core::LogLevel>(i)).name() + ';');
    row->addWidget(name);
    bars_[i] = new QProgressBar;
    bars_[i]->setTextVisible(false);
    bars_[i]->setRange(0, 1000);
    bars_[i]->setValue(0);
    bars_[i]->setStyleSheet("QProgressBar::chunk { background: " +
                            level_color(static_cast<core::LogLevel>(i)).name() + "; }");
    row->addWidget(bars_[i], 1);
    barCounts_[i] = label("0");
    barCounts_[i]->setAlignment(Qt::AlignRight);
    barCounts_[i]->setFixedWidth(70);
    row->addWidget(barCounts_[i]);
    dist->addLayout(row);
  }
  dashboard->addWidget(distribution);
  dashboard->addWidget(label("Burst alerts", "font-size: 16px; font-weight: 600;"));
  ruleSettings_ = new QPushButton{"Alert rule"};
  ruleSettings_->setObjectName("alertRule");
  dashboard->addWidget(ruleSettings_, 0, Qt::AlignLeft);
  ruleDescription_ = label("5 events at Error or higher in 60 seconds per source.");
  ruleDescription_->setTextFormat(Qt::PlainText);
  ruleDescription_->setWordWrap(true);
  dashboard->addWidget(ruleDescription_);
  connect(ruleSettings_, &QPushButton::clicked, this, &MainWindow::configureRule);
  alerts_ = new QListWidget;
  alerts_->setObjectName("burstAlerts");
  alerts_->setAccessibleName("Burst alerts");
  alerts_->setUniformItemSizes(true);
  alerts_->setMaximumHeight(140);
  noBursts_ = label("Import logs to check for error bursts.", "color: #acafc2;");
  dashboard->addWidget(noBursts_);
  alerts_->hide();
  dashboard->addWidget(alerts_);
  auto* openBurst = new QPushButton{"Open matching records"};
  openBurst->setObjectName("openBurst");
  openBurst->setEnabled(false);
  dashboard->addWidget(openBurst, 0, Qt::AlignLeft);
  connect(alerts_, &QListWidget::currentRowChanged, this,
          [openBurst](int row) { openBurst->setEnabled(row >= 0); });
  connect(openBurst, &QPushButton::clicked, this, [this] { showBurst(alerts_->currentRow()); });
  connect(alerts_, &QListWidget::itemActivated, this,
          [this](QListWidgetItem* item) { showBurst(alerts_->row(item)); });
  dashboard->addWidget(label("Import diagnostics", "font-size: 16px; font-weight: 600;"));
  diagnostics_ = textPanel("No files imported. Select Load Sample Logs for a guided first "
                           "session.\nValid records and malformed records are kept separately.");
  diagnostics_->setObjectName("diagnostics");
  diagnostics_->setMinimumHeight(120);
  dashboard->addWidget(diagnostics_);
  dashboard->addStretch();
  scroll->setWidget(overview);
  pages_->addWidget(scroll);
  auto* explorer = new QWidget;
  auto* logs = new QVBoxLayout(explorer);
  logs->setContentsMargins(28, 24, 28, 24);
  logs->setSpacing(16);
  logs->addWidget(label("Log Explorer", "font-size: 26px; font-weight: 600;"));
  explorerCount_ =
      label("No entries yet. Import logs to explore their contents.", "color: #acafc2;");
  logs->addWidget(explorerCount_);
  auto* actions = new QHBoxLayout;
  clearBurst_ = new QPushButton{"Clear drill-down"};
  exportVisible_ = new QPushButton{"Export visible records"};
  exportVisible_->setEnabled(false);
  actions->addWidget(clearBurst_);
  actions->addStretch();
  actions->addWidget(exportVisible_);
  logs->addLayout(actions);
  connect(exportVisible_, &QPushButton::clicked, this, [this] { exportReport(true); });
  clearBurst_->hide();
  connect(clearBurst_, &QPushButton::clicked, this, [this] {
    proxy_->setBurst({});
    proxy_->setCorrelation({}, {});
    clearBurst_->hide();
  });
  auto* filters = new QHBoxLayout;
  search_ = new QLineEdit;
  search_->setPlaceholderText("Search message text…");
  search_->setClearButtonEnabled(true);
  search_->setObjectName("search");
  search_->setAccessibleName("Search message text");
  severity_ = new QComboBox;
  severity_->addItem("All severities", -1);
  severity_->setAccessibleName("Severity filter");
  for (int i = 0; i < 7; ++i)
    severity_->addItem(levels[i], i);
  source_ = new QComboBox;
  source_->addItem("All sources");
  source_->setAccessibleName("Source filter");
  source_->setMinimumWidth(150);
  filters->addWidget(search_, 1);
  filters->addWidget(severity_);
  filters->addWidget(source_);
  logs->addLayout(filters);
  auto* timeline = new QGridLayout;
  logs->addWidget(label("Session timeline · UTC · bars show error and critical counts"));
  for (std::size_t i = 0; i < timelineSize; ++i) {
    auto* bar = timelineBars_[i] = new QProgressBar;
    bar->setRange(0, 1);
    bar->setValue(0);
    bar->setTextVisible(false);
    bar->setFixedHeight(6);
    bar->setStyleSheet("QProgressBar::chunk { background: #ff9393; }");
    auto* button = timelineButtons_[i] = new QPushButton{"—"};
    button->setStyleSheet("padding: 4px;");
    button->setObjectName(QString("timelineBucket%1").arg(i));
    button->setEnabled(false);
    timeline->addWidget(bar, 0, static_cast<int>(i));
    timeline->addWidget(button, 1, static_cast<int>(i));
    timeline->setColumnStretch(static_cast<int>(i), 1);
    connect(button, &QPushButton::clicked, this, [this, i] {
      const auto& [from, through] = timelineRanges_[i];
      from_->setText(from);
      through_->setText(through);
      applyTimeRange();
    });
  }
  timelineStart_ = label({}, "color: #acafc2;");
  timelineEnd_ = label({}, "color: #acafc2;");
  timeline->addWidget(timelineStart_, 2, 0, 1, static_cast<int>(timelineSize / 2));
  timeline->addWidget(timelineEnd_, 2, static_cast<int>(timelineSize / 2), 1,
                      static_cast<int>(timelineSize / 2), Qt::AlignRight);
  logs->addLayout(timeline);
  auto* timeFilters = new QHBoxLayout;
  from_ = new QLineEdit;
  through_ = new QLineEdit;
  from_->setObjectName("timeFrom");
  through_->setObjectName("timeThrough");
  from_->setAccessibleName("From UTC, inclusive");
  through_->setAccessibleName("Through UTC, inclusive");
  for (auto* edit : {from_, through_}) {
    edit->setPlaceholderText("YYYY-MM-DD HH:MM:SS");
    connect(edit, &QLineEdit::returnPressed, this, &MainWindow::applyTimeRange);
  }
  timeFilters->addWidget(label("From UTC"));
  timeFilters->addWidget(from_, 1);
  timeFilters->addWidget(label("Through UTC"));
  timeFilters->addWidget(through_, 1);
  auto* apply = new QPushButton{"Apply"};
  auto* clear = new QPushButton{"Clear range"};
  clear->setObjectName("clearTimeRange");
  timeFilters->addWidget(apply);
  timeFilters->addWidget(clear);
  logs->addLayout(timeFilters);
  timeRangeError_ = label({}, "color: #ff9393;");
  timeRangeError_->setWordWrap(true);
  timeRangeError_->hide();
  logs->addWidget(timeRangeError_);
  connect(apply, &QPushButton::clicked, this, &MainWindow::applyTimeRange);
  connect(clear, &QPushButton::clicked, this, [this] {
    from_->clear();
    through_->clear();
    applyTimeRange();
  });
  auto* splitter = new QSplitter{Qt::Vertical};
  table_ = new QTableView;
  table_->setObjectName("logTable");
  model_ = new LogModel{this};
  proxy_ = new LogFilter{this};
  proxy_->setSourceModel(model_);
  table_->setModel(proxy_);
  table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  table_->setSelectionMode(QAbstractItemView::SingleSelection);
  table_->setSortingEnabled(true);
  table_->sortByColumn(0, Qt::AscendingOrder);
  table_->setAlternatingRowColors(true);
  table_->setShowGrid(false);
  table_->setWordWrap(false);
  table_->verticalHeader()->hide();
  table_->verticalHeader()->setDefaultSectionSize(36);
  table_->setColumnWidth(0, 210);
  table_->setColumnWidth(1, 105);
  table_->setColumnWidth(2, 140);
  table_->horizontalHeader()->setStretchLastSection(true);
  splitter->addWidget(table_);
  details_ = textPanel("Select an entry to inspect its message, file path and raw log record.");
  details_->setObjectName("details");
  splitter->addWidget(details_);
  splitter->setSizes({390, 180});
  logs->addWidget(splitter, 1);
  auto* correlations = new QHBoxLayout;
  trace_ = new QPushButton{"Show trace events"};
  request_ = new QPushButton{"Show request events"};
  trace_->setObjectName("showTrace");
  request_->setObjectName("showRequest");
  for (auto* button : {trace_, request_}) {
    button->hide();
    correlations->addWidget(button);
  }
  correlations->addStretch();
  logs->addLayout(correlations);
  connect(trace_, &QPushButton::clicked, this, [this] { showCorrelation("trace_id"); });
  connect(request_, &QPushButton::clicked, this, [this] { showCorrelation("request_id"); });
  pages_->addWidget(explorer);
  status_ = label("Ready");
  status_->setObjectName("statusText");
  status_->setTextFormat(Qt::PlainText);
  status_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  statusBar()->addWidget(status_, 1);
  progress_ = new QProgressBar;
  progress_->setRange(0, 0);
  progress_->setFixedWidth(110);
  progress_->hide();
  progress_->setAccessibleName("Import progress");
  statusBar()->addPermanentWidget(progress_);
  cancelImport_ = new QPushButton{"Cancel import"};
  cancelImport_->setObjectName("cancelImport");
  cancelImport_->hide();
  statusBar()->addPermanentWidget(cancelImport_);
  connect(cancelImport_, &QPushButton::clicked, this, [this] {
    importStop_.request_stop();
    cancelImport_->setEnabled(false);
    status_->setText("Cancelling import…");
  });
  connect(&importer_, &QFutureWatcher<ImportOutcome>::progressValueChanged, progress_,
          &QProgressBar::setValue);
  session_ = label("Files: 0    Entries: 0");
  statusBar()->addPermanentWidget(session_);
  connect(navigation, &QListWidget::currentRowChanged, pages_, &QStackedWidget::setCurrentIndex);
  connect(samples_, &QPushButton::clicked, this, &MainWindow::loadSamples);
  connect(import_, &QPushButton::clicked, this, [this] {
    const auto paths =
        QFileDialog::getOpenFileNames(this, "Import logs — replaces current session", {},
                                      "Log files (*.log *.txt *.jsonl *.ndjson);;All files (*)");
    importFiles(paths);
  });
  connect(export_, &QPushButton::clicked, this, [this] { exportReport(); });
  auto* debounce = new QTimer{this};
  debounce->setSingleShot(true);
  debounce->setInterval(180);
  connect(search_, &QLineEdit::textChanged, debounce, [debounce] { debounce->start(); });
  connect(debounce, &QTimer::timeout, this, [this] { proxy_->setSearch(search_->text()); });
  connect(severity_, &QComboBox::currentIndexChanged, this,
          [this] { proxy_->setSeverity(severity_->currentData().toInt()); });
  connect(source_, &QComboBox::currentIndexChanged, this, [this] {
    proxy_->setSource(source_->currentIndex() == 0 ? QString{} : source_->currentText());
  });
  const auto updateCount = [this] {
    explorerCount_->setText(QString("%1 of %2 entries · select a row for the original record")
                                .arg(proxy_->rowCount())
                                .arg(model_->rowCount()));
    exportVisible_->setEnabled(!busy_ && proxy_->rowCount() > 0);
    showDetails(table_->currentIndex());
  };
  connect(proxy_, &QAbstractItemModel::modelReset, this, updateCount);
  connect(proxy_, &QAbstractItemModel::layoutChanged, this, updateCount);
  connect(proxy_, &QAbstractItemModel::rowsInserted, this, updateCount);
  connect(proxy_, &QAbstractItemModel::rowsRemoved, this, updateCount);
  connect(table_->selectionModel(), &QItemSelectionModel::currentRowChanged, this,
          [this](const QModelIndex& index) { showDetails(index); });
  connect(&importer_, &QFutureWatcher<ImportOutcome>::finished, this, [this] {
    cancelImport_->hide();
    if (importStop_.stop_requested()) {
      setBusy(false, "Import cancelled — current session kept.");
      return;
    }
    try {
      auto [next, error] = importer_.result();
      if (!next) {
        setBusy(false, "Import failed: " + error + ". Current session kept.");
        return;
      }
      if (next->importedFiles == 0U) {
        diagnostics_->setPlainText(next->diagnostics);
        setBusy(false, "Import failed — current session kept. See Overview diagnostics.");
        return;
      }
      dataset_ = std::move(next);
      refresh();
      setBusy(false, (dataset_->malformed || dataset_->importedFiles != dataset_->files.size())
                         ? "Imported with issues — see diagnostics."
                         : "Import complete.");
    } catch (const std::exception& error) {
      setBusy(false, "Import failed: " + QString::fromUtf8(error.what()));
    } catch (...) {
      setBusy(false, "Import failed unexpectedly. Current session kept.");
    }
  });
  connect(&exporter_, &QFutureWatcher<QString>::finished, this, [this] {
    try {
      setBusy(false, exporter_.result());
    } catch (...) {
      setBusy(false, "Export failed unexpectedly.");
    }
  });
}
void MainWindow::setBusy(bool busy, const QString& text) {
  busy_ = busy;
  import_->setEnabled(!busy);
  mapping_->setEnabled(!busy);
  ruleSettings_->setEnabled(!busy);
  exportVisible_->setEnabled(!busy && proxy_->rowCount() > 0);
  samples_->setEnabled(!busy);
  export_->setEnabled(!busy && dataset_ && dataset_->importedFiles > 0U);
  progress_->setVisible(busy);
  status_->setText(text);
  status_->setToolTip(text);
}
void MainWindow::importFiles(const QStringList& paths) {
  if (busy_ || paths.isEmpty())
    return;
  auto unique = paths;
  unique.removeDuplicates();
  setBusy(true, QString("Importing %1 file(s)… You can keep exploring the current session.")
                    .arg(unique.size()));
  importStop_ = std::stop_source{};
  progress_->setRange(0, 100);
  progress_->setValue(0);
  cancelImport_->setEnabled(true);
  cancelImport_->show();
  importer_.setFuture(
      QtConcurrent::run([unique, stop = importStop_.get_token(), config = jsonConfig_,
                         rule = burstRule_](QPromise<ImportOutcome>& promise) {
        promise.setProgressRange(0, 100);
        try {
          const auto progress = [&promise, last = -1](int value) mutable {
            if (value != last) {
              last = value;
              promise.setProgressValue(value);
            }
          };
          promise.addResult(ImportOutcome{load_files(unique, stop, progress, config, rule), {}});
        } catch (const std::exception& error) {
          promise.addResult(ImportOutcome{nullptr, QString::fromUtf8(error.what())});
        }
      }));
}
void MainWindow::loadSamples() {
  importFiles({QApplication::applicationDirPath() + "/samples/atlas.log"});
}
void MainWindow::refresh() {
  resetExplorerFilters();
  model_->setDataset(dataset_);
  refreshTimeline();
  bursts_ = dataset_->bursts;
  refreshBursts();
  {
    const QSignalBlocker blocker{source_};
    source_->clear();
    source_->addItem("All sources");
    source_->addItems(dataset_->sources);
  }
  proxy_->setSource({});
  showDetails({});
  const std::array values{dataset_->entries.size(), dataset_->counts[4] + dataset_->counts[5],
                          dataset_->counts[3], dataset_->malformed};
  for (std::size_t i = 0; i < 4; ++i)
    metrics_[i]->setText(QString::number(values[i]));
  const auto peak = *std::max_element(dataset_->counts.begin(), dataset_->counts.end());
  for (std::size_t i = 0; i < 7; ++i) {
    barCounts_[i]->setText(QString::number(dataset_->counts[i]));
    bars_[i]->setValue(peak == 0U
                           ? 0
                           : static_cast<int>(1000.0 * static_cast<double>(dataset_->counts[i]) /
                                              static_cast<double>(peak)));
  }
  overviewText_->setText(QString("%1 imported file(s) · %2 sources · %3 malformed records. Import "
                                 "again to replace this session.")
                             .arg(dataset_->importedFiles)
                             .arg(dataset_->sources.size())
                             .arg(dataset_->malformed));
  diagnostics_->setPlainText(dataset_->diagnostics);
  session_->setText(QString("Files: %1    Entries: %2")
                        .arg(dataset_->importedFiles)
                        .arg(dataset_->entries.size()));
}
void MainWindow::configureJson() {
  QDialog dialog{this};
  dialog.setWindowTitle("JSON Lines fields");
  QFormLayout layout{&dialog};
  QLineEdit timestamp{QString::fromStdString(jsonConfig_.timestampFieldPath)};
  QLineEdit severity{QString::fromStdString(jsonConfig_.levelFieldPath)};
  QLineEdit source{QString::fromStdString(jsonConfig_.sourceFieldPath.value_or(""))};
  QLineEdit message{QString::fromStdString(jsonConfig_.messageFieldPath)};
  timestamp.setObjectName("jsonTimestampPath");
  severity.setObjectName("jsonSeverityPath");
  source.setObjectName("jsonSourcePath");
  message.setObjectName("jsonMessagePath");
  source.setPlaceholderText("Use filename if blank");
  layout.addRow("&Timestamp", &timestamp);
  layout.addRow("&Severity", &severity);
  layout.addRow("So&urce", &source);
  layout.addRow("&Message", &message);
  QLabel help{"Use dots for nested fields. Changes apply to the next import."};
  layout.addRow(&help);
  QLabel error{"Use distinct field paths without empty segments. Only Source can be blank."};
  error.setWordWrap(true);
  error.hide();
  layout.addRow(&error);
  QDialogButtonBox buttons{QDialogButtonBox::Ok | QDialogButtonBox::Cancel};
  layout.addRow(&buttons);
  connect(&buttons, &QDialogButtonBox::accepted, &dialog, [&] {
    const auto sourcePath = source.text().trimmed().toStdString();
    parsers::JsonParserConfig config{
        .timestampFieldPath = timestamp.text().trimmed().toStdString(),
        .levelFieldPath = severity.text().trimmed().toStdString(),
        .sourceFieldPath = sourcePath.empty() ? std::nullopt : std::optional{sourcePath},
        .messageFieldPath = message.text().trimmed().toStdString()};
    if (parsers::validate_json_parser_config(config)) {
      error.show();
      return;
    }
    jsonConfig_ = std::move(config);
    dialog.accept();
  });
  connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  dialog.exec();
}

void MainWindow::configureRule() {
  QDialog dialog{this};
  dialog.setWindowTitle("Burst alert rule");
  QFormLayout layout{&dialog};
  QSpinBox threshold;
  threshold.setObjectName("burstThreshold");
  threshold.setRange(2, static_cast<int>(parsers::ImportBudget::maxLines));
  threshold.setValue(static_cast<int>(burstRule_.threshold));
  QSpinBox seconds;
  seconds.setObjectName("burstWindow");
  seconds.setRange(1, 86400);
  seconds.setValue(static_cast<int>(burstRule_.window.count()));
  QComboBox severity;
  severity.setObjectName("burstSeverity");
  severity.addItems({"Trace", "Debug", "Info", "Warning", "Error", "Critical"});
  severity.setCurrentIndex(static_cast<int>(burstRule_.minimumLevel));
  QLineEdit source{QString::fromStdString(burstRule_.source)};
  source.setObjectName("burstSource");
  source.setPlaceholderText("All sources if blank");
  layout.addRow("&Event count", &threshold);
  layout.addRow("&Window (seconds)", &seconds);
  layout.addRow("&Minimum severity", &severity);
  layout.addRow("&Source (exact match)", &source);
  QDialogButtonBox buttons{QDialogButtonBox::Ok | QDialogButtonBox::Cancel};
  layout.addRow(&buttons);
  connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  if (dialog.exec() != QDialog::Accepted)
    return;
  burstRule_ = {.threshold = static_cast<std::size_t>(threshold.value()),
                .window = std::chrono::seconds{seconds.value()},
                .minimumLevel = static_cast<core::LogLevel>(severity.currentIndex()),
                .source = source.text().toStdString()};
  if (proxy_->activeFilters().contains("alert")) {
    proxy_->setBurst({});
    clearBurst_->hide();
  }
  if (dataset_)
    bursts_ = rules::find_error_bursts(dataset_->entries, burstRule_);
  refreshBursts();
}

void MainWindow::refreshBursts() {
  const auto sources =
      burstRule_.source.empty()
          ? QString{"per source"}
          : QString{"from %1"}.arg(QString::fromStdString(burstRule_.source).left(80));
  ruleDescription_->setText(
      QString("%1 events at %2 or higher in %3 seconds %4.")
          .arg(burstRule_.threshold)
          .arg(QString::fromStdString(std::string{core::to_string(burstRule_.minimumLevel)}))
          .arg(burstRule_.window.count())
          .arg(sources));
  alerts_->clear();
  alerts_->setVisible(!bursts_.empty());
  noBursts_->setText(dataset_ ? "No bursts found."
                              : "Import logs to check for error bursts.");
  noBursts_->setVisible(bursts_.empty());
  for (const auto& burst : bursts_) {
    alerts_->addItem(
        QString("%1 · %2 → %3")
            .arg(QString::fromStdString(burst.front()->source.name).left(80),
                 QString::fromStdString(app::format_timestamp(burst.front()->timestamp)),
                 QString::fromStdString(app::format_timestamp(burst.back()->timestamp))));
  }
}

void MainWindow::resetExplorerFilters() {
  proxy_->setBurst({});
  proxy_->setCorrelation({}, {});
  clearBurst_->hide();
  search_->clear();
  proxy_->setSearch({});
  severity_->setCurrentIndex(0);
  source_->setCurrentIndex(0);
  from_->clear();
  through_->clear();
  applyTimeRange();
}

void MainWindow::showBurst(int index) {
  if (index < 0 || static_cast<std::size_t>(index) >= bursts_.size())
    return;
  resetExplorerFilters();
  proxy_->setBurst(bursts_[static_cast<std::size_t>(index)], alerts_->item(index)->text());
  clearBurst_->show();
  navigation_->setCurrentRow(1);
  table_->setCurrentIndex(proxy_->index(0, 0));
  table_->setFocus();
}

void MainWindow::showCorrelation(const char* key) {
  const auto* entry = model_->entry(proxy_->mapToSource(table_->currentIndex()).row());
  if (!entry || !entry->attributes.contains(key))
    return;
  const auto id = entry->attributes.at(key);
  resetExplorerFilters();
  proxy_->setCorrelation(key, id);
  table_->sortByColumn(0, Qt::AscendingOrder);
  clearBurst_->show();
  table_->setCurrentIndex(proxy_->index(0, 0));
  table_->setFocus();
}

void MainWindow::applyTimeRange() {
  const auto fromText = from_->text().trimmed();
  const auto throughText = through_->text().trimmed();
  const auto from = parsers::parse_timestamp(fromText.toStdString(), true);
  const auto through = parsers::parse_timestamp(throughText.toStdString(), true);
  QString error;
  if ((!fromText.isEmpty() && !from) || (!throughText.isEmpty() && !through))
    error = "Enter UTC timestamps as YYYY-MM-DD HH:MM:SS, or leave a bound blank.";
  else if (from && through && *from > *through)
    error = "From UTC must be earlier than or equal to Through UTC.";
  timeRangeError_->setText(error);
  timeRangeError_->setVisible(!error.isEmpty());
  if (error.isEmpty())
    proxy_->setTimeRange(from, through);
}

void MainWindow::refreshTimeline() {
  using namespace std::chrono;
  std::array<int, timelineSize> totals{};
  std::array<int, timelineSize> errors{};
  std::array<std::pair<core::LogTimestamp, core::LogTimestamp>, timelineSize> ranges{};
  const auto& entries = dataset_->entries;
  const auto bounds =
      std::minmax_element(entries.begin(), entries.end(),
                          [](const auto* a, const auto* b) { return a->timestamp < b->timestamp; });
  const auto first = entries.empty() ? sys_seconds{} : floor<seconds>((*bounds.first)->timestamp);
  const auto last = entries.empty() ? first : floor<seconds>((*bounds.second)->timestamp);
  timelineStart_->setText(
      entries.empty() ? QString{}
                      : QString::fromStdString(app::format_timestamp((*bounds.first)->timestamp)));
  timelineEnd_->setText(
      entries.empty() ? QString{}
                      : QString::fromStdString(app::format_timestamp((*bounds.second)->timestamp)));
  const auto width = seconds{(last - first).count() / static_cast<seconds::rep>(timelineSize) + 1};
  for (const auto* entry : entries) {
    const auto index = static_cast<std::size_t>((floor<seconds>(entry->timestamp) - first) / width);
    auto& [start, end] = ranges[index];
    if (totals[index]++ == 0) {
      start = end = entry->timestamp;
    } else {
      start = std::min(start, entry->timestamp);
      end = std::max(end, entry->timestamp);
    }
    if (entry->level == core::LogLevel::error || entry->level == core::LogLevel::critical)
      ++errors[index];
  }
  const auto peak = std::max(1, *std::max_element(errors.begin(), errors.end()));
  for (std::size_t i = 0; i < timelineSize; ++i) {
    const bool active = totals[i] > 0;
    auto* button = timelineButtons_[i];
    button->setEnabled(active);
    button->setText(entries.empty() ? QString{"—"} : QString::number(totals[i]));
    QString description;
    if (active) {
      const auto& [from, through] = ranges[i];
      auto& [startText, endText] = timelineRanges_[i];
      startText = QString::fromStdString(app::format_timestamp(from));
      endText = QString::fromStdString(app::format_timestamp(through));
      description = QString("%1 through %2 · %3 events · %4 errors or critical")
                        .arg(startText, endText)
                        .arg(totals[i])
                        .arg(errors[i]);
    }
    button->setToolTip(description);
    button->setAccessibleName(description);
    timelineBars_[i]->setRange(0, peak);
    timelineBars_[i]->setValue(errors[i]);
  }
}

void MainWindow::showDetails(const QModelIndex& index) {
  const auto mapped = proxy_->mapToSource(index);
  const auto* entry = model_->entry(mapped.row());
  trace_->setVisible(entry && entry->attributes.contains("trace_id"));
  request_->setVisible(entry && entry->attributes.contains("request_id"));
  if (!index.isValid() || !entry) {
    details_->clear();
    return;
  }
  std::string ids;
  for (const auto& [key, value] : entry->attributes)
    ids += "\n" + key + ": " + value;
  details_->setPlainText(QString::fromStdString(
      app::format_timestamp(entry->timestamp) + "   " + std::string{core::to_string(entry->level)} +
      "   " + entry->source.name + "\nFile: " + entry->source.file_path.value_or("") + ids +
      "\n\n" + entry->message + "\n\nRAW RECORD\n" + entry->rawContent));
}
void MainWindow::exportReport(bool visibleOnly) {
  if (busy_ || !dataset_)
    return;
  const auto path = QFileDialog::getSaveFileName(
      this, visibleOnly ? "Export visible records" : "Export session", {}, "JSON report (*.json)",
      nullptr, QFileDialog::DontConfirmOverwrite);
  if (path.isEmpty())
    return;
  exportTo(path, visibleOnly);
}
void MainWindow::exportTo(const QString& path, bool visibleOnly) {
  if (busy_ || !dataset_ || path.isEmpty())
    return;
  const auto snapshot = dataset_;
  std::vector<const core::LogEntry*> entries;
  core::LogAttributes filters;
  if (visibleOnly) {
    entries.reserve(static_cast<std::size_t>(proxy_->rowCount()));
    for (int row = 0; row < proxy_->rowCount(); ++row)
      entries.push_back(model_->entry(proxy_->mapToSource(proxy_->index(row, 0)).row()));
    filters = proxy_->activeFilters();
  }
  progress_->setRange(0, 0);
  setBusy(true, visibleOnly ? "Exporting visible records…" : "Exporting session…");
  exporter_.setFuture(QtConcurrent::run(
      [snapshot, path, visibleOnly, entries = std::move(entries), filters = std::move(filters)] {
        try {
          const std::filesystem::path destination{path.toStdString()};
          if (visibleOnly)
            app::save_visible_report(destination, entries, filters);
          else
            app::save_json_report(destination, snapshot->files);
          return QString{"Report saved: "} + path;
        } catch (const std::exception& error) {
          return QString{"Export failed: "} + QString::fromUtf8(error.what());
        }
      }));
}
void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
  if (!busy_ && event->mimeData()->hasUrls()) {
    const auto urls = event->mimeData()->urls();
    if (std::all_of(urls.begin(), urls.end(), [](const auto& url) { return url.isLocalFile(); }))
      event->acceptProposedAction();
  }
}
void MainWindow::dropEvent(QDropEvent* event) {
  QStringList paths;
  for (const auto& url : event->mimeData()->urls())
    if (url.isLocalFile())
      paths.push_back(url.toLocalFile());
  importFiles(paths);
  event->acceptProposedAction();
}
void MainWindow::closeEvent(QCloseEvent* event) {
  if (busy_) {
    status_->setText("Please wait for the current import or export before closing.");
    event->ignore();
  } else
    QMainWindow::closeEvent(event);
}
} // namespace loginsight::gui
