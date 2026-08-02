#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSurfaceFormat>
#include "src/revolifcontroller.h"

int main(int argc, char *argv[])
{
    // Multisample antialiasing for the whole app. Without this, every
    // rounded-corner Rectangle (input fields, cards, dialogs, the logo
    // badge circle) renders with a jagged/stair-stepped edge that reads as
    // "pointy" corners instead of smoothly rounded ones.
    QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
    fmt.setSamples(4);
    QSurfaceFormat::setDefaultFormat(fmt);

    QGuiApplication app(argc, argv);
    app.setOrganizationName("Revolif");
    app.setApplicationName("Revolif Life Manager");

    RevolifController controller;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("revolif", &controller);

    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
