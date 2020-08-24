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
#include "altitudecontroller.h"

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
    bool getControllerStandby();
    void setControllerStandby(const bool standby);
    Q_PROPERTY(bool controllerStandby READ getControllerStandby WRITE setControllerStandby);
    bool getControllerActive();
    Q_PROPERTY(bool controllerActive READ getControllerActive NOTIFY controllerActiveChanged);

    // Landing mode
    Q_INVOKABLE void setDesiredJvLanding(const int newMode);

    // Comms
    Q_INVOKABLE bool sendChannelsWithMode(); //
    Q_INVOKABLE bool sendPing(const bool response=true); //
    // Files
    Q_INVOKABLE bool openFiles(); //
    Q_INVOKABLE void closeFiles(); //
    void setFileSuffix(QString suffix_in); //
    QString getFileSuffix(); //
    Q_PROPERTY(QString suffix READ getFileSuffix() WRITE setFileSuffix);
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
    void espModeChanged(const int mode); //
    void desiredEspModeChanged(const int mode); //
    void controllerActiveChanged(const bool ctrlActv);
    void jvControllerChanged(const int mode);
    void jvLandingChanged(const int mode);
    void desiredJvControllerChanged(const int mode);
    void desiredJvLandingChanged(const int mode);

    // Comms
    void pingReceived(int pingLoopTime); //
    void logReceived(int device, QString logText, int logLevel);
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
    double saturate(double channelValue); //

    // Mode
    const int MODE_PC = 1;
    const int MODE_JV = 2;
    const int MODE_ERR = 0;
    int currentEspMode;
    int desiredEspMode;
    bool armed = false;
    bool txTransmit = false;
    bool controllerStandby = false;
    bool controllerActive = false;
    const int SET_MODE_CHANNEL = 5; // Channel which operates the esp mode
    const int SET_ARM_CHANNEL = 4; // Channel which sets the FC arm
    void updateCurrentEspMode(const int newMode); //
    void setDesiredEspMode(const int newMode); //
    void setModeFromChannel(const int channel, const double value); //
    void setControllerActive(const bool ctrlActv);

    // Jevois states
    const int JV_CTRL_TRUE = 1;
    const int JV_CTRL_FALSE = 2;
    const int JV_CTRL_ERR = 0;
    int current_jvController = JV_CTRL_ERR;
    int desired_jvController = JV_CTRL_FALSE;
    void updateCurrentJvController(const int newMode);
    void setDesiredJvController(const int newMode);
    bool sendJvController(const int mode, const bool response=true);
    void parseJvController(QJsonObject mode_obj);

    bool sendJvQuit();

    const int JV_LAND_TRUE = 1;
    const int JV_LAND_FALSE = 2;
    const int JV_LAND_ERR = 0;
    int current_jvLanding = JV_LAND_ERR;
    int desired_jvLanding = JV_LAND_FALSE;
    void updateCurrentJvLanding(const int newMode);
    bool sendJvLanding(const int mode, const bool response=true);
    void parseJvLanding(QJsonObject land_obj);


    // Comms
    QUdpSocket *socket;
    const QHostAddress IP_ADDR_ESP = QHostAddress("192.168.15.23");
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
    void parseEspMode(QJsonObject mode_obj); //
    void parsePing(QJsonObject ping_obj); //
    void parseAltitude(QJsonObject alt_obj); //
    void parseLog(QJsonObject log_obj, int dev); //


    // Control system
    std::array<double, 4> controllerChannels = {0, 0, 0, 0};
    AltitudeController* altController;
    AltitudeEstimator* altEstimator;

    // Files
    std::ofstream file_log;
    bool filesOpen = false;
    std::string header_log = "time_esp_ms,time_esp_prop,Delta_t_prop_ms,z_prop,z_dot_prop,chnThr,chnEle,chnAil,chnRud";
    std::string prefix_log = "alt_prop_ctrl_";
    std::string format = ".txt";
    std::string suffix = "temp";
    std::string fileDirectory = "";

};

#endif // TRANSMITTER_H
