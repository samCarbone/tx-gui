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
    updateCurrentEspMode(MODE_ERR); // Esp mode is initially unknown
    setDesiredEspMode(MODE_PC); // Start with the PC mode

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

void Transmitter::updateCurrentEspMode(const int newMode)
{
    currentEspMode = newMode;
    emit espModeChanged(currentEspMode);
}

void Transmitter::setDesiredEspMode(const int newMode)
{
    desiredEspMode = newMode;
    emit desiredEspModeChanged(desiredEspMode);
}

int Transmitter::getCurrentEspMode()
{
    return currentEspMode;
}

int Transmitter::getDesiredEspMode()
{
    return desiredEspMode;
}

void Transmitter::setModeFromChannel(const int channel, const double value)
{
    if(channel == SET_MODE_CHANNEL) {
        if(!controllerStandby) {
            if(value > 50 && armed) {
                setDesiredEspMode(MODE_JV);
                setDesiredJvController(JV_CTRL_TRUE);
            }
            else {
                setDesiredEspMode(MODE_PC);
                setDesiredJvController(JV_CTRL_FALSE);
                setDesiredJvLanding(JV_LAND_FALSE);
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
            setDesiredEspMode(MODE_PC);
            setDesiredJvController(JV_CTRL_FALSE);
            setDesiredJvLanding(JV_LAND_FALSE);
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
        setDesiredEspMode(MODE_PC);
        setDesiredJvController(JV_CTRL_FALSE);
        setDesiredJvLanding(JV_LAND_FALSE);
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

void Transmitter::updateCurrentJvController(const int newMode)
{
    current_jvController = newMode;
    emit jvControllerChanged(current_jvController);
}

void Transmitter::setDesiredJvController(const int newMode)
{
    desired_jvController = newMode;
    emit desiredJvControllerChanged(desired_jvController);

}

void Transmitter::updateCurrentJvLanding(const int newMode)
{
    current_jvLanding = newMode;
    emit jvLandingChanged(current_jvLanding);
}

void Transmitter::setDesiredJvLanding(const int newMode)
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

        if(desiredEspMode == MODE_PC) {
            double chnThr, chnEle, chnAil, chnRud;

            if(controllerStandby) {
            // Calculate propagated state
                AltState_t propAltState = altEstimator->getPropagatedStateEstimate_safe(timerPc.elapsed()+pingLoopTime, PING_TIMEOUT);

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
                emit altPropStateEstimate(propAltState.timeEsp_ms, propAltState.z, propAltState.z_dot);

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
    QString respString = response ? "\"true\"" : "\"false\"";
    QString preString = "{\"snd\":\"pc\",\"dst\":\"fc\",\"typ\":\"msp\",\"rsp\":" + respString + ",\"ctrl\":\"true\",\"msp\":\"";
    QByteArray preData = preString.toLatin1();
    QString postString = "\"}";
    QByteArray postData = postString.toLatin1();
    QByteArray allData = QByteArray();
    allData.append(preData); allData.append(Data); allData.append(postData);

    return socket->writeDatagram(allData, IP_ADDR_ESP, PORT_ESP) != -1;

}

// Send a ping to the esp
bool Transmitter::sendPing(const bool response)
{

    // Construct JSON object
    QString rspStr = response ? "true" : "false";
    QJsonObject object {
        {"snd", "pc"},
        {"dst", "esp"},
        {"typ", "ping"},
        {"rsp", rspStr}
    };

    object["ping"] = timerPc.elapsed();

    QJsonDocument jsonDoc = QJsonDocument(object);
    QByteArray jsonBytes = jsonDoc.toJson(QJsonDocument::Compact); // Compact representation
    return socket->writeDatagram(jsonBytes, IP_ADDR_ESP, PORT_ESP) != -1;

}

// Note: returns true if send was successful --> does not wait for acknowledgement
bool Transmitter::sendEspMode(const int mode, const bool response)
{

    QString rspStr = response ? "true" : "false";

    QString modeStr = "";
    if(mode == MODE_JV)
        modeStr = "jv";
    else if(mode == MODE_PC)
        modeStr = "pc";
    else {
        std::cout << "[warn] cannot send invalid esp mode: " << mode << std::endl;
        return false;
    }

    // Construct JSON object
    QJsonObject object {
        {"snd", "pc"},
        {"dst", "esp"},
        {"typ", "mode"},
        {"mode", modeStr},
        {"rsp", rspStr},
    };

    QJsonDocument jsonDoc = QJsonDocument(object);
    QByteArray jsonBytes = jsonDoc.toJson(QJsonDocument::Compact); // Compact representation
    return socket->writeDatagram(jsonBytes, IP_ADDR_ESP, PORT_ESP) != -1;

}

bool Transmitter::sendJvController(const int mode, const bool response)
{

    QString rspStr = response ? "true" : "false";

    int modeInt = 0;
    if(mode == JV_CTRL_TRUE)
        modeInt = JV_CTRL_TRUE;
    else if(mode == JV_CTRL_FALSE)
        modeInt = JV_CTRL_FALSE;
    else {
        std::cout << "[warn] cannot send invalid jv controller mode: " << mode << std::endl;
        return false;
    }

    // Construct JSON object
    QJsonObject object {
        {"snd", "pc"},
        {"dst", "jv"},
        {"typ", "mode"},
        {"mode", modeInt},
        {"rsp", rspStr},
    };

    QJsonDocument jsonDoc = QJsonDocument(object);
    QByteArray jsonBytes = jsonDoc.toJson(QJsonDocument::Compact); // Compact representation
    return socket->writeDatagram(jsonBytes, IP_ADDR_ESP, PORT_ESP) != -1;

}

bool Transmitter::sendJvLanding(const int mode, const bool response)
{

    QString rspStr = response ? "true" : "false";

    int modeInt = 0;
    if(mode == JV_CTRL_TRUE)
        modeInt = JV_CTRL_TRUE;
    else if(mode == JV_CTRL_FALSE)
        modeInt = JV_CTRL_FALSE;
    else {
        std::cout << "[warn] cannot send invalid jv landing mode: " << mode << std::endl;
        return false;
    }

    // Construct JSON object
    QJsonObject object {
        {"snd", "pc"},
        {"dst", "jv"},
        {"typ", "land"},
        {"land", modeInt},
        {"rsp", rspStr},
    };

    QJsonDocument jsonDoc = QJsonDocument(object);
    QByteArray jsonBytes = jsonDoc.toJson(QJsonDocument::Compact); // Compact representation
    return socket->writeDatagram(jsonBytes, IP_ADDR_ESP, PORT_ESP) != -1;

}

bool Transmitter::sendJvQuit()
{

    // Construct JSON object
    QJsonObject object {
        {"snd", "pc"},
        {"dst", "jv"},
        {"typ", "quit"}
    };

    QJsonDocument jsonDoc = QJsonDocument(object);
    QByteArray jsonBytes = jsonDoc.toJson(QJsonDocument::Compact); // Compact representation
    return socket->writeDatagram(jsonBytes, IP_ADDR_ESP, PORT_ESP) != -1;

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
    // Convert the byte array into a json document
    QJsonDocument document = QJsonDocument::fromJson(data);
    // Check it is a valid json packet
    if(document.isNull() || document.isEmpty()) {return;}
    // Extract the json object
    QJsonObject object = document.object();

    // Must contain a sender, destination and type -- not necessary since the resultant if statements would fail anyway
    // if(!(object.contains("snd") && object.contains("dst") && object.contains("typ"))) {return;}
    // Check the destination is the pc
    if(object["dst"] != "pc") {return;}
    // If the esp is the original sender (as opposed to the jevois)
    if(object["snd"] == "esp") {

        // If the type is a 'mode' message
        if(object["typ"] == "mode") {
            parseEspMode(object);
        }

        else if(object["typ"] == "msp") {
        }

        else if(object["typ"] == "alt") {
            parseAltitude(object["alt"].toObject());
        }

        else if(object["typ"] == "ping") {
            parsePing(object);

        }

        else if(object["typ"] == "log") {
            parseLog(object, 0);
        }
    }

    else if(object["snd"] == "jv") {

        if(object["typ"] == "log") {
            parseLog(object, 1);
        }

        else if(object["typ"] == "mode") {
            parseJvController(object);
        }

        else if(object["typ"] == "land") {
            parseJvLanding(object);
        }
    }
}

void Transmitter::parseEspMode(QJsonObject mode_obj)
{
    if(mode_obj["mode"] == "pc") {updateCurrentEspMode(MODE_PC);}
    else if(mode_obj["mode"] == "jv") {updateCurrentEspMode(MODE_JV);}
    else {updateCurrentEspMode(MODE_ERR);}
}

void Transmitter::parseAltitude(QJsonObject alt_obj)
{

    if(alt_obj.contains("sigrt") && alt_obj.contains("ambrt") && alt_obj.contains("sigma")
            && alt_obj.contains("spad") && alt_obj.contains("range") && alt_obj.contains("time")
            && alt_obj.contains("status") )
    {
        RangingData_t altData;
        altData.signal_rate = alt_obj["sigrt"].toDouble();
        altData.ambient_rate = alt_obj["ambrt"].toDouble();
        altData.sigma_mm = alt_obj["sigma"].toDouble();
        altData.eff_spad_count = alt_obj["spad"].toDouble()/256; // divide by 256 for real value
        altData.range_mm = alt_obj["range"].toInt();
        altData.timeEsp_ms = alt_obj["time"].toInt();
        altData.status = alt_obj["status"].toInt();
        altData.timePc_ms = (int)timerPc.elapsed();

        // Update state estimate
        altEstimator->addRangeMeasurement(altData);
        AltState_t estimatedState = altEstimator->getStateEstimate();

        // Update controller
        if(controllerActive) {
            altController->addEstState(estimatedState);
        }

        emit altRangeReceived(altData.timeEsp_ms, altData.range_mm);
        emit altStateEstimate(estimatedState.timeEsp_ms, estimatedState.z, estimatedState.z_dot);

    }
}

void Transmitter::parsePing(QJsonObject ping_obj)
{

    // Get the round-trip time for the ping
    if(ping_obj.contains("ping")) {
        pingLoopTime = timerPc.elapsed() - ping_obj["ping"].toInt();
        if(pingLoopTime > PING_TIMEOUT) {
            std::cout << "[warn] long ping time: " << pingLoopTime << std::endl;
        }
        emit pingReceived(pingLoopTime);
    }
}

void Transmitter::parseLog(QJsonObject log_obj, int dev)
{

    // Get the round-trip time for the ping
    if(log_obj.contains("log") && log_obj.contains("lvl")) {

        emit logReceived(dev, log_obj["log"].toString(), log_obj["lvl"].toInt());
    }
}

void Transmitter::parseJvController(QJsonObject mode_obj)
{
    if(mode_obj["mode"].toInt() == JV_CTRL_TRUE) {updateCurrentJvController(JV_CTRL_TRUE);}
    else if(mode_obj["mode"].toInt() == JV_CTRL_FALSE) {updateCurrentJvController(JV_CTRL_FALSE);}
    else {updateCurrentJvController(JV_CTRL_ERR);}
}


void Transmitter::parseJvLanding(QJsonObject land_obj)
{
    if(land_obj["land"].toInt() == JV_LAND_TRUE) {updateCurrentJvLanding(JV_LAND_TRUE);}
    else if(land_obj["land"].toInt() == JV_LAND_FALSE) {updateCurrentJvLanding(JV_LAND_FALSE);}
    else {updateCurrentJvLanding(JV_LAND_ERR);}
}


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


