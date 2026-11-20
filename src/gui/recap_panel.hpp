#pragma once

#include <QWidget>
#include <array>

class QLabel;
class QProgressBar;

namespace loginsight::gui {
struct Dataset;
class SeverityDonut;

class RecapPanel final : public QWidget {
public:
  explicit RecapPanel(QWidget* parent = nullptr);
  void setDataset(const Dataset& dataset);

private:
  QLabel* summary_{};
  QWidget* charts_{};
  SeverityDonut* donut_{};
  std::array<QLabel*, 7> severityTotals_{};
  std::array<QWidget*, 5> sourceRows_{};
  std::array<QLabel*, 5> sourceLabels_{};
  std::array<QProgressBar*, 5> sourceBars_{};
  std::array<QLabel*, 5> sourceTotals_{};
};
} // namespace loginsight::gui
