#pragma once
#include "log_model.hpp"
#include <QFutureWatcher>
#include <QMainWindow>
#include <array>

class QLabel;
class QPushButton;
class QProgressBar;
class QStackedWidget;
class QTableView;
class QTextEdit;
class QLineEdit;
class QComboBox;

namespace loginsight::gui {
class MainWindow final : public QMainWindow {
public:
  MainWindow();
  void importFiles(const QStringList& paths);
  void loadSamples();
  void exportTo(const QString& path);
  bool busy() const { return busy_; }
  const LogModel* logModel() const { return model_; }

protected:
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dropEvent(QDropEvent* event) override;
  void closeEvent(QCloseEvent* event) override;

private:
  void refresh();
  void setBusy(bool busy, const QString& text);
  void exportReport();
  void showDetails(const QModelIndex& index);
  std::shared_ptr<const Dataset> dataset_;
  QFutureWatcher<std::shared_ptr<const Dataset>> importer_;
  QFutureWatcher<QString> exporter_;
  bool busy_{};
  LogModel* model_{};
  LogFilter* proxy_{};
  QStackedWidget* pages_{};
  QTableView* table_{};
  QTextEdit* details_{};
  QTextEdit* diagnostics_{};
  QLabel* status_{};
  QLabel* session_{};
  QLabel* overviewText_{};
  QLabel* explorerCount_{};
  QLineEdit* search_{};
  QComboBox* severity_{};
  QComboBox* source_{};
  QPushButton* import_{};
  QPushButton* samples_{};
  QPushButton* export_{};
  QProgressBar* progress_{};
  std::array<QLabel*, 4> metrics_{};
  std::array<QProgressBar*, 7> bars_{};
  std::array<QLabel*, 7> barCounts_{};
};
} // namespace loginsight::gui
