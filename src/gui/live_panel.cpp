#include "live_panel.hpp"

#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>

#ifdef Q_OS_UNIX
#include <sys/stat.h>
#endif

namespace loginsight::gui {
namespace {
constexpr qsizetype maxRecord = 4096;
constexpr qint64 readBatch = 64 * 1024;
} // namespace

LivePanel::LivePanel() {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(18, 14, 18, 14);
  auto* title = new QLabel{"Live file follow"};
  title->setStyleSheet("font-size: 25px; font-weight: 600;");
  layout->addWidget(title);
  auto* controls = new QHBoxLayout;
  path_ = new QLineEdit;
  path_->setObjectName("livePath");
  path_->setAccessibleName("Log file to follow");
  path_->setPlaceholderText("Select a log file");
  auto* browse = browse_ = new QPushButton{"Browse…"};
  start_ = new QPushButton{"Follow"};
  start_->setObjectName("startLive");
  stop_ = new QPushButton{"Stop"};
  stop_->setObjectName("stopLive");
  stop_->setEnabled(false);
  controls->addWidget(path_, 1);
  controls->addWidget(browse);
  controls->addWidget(start_);
  controls->addWidget(stop_);
  layout->addLayout(controls);
  status_ = new QLabel{"Shows the most recent 5,000 lines while following."};
  status_->setTextFormat(Qt::PlainText);
  status_->setWordWrap(true);
  layout->addWidget(status_);
  output_ = new QPlainTextEdit;
  output_->setObjectName("liveRecords");
  output_->setAccessibleName("Live log records");
  output_->setReadOnly(true);
  output_->setLineWrapMode(QPlainTextEdit::NoWrap);
  output_->setMaximumBlockCount(5000);
  output_->setStyleSheet("font-family: monospace; background: #191b24; border: 1px solid #343746;");
  layout->addWidget(output_, 1);
  timer_ = new QTimer{this};
  timer_->setInterval(200);
  connect(timer_, &QTimer::timeout, this, &LivePanel::poll);
  connect(browse, &QPushButton::clicked, this, [this] {
    const auto path = QFileDialog::getOpenFileName(this, "Choose a log file to follow");
    if (!path.isEmpty())
      path_->setText(path);
  });
  connect(start_, &QPushButton::clicked, this, &LivePanel::start);
  connect(stop_, &QPushButton::clicked, this, &LivePanel::stop);
}

void LivePanel::start() {
  if (path_->text().isEmpty() || !QFileInfo{path_->text()}.isFile()) {
    status_->setText("Choose an existing regular file.");
    return;
  }
  file_.setFileName(QFileInfo{path_->text()}.absoluteFilePath());
  if (!file_.open(QIODevice::ReadOnly | QIODevice::Unbuffered)) {
    status_->setText("Could not open the log file: " + file_.errorString());
    return;
  }
  pending_.clear();
  checkpoint_.clear();
  lineClipped_ = false;
  output_->clear();
  path_->setReadOnly(true);
  start_->setEnabled(false);
  browse_->setEnabled(false);
  stop_->setEnabled(true);
  status_->setText("Following " + file_.fileName());
  timer_->start();
  poll();
}

void LivePanel::stop() {
  timer_->stop();
  file_.close();
  path_->setReadOnly(false);
  start_->setEnabled(true);
  browse_->setEnabled(true);
  stop_->setEnabled(false);
  status_->setText("Stopped. Live records remain visible.");
}

bool LivePanel::replaced() const {
#ifdef Q_OS_UNIX
  struct stat openFile {};
  struct stat currentPath {};
  return ::fstat(file_.handle(), &openFile) == 0 &&
         ::stat(QFile::encodeName(file_.fileName()).constData(), &currentPath) == 0 &&
         (openFile.st_dev != currentPath.st_dev || openFile.st_ino != currentPath.st_ino);
#else
  // shortcut: equal-size replacements with an identical prefix need a native file identity check.
  return false;
#endif
}

void LivePanel::poll() {
  const auto position = file_.pos();
  const bool rotated = !file_.isOpen() || replaced();
  if (rotated && (!file_.isOpen() || file_.atEnd())) {
    file_.close();
    if (!QFileInfo{file_.fileName()}.isFile() ||
        !file_.open(QIODevice::ReadOnly | QIODevice::Unbuffered)) {
      status_->setText("Waiting for the replacement log file…");
      return;
    }
    pending_.clear();
    checkpoint_.clear();
    lineClipped_ = false;
    output_->appendPlainText("[Log rotated]");
    status_->setText("Following " + file_.fileName());
  } else if (!rotated) {
    bool truncated = file_.size() < position;
    if (!truncated && !checkpoint_.isEmpty()) {
      truncated = !file_.seek(position - checkpoint_.size()) ||
                  file_.read(checkpoint_.size()) != checkpoint_;
    }
    if (truncated) {
      if (!file_.seek(0)) {
        stop();
        status_->setText("Could not resume reading after truncation.");
        return;
      }
      pending_.clear();
      checkpoint_.clear();
      lineClipped_ = false;
      output_->appendPlainText("[Log truncated]");
    }
  }
  const auto bytes = file_.read(readBatch);
  if (file_.error() != QFileDevice::NoError) {
    const auto error = file_.errorString();
    stop();
    status_->setText("Read failed: " + error);
    return;
  }
  if (bytes.isEmpty())
    return;
  checkpoint_ = (checkpoint_ + bytes).right(64);
  append(bytes);
}

void LivePanel::append(const QByteArray& bytes) {
  pending_ += bytes;
  QStringList lines;
  qsizetype start = 0;
  for (auto end = pending_.indexOf('\n'); end >= 0; end = pending_.indexOf('\n', start)) {
    auto line = pending_.mid(start, std::min(end - start, maxRecord));
    if (line.endsWith('\r'))
      line.chop(1);
    lines.push_back(QString::fromUtf8(line) +
                    (lineClipped_ || end - start > maxRecord ? " [line truncated]" : ""));
    lineClipped_ = false;
    start = end + 1;
  }
  pending_.remove(0, start);
  if (pending_.size() > maxRecord) {
    pending_.truncate(maxRecord);
    lineClipped_ = true;
  }
  if (lines.isEmpty())
    return;
  auto* scroll = output_->verticalScrollBar();
  const bool atEnd = scroll->value() == scroll->maximum();
  output_->appendPlainText(lines.join('\n'));
  if (atEnd)
    scroll->setValue(scroll->maximum());
}

} // namespace loginsight::gui
