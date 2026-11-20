#include "main_window.hpp"
#include <QAbstractItemModelTester>
#include <QAction>
#include <QClipboard>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSignalSpy>
#include <QSpinBox>
#include <QSplitter>
#include <QTableView>
#include <QTemporaryDir>
#include <QTest>
#include <QTextEdit>
#include <QTimer>
#include <fstream>

using namespace loginsight::gui;
class GuiTest final : public QObject {
  Q_OBJECT
private slots:
  void modelAndFilters() {
    LogModel model;
    QAbstractItemModelTester tester{&model, QAbstractItemModelTester::FailureReportingMode::QtTest};
    model.setDataset(load_files({LOGINSIGHT_SAMPLE_PATH}));
    QCOMPARE(model.rowCount(), 13);
    QCOMPARE(model.columnCount(), 4);
    LogFilter proxy;
    proxy.setSourceModel(&model);
    QAbstractItemModelTester proxyTester{&proxy,
                                         QAbstractItemModelTester::FailureReportingMode::QtTest};
    QCOMPARE(proxy.rowCount(), 13);
    proxy.setSeverity(4);
    QCOMPARE(proxy.rowCount(), 2);
    proxy.setSearch("DATABASE");
    QCOMPARE(proxy.rowCount(), 1);
    proxy.setSource("catalog");
    QCOMPARE(proxy.rowCount(), 0);
    proxy.setSearch({});
    proxy.setSeverity(-1);
    QCOMPARE(proxy.rowCount(), 3);
    proxy.setSource({});
    proxy.sort(0, Qt::DescendingOrder);
    QCOMPARE(proxy.index(0, 0).data().toString(), QString{"2026-10-20T09:00:40Z"});
  }
  void asynchronousImportAndFailurePreserveSession() {
    MainWindow window;
    window.show();
    QCOMPARE(window.logModel()->rowCount(), 0);
    window.importFiles({LOGINSIGHT_SAMPLE_PATH});
    QVERIFY(window.busy());
    QTRY_VERIFY_WITH_TIMEOUT(!window.busy(), 10000);
    QCOMPARE(window.logModel()->rowCount(), 13);
    QVERIFY(window.findChild<QTextEdit*>("diagnostics")->toPlainText().contains("Line 14"));
    const auto capture = qEnvironmentVariable("LOGINSIGHT_CAPTURE_DIR");
    if (!capture.isEmpty()) {
      QApplication::processEvents();
      QVERIFY(window.grab().save(capture + "/overview.png"));
    }
    window.findChild<QListWidget*>("navigation")->setCurrentRow(1);
    auto* table = window.findChild<QTableView*>("logTable");
    table->setCurrentIndex(table->model()->index(0, 0));
    QVERIFY(window.findChild<QTextEdit*>("details")->toPlainText().contains("RAW RECORD"));
    if (!capture.isEmpty()) {
      QApplication::processEvents();
      QVERIFY(window.grab().save(capture + "/explorer.png"));
    }
    table->setCurrentIndex(table->model()->index(10, 0));
    QVERIFY(window.findChild<QTextEdit*>("details")->toPlainText().contains("capacity exhausted"));
    for (auto* combo : window.findChildren<QComboBox*>()) {
      if (combo->accessibleName() == "Source filter") {
        combo->setCurrentIndex(combo->findText("worker"));
        QCOMPARE(table->model()->rowCount(), 5);
        QVERIFY(
            window.findChild<QTextEdit*>("details")->toPlainText().contains("capacity exhausted"));
        bool countUpdated = false;
        for (auto* text : window.findChildren<QLabel*>()) {
          if (text->text().startsWith("5 of 13 entries"))
            countUpdated = true;
        }
        QVERIFY(countUpdated);
        combo->setCurrentIndex(0);
      }
    }
    auto* search = window.findChild<QLineEdit*>("search");
    search->setText("no matching message");
    QTRY_COMPARE(table->model()->rowCount(), 0);
    search->clear();
    QTRY_COMPARE(table->model()->rowCount(), 13);
    window.importFiles({"/does/not/exist.log"});
    QTRY_VERIFY_WITH_TIMEOUT(!window.busy(), 10000);
    QCOMPARE(window.logModel()->rowCount(), 13);
    QVERIFY(window.findChild<QLabel*>("statusText")->text().contains("Import failed"));
  }
  void overviewLinksAndExplorerControls() {
    MainWindow window;
    window.show();
    window.importFiles({LOGINSIGHT_SAMPLE_PATH});
    QTRY_VERIFY_WITH_TIMEOUT(!window.busy(), 10000);
    auto* navigation = window.findChild<QListWidget*>("navigation");
    auto* table = window.findChild<QTableView*>("logTable");
    auto* splitter = window.findChild<QSplitter*>("explorerSplit");
    QCOMPARE(splitter->orientation(), Qt::Horizontal);

    window.findChild<QPushButton*>("malformedMetric")->click();
    QCOMPARE(navigation->currentRow(), 3);
    QTRY_VERIFY(window.findChild<QTableView*>("malformedTable")->hasFocus());
    navigation->setCurrentRow(0);
    window.findChild<QPushButton*>("errorsMetric")->click();
    QCOMPARE(navigation->currentRow(), 1);
    QCOMPARE(table->model()->rowCount(), 3);
    QCOMPARE(window.findChild<QPushButton*>("exportFiltered")->text(),
             QString{"Export filtered results (3)"});
    auto* severity = window.findChild<QPushButton*>("severityChip");
    QVERIFY(severity->isVisible());
    severity->click();
    QCOMPARE(table->model()->rowCount(), 13);
    QVERIFY(!severity->isVisible());

    auto* bucket = window.findChild<QPushButton*>("timelineBucket0");
    bucket->click();
    QVERIFY(bucket->isChecked());
    auto* timeChip = window.findChild<QPushButton*>("timeChip");
    QVERIFY(timeChip->isVisible());
    timeChip->click();
    QVERIFY(!bucket->isChecked());
    QCOMPARE(table->model()->rowCount(), 13);

    auto* timeControls = window.findChild<QWidget*>("timeControls");
    QVERIFY(!timeControls->isVisible());
    auto* timeToggle = window.findChild<QPushButton*>("toggleTimeRange");
    timeToggle->click();
    QVERIFY(timeControls->isVisible());
    window.resize(840, 620);
    QCOMPARE(window.size(), QSize(840, 620));
    QCOMPARE(splitter->orientation(), Qt::Vertical);
    timeToggle->click();
    auto* search = window.findChild<QLineEdit*>("search");
    search->setText("atlas");
    QTRY_VERIFY(window.findChild<QPushButton*>("searchChip")->isVisible());
    window.findChild<QPushButton*>("clearFilters")->click();
    QCOMPARE(table->model()->rowCount(), 13);
    QVERIFY(search->text().isEmpty());

    const auto capture = qEnvironmentVariable("LOGINSIGHT_CAPTURE_DIR");
    if (!capture.isEmpty()) {
      table->setCurrentIndex(table->model()->index(0, 0));
      QApplication::processEvents();
      QVERIFY(window.grab().save(capture + "/explorer-compact.png"));
    }
  }
  void savesFiltersAcrossWindowsAndBookmarksWithinSession() {
    QTemporaryDir settingsDirectory;
    QVERIFY(settingsDirectory.isValid());
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory.path());
    {
      MainWindow window;
      window.show();
      window.importFiles({LOGINSIGHT_SAMPLE_PATH});
      QTRY_VERIFY_WITH_TIMEOUT(!window.busy(), 10000);
      window.findChild<QPushButton*>("errorsMetric")->click();
      auto* menu = window.findChild<QPushButton*>("savedViews")->menu();
      QVERIFY(QMetaObject::invokeMethod(menu, "aboutToShow"));
      QTimer::singleShot(0, &window, [&] {
        auto* dialog = window.findChild<QInputDialog*>();
        dialog->setTextValue("Incident errors");
        dialog->accept();
      });
      menu->actions().front()->trigger();
    }
    MainWindow window;
    window.show();
    window.importFiles({LOGINSIGHT_SAMPLE_PATH});
    QTRY_VERIFY_WITH_TIMEOUT(!window.busy(), 10000);
    window.findChild<QListWidget*>("navigation")->setCurrentRow(1);
    auto* menu = window.findChild<QPushButton*>("savedViews")->menu();
    QVERIFY(QMetaObject::invokeMethod(menu, "aboutToShow"));
    auto* apply = menu->actions()[2]->menu();
    QVERIFY(apply != nullptr);
    QCOMPARE(apply->actions().front()->text(), QString{"Incident errors"});
    apply->actions().front()->trigger();

