// #include <QGuiApplication>
#include <QApplication>
#include <QQmlContext>
#include <QQmlApplicationEngine>
#include <QJoysticks.h>
#include <transmitter.h>
#include <QQuickStyle>

#ifdef Q_OS_WIN
    #ifdef main
        #undef main
    #endif
#endif

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    // QGuiApplication app(argc, argv);
    QApplication app(argc, argv);

    QQmlApplicationEngine engine;

    /*
     * QJoysticks is single instance, you can use the "getInstance()" function
     * directly if you want, or you can create a pointer to it to make code
     * easier to read;
     */
    QJoysticks* joyInstance = QJoysticks::getInstance();
    Transmitter* tx = new Transmitter();

    // Connect joystick axisChanged signal with the transmitter axisChanged slot
    QObject::connect(joyInstance, &QJoysticks::axisChanged, tx, &Transmitter::joystickAxisChanged);
    // Connect joystick buttonChanged signal with the transmitter buttonChanged slot
    QObject::connect(joyInstance, &QJoysticks::buttonChanged, tx, &Transmitter::joystickButtonChanged);

    /* Disable the virtual joystick */
    // joyInstance->setVirtualJoystickRange (1);
    joyInstance->setVirtualJoystickEnabled (true);

    /*
     * Register the QJoysticks with the QML engine, so that the QML interface
     * can easilly use it.
     */
    engine.rootContext()->setContextProperty("QJoysticks", joyInstance);
    engine.rootContext()->setContextProperty("Transmitter", tx);

    QQuickStyle::setStyle("Default");

    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);


    return app.exec();
}
