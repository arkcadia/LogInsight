#pragma once
#include "log_model.hpp"
#include <QFutureWatcher>
#include <QMainWindow>
#include <QVariantMap>
#include <array>
#include <utility>

class QLabel;
class QPushButton;
class QProgressBar;
class QStackedWidget;
class QTableView;
class QTextEdit;
class QLineEdit;
class QComboBox;
class QMenu;
class QListWidget;
class QSplitter;
class QResizeEvent;

namespace loginsight::gui {
class RecapPanel;
class MalformedPanel;
class MainWindow final : public QMainWindow {
public:
  MainWindow();
  void importFiles(const QStringList& paths);
  void loadSamples();
  void exportTo(const QString& path, bool visibleOnly = false);
  bool busy() const { return busy_; }
  const LogModel* logModel() const { return model_; }

protected:
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dropEvent(QDropEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void closeEvent(QCloseEvent* event) override;

private:
  void refresh();
  void refreshTimeline();
  void applyTimeRange();
  void clearTimeRange();
  void refreshFilterChips();
  void populateViewsMenu(QMenu& menu);
  void saveView();
  void applyView(const QVariantMap& filters);
  void setBookmarkScope(bool enabled);
  void toggleBookmark();
  void showBurst(int index);
  void setBusy(bool busy, const QString& text);
  void exportReport(bool visibleOnly = false);
  void configureJson();
  void configureCsv();
  void configureRule();
  void refreshBursts();
  void resetExplorerFilters();
  void showCorrelation(const char* key);
  void showDetails(const QModelIndex& index);
  std::shared_ptr<const Dataset> dataset_;
  using ImportOutcome = std::pair<std::shared_ptr<const Dataset>, QString>;
  QFutureWatcher<ImportOutcome> importer_;
  std::stop_source importStop_;
  QFutureWatcher<QString> exporter_;
  bool busy_{};
  LogModel* model_{};
  LogFilter* proxy_{};
  QStackedWidget* pages_{};
  RecapPanel* recap_{};
  MalformedPanel* malformed_{};
  QTableView* table_{};
  QSplitter* splitter_{};
  QTextEdit* details_{};
  QTextEdit* diagnostics_{};
  QLabel* status_{};
  QLabel* session_{};
  QLabel* overviewText_{};
  QLabel* explorerCount_{};
  QLineEdit* search_{};
  QLineEdit* from_{};
  QLineEdit* through_{};
  QLabel* timeRangeError_{};
  QLabel* timelineStart_{};
  QLabel* timelineEnd_{};
  static constexpr std::size_t timelineSize = 8;
  std::array<QPushButton*, timelineSize> timelineButtons_{};
  std::array<QProgressBar*, timelineSize> timelineBars_{};
  std::array<std::pair<QString, QString>, timelineSize> timelineRanges_{};
  QComboBox* severity_{};
  QComboBox* source_{};
  QListWidget* navigation_{};
  QListWidget* alerts_{};
  QLabel* noBursts_{};
  QLabel* ruleDescription_{};
  QWidget* chipRow_{};
  std::array<QPushButton*, 6> chips_{};
  QWidget* timeControls_{};
  QPushButton* exportVisible_{};
  QPushButton* trace_{};
  QPushButton* bookmark_{};
  std::set<const core::LogEntry*> bookmarks_;
  bool bookmarksOnly_{};
  QPushButton* request_{};
  QPushButton* ruleSettings_{};
  QPushButton* openBurst_{};
  parsers::JsonParserConfig jsonConfig_;
  parsers::CsvColumns csvColumns_;
  rules::BurstRule burstRule_;
  std::vector<rules::ErrorBurst> bursts_;
  QPushButton* cancelImport_{};
  QPushButton* import_{};
  QPushButton* samples_{};
  QPushButton* export_{};
  QProgressBar* progress_{};
  std::array<QPushButton*, 4> metrics_{};
  std::array<QProgressBar*, 7> bars_{};
  std::array<QLabel*, 7> barCounts_{};
};
} // namespace loginsight::gui
