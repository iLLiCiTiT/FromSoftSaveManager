#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include "ui/MainWindow.h"
#include "ui/Controller.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("FromSoftSaveManager");
    app.setApplicationVersion("0.1.0");
    app.setWindowIcon(QIcon(":/icons/icon.png"));

    QFontDatabase::addApplicationFont(":/fonts/NotoSans-Regular.ttf");

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
