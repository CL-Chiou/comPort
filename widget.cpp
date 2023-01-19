#include "widget.h"

#include "./ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::Widget),
      timerCheckReceivedMessage(new QTimer(this)),
      timerSendPeriodically(new QTimer(this)),
      serialPort(new QSerialPort(this)) {
    ui->setupUi(this);
    this->setWindowTitle("serialPort");
    initialization();
}

Widget::~Widget() {
    delete ui;
}

void Widget::initialization() {
    /* Declaration and Definition */
    QButtonGroup   *recvOptions = new QButtonGroup(this);
    QButtonGroup   *sendOptions = new QButtonGroup(this);
    QSerialPortInfo infoSerialPort;
    QList<qint32>   baudRates = infoSerialPort.standardBaudRates();  // What baudrates does my computer support ?
    QList<QString>  stringBaudRates;
    quint8          indexBaudrate;
    QTimer         *timerDisplayTime = new QTimer(this);
    /* ui layout */
    recvOptions->addButton(ui->isRecvASCII, 0);
    recvOptions->addButton(ui->isRecvHEX, 1);
    sendOptions->addButton(ui->isSendASCII, 0);
    sendOptions->addButton(ui->isSendHEX, 1);
    ui->recvCountBox->setText(QString::number(recvCount));
    ui->sendCountBox->setText(QString::number(sendCount));
    ui->isRecvASCII->setChecked(true);
    ui->isSendASCII->setChecked(true);
    ui->periodEdit->setValidator(new QIntValidator(0, __INT_MAX__, this));

    /* 連接 */
    connect(recvOptions, SIGNAL(idClicked(int)), this, SLOT(checkRecvFormatting(int)));
    connect(sendOptions, SIGNAL(idClicked(int)), this, SLOT(checkSendFormatting(int)));
    connect(ui->isPeriod, SIGNAL(stateChanged(int)), this, SLOT(isSendPeriodChecked(int)));
    connect(timerDisplayTime, SIGNAL(timeout()), this, SLOT(showTime()));
    connect(timerCheckReceivedMessage, SIGNAL(timeout()), this, SLOT(recvMessage()));
    connect(timerSendPeriodically, SIGNAL(timeout()), this, SLOT(sendMessage()));

    showTime();
    timerDisplayTime->start(250);

    /* Search all available serial ports */
    foreach (const QSerialPortInfo &inf0, QSerialPortInfo::availablePorts()) {
        comPortName << (inf0.portName() + " #" + inf0.description());
    }
    ui->portNameBox->addItems(comPortName);

    /* Show supported baud rates */
    for (int i = 0; i < baudRates.size(); i++) {
        if (baudRates.at(i) == 115200) {
            indexBaudrate = i;
        }

        stringBaudRates.append(QString::number(baudRates.at(i)));
    }
    ui->baudrateBox->addItems(stringBaudRates);
    ui->baudrateBox->setCurrentText(QString::number(baudRates.at(indexBaudrate)));
}

void Widget::on_openButton_clicked() {
    static bool isOpen = false;
    portName           = ui->portNameBox->currentText();
    splitedPortName    = portName.split(" ");
    if (isOpen == false) {
        /* 串列埠設定 */
        if (serialPort->isOpen() == false) {
            serialPort->setPortName(splitedPortName[0]);
            serialPort->setBaudRate(ui->baudrateBox->currentText().toInt());

            QString configDataBits = ui->databitsBox->currentText();
            if (configDataBits == "5 Bits") {
                serialPort->setDataBits(QSerialPort::Data5);
            } else if ((configDataBits == "6 Bits")) {
                serialPort->setDataBits(QSerialPort::Data6);
            } else if (configDataBits == "7 Bits") {
                serialPort->setDataBits(QSerialPort::Data7);
            } else if (configDataBits == "8 Bits") {
                serialPort->setDataBits(QSerialPort::Data8);
            }

            QString configStopBits = ui->stopbitsBox->currentText();
            if (configStopBits == "1 Bit") {
                serialPort->setStopBits(QSerialPort::OneStop);
            } else if (configStopBits == "1.5 Bits") {
                serialPort->setStopBits(QSerialPort::OneAndHalfStop);
            } else if (configStopBits == "2 Bits") {
                serialPort->setStopBits(QSerialPort::TwoStop);
            }

            QString configParity = ui->parityBox->currentText();
            if (configParity == "No Parity") {
                serialPort->setParity(QSerialPort::NoParity);
            } else if (configParity == "Even Parity") {
                serialPort->setParity(QSerialPort::EvenParity);
            } else if (configParity == "Odd Parity") {
                serialPort->setParity(QSerialPort::OddParity);
            } else if (configParity == "Mark Parity") {
                serialPort->setParity(QSerialPort::MarkParity);
            } else if (configParity == "Space Parity") {
                serialPort->setParity(QSerialPort::SpaceParity);
            }

            QString configFlowControl = ui->flowcontrolBox->currentText();
            if (configFlowControl == "No Flow Control") {
                serialPort->setFlowControl(QSerialPort::NoFlowControl);
            } else if (configFlowControl == "Hardware Flow Control") {
                serialPort->setFlowControl(QSerialPort::HardwareControl);
            } else if (configFlowControl == "Software Flow Control") {
                serialPort->setFlowControl(QSerialPort::SoftwareControl);
            }
        }

        /* 開啟串列埠Hint框 */
        ui->dataLogView->setTextColor(Qt::black);  // Receieved message's color is black.
        if (true == serialPort->open(QIODevice::ReadWrite)) {
            isOpen = true;
            ui->openButton->setText("Close");
            ui->dataLogView->append("---- Serial port " + splitedPortName[0] + " is open ----");
            timerCheckReceivedMessage->start(5);
        } else {
            ui->dataLogView->append("**** Unable to open serial port " + splitedPortName[0] + ". ****");
        }
    } else {
        isOpen = false;
        ui->openButton->setText("Open");
        if (serialPort->isOpen() == true) {
            ui->dataLogView->setTextColor(Qt::black);  // Receieved message's color is black.
            ui->dataLogView->append("---- Serial port " + splitedPortName[0] + " closed ----");
            serialPort->close();
            timerCheckReceivedMessage->stop();
            timerSendPeriodically->stop();
            ui->dataSendEdit->setEnabled(true);
            ui->sendButton->setText("Send");
            ui->isPeriod->setEnabled(true);
        }
    }
}