    auto* table = window.findChild<QTableView*>("logTable");
    QCOMPARE(table->model()->rowCount(), 3);
    table->setCurrentIndex(table->model()->index(1, 0));
    auto* bookmark = window.findChild<QPushButton*>("bookmarkRecord");
    bookmark->click();
    QCOMPARE(bookmark->text(), QString{"Remove bookmark"});
    QVERIFY(QMetaObject::invokeMethod(menu, "aboutToShow"));
    menu->findChild<QAction*>("bookmarksOnly")->trigger();
    QCOMPARE(table->model()->rowCount(), 1);
    table->setCurrentIndex(table->model()->index(0, 0));
    bookmark->click();
    QCOMPARE(table->model()->rowCount(), 0);
    window.findChild<QPushButton*>("bookmarkChip")->click();
    QCOMPARE(table->model()->rowCount(), 3);
  }

  void browsesAndCopiesMalformedRecords() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto other = directory.filePath("other.log");
    std::ofstream{other.toStdString()} << "bad raw sample\n";
    MainWindow window;
    window.show();
    window.importFiles({LOGINSIGHT_SAMPLE_PATH, other});
    QTRY_VERIFY_WITH_TIMEOUT(!window.busy(), 10000);
    window.findChild<QPushButton*>("malformedMetric")->click();
    auto* table = window.findChild<QTableView*>("malformedTable");
    QCOMPARE(table->model()->rowCount(), 4);
    auto* files = window.findChild<QComboBox*>("malformedFile");
    files->setCurrentIndex(files->findData(QString{LOGINSIGHT_SAMPLE_PATH}));
    QCOMPARE(table->model()->rowCount(), 3);
    auto* reason = window.findChild<QLineEdit*>("malformedReason");
    reason->setText("missing message");
    QTRY_COMPARE(table->model()->rowCount(), 1);
    QCOMPARE(table->model()->index(0, 1).data().toInt(), 16);
    auto* raw = window.findChild<QPlainTextEdit*>("malformedRaw");
    QVERIFY(!raw->toPlainText().isEmpty());
    auto* copy = window.findChild<QPushButton*>("copyMalformed");
    copy->click();
    QCOMPARE(QApplication::clipboard()->text(), raw->toPlainText());

    files->setCurrentIndex(files->findData(other));
    QCOMPARE(table->model()->rowCount(), 0);
    QVERIFY(!copy->isEnabled());
    reason->clear();
    QTRY_COMPARE(table->model()->rowCount(), 1);
    QCOMPARE(raw->toPlainText(), QString{"bad raw sample"});
  }

  void importsCsvWithDesktopColumnMapping() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath("worker.CSV");
    std::ofstream{path.toStdString()} << "when,severity,details\n"
                                         "2026-10-20T09:00:00Z,ERROR,Failed\n";
    MainWindow window;
    window.show();
    QTimer::singleShot(0, &window, [&] {
      auto* dialog = window.findChild<QDialog*>();
      const auto fields = dialog->findChildren<QLineEdit*>();
      QCOMPARE(fields.size(), 4);
      fields[0]->setText("when");
      fields[1]->setText("severity");
      fields[2]->clear();
      fields[3]->setText("details");
      dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
    });
    window.findChild<QAction*>("csvColumns")->trigger();
    window.importFiles({path});
    QTRY_VERIFY_WITH_TIMEOUT(!window.busy(), 10000);
    QCOMPARE(window.logModel()->rowCount(), 1);
    QCOMPARE(window.logModel()->index(0, 2).data().toString(), QString{"worker"});
  }

  void recapTracksImportedSession() {
    MainWindow window;
    window.resize(840, 620);
    window.show();
    auto* navigation = window.findChild<QListWidget*>("navigation");
    navigation->setCurrentRow(2);
    auto* summary = window.findChild<QLabel*>("recapSummary");
    auto* errors = window.findChild<QLabel*>("recapSeverity4");
    QVERIFY(summary->text().contains("Import logs"));
    QVERIFY(!errors->isVisible());

    window.importFiles({LOGINSIGHT_SAMPLE_PATH});
    QTRY_VERIFY_WITH_TIMEOUT(!window.busy(), 10000);
    QCOMPARE(summary->text(), QString{"13 valid entries across 3 sources · 3 malformed records"});
    QCOMPARE(errors->text(), QString{"2 (15.4%)"});
    QVERIFY(errors->isVisible());
    QCOMPARE(window.findChild<QLabel*>("recapSourceTotal0")->text(), QString{"5"});
    QCOMPARE(window.findChild<QLabel*>("recapSourceTotal2")->text(), QString{"3"});
    QVERIFY(!window.findChild<QLabel*>("recapSourceTotal3")->isVisible());
    QCOMPARE(window.size(), QSize(840, 620));
    const auto capture = qEnvironmentVariable("LOGINSIGHT_CAPTURE_DIR");
    if (!capture.isEmpty()) {
      QApplication::processEvents();
      QVERIFY(window.grab().save(capture + "/recap.png"));
    }

    auto* donut = window.findChild<QWidget*>("severityDonut");
    QVERIFY(donut->isVisible());
    const int radius = std::min({donut->width() - 28, donut->height() - 28, 180}) / 2;
    QTest::mouseClick(donut, Qt::LeftButton, Qt::NoModifier,
                      QPoint{donut->width() / 2 + radius, donut->height() / 2});
    QCOMPARE(navigation->currentRow(), 1);
    auto* table = window.findChild<QTableView*>("logTable");
    QCOMPARE(table->model()->rowCount(), 6);
    navigation->setCurrentRow(2);
    auto* source = window.findChild<QPushButton*>("recapSource0");
    QTest::mouseClick(source, Qt::LeftButton);
    QCOMPARE(navigation->currentRow(), 1);
    QCOMPARE(table->model()->rowCount(), 5);
    QCOMPARE(table->model()->index(0, 2).data().toString(), QString{"gateway"});
    navigation->setCurrentRow(2);

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto empty = directory.filePath("empty.log");
    QFile file{empty};
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();
    window.importFiles({empty});
    QTRY_VERIFY_WITH_TIMEOUT(!window.busy(), 10000);
    QVERIFY(summary->text().startsWith("0 valid entries"));
    QVERIFY(!errors->isVisible());
  }
  void jsonLinesTimelineAndTimeRange() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath("events.JSONL");
    QFile input{path};
    QVERIFY(input.open(QIODevice::WriteOnly));
    QVERIFY(input.write("{\"timestamp\":\"2026-10-20T09:00:00Z\",\"level\":\"INFO\",\"source\":"
                        "\"svc\",\"message\":\"Started\"}\n"
                        "malformed\n"
                        "{\"timestamp\":\"2026-10-20T09:00:07Z\",\"level\":\"ERROR\",\"source\":"
                        "\"svc\",\"message\":\"Failed\"}\n") > 0);
    input.close();
    MainWindow window;
    window.show();
    window.importFiles({path});
    QTRY_VERIFY_WITH_TIMEOUT(!window.busy(), 10000);
    auto* table = window.findChild<QTableView*>("logTable");
    QCOMPARE(table->model()->rowCount(), 2);
    QVERIFY(window.findChild<QTextEdit*>("diagnostics")->toPlainText().contains("Line 2"));
    window.findChild<QListWidget*>("navigation")->setCurrentRow(1);
    window.findChild<QPushButton*>("timelineBucket7")->click();
    QCOMPARE(table->model()->rowCount(), 1);
    QCOMPARE(table->model()->index(0, 3).data().toString(), QString{"Failed"});

    auto* through = window.findChild<QLineEdit*>("timeThrough");
    through->setText("2026-10-20 09:00:00");
    QTest::keyClick(through, Qt::Key_Return);
    QCOMPARE(table->model()->rowCount(), 1);
    window.findChild<QPushButton*>("clearTimeRange")->click();
    QCOMPARE(table->model()->rowCount(), 2);
    through->setText("2026-10-20 09:00:00");
    QTest::keyClick(through, Qt::Key_Return);
    QCOMPARE(table->model()->rowCount(), 1);
    QCOMPARE(table->model()->index(0, 3).data().toString(), QString{"Started"});
    window.importFiles({LOGINSIGHT_SAMPLE_PATH});
    QTRY_VERIFY_WITH_TIMEOUT(!window.busy(), 10000);
    QCOMPARE(table->model()->rowCount(), 13);
  }
  void mappedJsonCorrelationAndVisibleExport() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto a = directory.filePath("a.jsonl");
    const auto b = directory.filePath("b.ndjson");
    std::ofstream{a.toStdString()}
        << R"({"event":{"ts":"2026-10-20T11:00:00.250+02:00"},"severity":"INFO","service":"api","msg":"First","trace_id":"trace-1","request_id":"req-1"})"
        << '\n'
        << R"({"event":{"ts":"2026-10-20T09:00:00.300Z"},"severity":"INFO","service":"api","msg":"Different trace","trace_id":"trace-2","request_id":"req-1"})"
        << '\n';
    std::ofstream{b.toStdString()}
        << R"({"event":{"ts":"2026-10-20T04:00:00.5-05:00"},"severity":"ERROR","service":"worker","msg":"Second","trace_id":"trace-1","request_id":"req-2"})"
        << '\n';

    MainWindow window;
    window.show();
    QTimer::singleShot(0, &window, [&] {
      auto* dialog = window.findChild<QDialog*>();
      dialog->findChild<QLineEdit*>("jsonTimestampPath")->setText("event.ts");
      dialog->findChild<QLineEdit*>("jsonSeverityPath")->setText("severity");
      dialog->findChild<QLineEdit*>("jsonSourcePath")->setText("service");
      dialog->findChild<QLineEdit*>("jsonMessagePath")->setText("msg");
      dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
    });
    window.findChild<QAction*>("jsonFields")->trigger();
    window.importFiles({b, a});
    QTRY_VERIFY_WITH_TIMEOUT(!window.busy(), 10000);
    auto* table = window.findChild<QTableView*>("logTable");
    QCOMPARE(table->model()->rowCount(), 3);
    window.findChild<QListWidget*>("navigation")->setCurrentRow(1);
    window.findChild<QPushButton*>("timelineBucket0")->click();
    QCOMPARE(table->model()->rowCount(), 3);
    table->setCurrentIndex(table->model()->index(0, 0));
    window.findChild<QPushButton*>("showRequest")->click();
    QCOMPARE(table->model()->rowCount(), 2);
    QCOMPARE(table->model()->index(1, 3).data().toString(), QString{"Different trace"});
    window.findChild<QPushButton*>("showTrace")->click();
    QCOMPARE(table->model()->rowCount(), 2);
    QCOMPARE(table->model()->index(0, 0).data().toString(), QString{"2026-10-20T09:00:00.25Z"});
    QCOMPARE(table->model()->index(1, 2).data().toString(), QString{"worker"});

    const auto path = directory.filePath("visible.json");
    window.exportTo(path, true);
    QTRY_VERIFY_WITH_TIMEOUT(!window.busy(), 10000);
    QFile exported{path};
    QVERIFY(exported.open(QIODevice::ReadOnly));
    const auto report = QJsonDocument::fromJson(exported.readAll()).object();
    QCOMPARE(report.value("scope").toString(), QString{"visible"});
    QCOMPARE(report.value("filters").toObject().value("trace_id").toString(), QString{"trace-1"});
    const auto entries = report.value("entries").toArray();
    QCOMPARE(entries.size(), 2);
    QCOMPARE(entries[1].toObject().value("path").toString(), b);
    QCOMPARE(entries[1].toObject().value("timestamp").toString(),
             QString{"2026-10-20T09:00:00.5Z"});
  }
  void reconfiguresRuleForCurrentSession() {
    MainWindow window;
    window.show();
    window.importFiles({LOGINSIGHT_SAMPLE_PATH});
    QTRY_VERIFY_WITH_TIMEOUT(!window.busy(), 10000);
    auto* alerts = window.findChild<QListWidget*>("burstAlerts");
    QCOMPARE(alerts->count(), 0);
    QTimer::singleShot(0, &window, [&] {
      auto* dialog = window.findChild<QDialog*>();
      dialog->findChild<QSpinBox*>("burstThreshold")->setValue(2);
      dialog->findChild<QSpinBox*>("burstWindow")->setValue(14);
      dialog->findChild<QComboBox*>("burstSeverity")->setCurrentText("Error");
      dialog->findChild<QLineEdit*>("burstSource")->setText("worker");
      dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
    });
    window.findChild<QPushButton*>("alertRule")->click();
    QCOMPARE(alerts->count(), 1);
    alerts->setCurrentRow(0);
    window.findChild<QPushButton*>("openBurst")->click();
    auto* table = window.findChild<QTableView*>("logTable");
    QCOMPARE(table->model()->rowCount(), 2);
    QCOMPARE(table->model()->index(0, 2).data().toString(), QString{"worker"});
  }
  void opensExactErrorBurstAcrossFiles() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto first = directory.filePath("one.log");
    const auto second = directory.filePath("two.log");
    std::ofstream{first.toStdString()} << "2026-10-20 09:01:10 ERROR worker fifth\n"
                                          "2026-10-20 08:59:49 ERROR worker too early\n"
                                          "2026-10-20 09:00:10 ERROR worker first\n"
                                          "2026-10-20 09:00:20 WARNING worker excluded\n"
                                          "2026-10-20 09:00:30 ERROR other excluded\n";
    std::ofstream{second.toStdString()} << "2026-10-20 09:00:40 CRITICAL worker third\n"
                                           "2026-10-20 09:00:50 ERROR worker fourth\n"
                                           "2026-10-20 09:00:30 ERROR worker second\n"
                                           "2026-10-20 09:01:11 ERROR worker no duplicate burst\n";

    MainWindow window;
    window.show();
    window.importFiles({first, second});
    QTRY_VERIFY_WITH_TIMEOUT(!window.busy(), 10000);
    auto* alerts = window.findChild<QListWidget*>("burstAlerts");
    QCOMPARE(alerts->count(), 1);
    alerts->setCurrentRow(0);
    window.findChild<QPushButton*>("openBurst")->click();
    auto* table = window.findChild<QTableView*>("logTable");
    QCOMPARE(table->model()->rowCount(), 5);
    QCOMPARE(table->model()->index(0, 3).data().toString(), QString{"first"});
    QCOMPARE(table->model()->index(4, 3).data().toString(), QString{"fifth"});
  }
  void cancellationAndOversizeKeepPriorSession() {
    MainWindow window;
    window.show();
    window.importFiles({LOGINSIGHT_SAMPLE_PATH});
    QTRY_VERIFY_WITH_TIMEOUT(!window.busy(), 10000);
    QCOMPARE(window.logModel()->rowCount(), 13);

    window.importFiles({LOGINSIGHT_SAMPLE_PATH});
    auto* cancel = window.findChild<QPushButton*>("cancelImport");
    QVERIFY(cancel->isVisible());
    cancel->click();
    QTRY_VERIFY_WITH_TIMEOUT(!window.busy(), 10000);
    auto* status = window.findChild<QLabel*>("statusText");
    QVERIFY(status->text().contains("Import cancelled"));
    QCOMPARE(window.logModel()->rowCount(), 13);

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto oversized = directory.filePath("oversized.log");
    QFile file{oversized};
    QVERIFY(file.open(QIODevice::WriteOnly));
    QVERIFY(file.resize(static_cast<qint64>(loginsight::parsers::ImportBudget::maxBytes) + 1));
    file.close();
    window.importFiles({LOGINSIGHT_SAMPLE_PATH, oversized});
    QTRY_VERIFY_WITH_TIMEOUT(!window.busy(), 10000);
    QVERIFY(status->text().contains("32 MiB"));
    QCOMPARE(window.logModel()->rowCount(), 13);
  }
  void sampleActionExportAndBusyClose() {
    MainWindow window;
    window.show();
    window.findChild<QPushButton*>("loadSamples")->click();
    QVERIFY(window.busy());
    window.close();
    QVERIFY(window.isVisible());
    QTRY_VERIFY_WITH_TIMEOUT(!window.busy(), 10000);
    QCOMPARE(window.logModel()->rowCount(), 13);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto destination = directory.filePath("session.json");
    window.exportTo(destination);
    QVERIFY(window.busy());
    QTRY_VERIFY_WITH_TIMEOUT(!window.busy(), 10000);
    QFile report{destination};
    QVERIFY(report.open(QIODevice::ReadOnly));
    const auto data = QJsonDocument::fromJson(report.readAll());
    QCOMPARE(data.object()["files"].toArray()[0].toObject()["entries"].toArray().size(), 13);
    window.exportTo(destination);
    QTRY_VERIFY_WITH_TIMEOUT(!window.busy(), 10000);
    QVERIFY(window.findChild<QLabel*>("statusText")->text().contains("already exists"));
  }
  void largerImportKeepsEventLoopActive() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath("large.log");
    {
      std::ofstream file{path.toStdString()};
      for (int i = 0; i < 30000; ++i)
        file << "2026-10-20 09:00:00 INFO worker Synthetic event " << i << '\n';
    }
    MainWindow window;
    bool ticked = false;
    QTimer::singleShot(0, &window, [&ticked] { ticked = true; });
    window.importFiles({path});
    QTRY_VERIFY_WITH_TIMEOUT(!window.busy(), 10000);
    QVERIFY(ticked);
    QCOMPARE(window.logModel()->rowCount(), 30000);
  }
  void emptyUnsupportedAndMixedFiles() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto empty = directory.filePath("empty.log");
    const auto unsupported = directory.filePath("invalid.xml");
    std::ofstream{empty.toStdString()}.close();
    std::ofstream{unsupported.toStdString()}.close();
    const auto result = load_files({empty, unsupported, LOGINSIGHT_SAMPLE_PATH});
    QCOMPARE(result->importedFiles, std::size_t{2});
    QCOMPARE(result->entries.size(), std::size_t{13});
    QCOMPARE(result->malformed, std::size_t{3});
    QVERIFY(result->diagnostics.contains("unsupported file format"));
  }
};
QTEST_MAIN(GuiTest)
#include "gui_test.moc"
