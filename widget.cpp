#include "widget.h"

#include "./ui_widget.h"

Widget::Widget(QWidget *parent) : QWidget(parent), ui(new Ui::Widget) {
    /* initiailize parameter */
    isRecvConvertToHex = false;
    isSendConvertToHex = false;
    recvCount          = 0;
    sendCount          = 0;

    ui->setupUi(this);
    this->setWindowTitle("serialPort");

    ui->label_recvCount->setText(QString::number(recvCount));
    ui->label_sendCount->setText(QString::number(sendCount));

    QButtonGroup *buttonGroup_RecvOptions = new QButtonGroup(this);
    buttonGroup_RecvOptions->addButton(ui->isRecvASCII, 0);
    buttonGroup_RecvOptions->addButton(ui->isRecvHEX, 1);
    connect(buttonGroup_RecvOptions, SIGNAL(idClicked(int)), this, SLOT(recvOptions(int)));
    ui->isRecvASCII->setChecked(true);

    QButtonGroup *buttonGroup_SendOptions = new QButtonGroup(this);
    buttonGroup_SendOptions->addButton(ui->isSendASCII, 0);
    buttonGroup_SendOptions->addButton(ui->isSendHEX, 1);
    connect(buttonGroup_SendOptions, SIGNAL(idClicked(int)), this, SLOT(sendOptions(int)));
    ui->isSendASCII->setChecked(true);

    connect(ui->checkBox_sendPeriod, SIGNAL(stateChanged(int)), this, SLOT(isSendPeriodChecked(int)));
    ui->lineEdit_2->setValidator(new QIntValidator(0, __INT_MAX__, this));

    timerDisplayTime          = new QTimer(this);
    timerCheckReceivedMessage = new QTimer(this);
    timerSendPeriodically     = new QTimer(this);
    connect(timerDisplayTime, SIGNAL(timeout()), this, SLOT(showTime()));
    connect(timerCheckReceivedMessage, SIGNAL(timeout()), this, SLOT(recvMessage()));
    connect(timerSendPeriodically, SIGNAL(timeout()), this, SLOT(on_sendButton_clicked()));

    ui->textBrowser->document()->setTextWidth(5);

    showTime();
    timerDisplayTime->start(250);

    /* 建立一個串列埠物件 */
    serialPort = new QSerialPort(this);

    /* 搜尋所有可用串列埠 */
    foreach (const QSerialPortInfo &inf0, QSerialPortInfo::availablePorts()) {
        comPortName << (inf0.portName() + " #" + inf0.description());
    }
    ui->portnameBox->addItems(comPortName);

    // Baud Rate Ratios
    QSerialPortInfo infoSerialPort;
    QList<qint32>   baudRates = infoSerialPort.standardBaudRates();  // What baudrates does my computer support ?
    QList<QString>  stringBaudRates;
    quint8          indexBaudrate;
    for (int i = 0; i < baudRates.size(); i++) {
        if (baudRates.at(i) == 115200) {
            indexBaudrate = i;
        }

        stringBaudRates.append(QString::number(baudRates.at(i)));
    }
    ui->baudrateBox->addItems(stringBaudRates);
    ui->baudrateBox->setCurrentText(QString::number(baudRates.at(indexBaudrate)));
}

Widget::~Widget() {
    delete ui;
}

void Widget::on_openButton_clicked() {
    static bool isComPortOpen = false;
    portName                  = ui->portnameBox->currentText();
    splitedPortName           = portName.split(" ");
    if (isComPortOpen == false) {
        isComPortOpen = true;
        ui->openButton->setText("Close");
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

        /* 開啟串列埠提示框 */
        ui->textBrowser->setTextColor(Qt::black);  // Receieved message's color is black.
        if (true == serialPort->open(QIODevice::ReadWrite)) {
            (void)serialPort->readAll();
            ui->textBrowser->append("---- 已開啟序列連接埠 " + splitedPortName[0] + " ----");
            timerCheckReceivedMessage->start(5);
        } else {
            ui->textBrowser->append("**** 無法開啟序列連接埠 " + splitedPortName[0] + "。 ****");
        }
    } else {
        isComPortOpen = false;
        ui->openButton->setText("Open");
        if (serialPort->isOpen() == true) {
            ui->textBrowser->setTextColor(Qt::black);  // Receieved message's color is black.
            ui->textBrowser->append("---- 已關閉序列連接埠 " + splitedPortName[0] + " ----");
            serialPort->close();
            timerCheckReceivedMessage->stop();
        }
    }
}

