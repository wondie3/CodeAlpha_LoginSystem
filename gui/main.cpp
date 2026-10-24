#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QStandardPaths>

#include "MainWindow.h"
#include "Style.h"
#include "UserStore.h"

// Usage: LoginGUI [path/to/users.db]
// Without an argument the data file lives in the user's application-data folder.
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName("CodeAlpha");
    QApplication::setApplicationName("Login System");
    QApplication::setStyle("Fusion");   // identical look on Windows, macOS and Linux
    app.setStyleSheet(appStyleSheet());

    const QStringList args = QApplication::arguments();
    QString path = args.size() > 1
                       ? args[1]
                       : QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/users.db";
    path = QDir::cleanPath(QFileInfo(path).absoluteFilePath());
    QDir().mkpath(QFileInfo(path).absolutePath());

    UserStore store(path.toStdString());
    std::string error;
    if (!store.load(error)) {
        QMessageBox::critical(nullptr, QObject::tr("Could not load the user data"),
                              QObject::tr("The data file could not be loaded:\n%1\n\n%2\n\n"
                                          "The file has NOT been changed. Fix or rename it, then start the "
                                          "program again.")
                                  .arg(QDir::toNativeSeparators(path), QString::fromStdString(error)));
        return 1;
    }

    MainWindow window(std::move(store));
    window.show();
    return app.exec();
}
