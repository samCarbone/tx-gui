#include "transmitter.h"
#include <iostream>
#include <math.h>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonDocument>
#include <QNetworkDatagram>
#include <QTextCodec>

Transmitter::Transmitter()
{
    // Default ID
    joystickID = 0;

    timer.start();

    // create a QUDP socket
    socket = new QUdpSocket(this);
    socket->bind(QHostAddress::Any, port);

    // Connect socket readyRead() signal with the transmitter readUDP() slot
    QObject::connect(socket, &QUdpSocket::readyRead, this, &Transmitter::readUdp);

    // Connect channelChanged() signal with the  setModeFromChannelChanged() slot
    QObject::connect(this, &Transmitter::channelChanged, this, &Transmitter::setModeFromChannelChanged);
}

//Transmitter::Transmitter(int ID)
//{
//    // Set ID
//    joystickID = ID;

//    // create a QUDP socket
//    socket = new QUdpSocket(this);

//}

//bool Transmitter::connectToHost(QString address, quint16 port)
//{
//    socket->connectToHost(QHostAddress(address), port); // Set this in UI
//    return socket->state() == QAbstractSocket::SocketState::ConnectedState && socket->isValid();
//}

void Transmitter::setJoystickID(int ID) { joystickID = ID; }

int Transmitter::getJoystickID() { return joystickID; }

double Transmitter::getChannelValue(int channel) { return channels[channel]; }

void Transmitter::setChannelValue(int channel, double value)
{

    if (value > MAX_CHANNEL_VALUE) {
        value = MAX_CHANNEL_VALUE;
        std::cout << "Warning: channel value out of range (greater)" << std::endl;
    }
    else if (value < MIN_CHANNEL_VALUE) {
        value = MIN_CHANNEL_VALUE;
        // std::cout << "Warning: channel value out of range (lower)" << std::endl;
    }

    channels.at(channel) = value;
    emit channelChanged(channel, value);

}

bool Transmitter::sendChannelsWithMode()
{
    bool success = true;
    if(espMode != desiredEspMode)
        success &= sendEspMode(true);

    if(desiredEspMode == MODE_PC)
        success &= sendChannels();

    return success;
}

bool Transmitter::sendChannels()
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

        int rangedValue = toTxRange(value);
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


//    // Construct JSON object
//    QJsonObject object {
//        {"snd", "pc"},
//        {"dst", "fc"},
//        {"typ", "msp"},
//        {"rsp", "false"}
//    };

//    QLatin1String latinString = QLatin1String(Data);
//    QJsonValue latinJsonValue = QJsonValue(latinString);
//    QString convertedString = QString(latinString);



//    object.insert("msp", QJsonValue(convertedString));

//    QJsonDocument jsonDoc = QJsonDocument(object);
//    QByteArray jsonBytes = jsonDoc.toJson(QJsonDocument::Compact); // Compact representation

//    jsonDoc.toJson().to

//    QString newString2 = QTextCodec::codecForUtfText(jsonBytes)->toUnicode(jsonBytes);
//    QString newString3 = QString::fromUtf8(jsonBytes);

//    QTextCodec *codec = QTextCodec::codecForName( "UTF-16" );
//    QString newString4 = codec->toUnicode( jsonBytes );

    // Hard-code json
    QString preString = "{\"snd\":\"pc\",\"dst\":\"fc\",\"typ\":\"msp\",\"rsp\":\"true\",\"ctrl\":\"true\",\"msp\":\"";
    QByteArray preData = preString.toLatin1();
    QString postString = "\"}";
    QByteArray postData = postString.toLatin1();
    QByteArray allData = QByteArray();
    allData.append(preData); allData.append(Data); allData.append(postData);

    return socket->writeDatagram(allData, espAddress, port) != -1;

}

// Send a ping to the esp
bool Transmitter::sendPing() {

    // Construct JSON object
    QJsonObject object {
        {"snd", "pc"},
        {"dst", "esp"},
        {"typ", "ping"},
        {"rsp", "true"}
    };

    object["ping"] = timer.elapsed();

    QJsonDocument jsonDoc = QJsonDocument(object);
    QByteArray jsonBytes = jsonDoc.toJson(QJsonDocument::Compact); // Compact representation
    return socket->writeDatagram(jsonBytes, espAddress, port) != -1;

}

int Transmitter::toTxRange(double value)
{
    if (value > MAX_CHANNEL_VALUE) {
        value = MAX_CHANNEL_VALUE;
        std::cout << "Warning: channel value out of range (greater)" << std::endl;
    }
    else if (value < MIN_CHANNEL_VALUE) {
        value = MIN_CHANNEL_VALUE;
        std::cout << "Warning: channel value out of range (lower)" << std::endl;
    }

    return round(value*6 + 1500); // Scaled from (-100, 100) to (900, 2100)
}

Transmitter::~Transmitter()
{
//    socket->disconnectFromHost();
}


// Slots

