#include "large_log_panel.hpp"
#include "loginsight/app/analysis_service.hpp"

#include <QHeaderView>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>
#include <sqlite3.h>
#include <stdexcept>
#include <utility>

namespace loginsight::gui {
namespace {
constexpr qint64 pageSize = 200;
using Database = std::unique_ptr<sqlite3, decltype(&sqlite3_close)>;
using Statement = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

void check(sqlite3* database, int result) {
  if (result != SQLITE_OK && result != SQLITE_ROW && result != SQLITE_DONE)
    throw std::runtime_error(database ? sqlite3_errmsg(database) : "cannot allocate log index");
}

Database open_index(const QString& path, int flags) {
  sqlite3* raw{};
  const auto result = sqlite3_open_v2(path.toUtf8().constData(), &raw, flags, nullptr);
  Database database{raw, sqlite3_close};
  check(database.get(), result);
  return database;
}

Statement prepare(sqlite3* database, const char* sql) {
  sqlite3_stmt* raw{};
  const auto result = sqlite3_prepare_v2(database, sql, -1, &raw, nullptr);
  Statement statement{raw, sqlite3_finalize};
  check(database, result);
  return statement;
}

void bind_text(sqlite3_stmt* statement, int column, std::string_view value) {
  const char* text = value.empty() ? "" : value.data();
  check(sqlite3_db_handle(statement),
        sqlite3_bind_text64(statement, column, text, value.size(), SQLITE_TRANSIENT, SQLITE_UTF8));
}

QString text_at(sqlite3_stmt* statement, int column) {
  const auto* bytes = reinterpret_cast<const char*>(sqlite3_column_text(statement, column));
  return QString::fromUtf8(bytes, sqlite3_column_bytes(statement, column));
}
} // namespace

LargeLogPanel::Session LargeLogPanel::buildIndex(const QStringList& paths,
                                                 parsers::JsonParserConfig json,
                                                 parsers::CsvColumns csv, std::stop_token stop) {
  try {
    Session index;
    index.directory = std::make_shared<QTemporaryDir>();
    if (!index.directory->isValid())
      throw std::runtime_error("could not create a temporary directory for the log index");
    const auto db = open_index(index.directory->filePath("logs.sqlite"),
                               SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    check(db.get(), sqlite3_exec(db.get(),
                                 "PRAGMA cache_size=-4096; BEGIN;"
                                 "CREATE TABLE logs (id INTEGER PRIMARY KEY, timestamp TEXT,"
                                 "level TEXT, source TEXT, message TEXT, path TEXT, raw TEXT);",
                                 nullptr, nullptr, nullptr));
    const auto insert = prepare(db.get(), "INSERT INTO logs VALUES (NULL, ?, ?, ?, ?, ?, ?)");
    const app::AnalysisService analysis{std::move(json), std::move(csv)};
    parsers::ImportBudget budget{.stop = stop, .progress = {}, .enforceBatchLimits = false};
    budget.onMalformed = [&index](const auto&) { ++index.malformed; };
    for (const auto& path : paths) {
      budget.checkCancelled();
      const auto file = path.toStdString();
      budget.onEntry = [&](const core::LogEntry& entry) {
        bind_text(insert.get(), 1, app::format_timestamp(entry.timestamp));
        bind_text(insert.get(), 2, core::to_string(entry.level));
        bind_text(insert.get(), 3, entry.source.name);
        bind_text(insert.get(), 4, entry.message);
        bind_text(insert.get(), 5, file);
        bind_text(insert.get(), 6, entry.rawContent);
        check(db.get(), sqlite3_step(insert.get()));
        check(db.get(), sqlite3_reset(insert.get()));
        ++index.entries;
      };
      const auto result = analysis.analyze(std::filesystem::path{file}, &budget);
      if (!result.completed())
        throw std::runtime_error(file + ": " + result.diagnostic);
      index.malformed += static_cast<qint64>(result.parsed.errors.size());
    }
    budget.checkCancelled();
    check(db.get(), sqlite3_exec(db.get(), "COMMIT", nullptr, nullptr, nullptr));
    return index;
  } catch (const std::exception& error) {
    return {.directory = {}, .error = QString::fromUtf8(error.what())};
  }
}

LargeLogPanel::LargeLogPanel(const std::function<void()>& chooseFiles) {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(18, 14, 18, 14);
  auto* title = new QLabel{"Large logs"};
  title->setStyleSheet("font-size: 25px; font-weight: 600;");
  layout->addWidget(title);
  status_ = new QLabel{"Index a batch on disk and browse 200 records at a time in file order."};
  status_->setObjectName("largeLogStatus");
  status_->setTextFormat(Qt::PlainText);
  status_->setWordWrap(true);
  layout->addWidget(status_);
  auto* actions = new QHBoxLayout;
  open_ = new QPushButton{"Index log files…"};
  cancel_ = new QPushButton{"Cancel indexing"};
  cancel_->hide();
  previous_ = new QPushButton{"Previous"};
  previous_->setObjectName("largePrevious");
  previous_->setEnabled(false);
  next_ = new QPushButton{"Next"};
  next_->setObjectName("largeNext");
  next_->setEnabled(false);
  page_ = new QLabel{"No index"};
  actions->addWidget(open_);
  actions->addWidget(cancel_);
  actions->addStretch();
  actions->addWidget(previous_);
  actions->addWidget(page_);
  actions->addWidget(next_);
  layout->addLayout(actions);
  table_ = new QTableWidget{0, 4};
  table_->setObjectName("largeLogTable");
  table_->setAccessibleName("Paged log records");
  table_->setHorizontalHeaderLabels({"Timestamp · UTC", "Severity", "Source", "Message"});
  table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  table_->setSelectionMode(QAbstractItemView::SingleSelection);
  table_->setAlternatingRowColors(true);
  table_->setWordWrap(false);
  table_->verticalHeader()->hide();
  table_->verticalHeader()->setDefaultSectionSize(28);
  table_->setColumnWidth(0, 190);
  table_->setColumnWidth(1, 85);
  table_->setColumnWidth(2, 105);
  table_->horizontalHeader()->setStretchLastSection(true);
  layout->addWidget(table_, 3);
  details_ = new QPlainTextEdit;
  details_->setObjectName("largeLogDetails");
  details_->setAccessibleName("Original indexed record");
  details_->setReadOnly(true);
  details_->setPlaceholderText("Select a row to see its original record.");
  layout->addWidget(details_, 1);

  connect(open_, &QPushButton::clicked, this, chooseFiles);
  connect(cancel_, &QPushButton::clicked, this, [this] {
    stop_.request_stop();
    cancel_->setEnabled(false);
    status_->setText("Cancelling indexing…");
  });
  connect(previous_, &QPushButton::clicked, this, [this] { showPage(offset_ - pageSize); });
  connect(next_, &QPushButton::clicked, this, [this] { showPage(offset_ + pageSize); });
  connect(table_, &QTableWidget::currentCellChanged, this, [this] { showRecord(); });
  connect(&worker_, &QFutureWatcher<Session>::finished, this, [this] {
    open_->setEnabled(true);
    cancel_->hide();
    auto result = worker_.result();
    if (stop_.stop_requested()) {
      status_->setText("Indexing cancelled. The previous index was kept.");
      return;
    }
    if (!result.directory) {
      status_->setText("Indexing failed: " + result.error);
      return;
    }
    session_ = std::move(result);
    status_->setText(QString{"%1 indexed records · %2 malformed records skipped"}
                         .arg(session_.entries)
                         .arg(session_.malformed));
    showPage(0);
  });
}

LargeLogPanel::~LargeLogPanel() { stop_.request_stop(); }

void LargeLogPanel::openFiles(QStringList paths, parsers::JsonParserConfig json,
                              parsers::CsvColumns csv) {
  if (!open_->isEnabled() || paths.isEmpty())
    return;
  paths.removeDuplicates();
  open_->setEnabled(false);
  cancel_->setEnabled(true);
  cancel_->show();
  status_->setText("Building temporary log index…");
  stop_ = std::stop_source{};
  worker_.setFuture(QtConcurrent::run(&LargeLogPanel::buildIndex, std::move(paths), std::move(json),
                                      std::move(csv), stop_.get_token()));
}

void LargeLogPanel::showPage(qint64 offset) {
  if (!session_.directory || offset < 0 || (offset > 0 && offset >= session_.entries))
    return;
  try {
    const auto db = open_index(session_.directory->filePath("logs.sqlite"), SQLITE_OPEN_READONLY);
    const auto query =
        prepare(db.get(), "SELECT id, timestamp, level, substr(source,1,512),"
                          "substr(message,1,512) FROM logs WHERE id > ? ORDER BY id LIMIT ?");
    check(db.get(), sqlite3_bind_int64(query.get(), 1, offset));
    check(db.get(), sqlite3_bind_int64(query.get(), 2, pageSize));
    const QSignalBlocker block{table_};
    table_->clearContents();
    table_->setRowCount(0);
    details_->clear();
    int code{};
    while ((code = sqlite3_step(query.get())) == SQLITE_ROW) {
      const int row = table_->rowCount();
      table_->insertRow(row);
      for (int column = 0; column < table_->columnCount(); ++column)
        table_->setItem(row, column, new QTableWidgetItem{text_at(query.get(), column + 1)});
      table_->item(row, 0)->setData(Qt::UserRole,
                                    static_cast<qlonglong>(sqlite3_column_int64(query.get(), 0)));
    }
    check(db.get(), code);
    offset_ = offset;
    previous_->setEnabled(offset_ > 0);
    next_->setEnabled(session_.entries - offset_ > pageSize);
    page_->setText(session_.entries > 0 ? QString{"%1–%2 of %3"}
                                              .arg(offset_ + 1)
                                              .arg(offset_ + table_->rowCount())
                                              .arg(session_.entries)
                                        : QString{"0 records"});
  } catch (const std::exception& error) {
    status_->setText("Could not read this page: " + QString::fromUtf8(error.what()));
  }
}

void LargeLogPanel::showRecord() {
  const auto* selected = table_->item(table_->currentRow(), 0);
  if (!session_.directory || !selected) {
    details_->clear();
    return;
  }
  try {
    const auto db = open_index(session_.directory->filePath("logs.sqlite"), SQLITE_OPEN_READONLY);
    const auto query = prepare(db.get(), "SELECT path, raw FROM logs WHERE id = ?");
    check(db.get(), sqlite3_bind_int64(query.get(), 1, selected->data(Qt::UserRole).toLongLong()));
    const auto code = sqlite3_step(query.get());
    check(db.get(), code);
    if (code == SQLITE_ROW)
      details_->setPlainText(text_at(query.get(), 0) + "\n\n" + text_at(query.get(), 1));
  } catch (const std::exception& error) {
    status_->setText("Could not read this record: " + QString::fromUtf8(error.what()));
  }
}

} // namespace loginsight::gui
