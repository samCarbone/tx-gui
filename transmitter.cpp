#include "transmitter.h"

Transmitter::Transmitter()
{
    // Timestamp for receiving esp messages
    timerPc.start();
    // Timer to send control signals
    sendTimer = new QTimer(this);
    sendTimer->setTimerType(Qt::PreciseTimer);
    QObject::connect(sendTimer, &QTimer::timeout, this, &Transmitter::sendTimerDone);
    sendTimer->start(SEND_CONTROL_PERIOD_MS);
    // Timer to send pings
    pingTimer = new QTimer(this);
    pingTimer->setTimerType(Qt::CoarseTimer);
    QObject::connect(pingTimer, &QTimer::timeout, this, &Transmitter::pingTimerDone);
    pingTimer->start(PING_PERIOD_MS);

    // create a QUDP socket
    socket = new QUdpSocket(this);
    socket->bind(QHostAddress::Any, PORT_ESP); // QHostAddress::Any IP_ADDR_ESP
    // Connect socket readyRead() signal with the transmitter readUDP() slot
    QObject::connect(socket, &QUdpSocket::readyRead, this, &Transmitter::readUDP);

    // Altitude controller
    altController = new AltitudeController();
    altEstimator = new AltitudeEstimator();

    // Initialise modes
    updateCurrentEspMode(ESP_MODE_ERR); // Esp mode is initially unknown
    setDesiredEspMode(ESP_MODE_PC); // Start with the PC mode

    // Force qml to update channels
    emit joyChannelChanged(0, joyChannels.at(0));

}

Transmitter::~Transmitter()
{
//    socket->disconnectFromHost();
      closeFiles();
      delete sendTimer;
      delete pingTimer;
      delete socket;
      delete altController;
      delete altEstimator;
}

// *********************************************************
// Joystick
// *********************************************************

void Transmitter::setJoystickID(const int ID) { joystickID = ID; }

int Transmitter::getJoystickID() { return joystickID; }

void Transmitter::joystickAxisChanged(const int js, const int axis, const double value)
{
    if(js != joystickID)
        return;

    if(axis < 0 || axis >= (int)channelMap.size())
        return;

    // Convert value from range of (-1,1) to (-100, 100)
    double mappedValue = joyMultipliers.at(axis)*value + joyOffsets.at(axis);

    setJoyChannelValue(channelMap.at(axis), mappedValue);
}


void Transmitter::joystickButtonChanged(const int js, const int button, const bool pressed)
{

    if(js != joystickID)
        return;

    if(button < 0 || button >= (int)buttonMap.size())
        return;

    double value = pressed ? 100 : -100;

    setJoyChannelValue(buttonMap.at(button), value);
}

// *********************************************************
// Channels
// *********************************************************

void Transmitter::setJoyChannelValue(const int channel, double value)
{

    if (value > MAX_CHANNEL_VALUE) {
        value = MAX_CHANNEL_VALUE;
        std::cout << "[warn] channel value out of range (greater)" << std::endl;
    }
    else if (value < MIN_CHANNEL_VALUE) {
        value = MIN_CHANNEL_VALUE;
        // std::cout << "[warn] channel value out of range (lower)" << std::endl;
    }

    joyChannels.at(channel) = value;
    setModeFromChannel(channel, value);
    emit joyChannelChanged(channel, value);

}

double Transmitter::getJoyChannelValue(const int channel) { return joyChannels.at(channel); }

double Transmitter::getControllerChannelValue(const int channel) { return controllerChannels.at(channel); }

int Transmitter::channelToTxRange(double value)
{
    if (value > MAX_CHANNEL_VALUE) {
        value = MAX_CHANNEL_VALUE;
        std::cout << "[warn] channel value out of range (greater)" << std::endl;
    }
    else if (value < MIN_CHANNEL_VALUE) {
        value = MIN_CHANNEL_VALUE;
        std::cout << "[warn] channel value out of range (lower)" << std::endl;
    }

    return round(value*6 + 1500); // Scaled from (-100, 100) to (900, 2100)
}