void Widget::on_refreshButton_clicked() {
    comPortName.clear();
    ui->portNameBox->clear();
    /* 搜尋所有可用串列埠 */
    foreach (const QSerialPortInfo &inf0, QSerialPortInfo::availablePorts()) {
        comPortName << (inf0.portName() + " #" + inf0.description());
    }

    ui->portNameBox->addItems(comPortName);
}

void Widget::showTime() {
    currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd dddd ap hh:mm:ss");
    ui->currentTimeLabel->setText(currentTime);
}

void Widget::recvMessage() {
    static qint64 prevBytesAvailable = 0;
    quint8        positionCharOfSpace;
    QByteArray    receivedMessage;
    QByteArray    consoleMessage;
    QString       timeReceivedMessage;
    if ((serialPort->bytesAvailable() == prevBytesAvailable) && prevBytesAvailable != 0) {
        receivedMessage = serialPort->readAll();
        if (isRecvConvertToHex == true) {
            for (int i = 0; i < receivedMessage.toHex().size(); i++) {
                consoleMessage.append(receivedMessage.toHex().at(i));
                if (++positionCharOfSpace >= 2 && i < (receivedMessage.toHex().size() - 1)) {
                    positionCharOfSpace = 0;
                    consoleMessage.append(' ');
                }
            }
        } else {
            consoleMessage = receivedMessage;
        }
        ui->dataLogView->setTextColor(Qt::gray);
        timeReceivedMessage = "[" + QDateTime::currentDateTime().toString("yyyy-MM-dd") + " " +
                              QDateTime::currentDateTime().toString("hh:mm:ss.zzz") + "]# RECV " +
                              ((isRecvConvertToHex == true) ? "HEX" : "ASCII");
        ui->dataLogView->append(timeReceivedMessage);

        ui->dataLogView->setTextColor(Qt::blue);
        ui->dataLogView->append(consoleMessage);
        recvCount++;
        ui->recvCountBox->setText(QString::number(recvCount));
    }
    prevBytesAvailable = serialPort->bytesAvailable();
}

