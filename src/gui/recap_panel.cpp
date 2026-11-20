#include "recap_panel.hpp"
#include "log_model.hpp"

#include <QFontMetrics>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <numeric>

namespace loginsight::gui {
namespace {
QLabel* heading(const QString& title, int size) {
  auto* label = new QLabel{title};
  label->setStyleSheet(QString("font-size: %1px; font-weight: 600;").arg(size));
  return label;
}
} // namespace

class SeverityDonut final : public QWidget {
public:
  explicit SeverityDonut(std::function<void(int)> open) : open_(std::move(open)) {
    setObjectName("severityDonut");
    setMinimumSize(144, 144);
    setStyleSheet("background: transparent;");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  }

  void setCounts(const std::array<std::size_t, 7>& counts) {
    counts_ = counts;
    total_ = std::accumulate(counts_.begin(), counts_.end(), std::size_t{});
    setAccessibleName(QString{"Severity chart, %1 valid entries"}.arg(total_));
    update();
  }

protected:
  void paintEvent(QPaintEvent*) override {
    if (total_ == 0)
      return;
    QPainter painter{this};
    painter.setRenderHint(QPainter::Antialiasing);
    const auto ring = bounds();
    std::size_t cumulative{};
    int end{};
    for (std::size_t i = 0; i < counts_.size(); ++i) {
      cumulative += counts_[i];
      const int start = end;
      end = static_cast<int>(cumulative * fullCircle / total_);
      if (end == start)
        continue;
      painter.setPen(
          QPen{level_color(static_cast<core::LogLevel>(i)), 22, Qt::SolidLine, Qt::FlatCap});
      painter.drawArc(ring, 90 * 16 - start, start - end);
    }
    painter.setPen(QColor{"#e6e6ef"});
    auto displayFont = font();
    displayFont.setPointSize(18);
    displayFont.setBold(true);
    painter.setFont(displayFont);
    painter.drawText(ring.adjusted(0, 0, 0, -14), Qt::AlignCenter, QString::number(total_));
    painter.setFont(font());
    painter.drawText(ring.adjusted(0, 30, 0, 0), Qt::AlignCenter, "valid entries");
  }