double Transmitter::saturate(double channelValue)
{
    if (channelValue > MAX_CHANNEL_VALUE) {
        channelValue = MAX_CHANNEL_VALUE;
    }
    else if (channelValue < MIN_CHANNEL_VALUE) {
        channelValue = MIN_CHANNEL_VALUE;
    }
    return channelValue;
}

// *********************************************************
// Mode
// *********************************************************

void Transmitter::updateCurrentEspMode(const char newMode)
{
    currentEspMode = newMode;
    emit espModeChanged(currentEspMode);
}

void Transmitter::setDesiredEspMode(const char newMode)
{
    desiredEspMode = newMode;
    emit desiredEspModeChanged(desiredEspMode);
}

char Transmitter::getCurrentEspMode()
{
    return currentEspMode;
}

char Transmitter::getDesiredEspMode()
{
    return desiredEspMode;
}

void Transmitter::setModeFromChannel(const int channel, const double value)
{
    if(channel == SET_MODE_CHANNEL) {
        if(!controllerStandby) {
            if(value > 50 && armed) {
                setDesiredEspMode(ESP_MODE_JV);
                setDesiredJvController(JV_CTRL_ENA);
            }
            else {
                setDesiredEspMode(ESP_MODE_PC);
                setDesiredJvController(JV_CTRL_DIS);
                setDesiredJvLanding(JV_LAND_DIS);
            }

        }
        else {
            if(value > 50)
                setControllerActive(true);
            else
                setControllerActive(false);
        }
    }

    else if(channel == SET_ARM_CHANNEL) {
        if(value > 50) {
            armed = true;
        }
        else {
            armed = false;
            setDesiredEspMode(ESP_MODE_PC);
            setDesiredJvController(JV_CTRL_DIS);
            setDesiredJvLanding(JV_LAND_DIS);
        }
    }
}

bool Transmitter::getTransmit()
{
    return txTransmit;
}

void Transmitter::setTransmit(const bool enabled)
{
    txTransmit = enabled;
}

bool Transmitter::getControllerStandby()
{
    return controllerStandby;
}

void Transmitter::setControllerStandby(const bool standby)
{
    controllerStandby = standby;
    if(standby == true) {
        setDesiredEspMode(ESP_MODE_PC);
        setDesiredJvController(JV_CTRL_DIS);
        setDesiredJvLanding(JV_LAND_DIS);
    }
    else {
        setControllerActive(false);
    }
}

bool Transmitter::getControllerActive()
{
    return controllerActive;
}

void Transmitter::setControllerActive(const bool ctrlActv)
{
    controllerActive = ctrlActv;
    altController->resetState();

    // Make a better way of setting this
    AltTarget_t targetTemp; targetTemp.z = -0.5; targetTemp.z_dot = 0;
    altController->setTarget(targetTemp);
    emit controllerActiveChanged(ctrlActv);
}

void Transmitter::updateCurrentJvController(const char newMode)
{
    current_jvController = newMode;
    emit jvControllerChanged(current_jvController);
}

void Transmitter::setDesiredJvController(const char newMode)
{
    desired_jvController = newMode;
    emit desiredJvControllerChanged(desired_jvController);

}

void Transmitter::updateCurrentJvLanding(const char newMode)
{
    current_jvLanding = newMode;
    emit jvLandingChanged(current_jvLanding);
}

void Transmitter::setDesiredJvLanding(const char newMode)
{
    desired_jvLanding = newMode;
    desiredJvLandingChanged(desired_jvLanding);
}


// *********************************************************
// Comms
// *********************************************************

void Transmitter::sendTimerDone()
{
    // Note: can check txTransmit here.
    sendChannelsWithMode();
}

void Transmitter::pingTimerDone()
{
    if(txTransmit) {
        sendPing(true);
    }
}

