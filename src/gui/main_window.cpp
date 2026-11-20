#include "main_window.hpp"
#include "comparison_panel.hpp"
#include "large_log_panel.hpp"
#include "live_panel.hpp"
#include "loginsight/parsers/timestamp_parser.hpp"
#include "malformed_panel.hpp"
#include "recap_panel.hpp"
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetrics>
#include <QFormLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPromise>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSettings>
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
#include <ostream>
#include <streambuf>

namespace loginsight::gui {
namespace {
constexpr auto savedViewsKey = "explorer/savedFilters";
constexpr std::array metricNames{"Entries", "Errors + critical", "Warnings", "Malformed"};

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
    QPushButton:focus { border-color: #c5acef; }
    QPushButton:checked { background: #493961; border-color: #c5acef; color: #f1e8ff; }
    QPushButton[filterChip="true"] { background: #30283f; border-color: #62527a; padding: 4px 8px; }
    QPushButton[filterChip="true"]:hover, QPushButton[filterChip="true"]:focus { border-color: #c5acef; }
    QPushButton[metric="true"] { text-align: left; padding: 12px; }
    QPushButton:hover { background: #343044; border-color: #a38ae4; }
    QPushButton:checked:hover { background: #493961; border-color: #e1c9ff; }
    QPushButton:disabled { color: #686b7c; border-color: #292b35; }
    QPushButton#primary { background: #664ba4; border-color: #a18bd1; color: white; }
    QPushButton#primary:focus { border-color: #eee2ff; }
    QListWidget { background: #181a22; border: none; padding: 12px; outline: none; }
    QListWidget::item { padding: 13px; margin: 3px 0; border-radius: 5px; }
    QListWidget::item:selected { background: #30283f; color: #c5acef; }
    QLineEdit, QComboBox, QTextEdit { background: #191b24; border: 1px solid #343746; border-radius: 5px; padding: 7px; }
    QLineEdit:focus, QComboBox:focus { border-color: #a38ae4; }
    QComboBox QAbstractItemView { background: #242633; selection-background-color: #554370; }
    QTableView { background: #191b23; alternate-background-color: #1e202b; border: 1px solid #30323f; gridline-color: #292c39; selection-background-color: #39304d; selection-color: white; }
    QHeaderView::section { background: #242633; color: #b9bbca; border: none; border-bottom: 1px solid #363849; padding: 6px 8px; font-weight: bold; }
    QProgressBar { background: #2a2d39; border: none; border-radius: 3px; max-height: 9px; }
    QProgressBar::chunk { background: #a38ae4; border-radius: 3px; }
    QStatusBar { background: #181a22; color: #b4b7c9; border-top: 1px solid #30323f; }
    QSplitter::handle { background: #404354; }
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
  header->setContentsMargins(18, 10, 18, 10);
  header->setSpacing(10);
  auto* brand = label("LogInsight", "font-size: 24px; font-weight: 700;");
  header->addWidget(brand);
  header->addStretch();
  samples_ = new QPushButton{"Load Sample Logs"};
  samples_->setObjectName("loadSamples");
  import_ = new QPushButton{"Import Logs"};
  import_->setObjectName("primary");
  export_ = new QPushButton{"Export full session"};
  export_->setEnabled(false);
  header->addWidget(samples_);
  header->addWidget(import_);
  auto* importMenu = new QMenu{import_};
  auto* chooseFiles = importMenu->addAction("Choose log files…");
  chooseFiles->setShortcut(QKeySequence::Open);
  addAction(chooseFiles);
  auto* mapping = importMenu->addAction("JSON field mappings…");
  mapping->setObjectName("jsonFields");
  connect(mapping, &QAction::triggered, this, &MainWindow::configureJson);
  auto* csvMapping = importMenu->addAction("CSV column mappings…");
  csvMapping->setObjectName("csvColumns");
  connect(csvMapping, &QAction::triggered, this, &MainWindow::configureCsv);
  import_->setMenu(importMenu);
  header->addWidget(export_);
  outer->addLayout(header);
  auto* body = new QHBoxLayout;
  body->setSpacing(0);
  auto* sidebar = new QWidget;
  sidebar->setFixedWidth(144);
  auto* side = new QVBoxLayout(sidebar);
  side->setContentsMargins(0, 0, 0, 12);
  auto* navigation = navigation_ = new QListWidget;
  navigation->addItems(
      {"Overview", "Log Explorer", "Recap", "Malformed", "Compare", "Live", "Large logs"});
  navigation->setCurrentRow(0);
  navigation->setObjectName("navigation");
  side->addWidget(navigation);
  body->addWidget(sidebar);
  pages_ = new QStackedWidget;
  body->addWidget(pages_, 1);
  outer->addLayout(body, 1);
  auto* scroll = new QScrollArea;
  scroll->setWidgetResizable(true);
  auto* overview = new QWidget;
  auto* dashboard = new QVBoxLayout(overview);
  dashboard->setContentsMargins(18, 14, 18, 14);
  dashboard->setSpacing(12);
  dashboard->addWidget(label("Session overview", "font-size: 25px; font-weight: 600;"));
  overviewText_ =
      label("Start with the sample session, or import plain-text or JSON Lines logs. You can "
            "also drop files here.");
  overviewText_->setWordWrap(true);
  overviewText_->setStyleSheet("color: #acafc2;");
  dashboard->addWidget(overviewText_);
  auto* cards = new QGridLayout;
  const QStringList metricIds{"entriesMetric", "errorsMetric", "warningsMetric", "malformedMetric"};
  for (int i = 0; i < 4; ++i) {
    auto* metric =
        new QPushButton{QString::fromUtf8(metricNames[static_cast<std::size_t>(i)]) + ": —"};
    metric->setProperty("metric", true);
    metric->setObjectName(metricIds[i]);
    metric->setEnabled(false);
    metrics_[static_cast<std::size_t>(i)] = metric;
    cards->addWidget(metric, 0, i);
    cards->setColumnStretch(i, 1);
    connect(metric, &QPushButton::clicked, this, [this, i] {
      if (i == 3) {
        navigation_->setCurrentRow(3);
        malformed_->focusRecords();
        return;
      }
      resetExplorerFilters();
      const int level = i == 1   ? LogFilter::errorAndCritical
                        : i == 2 ? static_cast<int>(core::LogLevel::warning)
                                 : -1;
      severity_->setCurrentIndex(severity_->findData(level));
      navigation_->setCurrentRow(1);
      table_->setFocus();
    });
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
  auto* alertHeader = new QHBoxLayout;
  alertHeader->addWidget(label("Burst alerts", "font-size: 16px; font-weight: 600;"));
  alertHeader->addStretch();
  dashboard->addLayout(alertHeader);
  ruleSettings_ = new QPushButton{"Alert rules"};
  ruleSettings_->setObjectName("alertRule");
  alertHeader->addWidget(ruleSettings_);
  ruleDescription_ = label("Active rules: 1");
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
  openBurst_ = new QPushButton{"Open matching records"};
  openBurst_->setObjectName("openBurst");
  openBurst_->setEnabled(false);
  alertHeader->insertWidget(2, openBurst_);
  openBurst_->hide();
  connect(alerts_, &QListWidget::currentRowChanged, this,
          [this](int row) { openBurst_->setEnabled(row >= 0); });
  connect(openBurst_, &QPushButton::clicked, this, [this] { showBurst(alerts_->currentRow()); });
  connect(alerts_, &QListWidget::itemActivated, this,
          [this](QListWidgetItem* item) { showBurst(alerts_->row(item)); });
  dashboard->addWidget(label("Import diagnostics", "font-size: 16px; font-weight: 600;"));
  diagnostics_ = textPanel("No files imported. Select Load Sample Logs for a guided first "
                           "session.\nValid records and malformed records are kept separately.");
  diagnostics_->setObjectName("diagnostics");
  diagnostics_->setAccessibleName("Import diagnostics");
  diagnostics_->setMinimumHeight(120);
  dashboard->addWidget(diagnostics_);
  dashboard->addWidget(distribution);
  dashboard->addStretch();
  scroll->setWidget(overview);
  pages_->addWidget(scroll);
  auto* explorer = new QWidget;
  auto* logs = new QVBoxLayout(explorer);
  logs->setContentsMargins(16, 10, 16, 10);
  logs->setSpacing(8);
  auto* actions = new QHBoxLayout;
  actions->addWidget(label("Log Explorer", "font-size: 22px; font-weight: 600;"));
  explorerCount_ = label("No entries", "color: #acafc2;");
  actions->addWidget(explorerCount_);
  actions->addStretch();
  auto* views = new QPushButton{"Saved views"};
  views->setObjectName("savedViews");
  auto* viewsMenu = new QMenu{views};
  views->setMenu(viewsMenu);
  connect(viewsMenu, &QMenu::aboutToShow, this,
          [this, viewsMenu] { populateViewsMenu(*viewsMenu); });
  actions->addWidget(views);
  exportVisible_ = new QPushButton{"Export filtered results (0)"};
  exportVisible_->setObjectName("exportFiltered");
  exportVisible_->setEnabled(false);
  actions->addWidget(exportVisible_);
  logs->addLayout(actions);

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
  severity_->addItem("Error + critical", LogFilter::errorAndCritical);
  source_ = new QComboBox;
  source_->addItem("All sources");
  source_->setAccessibleName("Source filter");
  source_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
  source_->setMinimumContentsLength(8);
  filters->addWidget(search_, 1);
  filters->addWidget(severity_);
  filters->addWidget(source_);
  auto* toggleTime = new QPushButton{"Time range"};
  toggleTime->setObjectName("toggleTimeRange");
  toggleTime->setCheckable(true);
  filters->addWidget(toggleTime);
  logs->addLayout(filters);
  chipRow_ = new QWidget;
  auto* chipLayout = new QHBoxLayout(chipRow_);
  chipLayout->setContentsMargins(0, 0, 0, 0);
  chipLayout->setSpacing(5);
  const QStringList chipIds{"searchChip", "severityChip",  "sourceChip",
                            "timeChip",   "drillDownChip", "bookmarkChip"};
  for (std::size_t i = 0; i < chips_.size(); ++i) {
    auto* chip = chips_[i] = new QPushButton;
    chip->setObjectName(chipIds[static_cast<qsizetype>(i)]);
    chip->setProperty("filterChip", true);
    chipLayout->addWidget(chip);
  }
  chipLayout->addStretch();
  auto* clearAll = new QPushButton{"Clear all"};
  clearAll->setObjectName("clearFilters");
  clearAll->setStyleSheet("padding: 4px 8px;");
  chipLayout->addWidget(clearAll);
  logs->addWidget(chipRow_);
  chipRow_->hide();
  connect(chips_[0], &QPushButton::clicked, this, [this] {
    search_->clear();
    proxy_->setSearch({});
    refreshFilterChips();
  });
  connect(chips_[1], &QPushButton::clicked, this, [this] { severity_->setCurrentIndex(0); });
  connect(chips_[2], &QPushButton::clicked, this, [this] { source_->setCurrentIndex(0); });
  connect(chips_[3], &QPushButton::clicked, this, &MainWindow::clearTimeRange);
  connect(chips_[4], &QPushButton::clicked, this, [this] {
    proxy_->setBurst({});
    proxy_->setCorrelation({}, {});
    refreshFilterChips();
  });
  connect(chips_[5], &QPushButton::clicked, this, [this] { setBookmarkScope(false); });
  connect(clearAll, &QPushButton::clicked, this, &MainWindow::resetExplorerFilters);
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
    button->setCheckable(true);
    timeline->addWidget(bar, 0, static_cast<int>(i));
    timeline->addWidget(button, 1, static_cast<int>(i));
    timeline->setColumnStretch(static_cast<int>(i), 1);
    connect(button, &QPushButton::clicked, this, [this, i] {
      const auto& [from, through] = timelineRanges_[i];
      from_->setText(from);
      through_->setText(through);
      applyTimeRange();
      timelineButtons_[i]->setChecked(true);
    });
  }
  timelineStart_ = label({}, "color: #acafc2;");
  timelineEnd_ = label({}, "color: #acafc2;");
  timeline->addWidget(timelineStart_, 2, 0, 1, static_cast<int>(timelineSize / 2));
  timeline->addWidget(timelineEnd_, 2, static_cast<int>(timelineSize / 2), 1,
                      static_cast<int>(timelineSize / 2), Qt::AlignRight);
  logs->addLayout(timeline);
  timeControls_ = new QWidget;
  timeControls_->setObjectName("timeControls");
  auto* timeLayout = new QVBoxLayout(timeControls_);
  timeLayout->setContentsMargins(0, 0, 0, 0);
  auto* timeFilters = new QHBoxLayout;
  timeLayout->addLayout(timeFilters);
  logs->addWidget(timeControls_);
  timeControls_->hide();
  connect(toggleTime, &QPushButton::toggled, timeControls_, &QWidget::setVisible);
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
  timeRangeError_ = label({}, "color: #ff9393;");
  timeRangeError_->setWordWrap(true);
  timeRangeError_->hide();
  timeLayout->addWidget(timeRangeError_);
  connect(apply, &QPushButton::clicked, this, &MainWindow::applyTimeRange);
  connect(clear, &QPushButton::clicked, this, &MainWindow::clearTimeRange);
  splitter_ = new QSplitter{Qt::Vertical};
  splitter_->setObjectName("explorerSplit");
  splitter_->setChildrenCollapsible(false);
  splitter_->setHandleWidth(5);
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
  table_->verticalHeader()->setDefaultSectionSize(28);
  table_->setColumnWidth(0, 190);
  table_->setColumnWidth(1, 82);
  table_->setColumnWidth(2, 95);
  table_->horizontalHeader()->setStretchLastSection(true);
  splitter_->addWidget(table_);
  auto* inspector = new QWidget;
  auto* inspectorLayout = new QVBoxLayout(inspector);
  inspectorLayout->setContentsMargins(8, 0, 0, 0);
  inspectorLayout->setSpacing(5);
  auto* inspectorHeader = new QHBoxLayout;
  inspectorHeader->addWidget(label("Record details", "font-weight: 600;"));
  inspectorHeader->addStretch();
  bookmark_ = new QPushButton{"Bookmark"};
  bookmark_->setObjectName("bookmarkRecord");
  bookmark_->setEnabled(false);
  bookmark_->setStyleSheet("padding: 4px 7px;");
  inspectorHeader->addWidget(bookmark_);
  inspectorLayout->addLayout(inspectorHeader);
  connect(bookmark_, &QPushButton::clicked, this, &MainWindow::toggleBookmark);
  details_ = textPanel("Select an entry to inspect its message, file path and raw log record.");
  details_->setObjectName("details");
  details_->setAccessibleName("Record details");
  details_->setStyleSheet("font-family: monospace;");
  inspectorLayout->addWidget(details_, 1);
  splitter_->addWidget(inspector);
  splitter_->setStretchFactor(0, 3);
  splitter_->setStretchFactor(1, 1);
  logs->addWidget(splitter_, 1);
  auto* correlations = new QHBoxLayout;
  trace_ = new QPushButton{"Follow trace"};
  request_ = new QPushButton{"Follow request"};
  trace_->setObjectName("showTrace");
  request_->setObjectName("showRequest");
  for (auto* button : {trace_, request_}) {
    button->hide();
    correlations->addWidget(button);
  }
  correlations->addStretch();
  inspectorLayout->addLayout(correlations);
  connect(trace_, &QPushButton::clicked, this, [this] { showCorrelation("trace_id"); });
  connect(request_, &QPushButton::clicked, this, [this] { showCorrelation("request_id"); });
  pages_->addWidget(explorer);
  recap_ = new RecapPanel{[this](int level) {
                            resetExplorerFilters();
                            severity_->setCurrentIndex(severity_->findData(level));
                            navigation_->setCurrentRow(1);
                            table_->setFocus();
                          },
                          [this](const QString& source) {
                            const int index = source_->findData(source);
                            if (index <= 0)
                              return;
                            resetExplorerFilters();
                            source_->setCurrentIndex(index);
                            navigation_->setCurrentRow(1);
                            table_->setFocus();
                          }};
  pages_->addWidget(recap_);
  malformed_ = new MalformedPanel;
  pages_->addWidget(malformed_);
  comparison_ = new ComparisonPanel;
  pages_->addWidget(comparison_);
  pages_->addWidget(new LivePanel);
  large_ = new LargeLogPanel([this] {
    indexLargeFiles(QFileDialog::getOpenFileNames(
        this, "Index large log files", {},
        "Log files (*.log *.txt *.jsonl *.ndjson *.csv);;All files (*)"));
  });
  pages_->addWidget(large_);
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
  connect(chooseFiles, &QAction::triggered, this, [this] {
    if (busy_)
      return;
    const auto paths = QFileDialog::getOpenFileNames(
        this, "Import logs — replaces current session", {},
        "Log files (*.log *.txt *.jsonl *.ndjson *.csv);;All files (*)");
    importFiles(paths);
  });
  for (auto* button : {export_, exportVisible_}) {
    const bool visible = button == exportVisible_;
    auto* menu = new QMenu{button};
    menu->addAction("Save JSON…", this, [this, visible] { exportReport(visible, false); });
    auto* redact = menu->addAction("Redact and preview…", this,
                                   [this, visible] { exportReport(visible, true); });
    redact->setObjectName("redactedExport");
    button->setMenu(menu);
  }
  auto* debounce = new QTimer{this};
  debounce->setSingleShot(true);
  debounce->setInterval(180);
  connect(search_, &QLineEdit::textChanged, debounce, [debounce] { debounce->start(); });
  connect(debounce, &QTimer::timeout, this, [this] {
    proxy_->setSearch(search_->text());
    refreshFilterChips();
  });
  connect(severity_, &QComboBox::currentIndexChanged, this, [this] {
    proxy_->setSeverity(severity_->currentData().toInt());
    refreshFilterChips();
  });
  connect(source_, &QComboBox::currentIndexChanged, this, [this] {
    proxy_->setSource(source_->currentIndex() == 0 ? QString{} : source_->currentText());
    refreshFilterChips();
  });
  const auto updateCount = [this] {
    explorerCount_->setText(
        QString("%1 of %2 entries").arg(proxy_->rowCount()).arg(model_->rowCount()));
    exportVisible_->setText(QString("Export filtered results (%1)").arg(proxy_->rowCount()));
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
  importer_.setFuture(QtConcurrent::run([unique, stop = importStop_.get_token(),
                                         config = jsonConfig_, csv = csvColumns_,
                                         rules = activeRules_](QPromise<ImportOutcome>& promise) {
    promise.setProgressRange(0, 100);
    try {
      const auto progress = [&promise, last = -1](int value) mutable {
        if (value != last) {
          last = value;
          promise.setProgressValue(value);
        }
      };
      promise.addResult(ImportOutcome{load_files(unique, stop, progress, config, rules, csv), {}});
    } catch (const std::exception& error) {
      promise.addResult(ImportOutcome{nullptr, QString::fromUtf8(error.what())});
    }
  }));
}
void MainWindow::indexLargeFiles(const QStringList& paths) {
  large_->openFiles(paths, jsonConfig_, csvColumns_);
}

void MainWindow::loadSamples() {
  const QDir executableDirectory{QApplication::applicationDirPath()};
  const auto adjacentSample = executableDirectory.filePath("samples/atlas.log");
  importFiles({QFileInfo::exists(adjacentSample)
                   ? adjacentSample
                   : executableDirectory.filePath(LOGINSIGHT_INSTALLED_SAMPLE)});
}
void MainWindow::refresh() {
  resetExplorerFilters();
  bookmarks_.clear();
  model_->setDataset(dataset_);
  refreshTimeline();
  recap_->setDataset(*dataset_);
  malformed_->setDataset(dataset_);
  comparison_->setDataset(*dataset_);
  bursts_ = dataset_->bursts;
  refreshBursts();
  {
    const QSignalBlocker blocker{source_};
    source_->clear();
    source_->addItem("All sources");
    for (const auto& source : dataset_->sources)
      source_->addItem(source, source);
  }
  proxy_->setSource({});
  showDetails({});
  const std::array values{dataset_->entries.size(), dataset_->counts[4] + dataset_->counts[5],
                          dataset_->counts[3], dataset_->malformed};
  for (std::size_t i = 0; i < values.size(); ++i) {
    metrics_[i]->setText(QString("%1: %2").arg(QString::fromUtf8(metricNames[i])).arg(values[i]));
    metrics_[i]->setEnabled(values[i] > 0);
  }
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
  if (busy_)
    return;
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

void MainWindow::configureCsv() {
  if (busy_)
    return;
  QDialog dialog{this};
  dialog.setWindowTitle("CSV column mappings");
  QFormLayout layout{&dialog};
  QLineEdit timestamp{QString::fromStdString(csvColumns_.timestamp)};
  QLineEdit level{QString::fromStdString(csvColumns_.level)};
  QLineEdit source{QString::fromStdString(csvColumns_.source)};
  QLineEdit message{QString::fromStdString(csvColumns_.message)};
  source.setPlaceholderText("Use filename if blank");
  layout.addRow("&Timestamp", &timestamp);
  layout.addRow("&Severity", &level);
  layout.addRow("So&urce", &source);
  layout.addRow("&Message", &message);
  QLabel error{"Use distinct column names. Only Source can be blank."};
  error.hide();
  layout.addRow(&error);
  QDialogButtonBox buttons{QDialogButtonBox::Ok | QDialogButtonBox::Cancel};
  layout.addRow(&buttons);
  connect(&buttons, &QDialogButtonBox::accepted, &dialog, [&] {
    parsers::CsvColumns columns{timestamp.text().toStdString(), level.text().toStdString(),
                                source.text().toStdString(), message.text().toStdString()};
    if (!columns.valid()) {
      error.show();
      return;
    }
    csvColumns_ = std::move(columns);
    dialog.accept();
  });
  connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  dialog.exec();
}

void MainWindow::configureRule() {
  QDialog dialog{this};
  dialog.setWindowTitle("Burst alert rules");
  QFormLayout layout{&dialog};
  auto draft = activeRules_;
  QComboBox chooser;
  chooser.setObjectName("burstRuleChooser");
  auto* actions = new QHBoxLayout;
  QPushButton add{"Add rule"};
  add.setObjectName("addBurstRule");
  QPushButton remove{"Remove rule"};
  actions->addWidget(&add);
  actions->addWidget(&remove);
  QLineEdit name;
  name.setObjectName("burstRuleName");
  QSpinBox threshold;
  threshold.setObjectName("burstThreshold");
  threshold.setRange(2, static_cast<int>(parsers::ImportBudget::maxLines));
  QSpinBox seconds;
  seconds.setObjectName("burstWindow");
  seconds.setRange(1, 86400);
  QComboBox severity;
  severity.setObjectName("burstSeverity");
  severity.addItems({"Trace", "Debug", "Info", "Warning", "Error", "Critical"});
  QLineEdit source;
  source.setObjectName("burstSource");
  source.setPlaceholderText("All sources if blank");
  layout.addRow("&Rule", &chooser);
  layout.addRow(actions);
  layout.addRow("&Name", &name);
  layout.addRow("&Event count", &threshold);
  layout.addRow("&Window (seconds)", &seconds);
  layout.addRow("&Minimum severity", &severity);
  layout.addRow("&Source (exact match)", &source);
  QLabel error;
  error.setWordWrap(true);
  error.hide();
  layout.addRow(&error);
  int selected = 0;
  const auto save = [&] {
    draft[static_cast<std::size_t>(selected)] = {
        name.text().trimmed(),
        {.threshold = static_cast<std::size_t>(threshold.value()),
         .window = std::chrono::seconds{seconds.value()},
         .minimumLevel = static_cast<core::LogLevel>(severity.currentIndex()),
         .source = source.text().toStdString()}};
    chooser.setItemText(selected, name.text().trimmed());
  };
  const auto open = [&](int index) {
    selected = index;
    const auto& entry = draft[static_cast<std::size_t>(index)];
    name.setText(entry.name);
    threshold.setValue(static_cast<int>(entry.rule.threshold));
    seconds.setValue(static_cast<int>(entry.rule.window.count()));
    severity.setCurrentIndex(static_cast<int>(entry.rule.minimumLevel));
    source.setText(QString::fromStdString(entry.rule.source));
    remove.setEnabled(draft.size() > 1);
    add.setEnabled(draft.size() < 16);
  };
  for (const auto& entry : draft)
    chooser.addItem(entry.name);
  open(0);
  connect(&chooser, &QComboBox::currentIndexChanged, &dialog, [&](int index) {
    save();
    open(index);
  });
  connect(&add, &QPushButton::clicked, &dialog, [&] {
    save();
    const QSignalBlocker blocker{chooser};
    draft.push_back({QString{"New rule %1"}.arg(draft.size() + 1), {}});
    chooser.addItem(draft.back().name);
    chooser.setCurrentIndex(chooser.count() - 1);
    open(chooser.currentIndex());
    name.setFocus();
    name.selectAll();
  });
  connect(&remove, &QPushButton::clicked, &dialog, [&] {
    const QSignalBlocker blocker{chooser};
    draft.erase(draft.begin() + selected);
    chooser.removeItem(selected);
    open(chooser.currentIndex());
  });
  QDialogButtonBox buttons{QDialogButtonBox::Ok | QDialogButtonBox::Cancel};
  layout.addRow(&buttons);
  connect(&buttons, &QDialogButtonBox::accepted, &dialog, [&] {
    save();
    std::set<QString> names;
    for (const auto& entry : draft) {
      if (entry.name.isEmpty() || !names.insert(entry.name.toCaseFolded()).second) {
        error.setText("Give each rule a distinct, non-empty name.");
        error.show();
        return;
      }
    }
    dialog.accept();
  });
  connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  if (dialog.exec() != QDialog::Accepted)
    return;
  activeRules_ = std::move(draft);
  if (proxy_->activeFilters().contains("alert")) {
    proxy_->setBurst({});
    refreshFilterChips();
  }
  if (dataset_)
    bursts_ = evaluate_rules(dataset_->entries, activeRules_);
  refreshBursts();
}

void MainWindow::refreshBursts() {
  ruleDescription_->setText(QString{"Active rules: %1"}.arg(activeRules_.size()));
  alerts_->clear();
  alerts_->setVisible(!bursts_.empty());
  openBurst_->setVisible(!bursts_.empty());
  noBursts_->setText(dataset_ ? "No bursts found." : "Import logs to check for error bursts.");
  noBursts_->setVisible(bursts_.empty());
  for (const auto& burst : bursts_) {
    alerts_->addItem(
        QString("%1 · %2 · %3 → %4")
            .arg(burst.ruleName,
                 QString::fromStdString(burst.entries.front()->source.name).left(80),
                 QString::fromStdString(app::format_timestamp(burst.entries.front()->timestamp)),
                 QString::fromStdString(app::format_timestamp(burst.entries.back()->timestamp))));
  }
}

void MainWindow::populateViewsMenu(QMenu& menu) {
  for (auto* action : menu.actions()) {
    if (auto* submenu = action->menu())
      delete submenu;
  }
  menu.clear();
  auto* save = menu.addAction("Save current filters…", this, &MainWindow::saveView);
  const auto filters = proxy_->activeFilters();
  save->setEnabled(!filters.contains("alert") && !bookmarksOnly_ &&
                   (!filters.empty() || !search_->text().isEmpty()));
  auto* bookmarks = menu.addAction(QString("Bookmarks only (%1)").arg(bookmarks_.size()));
  bookmarks->setObjectName("bookmarksOnly");
  bookmarks->setCheckable(true);
  bookmarks->setChecked(bookmarksOnly_);
  bookmarks->setEnabled(dataset_ != nullptr);
  connect(bookmarks, &QAction::toggled, this, &MainWindow::setBookmarkScope);
  QSettings settings{QSettings::IniFormat, QSettings::UserScope, "LogInsight", "LogInsight"};
  const auto views = settings.value(savedViewsKey).toMap();
  if (views.empty())
    return;
  auto* apply = menu.addMenu("Apply saved filters");
  auto* remove = menu.addMenu("Delete saved filters");
  for (auto it = views.cbegin(); it != views.cend(); ++it) {
    const auto name = it.key();
    const auto filtersToApply = it.value().toMap();
    auto caption = name;
    caption.replace("&", "&&");
    apply->addAction(caption, this, [this, filtersToApply] { applyView(filtersToApply); });
    remove->addAction(caption, this, [this, name] {
      if (QMessageBox::question(this, "Delete saved filters",
                                QString{"Delete the saved view “%1”?"}.arg(name.toHtmlEscaped())) !=
          QMessageBox::Yes)
        return;
      QSettings writable{QSettings::IniFormat, QSettings::UserScope, "LogInsight", "LogInsight"};
      auto latest = writable.value(savedViewsKey).toMap();
      latest.remove(name);
      writable.setValue(savedViewsKey, latest);
      writable.sync();
      if (writable.status() != QSettings::NoError)
        QMessageBox::warning(this, "Saved views", "Could not update the saved views file.");
    });
  }
}

void MainWindow::saveView() {
  if (bookmarksOnly_ || proxy_->activeFilters().contains("alert"))
    return;
  proxy_->setSearch(search_->text());
  refreshFilterChips();
  QVariantMap filters;
  for (const auto& [key, value] : proxy_->activeFilters())
    filters.insert(QString::fromStdString(key), QString::fromStdString(value));
  if (filters.empty())
    return;
  bool accepted{};
  const auto name = QInputDialog::getText(this, "Save current filters", "View name",
                                          QLineEdit::Normal, {}, &accepted)
                        .trimmed();
  if (!accepted || name.isEmpty())
    return;
  QSettings settings{QSettings::IniFormat, QSettings::UserScope, "LogInsight", "LogInsight"};
  auto views = settings.value(savedViewsKey).toMap();
  if (views.contains(name)) {
    QMessageBox::warning(this, "Saved views", "A view with this name already exists.");
    return;
  }
  views.insert(name, filters);
  settings.setValue(savedViewsKey, views);
  settings.sync();
  if (settings.status() != QSettings::NoError)
    QMessageBox::warning(this, "Saved views", "Could not update the saved views file.");
}

void MainWindow::applyView(const QVariantMap& filters) {
  resetExplorerFilters();
  search_->setText(filters.value("message_contains").toString());
  proxy_->setSearch(search_->text());
  const auto source = filters.value("source").toString();
  if (!source.isEmpty()) {
    int index = source_->findData(source);
    if (index < 0) {
      source_->addItem(source, source);
      index = source_->count() - 1;
    }
    source_->setCurrentIndex(index);
  }
  const auto severity = filters.value("severity").toString().toStdString();
  if (severity == "error_or_critical")
    severity_->setCurrentIndex(severity_->findData(LogFilter::errorAndCritical));
  else if (const auto level = core::log_level_from_string(severity))
    severity_->setCurrentIndex(severity_->findData(static_cast<int>(*level)));
  for (const auto* key : {"trace_id", "request_id"}) {
    const auto id = filters.value(key).toString();
    if (!id.isEmpty()) {
      proxy_->setCorrelation(key, id.toStdString());
      break;
    }
  }
  from_->setText(filters.value("from_utc").toString());
  through_->setText(filters.value("through_utc").toString());
  applyTimeRange();
}

void MainWindow::setBookmarkScope(bool enabled) {
  bookmarksOnly_ = enabled;
  proxy_->setBookmarks(bookmarks_, enabled);
  refreshFilterChips();
}

void MainWindow::toggleBookmark() {
  const auto* entry = model_->entry(proxy_->mapToSource(table_->currentIndex()).row());
  if (!entry)
    return;
  if (!bookmarks_.erase(entry))
    bookmarks_.insert(entry);
  if (bookmarksOnly_)
    setBookmarkScope(true);
  showDetails(table_->currentIndex());
}

void MainWindow::resetExplorerFilters() {
  proxy_->setBurst({});
  proxy_->setCorrelation({}, {});
  search_->clear();
  proxy_->setSearch({});
  severity_->setCurrentIndex(0);
  source_->setCurrentIndex(0);
  setBookmarkScope(false);
  clearTimeRange();
}

void MainWindow::showBurst(int index) {
  if (index < 0 || static_cast<std::size_t>(index) >= bursts_.size())
    return;
  resetExplorerFilters();
  proxy_->setBurst(bursts_[static_cast<std::size_t>(index)].entries, alerts_->item(index)->text());
  refreshFilterChips();
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
  refreshFilterChips();
  table_->setCurrentIndex(proxy_->index(0, 0));
  table_->setFocus();
}

void MainWindow::refreshFilterChips() {
  const auto filters = proxy_->activeFilters();
  const auto value = [&](const char* key) -> QString {
    const auto found = filters.find(key);
    return found == filters.end() ? QString{} : QString::fromStdString(found->second);
  };
  const auto search = value("message_contains");
  const auto severity = value("severity");
  const auto source = value("source");
  const auto from = value("from_utc");
  const auto through = value("through_utc");
  const auto alert = value("alert");
  const auto trace = value("trace_id");
  const auto request = value("request_id");
  const bool bookmarksOnly = filters.contains("bookmarked_only");
  const std::array labels{search.isEmpty() ? QString{} : QString{"Search: %1"}.arg(search),
                          severity.isEmpty() ? QString{} : severity_->currentText(),
                          source,
                          from.isEmpty() && through.isEmpty() ? QString{} : QString{"Time range"},
                          !trace.isEmpty()     ? QString{"Trace: %1"}.arg(trace)
                          : !request.isEmpty() ? QString{"Request: %1"}.arg(request)
                          : alert.isEmpty()    ? QString{}
                                               : QString{"Burst alert"},
                          bookmarksOnly ? QString{"Bookmarks"} : QString{}};
  const std::array tooltips{
      labels[0],
      labels[1],
      labels[2],
      QString{"From %1 · Through %2"}.arg(from.isEmpty() ? QString{"any time"} : from,
                                          through.isEmpty() ? QString{"any time"} : through),
      alert.isEmpty() ? labels[4] : alert,
      labels[5]};
  for (std::size_t i = 0; i < chips_.size(); ++i) {
    auto* chip = chips_[i];
    chip->setVisible(!labels[i].isEmpty());
    auto shortLabel = QFontMetrics{chip->font()}.elidedText(labels[i], Qt::ElideRight, 72);
    chip->setText(shortLabel.replace("&", "&&") + " ×");
    const QString description = QString{"Remove filter: %1"}.arg(tooltips[i]);
    chip->setAccessibleName(description);
    chip->setToolTip("<qt>" + description.toHtmlEscaped() + "</qt>");
  }
  chipRow_->setVisible(!filters.empty());
}

void MainWindow::clearTimeRange() {
  from_->clear();
  through_->clear();
  applyTimeRange();
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
  if (error.isEmpty()) {
    for (auto* button : timelineButtons_)
      button->setChecked(false);
    proxy_->setTimeRange(from, through);
    refreshFilterChips();
  }
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
  bookmark_->setEnabled(entry != nullptr);
  bookmark_->setText(entry && bookmarks_.contains(entry) ? "Remove bookmark" : "Bookmark");
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
namespace {
class PreviewBuffer final : public std::streambuf {
public:
  PreviewBuffer() { setp(buffer_.data(), buffer_.data() + buffer_.size()); }
  std::string_view text() const {
    return {buffer_.data(), static_cast<std::size_t>(pptr() - pbase())};
  }
  bool truncated{};

protected:
  int_type overflow(int_type) override {
    truncated = true;
    return traits_type::eof();
  }

private:
  std::array<char, 16 * 1024> buffer_{};
};
} // namespace

std::vector<const core::LogEntry*> MainWindow::visibleEntries() const {
  std::vector<const core::LogEntry*> entries;
  entries.reserve(static_cast<std::size_t>(proxy_->rowCount()));
  for (int row = 0; row < proxy_->rowCount(); ++row)
    entries.push_back(model_->entry(proxy_->mapToSource(proxy_->index(row, 0)).row()));
  return entries;
}

void MainWindow::exportReport(bool visibleOnly, bool redact) {
  if (busy_ || !dataset_)
    return;
  std::vector<std::string> terms;
  if (redact) {
    QDialog dialog{this};
    dialog.setWindowTitle("Redact and preview JSON");
    dialog.resize(660, 520);
    QVBoxLayout layout{&dialog};
    QLabel instructions{"Enter exact values to mask, one per line. Raw record text is removed "
                        "from redacted reports. Review before sharing."};
    instructions.setWordWrap(true);
    layout.addWidget(&instructions);
    QPlainTextEdit input;
    input.setObjectName("redactionTerms");
    input.setAccessibleName("Values to redact");
    input.setPlaceholderText("One case-sensitive value per line");
    input.setMaximumHeight(110);
    layout.addWidget(&input);
    QPushButton preview{"Preview redacted report"};
    preview.setObjectName("previewRedaction");
    layout.addWidget(&preview);
    QPlainTextEdit output;
    output.setObjectName("redactionPreview");
    output.setAccessibleName("Redacted report preview");
    output.setReadOnly(true);
    layout.addWidget(&output, 1);
    QDialogButtonBox buttons{QDialogButtonBox::Save | QDialogButtonBox::Cancel};
    auto* save = buttons.button(QDialogButtonBox::Save);
    save->setEnabled(false);
    layout.addWidget(&buttons);
    if (visibleOnly) {
      proxy_->setSearch(search_->text());
      refreshFilterChips();
    }
    const auto entries = visibleOnly ? visibleEntries() : std::vector<const core::LogEntry*>{};
    const auto filters = proxy_->activeFilters();
    connect(&input, &QPlainTextEdit::textChanged, &dialog, [&] {
      save->setEnabled(false);
      output.clear();
    });
    connect(&preview, &QPushButton::clicked, &dialog, [&] {
      terms.clear();
      for (const auto& value : input.toPlainText().split('\n', Qt::SkipEmptyParts))
        terms.push_back(value.toStdString());
      if (terms.empty() || terms.size() > 32) {
        output.setPlainText("Enter between 1 and 32 values.");
        save->setEnabled(false);
        return;
      }
      PreviewBuffer buffer;
      std::ostream stream{&buffer};
      stream.exceptions(std::ios::badbit | std::ios::failbit);
      try {
        if (visibleOnly)
          app::write_visible_report(stream, entries, filters, terms);
        else
          app::write_json_report(stream, dataset_->files, terms);
      } catch (const std::exception&) {
        if (!buffer.truncated) {
          output.setPlainText("Could not generate the preview.");
          save->setEnabled(false);
          return;
        }
      }
      output.setPlainText(
          QString::fromUtf8(buffer.text().data(), static_cast<qsizetype>(buffer.text().size())) +
          (buffer.truncated ? "\n[Preview limited to the first 16 KiB.]" : ""));
      save->setEnabled(true);
    });
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted)
      return;
  }
  const auto path = QFileDialog::getSaveFileName(
      this, visibleOnly ? "Export filtered results" : "Export full session", {},
      "JSON report (*.json)", nullptr, QFileDialog::DontConfirmOverwrite);
  if (!path.isEmpty())
    exportTo(path, visibleOnly, std::move(terms));
}

void MainWindow::exportTo(const QString& path, bool visibleOnly,
                          std::vector<std::string> redactions) {
  if (busy_ || !dataset_ || path.isEmpty())
    return;
  const auto snapshot = dataset_;
  auto entries = visibleOnly ? visibleEntries() : std::vector<const core::LogEntry*>{};
  auto filters = visibleOnly ? proxy_->activeFilters() : core::LogAttributes{};
  progress_->setRange(0, 0);
  setBusy(true, visibleOnly ? "Exporting visible records…" : "Exporting session…");
  exporter_.setFuture(
      QtConcurrent::run([snapshot, path, visibleOnly, entries = std::move(entries),
                         filters = std::move(filters), terms = std::move(redactions)] {
        try {
          const std::filesystem::path destination{path.toStdString()};
          if (visibleOnly)
            app::save_visible_report(destination, entries, filters, terms);
          else
            app::save_json_report(destination, snapshot->files, terms);
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
void MainWindow::resizeEvent(QResizeEvent* event) {
  QMainWindow::resizeEvent(event);
  if (!splitter_)
    return;
  const auto orientation = event->size().width() >= 1100 ? Qt::Horizontal : Qt::Vertical;
  if (splitter_->orientation() != orientation) {
    splitter_->setOrientation(orientation);
    splitter_->setSizes(orientation == Qt::Horizontal ? QList<int>{720, 300}
                                                      : QList<int>{390, 150});
  }
}

void MainWindow::closeEvent(QCloseEvent* event) {
  if (busy_) {
    status_->setText("Please wait for the current import or export before closing.");
    event->ignore();
  } else
    QMainWindow::closeEvent(event);
}
} // namespace loginsight::gui
