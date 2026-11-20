#pragma once

#include <QWidget>
#include <memory>

class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTableView;

namespace loginsight::gui {
struct Dataset;
class MalformedModel;

class MalformedPanel final : public QWidget {
public:
  explicit MalformedPanel(QWidget* parent = nullptr);
  void setDataset(std::shared_ptr<const Dataset> dataset);
  void focusRecords();

private:
  void applyFilters();
  void showRecord();

  MalformedModel* model_{};
  QTableView* table_{};
  QComboBox* files_{};
  QLineEdit* reason_{};
  QLabel* count_{};
  QPlainTextEdit* raw_{};
  QPushButton* copy_{};
};
} // namespace loginsight::gui
