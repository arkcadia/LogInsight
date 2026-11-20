#pragma once

#include <QWidget>
#include <array>
#include <map>

class QLabel;
class QPushButton;
class QTableWidget;

namespace loginsight::gui {
struct Dataset;
class SourceComparison;

class ComparisonPanel final : public QWidget {
public:
  ComparisonPanel();
  void setDataset(const Dataset& dataset);

private:
  void refresh();
  QLabel* summary_{};
  QPushButton* pin_{};
  QPushButton* clear_{};
  QWidget* charts_{};
  QTableWidget* severities_{};
  SourceComparison* sources_{};
  std::array<std::size_t, 7> current_{};
  std::array<std::size_t, 7> baseline_{};
  std::map<QString, std::size_t> currentSources_;
  std::map<QString, std::size_t> baselineSources_;
  QString currentName_;
  QString baselineName_;
};
} // namespace loginsight::gui