void Widget::on_refreshButton_clicked() {
    comPortName.clear();
    ui->portnameBox->clear();
    /* 搜尋所有可用串列埠 */
    foreach (const QSerialPortInfo &inf0, QSerialPortInfo::availablePorts()) {
        comPortName << (inf0.portName() + " #" + inf0.description());
    }

    ui->portnameBox->addItems(comPortName);
}

void Widget::showTime() {
    currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd dddd ap hh:mm:ss");
    ui->timeString->setText(currentTime);
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
        ui->textBrowser->setTextColor(Qt::gray);
        timeReceivedMessage = "[" + QDateTime::currentDateTime().toString("yyyy-MM-dd") + " " +
                              QDateTime::currentDateTime().toString("hh:mm:ss.zzz") + "]# RECV " +
                              ((isRecvConvertToHex == true) ? "HEX" : "ASCII");
        ui->textBrowser->append(timeReceivedMessage);

        ui->textBrowser->setTextColor(Qt::blue);
        ui->textBrowser->append(consoleMessage);
        recvCount++;
        ui->label_recvCount->setText(QString::number(recvCount));
    }
    prevBytesAvailable = serialPort->bytesAvailable();
}

void Widget::sendMessage() {
}

void Widget::on_clearButton_clicked() {
    ui->textBrowser->clear();
}

void Widget::on_sendButton_clicked() {
    uint8_t    nibbleByte    = 0;
    uint8_t    countNextByte = 0;
    uint8_t    aCompleteByte = 0;
    QByteArray hexMessage;
    QByteArray uiHexMessage;
    QString    timeMessage = "[" + QDateTime::currentDateTime().toString("yyyy-MM-dd") + " " +
                          QDateTime::currentDateTime().toString("hh:mm:ss.zzz") + "]# SEND " +
                          ((isSendConvertToHex == true) ? "HEX" : "ASCII");
    QString uiMessage = ui->lineEdit->text();
    if (serialPort->isOpen() == true) {
        if (isSendConvertToHex == true) {
            for (int index = 0; index < uiMessage.size(); index++) {
                char c = uiMessage.toUtf8().at(index);
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
                        if (index == (uiMessage.size() - 1)) {
                            hexMessage.append(aCompleteByte);
                        }
                    }
                    qDebug("nibbleByte = 0x%02x", nibbleByte);
                } else if ((index == (uiMessage.size() - 1)) || (';' == c)) {
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

            ui->textBrowser->setTextColor(Qt::gray);
            ui->textBrowser->append(timeMessage);
            ui->textBrowser->setTextColor(Qt::darkGreen);
            ui->textBrowser->append(uiHexMessage);
            serialPort->write(hexMessage);

        } else {
            ui->textBrowser->setTextColor(Qt::gray);
            ui->textBrowser->append(timeMessage);
            ui->textBrowser->setTextColor(Qt::darkGreen);
            ui->textBrowser->append(uiMessage);
            serialPort->write(uiMessage.toUtf8());
        }
        sendCount++;
        ui->label_sendCount->setText(QString::number(sendCount));
    } else {
        if (ui->checkBox_sendPeriod->isChecked() == false) {
            portName        = ui->portnameBox->currentText();
            splitedPortName = portName.split(" ");
            QMessageBox::information(this, "提示", "**** 尚未開啟序列連接埠 " + splitedPortName[0] + "。 ****");
        }
    }
}

void Widget::recvOptions(int buttonID) {
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
    qDebug("RecvOptions checkedId = %d", buttonID);
}

void Widget::sendOptions(int buttonID) {
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
    qDebug("SendOptions checkedId = %d", buttonID);
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

void Widget::on_clearButton_recvCount_clicked() {
    qDebug(__FUNCTION__);
    recvCount = 0;
    ui->label_recvCount->setText(QString::number(recvCount));
}

void Widget::on_clearButton_sendCount_clicked() {
    qDebug(__FUNCTION__);
    sendCount = 0;
    ui->label_sendCount->setText(QString::number(sendCount));
}

void Widget::isSendPeriodChecked(int isChecked) {
    qDebug(__FUNCTION__);
    bool enable = (isChecked == 0);
    ui->lineEdit_2->setEnabled(enable);
    ui->lineEdit->setEnabled(enable);
    if ((isChecked == 0) == true) {
        timerSendPeriodically->stop();
    } else {
        timerSendPeriodically->start(ui->lineEdit_2->text().toInt());
    }
}
