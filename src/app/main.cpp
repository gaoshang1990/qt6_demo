#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QScreen>
#include <QQmlContext>
#include <QQmlComponent>

#include "version.h"
#include "utils_log.h"


int main(int argc, char* argv[])
{
    // slog_init(1, nullptr, nullptr);
    print_app_info("QT6 demo", APP_VERSION, __DATE__, __TIME__);

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("UserCom", "Main");

    return app.exec();
}
