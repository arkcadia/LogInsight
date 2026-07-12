#include "loginsight/core/version.hpp"

#include <QApplication>
#include <QLabel>
#include <QString>
#include <QWidget>

#include <string>

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);

  const auto projectName = std::string(loginsight::core::project_name());
  const auto labelText = projectName + " " + std::string(loginsight::core::version());

  QWidget window;
  window.setWindowTitle(QString::fromStdString(projectName));
  window.resize(800, 480);

  auto* label = new QLabel(QString::fromStdString(labelText), &window);
  label->setAlignment(Qt::AlignCenter);
  label->setGeometry(window.rect());

  window.show();
  return app.exec();
}
