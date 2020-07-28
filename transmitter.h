#ifndef TRANSMITTER_H
#define TRANSMITTER_H

#include <QUdpSocket>
#include <QObject>
#include <QHostAddress>
#include <QElapsedTimer>
#include <QTimer>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonDocument>
#include <QNetworkDatagram>
#include <QTextCodec>

#include <fstream>
#include <array>
#include <string>
#include <iostream>
#include <math.h>
#include "def.h"
#include "altitudeestimator.h"
//#include "altitudecontroller.h"

class Transmitter : public QObject
{
    Q_OBJECT

public:
    Transmitter(); //
    ~Transmitter(); //

    // Joystick
    Q_INVOKABLE void setJoystickID(const int ID); //
    Q_INVOKABLE int getJoystickID(); //
    // Channels
    Q_INVOKABLE double getJoyChannelValue(const int channel); //
    Q_INVOKABLE double getControllerChannelValue(const int channel); //
    // Mode
    int getCurrentEspMode(); //
    int getDesiredEspMode(); //
    Q_PROPERTY(bool currentEspMode READ getCurrentEspMode NOTIFY espModeChanged);
    Q_PROPERTY(bool desiredEspMode READ getDesiredEspMode NOTIFY desiredEspModeChanged);
    bool getTransmit(); //
    void setTransmit(const bool enabled); //
    Q_PROPERTY(bool txTransmit READ getTransmit WRITE setTransmit);
    // Comms
    Q_INVOKABLE bool sendChannelsWithMode(); //
    Q_INVOKABLE bool sendPing(const bool response=true); //
    // Files
    Q_INVOKABLE bool openFiles(); //
    Q_INVOKABLE void closeFiles(); //
    void setSuffix(QString suffix_in); //
    QString getSuffix(); //
    Q_PROPERTY(QString suffix READ getSuffix() WRITE setSuffix);
    void setFileDirectory(QString directory); //
    QString getFileDirectory(); //
    Q_PROPERTY(QString fileDirectory READ getFileDirectory() WRITE setFileDirectory);

public slots:
    // Joystick
    void joystickAxisChanged (const int js, const int axis, const double value); //
    void joystickButtonChanged (const int js, const int button, const bool pressed); //
    // Comms
    void sendTimerDone();
    void pingTimerDone();
    void readUDP(); //

signals:
    // Channels
    void joyChannelChanged(const int channel, const double value); //
    void controllerChannelChanged(const int channel, const double value);
    // Mode
    void espModeChanged (const int mode); //
    void desiredEspModeChanged (const int mode); //
    // Comms
    void pingReceived(int pingLoopTime); //
    // State
    void altRangeReceived(int timeEsp_ms, int range); //
    void altStateEstimate(int timeEsp_ms, double z, double z_dot); //
    void altPropStateEstimate(int timeEsp_ms, double z, double z_dot);

private:

    // Joystick
    // chnl = multiplier*joy_raw + offset
    std::array<double, 16> joyMultipliers = {100, 100, 100, 100, 200, 200, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100};
    std::array<double, 16> joyOffsets = {0, 0, 0, 0, -100, -100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    std::array<int,16> channelMap = {2, 0, 1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    std::array<int, 0> buttonMap = {}; // Maps a button to a channel
    int joystickID = 0;

    // Channels
    std::array<double, 16> joyChannels = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    static constexpr double MAX_CHANNEL_VALUE = 100;
    static constexpr double MIN_CHANNEL_VALUE = -100;
    static constexpr unsigned char MSP_CHANNEL_ID = 200;
    void setJoyChannelValue(const int channel, double value); //
    static int channelToTxRange(double value); //

    // Mode
    const int MODE_PC = 1;
    const int MODE_JV = 2;
    const int MODE_ERR = 0;
    int currentEspMode;
    int desiredEspMode;
    bool txTransmit = false;
    const int SET_MODE_CHANNEL = 5; // Channel which operates the esp mode
    void updateCurrentEspMode(const int newMode); //
    void setDesiredEspMode(const int newMode); //
    void setModeFromChannel(const int channel, const double value); //

    // Comms
    QUdpSocket *socket;
    const QHostAddress IP_ADDR_ESP = QHostAddress("192.168.15.11");
    const int PORT_ESP = 123;
    const int PING_TIMEOUT = 100;
    const int SEND_CONTROL_PERIOD_MS = 50; // ms, time between sending control commansd
    const int PING_PERIOD_MS = 500; // ms, time between sending pings
    QElapsedTimer timerPc;
    QTimer* sendTimer;
    QTimer* pingTimer;
    int pingLoopTime = 0;
    bool sendChannels(const std::array<double, 16> &channels, const bool response=false); //
    bool sendEspMode(const int mode, const bool response=true); //
    void parsePacket(QByteArray &data); //
    void parseMode(QJsonObject mode_obj); //
    void parsePing(QJsonObject ping_obj); //
    void parseAltitude(QJsonObject alt_obj); //

    // Control system
    std::array<double, 4> controllerChannels = {0, 0, 0, 0};
    bool controllerIsActive = false;
//    AltitudeController* altController;
    AltitudeEstimator* altEstimator;
    // void updateControllerChannels(); // TODO: change

    // Files
    std::ofstream file_alt_meas;
    bool filesOpen = false;
    std::string header_log = "";
    std::string prefix_log = "";
    std::string format = ".txt";
    std::string suffix = "temp";
    std::string fileDirectory = "";

};

#endif // TRANSMITTER_H