// Note: this function should only be used for the joystick inputs
// due to the use of different mappings for each axis
void Transmitter::axisChanged(const int js, const int axis, const double value)
{
    if(js != joystickID)
        return;

    if(axis < 0 || axis >= (int)channelMap.size())
        return;

    // Convert value from range of (-1,1) to (-100, 100)
    double mappedValue = channelMultipliers.at(axis)*value + channelOffsets.at(axis);

    setChannelValue(channelMap.at(axis), mappedValue);
}


void Transmitter::buttonChanged(const int js, const int button, const bool pressed) {
    std::cout << button << std::endl;

    if(js != joystickID)
        return;



    if(button < 0 || button >= (int)buttonMap.size())
        return;

    double value = pressed ? 100 : -100;

    setChannelValue(buttonMap.at(button), value);
}




// Note: returns true if send was successful --> does not wait for acknowledgement
bool Transmitter::sendEspMode(bool rsp) {

    QString rspStr = "false";
    if(rsp)
        rspStr = "true";
    QString modeStr = "";
    if(desiredEspMode == MODE_JV)
        modeStr = "jv";
    else if(desiredEspMode == MODE_PC)
        modeStr = "pc";
    else {
        std::cout << "Warning: cannot send invalid esp mode: " << desiredEspMode << std::endl;
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
    return socket->writeDatagram(jsonBytes, espAddress, port) != -1;

}


// method to receive incoming data
// This method is called when data is available
void Transmitter::readUdp() {

    while (socket->hasPendingDatagrams()) {

        // Read the datagram
        QNetworkDatagram datagram = socket->receiveDatagram();

        // std::cout << datagram.senderAddress().toString().toStdString() << "," << datagram.destinationPort() << std::endl;
//        std::cout << datagram.senderAddress().isEqual(espAddress, QHostAddress::ConvertV4MappedToIPv4) << ", " << (datagram.destinationPort() != port) <<  std::endl;

        // Check that the sender is the esp and it is a valid datagram
        if(!datagram.senderAddress().isEqual(espAddress, QHostAddress::ConvertV4MappedToIPv4) || datagram.destinationPort() != port || datagram.isNull()) {return;}
        // Extract the payload data from the datagram
        QByteArray payload = datagram.data();
        // Parse the payload data/act on the info
        parsePacket(payload);
   }
}

void Transmitter::parsePacket(QByteArray &data) {

    // std::cout << data.toStdString() << std::endl;

    // Convert the byte array into a json document
    QJsonDocument document = QJsonDocument::fromJson(data);
    // Check it is a valid json packet
    if(document.isNull() || document.isEmpty()) {return;}
    // Extract the json object
    QJsonObject object = document.object();

    // Must contain a sender, destination and type -- not necessary since the resultant if statements would fail anyway
//    if(!(object.contains("snd") && object.contains("dst") && object.contains("typ"))) {return;}
    // Check the destination is the pc
    if(object["dst"] != "pc") {return;}
    // If the esp is the original sender (as opposed to the jevois)
    if(object["snd"] == "esp") {

        // If the type is a 'mode' message
        if(object["typ"] == "mode") {
            if(object["mode"] == "pc") {setEspMode(MODE_PC);}
            else if(object["mode"] == "jv") {setEspMode(MODE_JV);}
            else {setEspMode(MODE_ERR);}
        }

//        else if(object["typ"] == "msp") {
//        }

        else if(object["typ"] == "alt") {
            parseAltitude(object["alt"].toObject());
        }

        else if(object["typ"] == "ping") {
            parsePing(object);
        }
    }
}

void Transmitter::setEspMode(int newMode) {
    espMode = newMode;
    emit espModeChanged(espMode);
}

void Transmitter::setDesiredEspMode(int newMode) {
    desiredEspMode = newMode;
    emit desiredEspModeChanged(desiredEspMode);
}

int Transmitter::getEspMode() {
    return espMode;
}

int Transmitter::getDesiredEspMode() {
    return desiredEspMode;
}

// Change the esp mode based upon the channel values
void Transmitter::setModeFromChannelChanged(const int channel, const double value) {
    if(channel == SET_MODE_CHANNEL) {
        if(value > 50)
            setDesiredEspMode(MODE_JV);
        else
            setDesiredEspMode(MODE_PC);
    }
}

void Transmitter::parseAltitude(QJsonObject alt_obj) {

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
        altData.time_meas_ms = alt_obj["time"].toInt();
        altData.status = alt_obj["status"].toInt();
        altData.time_recv_ms = (int)timer.elapsed();

//        std::cout << altData.range_mm << ", " << altData.time_meas_ms << std::endl;

        emit altitudeReceived(altData);
        emit altitudeRangeReceived(altData.time_meas_ms, altData.range_mm);
    }
}

void Transmitter::parsePing(QJsonObject ping_obj) {

    // Get the round-trip time for the ping
    if(ping_obj.contains("ping")) {
        pingLoopTime = timer.elapsed() - ping_obj["ping"].toInt();
        if(pingLoopTime > 100) {
            std::cout << "[warn] long ping time: " << pingLoopTime << std::endl;
        }
        emit pingReceived(pingLoopTime);
    }
}