bool Transmitter::sendChannelsWithMode()
{
    bool success = false;

    if(txTransmit) {
        success = true;

        if(currentEspMode != desiredEspMode)
            success &= sendEspMode(desiredEspMode, true);

        if(desiredEspMode == ESP_MODE_PC) {
            double chnThr, chnEle, chnAil, chnRud;

            if(controllerStandby) {
            // Calculate propagated state
                AltState_t propAltState = altEstimator->getPropagatedStateEstimate_safe(timerPc.elapsed()+pingLoopTime, PING_LIMIT);

                if(controllerActive) {

                    controllerChannels.at(2) = saturate(altController->getControlTempState(propAltState));

                    std::array<double, 16> mixedChannels = joyChannels;
                    mixedChannels.at(2) = controllerChannels.at(2);
                    success &= sendChannels(mixedChannels, false);
                    emit controllerChannelChanged(2, mixedChannels.at(2));

                    chnThr = mixedChannels.at(2); chnEle = mixedChannels.at(1);
                    chnAil = mixedChannels.at(0); chnRud = mixedChannels.at(3);
                }

                if(filesOpen) {
                    // header
                    // "time_esp_ms,time_esp_prop,Delta_t_prop_ms,z_prop,z_dot_prop,chnThr,chnEle,chnAil,chnRud"
                    file_log << altEstimator->getCurrentTimeEsp_ms() << "," << propAltState.timeEsp_ms << "," << propAltState.timeEsp_ms-altEstimator->getCurrentTimeEsp_ms() << ","
                             << propAltState.z << "," << propAltState.z_dot << "," << chnThr << "," << chnEle << "," << chnAil << "," << chnRud << std::endl;
                }
                emit altPropStateEstimate((int)propAltState.timeEsp_ms, propAltState.z, propAltState.z_dot);

            }
            else {
                success &= sendChannels(joyChannels, false);
                chnThr = joyChannels.at(2); chnEle = joyChannels.at(1);
                chnAil = joyChannels.at(0); chnRud = joyChannels.at(3);
            }


        }

        if(current_jvController != desired_jvController) {
            success &= sendJvController(desired_jvController, true);
        }

        if(current_jvLanding != desired_jvLanding) {
            success &= sendJvLanding(desired_jvLanding, true);
        }

    }
    return success;
}

bool Transmitter::sendChannels(const std::array<double, 16> &channels, const bool response)
{
    QByteArray Data;
    unsigned char message_len = 2*channels.size(); // Could use a static cast to char to make sure that only
                                          // the least-significant bytes in the size is used.
    unsigned char message_id = MSP_CHANNEL_ID;

    // Header
    Data.append("$M<");
    Data.append(message_len);
    Data.append(message_id);

    // Channel values
    for(auto value : channels) {
        // int bytes = QByteArray::number(toTxRange(value));

        int rangedValue = channelToTxRange(value);
        unsigned char bytes_lower = static_cast<unsigned char>(rangedValue & 0xFF);
        unsigned char bytes_upper = static_cast<unsigned char>((rangedValue & 0xFF00) >> 8);

        // Arduino is little-endian | least-significant byte | most-significant byte |
        // Only uses the lower two bytes
        // Casting from an unsigned to a signed char --> needs to be converted back to unsigned on the arduino
        Data.append(static_cast<char>(bytes_lower));
        Data.append(static_cast<char>(bytes_upper));

    }

    // Checksum
    unsigned char checksum = message_len ^ message_id;
    for (int i=5; i < Data.length(); i++) {
        checksum ^= Data.at(i);
    }
    Data.append(checksum);

    // Hard-code json
//    QString respString = response ? "\"true\"" : "\"false\"";
//    QString preString = "{\"snd\":\"pc\",\"dst\":\"fc\",\"typ\":\"msp\",\"rsp\":" + respString + ",\"ctrl\":\"true\",\"msp\":\"";
//    QByteArray preData = preString.toLatin1();
//    QString postString = "\"}";
//    QByteArray postData = postString.toLatin1();
//    QByteArray allData = QByteArray();
//    allData.append(preData); allData.append(Data); allData.append(postData);

    QByteArray allData = QByteArray();
    resp_t req_response = response ? RSP_TRUE : RSP_FALSE;
    allData.append(SRC_PC);
    allData.append(DST_FC);
    allData.append(MID_MSP);
    allData.append(req_response);
    allData.append(Data);


    return socket->writeDatagram(allData, IP_ADDR_ESP, PORT_ESP) != -1;

}

