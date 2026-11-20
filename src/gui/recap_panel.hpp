#pragma once

#include <QWidget>
#include <array>
#include <functional>

class QLabel;
class QProgressBar;
class QPushButton;

namespace loginsight::gui {
struct Dataset;
class SeverityDonut;

class RecapPanel final : public QWidget {
public:
  RecapPanel(std::function<void(int)> openSeverity, std::function<void(const QString&)> openSource);
  void setDataset(const Dataset& dataset);

private:
  QLabel* summary_{};
  QWidget* charts_{};
  SeverityDonut* donut_{};
  std::array<QLabel*, 7> severityTotals_{};
  std::array<QPushButton*, 7> severityButtons_{};
  std::array<QPushButton*, 5> sourceRows_{};
  std::array<QLabel*, 5> sourceLabels_{};
  std::array<QProgressBar*, 5> sourceBars_{};
  std::array<QLabel*, 5> sourceTotals_{};
};
} // namespace loginsight::gui
