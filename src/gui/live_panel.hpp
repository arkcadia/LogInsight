#pragma once

#include <QFile>
#include <QWidget>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTimer;

namespace loginsight::gui {
class LivePanel final : public QWidget {
public:
  LivePanel();

private:
  void start();
  void stop();
  void poll();
  void append(const QByteArray& bytes);
  bool replaced() const;

  QLineEdit* path_{};
  QLabel* status_{};
  QPlainTextEdit* output_{};
  QPushButton* browse_{};
  QPushButton* start_{};
  QPushButton* stop_{};
  QTimer* timer_{};
  QFile file_;
  QByteArray pending_;
  QByteArray checkpoint_;
  bool lineClipped_{};
};
} // namespace loginsight::gui