// Send a ping to the esp
bool Transmitter::sendPing(const bool response)
{

//    // Construct JSON object
//    QString rspStr = response ? "true" : "false";
//    QJsonObject object {
//        {"snd", "pc"},
//        {"dst", "esp"},
//        {"typ", "ping"},
//        {"rsp", rspStr}
//    };
//    object["ping"] = timerPc.elapsed();
//    QJsonDocument jsonDoc = QJsonDocument(object);
//    QByteArray jsonBytes = jsonDoc.toJson(QJsonDocument::Compact); // Compact representation



    QByteArray send_data = QByteArray();
    send_data.append(SRC_PC);
    send_data.append(DST_ESP);
    send_data.append(MID_PING);
    resp_t req_response = response ? RSP_TRUE : RSP_FALSE;
    send_data.append(req_response);

    // Append timer data
    long long int eltime = timerPc.elapsed();
    for(int i = 0; i != sizeof(eltime); ++i) {
        send_data.append((char)((eltime & (0xFF << (i*8))) >> (i*8)));
    } // Conversion from : https://www.qtcentre.org/threads/10878-how-to-convert-an-int-to-QByteArray
    // Note: cannot send negative timestamps

    return socket->writeDatagram(send_data, IP_ADDR_ESP, PORT_ESP) != -1;

}

// Note: returns true if send was successful --> does not wait for acknowledgement
bool Transmitter::sendEspMode(const char mode, const bool response)
{

//    QString rspStr = response ? "true" : "false";

//    QString modeStr = "";
//    if(mode == MODE_JV)
//        modeStr = "jv";
//    else if(mode == MODE_PC)
//        modeStr = "pc";
//    else {
//        std::cout << "[warn] cannot send invalid esp mode: " << mode << std::endl;
//        return false;
//    }

//    // Construct JSON object
//    QJsonObject object {
//        {"snd", "pc"},
//        {"dst", "esp"},
//        {"typ", "mode"},
//        {"mode", modeStr},
//        {"rsp", rspStr},
//    };
//    QJsonDocument jsonDoc = QJsonDocument(object);
//    QByteArray jsonBytes = jsonDoc.toJson(QJsonDocument::Compact); // Compact representation

    if(mode == ESP_MODE_ERR) {
        std::cout << "[warn] cannot send invalid jv controller mode: " << mode << std::endl;
        return false;
    }

    QByteArray send_data = QByteArray();
    send_data.append(SRC_PC);
    send_data.append(DST_ESP);
    send_data.append(MID_MODE);
    resp_t req_response = response ? RSP_TRUE : RSP_FALSE;
    send_data.append(req_response);
    send_data.append(mode);

    return socket->writeDatagram(send_data, IP_ADDR_ESP, PORT_ESP) != -1;

}

bool Transmitter::sendJvController(const char mode, const bool response)
{

//    QString rspStr = response ? "true" : "false";

//    int modeInt = 0;
//    if(mode == JV_CTRL_TRUE)
//        modeInt = JV_CTRL_TRUE;
//    else if(mode == JV_CTRL_FALSE)
//        modeInt = JV_CTRL_FALSE;
//    else {
//        std::cout << "[warn] cannot send invalid jv controller mode: " << mode << std::endl;
//        return false;
//    }

//    // Construct JSON object
//    QJsonObject object {
//        {"snd", "pc"},
//        {"dst", "jv"},
//        {"typ", "mode"},
//        {"mode", modeInt},
//        {"rsp", rspStr},
//    };
//    QJsonDocument jsonDoc = QJsonDocument(object);
//    QByteArray jsonBytes = jsonDoc.toJson(QJsonDocument::Compact); // Compact representation

    if(mode == JV_CTRL_ERR) {
        std::cout << "[warn] cannot send invalid jv controller mode: " << mode << std::endl;
        return false;
    }

    QByteArray send_data = QByteArray();
    send_data.append(SRC_PC);
    send_data.append(DST_JV);
    send_data.append(MID_MODE);
    resp_t req_response = response ? RSP_TRUE : RSP_FALSE;
    send_data.append(req_response);
    send_data.append(mode);

    return socket->writeDatagram(send_data, IP_ADDR_ESP, PORT_ESP) != -1;

}

