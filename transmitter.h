#ifndef TRANSMITTER_H
#define TRANSMITTER_H

#include <QUdpSocket>
#include <QObject>
#include <QHostAddress>
#include <array>
#include <QElapsedTimer>
#include <fstream>
#include <string>
#include "def.h"
#include "altitudecontroller.h"

class Transmitter : public QObject
{
    Q_OBJECT

public:
    Transmitter();
//    Transmitter(int ID);
    ~Transmitter();
    Q_INVOKABLE void setJoystickID(int ID);
    Q_INVOKABLE int getJoystickID();
    Q_INVOKABLE double getChannelValue(int channel);
    void setChannelValue(int channel, double value);
//    bool connectToHost(QString address="192.168.15.34", quint16 port=123);
    static int toTxRange(double value);
    static bool sendChannels(const std::array<double, 16> &channels, QUdpSocket* socket, const QHostAddress &toAddress, const int &toPort);
    Q_INVOKABLE bool sendEspMode(bool rsp);
    Q_INVOKABLE bool sendChannelsWithMode();
    Q_INVOKABLE bool sendPing();
    void parsePacket(QByteArray &data);
    Q_INVOKABLE int getEspMode();
    Q_INVOKABLE int getDesiredEspMode();
    void setEspMode(int newMode);
    void setDesiredEspMode(int newMode);
    Q_PROPERTY(bool espMode READ getEspMode NOTIFY espModeChanged);
    Q_PROPERTY(bool desiredEspMode READ getDesiredEspMode NOTIFY desiredEspModeChanged);
    void parseAltitude(QJsonObject alt_obj);
    Q_INVOKABLE bool openFiles();
    Q_INVOKABLE void closeFiles();
    Q_INVOKABLE void setSuffix(QString suffix_in);
    Q_INVOKABLE std::string getSuffix();
    Q_INVOKABLE void setFileDirectory(QString directory);

    AltitudeController* alt_controller;

public slots:
    void axisChanged (const int js, const int axis, const double value);
    void buttonChanged (const int js, const int button, const bool pressed);
    void setModeFromChannelChanged(const int channel, const double value);
    void readUdp();

signals:
    void channelChanged(const int channel, const double value);
    void espModeChanged (const int mode);
    void desiredEspModeChanged (const int mode);
    void altitudeReceived(RangingData_t alt);
    void altitudeRangeReceived(int timestamp, int range);
    void altitudeForwardEstimate(int timestamp, double range, double velocity);
    void pingReceived(int pingLoopTime);

private:
    std::array<double, 16> channels = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    std::array<double, 16> controller_channels = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // chnl = multiplier*joy + offset
    std::array<double, 16> channelMultipliers = {100, 100, 100, 100, 200, 200, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100};
    std::array<double, 16> channelOffsets = {0, 0, 0, 0, -100, -100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    std::array<int,16> channelMap = {2, 0, 1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    std::array<int, 2> buttonMap = {6, 7}; // Maps a button to a channel
    int joystickID;
    static constexpr double MAX_CHANNEL_VALUE = 100;
    static constexpr double MIN_CHANNEL_VALUE = -100;
    static const unsigned char MSP_CHANNEL_ID = 200;
    const QHostAddress espAddress = QHostAddress("192.168.15.11");
    const int port = 123;
    const int MODE_PC = 1;
    const int MODE_JV = 2;
    const int MODE_ERR = 0;
    int espMode = MODE_ERR; // Mode is initially undefined
    int desiredEspMode = MODE_PC; // Start with the PC mode
    const int SET_MODE_CHANNEL = 5; // Channel which operates the esp mode
    QUdpSocket *socket;
    QElapsedTimer timer;
    int pingLoopTime = 0;
    void parsePing(QJsonObject ping_obj);


    std::ofstream file_alt_meas;
    bool record = false;
    std::string header_alt_meas = "time_esp_ms,time_recv_ms,range_mm,sigma_mm,signal_rate,ambient_rate,eff_spad_count,status";
    std::string prefix_alt_meas = "alt_meas_";
    std::string format = ".txt";
    std::string suffix = "temp";
    std::string fileDirectory = "";

    void updateControllerChannels();
    bool controllerActive = false;

};

#endif // TRANSMITTER_H
