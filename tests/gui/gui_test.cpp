#include "main_window.hpp"
#include <QAbstractItemModelTester>
#include <QComboBox>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSignalSpy>
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
    const auto unsupported = directory.filePath("invalid.csv");
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