bool Transmitter::sendJvLanding(const char mode, const bool response)
{

//    QString rspStr = response ? "true" : "false";

//    int modeInt = 0;
//    if(mode == JV_CTRL_TRUE)
//        modeInt = JV_CTRL_TRUE;
//    else if(mode == JV_CTRL_FALSE)
//        modeInt = JV_CTRL_FALSE;
//    else {
//        std::cout << "[warn] cannot send invalid jv landing mode: " << mode << std::endl;
//        return false;
//    }

//    // Construct JSON object
//    QJsonObject object {
//        {"snd", "pc"},
//        {"dst", "jv"},
//        {"typ", "land"},
//        {"land", modeInt},
//        {"rsp", rspStr},
//    };
//    QJsonDocument jsonDoc = QJsonDocument(object);
//    QByteArray jsonBytes = jsonDoc.toJson(QJsonDocument::Compact); // Compact representation


    if(mode == JV_LAND_ERR) {
        std::cout << "[warn] cannot send invalid jv controller mode: " << mode << std::endl;
        return false;
    }

    QByteArray send_data = QByteArray();
    send_data.append(SRC_PC);
    send_data.append(DST_JV);
    send_data.append(MID_LAND);
    resp_t req_response = response ? RSP_TRUE : RSP_FALSE;
    send_data.append(req_response);
    send_data.append(mode);

    return socket->writeDatagram(send_data, IP_ADDR_ESP, PORT_ESP) != -1;

}

bool Transmitter::sendJvQuit()
{

//    // Construct JSON object
//    QJsonObject object {
//        {"snd", "pc"},
//        {"dst", "jv"},
//        {"typ", "quit"}
//    };
//    QJsonDocument jsonDoc = QJsonDocument(object);
//    QByteArray jsonBytes = jsonDoc.toJson(QJsonDocument::Compact); // Compact representation

    QByteArray send_data = QByteArray();
    send_data.append(SRC_PC);
    send_data.append(DST_JV);
    send_data.append(MID_QUIT);
    send_data.append(RSP_FALSE);
    send_data.append(JV_QUIT_TRUE);

    return socket->writeDatagram(send_data, IP_ADDR_ESP, PORT_ESP) != -1;

}


// method to receive incoming data
// This method is called when data is available
void Transmitter::readUDP()
{

    while (socket->hasPendingDatagrams()) {
        // Read the datagram
        QNetworkDatagram datagram = socket->receiveDatagram();
        // Check that the sender is the esp and it is a valid datagram
        if(!datagram.senderAddress().isEqual(IP_ADDR_ESP, QHostAddress::ConvertV4MappedToIPv4) || datagram.destinationPort() != PORT_ESP || datagram.isNull()) {return;}
        // Extract the payload data from the datagram
        QByteArray payload = datagram.data();
        // Parse the payload data/act on the info
        parsePacket(payload);
   }

}

