#include "main_window.hpp"
#include <QApplication>
#include <QMessageBox>
#include <exception>
int main(int argc, char* argv[]) {
  QApplication application{argc, argv};
  QApplication::setApplicationName("LogInsight");
  QApplication::setApplicationVersion("0.1.0");
  QApplication::setStyle("Fusion");
  try {
    loginsight::gui::MainWindow window;
    window.show();
    if (application.arguments().contains("--sample"))
      window.loadSamples();
    return application.exec();
  } catch (const std::exception& error) {
    QMessageBox::critical(nullptr, "LogInsight", QString::fromUtf8(error.what()));
    return 1;
  }
}