  void mouseReleaseEvent(QMouseEvent* event) override {
    if (event->button() != Qt::LeftButton || total_ == 0)
      return;
    const auto ring = bounds();
    const auto delta = event->position() - ring.center();
    if (std::abs(std::hypot(delta.x(), delta.y()) - ring.width() / 2) > 11)
      return;
    auto angle = std::atan2(delta.x(), -delta.y()) / (2 * std::numbers::pi);
    if (angle < 0)
      angle += 1;
    const int segment = static_cast<int>(angle * fullCircle);
    std::size_t cumulative{};
    for (std::size_t i = 0; i < counts_.size(); ++i) {
      cumulative += counts_[i];
      if (segment < static_cast<int>(cumulative * fullCircle / total_)) {
        open_(static_cast<int>(i));
        return;
      }
    }
  }

private:
  static constexpr int fullCircle = 360 * 16;
  QRectF bounds() const {
    const qreal diameter = std::min({width() - 28, height() - 28, 180});
    return {(width() - diameter) / 2, (height() - diameter) / 2, diameter, diameter};
  }
  std::function<void(int)> open_;
  std::array<std::size_t, 7> counts_{};
  std::size_t total_{};
};

RecapPanel::RecapPanel(std::function<void(int)> openSeverity,
                       std::function<void(const QString&)> openSource) {
  auto* content = new QVBoxLayout(this);
  content->setContentsMargins(18, 14, 18, 14);
  content->setSpacing(12);
  content->addWidget(heading("Session recap", 25));
  summary_ = new QLabel{"Import logs or load the sample to see a recap."};
  summary_->setObjectName("recapSummary");
  summary_->setWordWrap(true);
  content->addWidget(summary_);

  charts_ = new QWidget;
  auto* columns = new QHBoxLayout(charts_);
  columns->setContentsMargins(0, 0, 0, 0);
  columns->setSpacing(12);
  auto* severity = new QWidget;
  severity->setObjectName("panel");
  auto* severityLayout = new QVBoxLayout(severity);
  severityLayout->addWidget(heading("Severity breakdown", 16));
  auto* breakdown = new QHBoxLayout;
  donut_ = new SeverityDonut{openSeverity};
  breakdown->addWidget(donut_, 1);
  auto* legend = new QVBoxLayout;
  const QStringList levels{"Trace", "Debug", "Info", "Warning", "Error", "Critical", "Unknown"};
  for (std::size_t i = 0; i < severityTotals_.size(); ++i) {
    auto* row = new QHBoxLayout;
    auto* name = severityButtons_[i] = new QPushButton{levels[static_cast<qsizetype>(i)]};
    name->setStyleSheet("padding: 2px 5px; text-align: left; color: " +
                        level_color(static_cast<core::LogLevel>(i)).name() + ';');
    connect(name, &QPushButton::clicked, this,
            [openSeverity, i] { openSeverity(static_cast<int>(i)); });
    row->addWidget(name);
    row->addStretch();
    severityTotals_[i] = new QLabel;
    severityTotals_[i]->setObjectName(QString("recapSeverity%1").arg(i));
    row->addWidget(severityTotals_[i], 0, Qt::AlignRight);
    legend->addLayout(row);
  }
  breakdown->addLayout(legend);
  severityLayout->addLayout(breakdown);
  columns->addWidget(severity, 3);

  auto* sources = new QWidget;
  sources->setObjectName("panel");
  auto* sourcesLayout = new QVBoxLayout(sources);
  sourcesLayout->addWidget(heading("Top sources", 16));
  sourcesLayout->addWidget(new QLabel{"Up to five sources by entry count"});
  for (std::size_t i = 0; i < sourceRows_.size(); ++i) {
    auto* group = sourceRows_[i] = new QPushButton;
    group->setObjectName(QString("recapSource%1").arg(i));
    group->setStyleSheet("padding: 4px;");
    connect(group, &QPushButton::clicked, this,
            [openSource, group] { openSource(group->property("sourceName").toString()); });
    auto* groupLayout = new QVBoxLayout(group);
    // The button's own size hint does not account for its labels and bar.
    groupLayout->setSizeConstraint(QLayout::SetMinimumSize);
    groupLayout->setContentsMargins(6, 5, 6, 5);
    groupLayout->setSpacing(4);
    auto* row = new QHBoxLayout;
    auto* name = sourceLabels_[i] = new QLabel;
    name->setTextFormat(Qt::PlainText);
    name->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    row->addWidget(name, 1);
    sourceTotals_[i] = new QLabel;
    sourceTotals_[i]->setObjectName(QString("recapSourceTotal%1").arg(i));
    row->addWidget(sourceTotals_[i]);
    groupLayout->addLayout(row);
    auto* bar = sourceBars_[i] = new QProgressBar;
    bar->setTextVisible(false);
    groupLayout->addWidget(bar);
    for (auto* child : {static_cast<QWidget*>(name), static_cast<QWidget*>(sourceTotals_[i]),
                        static_cast<QWidget*>(bar)})
      child->setAttribute(Qt::WA_TransparentForMouseEvents);
    sourcesLayout->addWidget(group);
  }
  sourcesLayout->addStretch();
  columns->addWidget(sources, 2);
  content->addWidget(charts_);
  charts_->hide();
  content->addStretch();
}

void RecapPanel::setDataset(const Dataset& dataset) {
  const auto total = dataset.entries.size();
  charts_->setVisible(total > 0);
  summary_->setText(QString{"%1 valid entries across %2 sources · %3 malformed records"}
                        .arg(total)
                        .arg(dataset.sources.size())
                        .arg(dataset.malformed));
  if (total == 0)
    return;
  donut_->setCounts(dataset.counts);
  for (std::size_t i = 0; i < severityTotals_.size(); ++i) {
    const auto count = dataset.counts[i];
    severityButtons_[i]->setEnabled(count > 0);
    severityButtons_[i]->setAccessibleName(
        QString{"Show %1 entries (%2)"}.arg(severityButtons_[i]->text()).arg(count));
    severityTotals_[i]->setText(QString{"%1 (%2%)"}.arg(count).arg(
        100.0 * static_cast<double>(count) / static_cast<double>(total), 0, 'f', 1));
  }
  const auto& top = dataset.topSources;
  for (std::size_t i = 0; i < sourceRows_.size(); ++i) {
    sourceRows_[i]->setVisible(i < top.size());
    if (i >= top.size())
      continue;
    const auto& [name, count] = top[i];
    auto* label = sourceLabels_[i];
    const auto displayName = name.isEmpty() ? QString{"(no source)"} : name;
    sourceRows_[i]->setProperty("sourceName", name);
    sourceRows_[i]->setAccessibleName(QString{"Show %1 entries (%2)"}.arg(displayName).arg(count));
    label->setText(QFontMetrics{label->font()}.elidedText(displayName, Qt::ElideRight, 142));
    label->setAccessibleName(displayName);
    label->setToolTip("<qt>" + displayName.toHtmlEscaped() + "</qt>");
    sourceTotals_[i]->setText(QString::number(count));
    sourceBars_[i]->setRange(0, static_cast<int>(top.front().second));
    sourceBars_[i]->setValue(static_cast<int>(count));
    sourceBars_[i]->setAccessibleName(QString{"%1, %2 entries"}.arg(displayName).arg(count));
  }
}
} // namespace loginsight::gui