void Transmitter::parsePacket(QByteArray &data)
{
//    // Convert the byte array into a json document
//    QJsonDocument document = QJsonDocument::fromJson(data);
//    // Check it is a valid json packet
//    if(document.isNull() || document.isEmpty()) {return;}
//    // Extract the json object
//    QJsonObject object = document.object();

//    // Must contain a sender, destination and type -- not necessary since the resultant if statements would fail anyway
//    // if(!(object.contains("snd") && object.contains("dst") && object.contains("typ"))) {return;}
//    // Check the destination is the pc
//    if(object["dst"] != "pc") {return;}
//    // If the esp is the original sender (as opposed to the jevois)
//    if(object["snd"] == "esp") {

//        // If the type is a 'mode' message
//        if(object["typ"] == "mode") {
//            parseEspMode(object);
//        }

//        else if(object["typ"] == "msp") {
//        }

//        else if(object["typ"] == "alt") {
//            parseAltitude(object["alt"].toObject());
//        }

//        else if(object["typ"] == "ping") {
//            parsePing(object);

//        }

//        else if(object["typ"] == "log") {
//            parseLog(object, 0);
//        }
//    }

//    else if(object["snd"] == "jv") {

//        if(object["typ"] == "log") {
//            parseLog(object, 1);
//        }

//        else if(object["typ"] == "mode") {
//            parseJvController(object);
//        }

//        else if(object["typ"] == "land") {
//            parseJvLanding(object);
//        }
//    }


    if(data.length()<4) { return; }
    char src = data.at(0);
    char dst = data.at(1);
    char mid = data.at(2);
    char resp = data.at(3);

    // Check the destination is the pc
    if(dst != DST_PC) {
        std::cout << "[error] pc not dest: ";
        for(int i=0; i<data.length(); i++) {
            std::cout << (int)data.at(i) << " ";
        }
        std::cout << std::endl;
        return;
    }

    // If the esp is the original sender
    if(src == SRC_ESP) {

        if(mid == MID_MODE && data.length() == 5) {
            updateCurrentEspMode(data.at(4));
        }
        else if(mid == MID_ALT) {
            QByteArray alt_data = data.mid(4, -1);
            parseAltitude(alt_data);
        }
        else if(mid == MID_PING && data.length() == 4+sizeof(long long int)) {

            long long int eltime_recv = 0;
            for(int i = 0; i != sizeof(eltime_recv); ++i) {
                eltime_recv = eltime_recv | (static_cast<long long int>(static_cast<unsigned char>(data.at(4+i))) << (i*8));
            }

            pingLoopTime = (long int)(timerPc.elapsed() - eltime_recv);
            if(pingLoopTime > PING_LIMIT) {
                std::cout << "[warn] long ping time: " << pingLoopTime << ", elapsed: " << timerPc.elapsed() << ", recv: " << eltime_recv << std::endl;
            }

            emit pingReceived((int)pingLoopTime);

        }
        else if(mid == MID_JSON && data.length() >= 6) {

            QByteArray message = data.mid(4, -1);
            QJsonDocument document = QJsonDocument::fromJson(message);
            // Check it is a valid json packet
            if(document.isNull() || document.isEmpty()) {return;}
            // Extract the json object
            QJsonObject object = document.object();

            if(object["typ"] == "log") {
                if(object.contains("log") && object.contains("lvl")) {
                    emit logReceived(SRC_ESP, object["log"].toString(), object["lvl"].toInt());
                }
            }

        }
        else {
            std::cout << "[error] esp data no match: ";
            for(int i=0; i<data.length(); i++) {
                std::cout << (int)data.at(i) << " ";
            }
            std::cout << std::endl;
        }

    }
    else if(src == SRC_JV) {
        if(mid == MID_MODE && data.length() == 5) {
            updateCurrentJvController(data.at(4));
        }
        else if(mid == MID_LAND && data.length() == 5) {
            updateCurrentJvLanding(data.at(4));
        }
        else if(mid == MID_JSON && data.length() >= 6) {

            QByteArray message = data.mid(4, -1);
            QJsonDocument document = QJsonDocument::fromJson(message);
            // Check it is a valid json packet
            if(document.isNull() || document.isEmpty()) {return;}
            // Extract the json object
            QJsonObject object = document.object();

            if(object["typ"] == "log") {
                if(object.contains("log") && object.contains("lvl")) {
                    emit logReceived(SRC_JV, object["log"].toString(), object["lvl"].toInt());
                }
            }

        }
        else {
            std::cout << "[error] jv data no match: ";
            for(int i=0; i<data.length(); i++) {
                std::cout << (int)data.at(i) << " ";
            }
            std::cout << std::endl;
        }

    }
    else if(src == SRC_FC) {
        if(mid == MID_MSP) {
            std::cout << "[info] msp received" << std::endl;
        }
    }
    else {
        std::cout << "[error] data no match: ";
        for(int i=0; i<data.length(); i++) {
            std::cout << (int)data.at(i) << " ";
        }
        std::cout << std::endl;
    }

}

//void Transmitter::parseEspMode(QJsonObject mode_obj)
//{
//    if(mode_obj["mode"] == "pc") {updateCurrentEspMode(MODE_PC);}
//    else if(mode_obj["mode"] == "jv") {updateCurrentEspMode(MODE_JV);}
//    else {updateCurrentEspMode(MODE_ERR);}
//}

