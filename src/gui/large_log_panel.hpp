#pragma once

#include "loginsight/parsers/parser_registry.hpp"

#include <QFutureWatcher>
#include <QWidget>
#include <functional>
#include <memory>
#include <stop_token>

class QLabel;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;
class QTemporaryDir;

namespace loginsight::gui {

class LargeLogPanel final : public QWidget {
public:
  explicit LargeLogPanel(const std::function<void()>& chooseFiles);
  ~LargeLogPanel() override;

  void openFiles(QStringList paths, parsers::JsonParserConfig json, parsers::CsvColumns csv);

private:
  struct Session {
    std::shared_ptr<QTemporaryDir> directory;
    qint64 entries{};
    qint64 malformed{};
    QString error;
  };

  static Session buildIndex(const QStringList& paths, parsers::JsonParserConfig json,
                            parsers::CsvColumns csv, std::stop_token stop);
  void showPage(qint64 offset);
  void showRecord();

  Session session_;
  QFutureWatcher<Session> worker_;
  std::stop_source stop_;
  qint64 offset_{};
  QLabel* status_{};
  QLabel* page_{};
  QPushButton* open_{};
  QPushButton* cancel_{};
  QPushButton* previous_{};
  QPushButton* next_{};
  QTableWidget* table_{};
  QPlainTextEdit* details_{};
};

} // namespace loginsight::gui
