#include <iostream>
#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include "ui/MainWindow.h"
#include "ui/Controller.h"

#ifdef _WIN32
#include <windows.h>
#include <cstdio>

static void attachConsoleIfPresent() {
    // If started from an existing console, this succeeds and we can use it
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE* fp;
#ifdef _MSC_VER
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
        freopen_s(&fp, "CONIN$",  "r", stdin);
#else
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        freopen("CONIN$",  "r", stdin);
#endif
        setvbuf(stdout, nullptr, _IONBF, 0);
        setvbuf(stderr, nullptr, _IONBF, 0);
    }
}
#endif

int main(int argc, char *argv[]) {
#ifdef _WIN32
    attachConsoleIfPresent();
#endif
    QApplication app(argc, argv);
    app.setApplicationName("FromSoftSaveManager");
    app.setApplicationVersion("0.6.0");
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