void Transmitter::parseAltitude(QByteArray &altData)
{

    if(altData.length() != 22) { return; }

    // Convert to a vector of unsigned chars
    std::vector<unsigned char> altData_u;
    altData_u.reserve(altData.length());
    for(int i=0; i<altData.length(); i++) {
        altData_u.push_back(static_cast<unsigned char>(altData.at(i)));
    }

    mod_VL53L1_RangingMeasurementData_t rawData = {0, 0, 0, 0, 0, 0, 0, 0};
    rawData.TimeStamp = (std::uint32_t)altData_u.at(0) | (std::uint32_t)altData_u.at(1) << 8 | (std::uint32_t)altData_u.at(2) << 16 | (std::uint32_t)altData_u.at(3) << 24;
    rawData.StreamCount = (std::uint8_t)altData_u.at(4);
    rawData.SignalRateRtnMegaCps = (FixPoint1616_t)altData_u.at(5) | (FixPoint1616_t)altData_u.at(6) << 8 | (FixPoint1616_t)altData_u.at(7) << 16 | (FixPoint1616_t)altData_u.at(8) << 24;
    rawData.AmbientRateRtnMegaCps = (FixPoint1616_t)altData_u.at(9) | (FixPoint1616_t)altData_u.at(10) << 8 | (FixPoint1616_t)altData_u.at(11) << 16 | (FixPoint1616_t)altData_u.at(12) << 24;
    rawData.EffectiveSpadRtnCount = (std::uint16_t)altData_u.at(13) | (std::uint16_t)altData_u.at(14) << 8;
    rawData.SigmaMilliMeter = (FixPoint1616_t)altData_u.at(15) | (FixPoint1616_t)altData_u.at(16) << 8 | (FixPoint1616_t)altData_u.at(17) << 16 | (FixPoint1616_t)altData_u.at(18) << 24;
    rawData.RangeMilliMeter = (std::int16_t)altData_u.at(19) | (std::int16_t)altData_u.at(20) << 8;
    rawData.RangeStatus = (std::uint8_t)altData_u.at(21);

    RangingData_t convData;
    convData.timeEsp_ms = (long int) rawData.TimeStamp;
    convData.timePc_ms = (long int) timerPc.elapsed();
    convData.signal_rate = rawData.SignalRateRtnMegaCps/65336.0;
    convData.ambient_rate = rawData.AmbientRateRtnMegaCps/65336.0;
    convData.eff_spad_count = rawData.EffectiveSpadRtnCount/256.0;
    convData.sigma_mm = rawData.SigmaMilliMeter/65336.0;
    convData.range_mm = rawData.RangeMilliMeter;
    convData.status = rawData.RangeStatus;

//    RangingData_t altData;
//    altData.signal_rate = alt_obj["sigrt"].toDouble();
//    altData.ambient_rate = alt_obj["ambrt"].toDouble();
//    altData.sigma_mm = alt_obj["sigma"].toDouble();
//    altData.eff_spad_count = alt_obj["spad"].toDouble()/256; // divide by 256 for real value
//    altData.range_mm = alt_obj["range"].toInt();
//    altData.timeEsp_ms = alt_obj["time"].toInt();
//    altData.status = alt_obj["status"].toInt();
//    altData.timePc_ms = (int)timerPc.elapsed();

    // Update state estimate
    altEstimator->addRangeMeasurement(convData);
    AltState_t estimatedState = altEstimator->getStateEstimate();

    // Update controller
    if(controllerActive) {
        altController->addEstState(estimatedState);
    }

    emit altRangeReceived((int)convData.timeEsp_ms, convData.range_mm);
    emit altStateEstimate((int)estimatedState.timeEsp_ms, estimatedState.z, estimatedState.z_dot);

}


//void Transmitter::parseAltitude(QJsonObject alt_obj)
//{