void Widget::sendMessage() {
    uint8_t    nibbleByte    = 0;
    uint8_t    countNextByte = 0;
    uint8_t    aCompleteByte = 0;
    QByteArray hexMessage;
    QByteArray uiHexMessage;
    QString    timeMessage = "[" + QDateTime::currentDateTime().toString("yyyy-MM-dd") + " " +
                          QDateTime::currentDateTime().toString("hh:mm:ss.zzz") + "]# SEND " +
                          ((isSendConvertToHex == true) ? "HEX" : "ASCII");

    if (uiTransmitMessage.size() != 0) {
        if (isSendConvertToHex == true) {
            for (int index = 0; index < uiTransmitMessage.size(); index++) {
                char c = uiTransmitMessage.toUtf8().at(index);
                if ((' ' == c) || (',' == c)) {
                    if (countNextByte != 0) {
                        countNextByte = 0;
                        hexMessage.append(aCompleteByte);
                    }
                } else if ((('0' <= c) && (c <= '9'))     // range: '0' - '9'
                           || (('a' <= c) && (c <= 'f'))  // range: 'a' - 'f'
                           || (('A' <= c) && (c <= 'F'))  // range: 'A' - 'F'
                ) {
                    nibbleByte = AsciiToHex(c);
                    if (++countNextByte >= 2) {
                        countNextByte = 0;
                        aCompleteByte <<= 4;
                        aCompleteByte |= nibbleByte;
                        hexMessage.append(aCompleteByte);
                    } else {
                        aCompleteByte = nibbleByte;
                        if (index == (uiTransmitMessage.size() - 1)) {
                            hexMessage.append(aCompleteByte);
                        }
                    }
                } else if ((index == (uiTransmitMessage.size() - 1)) || (';' == c)) {
                    if (countNextByte != 0) {
                        hexMessage.append(nibbleByte);
                    }
                    if (';' == c) {
                        break;
                    }
                }
            }
            for (uint64_t index = 0; index < hexMessage.size(); index++) {
                uiHexMessage.append(hexMessage.toHex().at((index << 1)));
                uiHexMessage.append(hexMessage.toHex().at((index << 1) + 1));
                if (index != hexMessage.size() - 1) {
                    uiHexMessage.append(' ');
                }
            }

            if (hexMessage.size() != 0) {
                ui->dataLogView->setTextColor(Qt::gray);
                ui->dataLogView->append(timeMessage);
                ui->dataLogView->setTextColor(Qt::darkGreen);
                ui->dataLogView->append(uiHexMessage);
                serialPort->write(hexMessage);
                sendCount++;
            } else {
                QMessageBox::information(this, "Hint", "Data Send can only fill in [0-9], [a-f], [A-F]");
            }
        } else {
            ui->dataLogView->setTextColor(Qt::gray);
            ui->dataLogView->append(timeMessage);
            ui->dataLogView->setTextColor(Qt::darkGreen);
            ui->dataLogView->append(uiTransmitMessage);
            serialPort->write(uiTransmitMessage.toUtf8());
            sendCount++;
        }
        ui->sendCountBox->setText(QString::number(sendCount));
    } else {
        timerSendPeriodically->stop();
        ui->dataSendEdit->setEnabled(true);
        ui->sendButton->setText("Send");
        ui->isPeriod->setEnabled(true);
        QMessageBox::information(this, "Hint", "Data Send field cannot be blank");
    }
}

void Widget::on_clearButton_clicked() {
    ui->dataLogView->clear();
}

void Widget::on_sendButton_clicked() {
    if (serialPort->isOpen() == true) {
        uiTransmitMessage = ui->dataSendEdit->text();
        if (ui->isPeriod->isChecked() == true) {
            if (timerSendPeriodically->isActive() == false) {
                ui->sendButton->setText("Stop");
                ui->dataSendEdit->setEnabled(false);
                ui->isPeriod->setEnabled(false);
                timerSendPeriodically->start(sendPeriod);
                sendMessage();
            } else {
                ui->sendButton->setText("Send");
                ui->dataSendEdit->setEnabled(true);
                ui->isPeriod->setEnabled(true);
                timerSendPeriodically->stop();
            }
        } else {
            if (ui->dataSendEdit->isEnabled() == false) {
                ui->dataSendEdit->setEnabled(true);
                ui->sendButton->setText("Send");
            }
            sendMessage();
        }

    } else {
        portName        = ui->portNameBox->currentText();
        splitedPortName = portName.split(" ");
        QMessageBox::information(this, "Hint",
                                 "**** The serial port " + splitedPortName[0] + " has not been opened. ****");
    }
}

void Widget::checkRecvFormatting(int buttonID) {
    qDebug(__FUNCTION__);
    switch (buttonID) {
        case 0:  // ASCII
            isRecvConvertToHex = false;
            break;
        case 1:  // Hex
            isRecvConvertToHex = true;
            break;

        default:
            break;
    }
}

void Widget::checkSendFormatting(int buttonID) {
    qDebug(__FUNCTION__);
    switch (buttonID) {
        case 0:  // ASCII
            isSendConvertToHex = false;
            break;
        case 1:  // Hex
            isSendConvertToHex = true;
            break;

        default:
            break;
    }
}

char Widget::AsciiToHex(char c) {
    if (('0' <= c) && (c <= '9')) {
        return (c - '0');
    } else if (('a' <= c) && (c <= 'f')) {
        return (c - 'a' + 0xa);
    } else if (('A' <= c) && (c <= 'F')) {
        return (c - 'A' + 0xa);
    } else {
        return -1;
    }
}

void Widget::on_resetRecvCountButton_clicked() {
    qDebug(__FUNCTION__);
    recvCount = 0;
    ui->recvCountBox->setText(QString::number(recvCount));
}

void Widget::on_resetSendCountButton_clicked() {
    qDebug(__FUNCTION__);
    sendCount = 0;
    ui->sendCountBox->setText(QString::number(sendCount));
}

void Widget::isSendPeriodChecked(int isChecked) {
    qDebug(__FUNCTION__);
    bool enable = (isChecked == 0);
    ui->periodEdit->setEnabled(enable);
    if ((isChecked == 0) == true) {
        sendPeriod = 0;
        timerSendPeriodically->stop();
    } else {
        sendPeriod = ui->periodEdit->text().toInt();
    }
}
