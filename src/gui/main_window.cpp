#include "main_window.hpp"
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMimeData>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
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
  header->addWidget(export_);
  outer->addLayout(header);
  auto* body = new QHBoxLayout;
  body->setSpacing(0);
  auto* sidebar = new QWidget;
  sidebar->setFixedWidth(186);
  auto* side = new QVBoxLayout(sidebar);
  side->setContentsMargins(0, 0, 0, 12);
  auto* navigation = new QListWidget;
  navigation->addItems({"Overview", "Log Explorer"});
  navigation->setCurrentRow(0);
  navigation->setObjectName("navigation");
  side->addWidget(navigation);
  auto* sideNote = label("  PORTFOLIO MVP\n\n  Plain-text analysis\n  Local processing",
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
  overviewText_ = label("Start with the sample session, or import .log and .txt files. You can "
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
  statusBar()->addPermanentWidget(progress_);
  session_ = label("Files: 0    Entries: 0");
  statusBar()->addPermanentWidget(session_);
  connect(navigation, &QListWidget::currentRowChanged, pages_, &QStackedWidget::setCurrentIndex);
  connect(samples_, &QPushButton::clicked, this, &MainWindow::loadSamples);
  connect(import_, &QPushButton::clicked, this, [this] {
    const auto paths =
        QFileDialog::getOpenFileNames(this, "Import logs — replaces current session", {},
                                      "Plain-text logs (*.log *.txt);;All files (*)");
    importFiles(paths);
  });
  connect(export_, &QPushButton::clicked, this, &MainWindow::exportReport);
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
    showDetails(table_->currentIndex());
  };
  connect(proxy_, &QAbstractItemModel::modelReset, this, updateCount);
  connect(proxy_, &QAbstractItemModel::layoutChanged, this, updateCount);
  connect(proxy_, &QAbstractItemModel::rowsInserted, this, updateCount);
  connect(proxy_, &QAbstractItemModel::rowsRemoved, this, updateCount);
  connect(table_->selectionModel(), &QItemSelectionModel::currentRowChanged, this,
          [this](const QModelIndex& index) { showDetails(index); });
  connect(&importer_, &QFutureWatcher<std::shared_ptr<const Dataset>>::finished, this, [this] {
    try {
      auto next = importer_.result();
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
  importer_.setFuture(QtConcurrent::run([unique] { return load_files(unique); }));
}
void MainWindow::loadSamples() {
  importFiles({QApplication::applicationDirPath() + "/samples/atlas.log"});
}
void MainWindow::refresh() {
  model_->setDataset(dataset_);
  search_->clear();
  proxy_->setSearch({});
  severity_->setCurrentIndex(0);
  proxy_->setSeverity(-1);
  {
    const QSignalBlocker blocker{source_};
    source_->clear();
    source_->addItem("All sources");
    source_->addItems(dataset_->sources);
  }
  proxy_->setSource({});
  details_->clear();
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
void MainWindow::showDetails(const QModelIndex& index) {
  const auto mapped = proxy_->mapToSource(index);
  const auto* entry = model_->entry(mapped.row());
  if (!index.isValid() || !entry) {
    details_->clear();
    return;
  }
  details_->setPlainText(QString::fromStdString(
      app::format_timestamp(entry->timestamp) + "   " + std::string{core::to_string(entry->level)} +
      "   " + entry->source.name + "\nFile: " + entry->source.file_path.value_or("") + "\n\n" +
      entry->message + "\n\nRAW RECORD\n" + entry->rawContent));
}
void MainWindow::exportReport() {
  if (busy_ || !dataset_)
    return;
  const auto path = QFileDialog::getSaveFileName(this, "Export session — choose a new file", {},
                                                 "JSON report (*.json)", nullptr,
                                                 QFileDialog::DontConfirmOverwrite);
  if (path.isEmpty())
    return;
  exportTo(path);
}
void MainWindow::exportTo(const QString& path) {
  if (busy_ || !dataset_ || path.isEmpty())
    return;
  const auto snapshot = dataset_;
  setBusy(true, "Exporting session…");
  exporter_.setFuture(QtConcurrent::run([snapshot, path] {
    try {
      app::save_json_report(std::filesystem::path{path.toStdString()}, snapshot->files);
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