//    if(alt_obj.contains("sigrt") && alt_obj.contains("ambrt") && alt_obj.contains("sigma")
//            && alt_obj.contains("spad") && alt_obj.contains("range") && alt_obj.contains("time")
//            && alt_obj.contains("status") )
//    {
//        RangingData_t altData;
//        altData.signal_rate = alt_obj["sigrt"].toDouble();
//        altData.ambient_rate = alt_obj["ambrt"].toDouble();
//        altData.sigma_mm = alt_obj["sigma"].toDouble();
//        altData.eff_spad_count = alt_obj["spad"].toDouble()/256; // divide by 256 for real value
//        altData.range_mm = alt_obj["range"].toInt();
//        altData.timeEsp_ms = alt_obj["time"].toInt();
//        altData.status = alt_obj["status"].toInt();
//        altData.timePc_ms = (int)timerPc.elapsed();

//        // Update state estimate
//        altEstimator->addRangeMeasurement(altData);
//        AltState_t estimatedState = altEstimator->getStateEstimate();

//        // Update controller
//        if(controllerActive) {
//            altController->addEstState(estimatedState);
//        }

//        emit altRangeReceived(altData.timeEsp_ms, altData.range_mm);
//        emit altStateEstimate(estimatedState.timeEsp_ms, estimatedState.z, estimatedState.z_dot);

//    }
//}


//void Transmitter::parsePing(QJsonObject ping_obj)
//{

//    // Get the round-trip time for the ping
//    if(ping_obj.contains("ping")) {
//        pingLoopTime = timerPc.elapsed() - ping_obj["ping"].toInt();
//        if(pingLoopTime > PING_TIMEOUT) {
//            std::cout << "[warn] long ping time: " << pingLoopTime << std::endl;
//        }
//        emit pingReceived(pingLoopTime);
//    }
//}

//void Transmitter::parseLog(QJsonObject log_obj, char dev)
//{
//    if(log_obj.contains("log") && log_obj.contains("lvl")) {
//        emit logReceived(dev, log_obj["log"].toString(), log_obj["lvl"].toInt());
//    }
//}

//void Transmitter::parseJvController(QJsonObject mode_obj)
//{
//    if(mode_obj["mode"].toInt() == JV_CTRL_TRUE) {updateCurrentJvController(JV_CTRL_TRUE);}
//    else if(mode_obj["mode"].toInt() == JV_CTRL_FALSE) {updateCurrentJvController(JV_CTRL_FALSE);}
//    else {updateCurrentJvController(JV_CTRL_ERR);}
//}


//void Transmitter::parseJvLanding(QJsonObject land_obj)
//{
//    if(land_obj["land"].toInt() == JV_LAND_TRUE) {updateCurrentJvLanding(JV_LAND_TRUE);}
//    else if(land_obj["land"].toInt() == JV_LAND_FALSE) {updateCurrentJvLanding(JV_LAND_FALSE);}
//    else {updateCurrentJvLanding(JV_LAND_ERR);}
//}


// *********************************************************
// Files
// *********************************************************

bool Transmitter::openFiles()
{

    if (file_log.is_open())
        return false;
    std::string name_log = fileDirectory + "/" + prefix_log + suffix + format;
    file_log.open(name_log, std::ios::out | std::ios::app); // Append the file contents to prevent overwrite
    bool status = file_log.is_open();
    status &= altEstimator->openFiles();
    status &= altController->openFiles();

    if (file_log.is_open()) {
        file_log << std::endl << header_log << std::endl;
        filesOpen = true;
    }

    return status;
}

void Transmitter::closeFiles()
{
    file_log.close();
    altEstimator->closeFiles();
    altController->closeFiles();
    filesOpen = false;
}

void Transmitter::setFileSuffix(QString suffix_in)
{
    suffix = suffix_in.toStdString();
    altEstimator->setFileSuffix(suffix);
    altController->setFileSuffix(suffix);
}

QString Transmitter::getFileSuffix()
{
    QString newSuffix = QString();
    newSuffix.fromStdString(suffix);
    return newSuffix;
}

void Transmitter::setFileDirectory(QString directory)
{
    fileDirectory = directory.toStdString();
    altEstimator->setFileDirectory(fileDirectory);
    altController->setFileDirectory(fileDirectory);
}

QString Transmitter::getFileDirectory()
{
    QString newStr = QString();
    newStr.fromStdString(fileDirectory);
    return newStr;
}


