#include <QApplication>
#include <QIcon>
#include <QFile>
#include <QDateTime>
#include "ui/MainWindow.h"
#include "ui/Controller.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("FromSoftSaveManager");
    app.setApplicationVersion("0.1.0");

    QFile f(":/stylesheet/app.qss");
    if (f.open(QFile::ReadOnly)) {
        const QString qss = QString::fromUtf8(f.readAll());
        app.setStyleSheet(qss);
    }

    Controller controller = Controller();
    MainWindow window = MainWindow(&controller, nullptr);
    window.show();

    return QApplication::exec();
}
