#include "widget.h"

#include "./ui_widget.h"

using namespace Qt::StringLiterals;

Widget::Widget(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::Widget),
      serialPort(new QSerialPort(this)),
      timerCheckReceivedMessage(new QTimer(this)),
      timerSendPeriodically(new QTimer(this)) {
    ui->setupUi(this);
    this->setWindowTitle("serialPort");
    initialization();
}

Widget::~Widget() {
    delete ui;
}

static const QRegularExpression &threeHexDigitsPattern() {
    static const QRegularExpression result(u"[[:xdigit:]]{3}"_s);
    Q_ASSERT(result.isValid());
    return result;
}

static const QRegularExpression &oneDigitAndSpacePattern() {
    static const QRegularExpression result(u"((\\s+)|^)([[:xdigit:]]{1})(\\s+)"_s);
    Q_ASSERT(result.isValid());
    return result;
}

const QRegularExpression &hexNumberPattern() {
    static const QRegularExpression result;
    Q_ASSERT(result.isValid());
    return result;
}

const QRegularExpression &twoSpacesPattern() {
    static const QRegularExpression result(u"([\\s]{2})"_s);
    Q_ASSERT(result.isValid());
    return result;
}

enum {
    MaxStandardId = 0x7FF,
    MaxExtendedId = 0x10000000
};

enum {
    MaxPayload   = 8,
    MaxPayloadFd = 64
};

static bool isEvenHex(QString input) {
    input.remove(u' ');
    return (input.size() % 2) == 0;
}

// Formats a string of hex characters with a space between every byte
// Example: "012345" -> "01 23 45"
static QString formatHexData(const QString &input) {
    QString            out = input;
    QRegularExpression regx(u"(\\s[[:xdigit:]])$"_s);

    while (true) {
        if (auto match = threeHexDigitsPattern().match(out); match.hasMatch()) {
            out.insert(match.capturedEnd() - 1, u' ');
        } else if (match = oneDigitAndSpacePattern().match(out); match.hasMatch()) {
            const auto pos = match.capturedEnd() - 1;
            if (out.at(pos) == u' ') out.insert(pos - 1, u'0');
        } else {
            if (match = regx.match(out); match.hasMatch()) {
                const auto pos = match.capturedEnd() - 1;
                out.insert(pos, u'0');
            }
            break;
        }
    }

    return out.simplified().toUpper();
}

HexIntegerValidator::HexIntegerValidator(QObject *parent) : QValidator(parent), m_maximum(MaxStandardId) {
}

QValidator::State HexIntegerValidator::validate(QString &input, int &) const {
    if (input.isEmpty()) return Intermediate;
    bool ok;
    uint value = input.toUInt(&ok, 16);
    return ok && value <= m_maximum ? Acceptable : Invalid;
}

void HexIntegerValidator::setMaximum(uint maximum) {
    m_maximum = maximum;
}

HexStringValidator::HexStringValidator(QObject *parent) : QValidator(parent), m_maxLength(MaxPayload) {
}

QValidator::State HexStringValidator::validate(QString &input, int &pos) const {
    QRegularExpression regx("[^[:xdigit:]|^ ]+");
    const int          maxSize = 2 * m_maxLength;
    QString            data    = input.replace(regx, "");

    data.remove(u' ');

    if (data.isEmpty()) return Intermediate;

    // limit maximum size
    // if (data.size() > maxSize) return Invalid;

    // check if all input is valid
    if (!hexNumberPattern().match(data).hasMatch()) return Invalid;

    // don't allow user to enter more than one space
    if (const auto match = twoSpacesPattern().match(input); match.hasMatch()) {
        input.replace(match.capturedStart(), 2, ' ');
        pos = match.capturedEnd() - 1;
    }

    // insert a space after every two hex nibbles
    while (true) {
        const QRegularExpressionMatch match = threeHexDigitsPattern().match(input);
        if (!match.hasMatch()) break;
        const auto start = match.capturedStart();
        const auto end   = match.capturedEnd();
        if (pos == start + 1) {
            // add one hex nibble before two - Abc
            input.insert(pos, u' ');
        } else if (pos == start + 2) {
            // add hex nibble in the middle - aBc
            input.insert(end - 1, u' ');
            pos = end;
        } else {
            // add one hex nibble after two - abC
            input.insert(end - 1, u' ');
            pos = end + 1;
        }
    }

    return Acceptable;
}

void HexStringValidator::setMaxLength(int maxLength) {
    m_maxLength = maxLength;
}

void Widget::initialization() {
    /* Declaration and Definition */
    QButtonGroup       *recvOptions = new QButtonGroup(this);
    QButtonGroup       *sendOptions = new QButtonGroup(this);
    QSerialPortInfo     infoSerialPort;
    QList<qint32>       baudRates = infoSerialPort.standardBaudRates();  // What baudrates does my computer support ?
    QList<QString>      stringBaudRates;
    quint8              indexBaudrate;
    QTimer             *timerDisplayTime   = new QTimer(this);
    HexStringValidator *hexStringValidator = new HexStringValidator(this);

    /* ui layout */
    recvOptions->addButton(ui->isRecvASCII, 0);
    recvOptions->addButton(ui->isRecvHEX, 1);
    sendOptions->addButton(ui->isSendASCII, 0);
    sendOptions->addButton(ui->isSendHEX, 1);
    ui->recvCountBox->setText(QString::number(recvCount));
    ui->sendCountBox->setText(QString::number(sendCount));
    ui->isRecvASCII->setChecked(true);
    ui->isSendASCII->setChecked(true);
    ui->periodEdit->setValidator(new QIntValidator(0, INT_FAST16_MAX, this));
    // ui->dataSendEdit->setValidator(hexStringValidator);
    ui->payloadEdit->setValidator(hexStringValidator);

    /* 連接 */
    connect(ui->refreshButton, &QPushButton::clicked, this, [this]() {
        comPortName.clear();
        ui->portNameBox->clear();
        /* 搜尋所有可用串列埠 */
        foreach (const QSerialPortInfo &inf0, QSerialPortInfo::availablePorts()) {
            comPortName << (inf0.portName() + " #" + inf0.description());
        }

        ui->portNameBox->addItems(comPortName);
        qDebug("refreshButton is Clicked !");
    });
    connect(ui->clearButton, &QPushButton::clicked, this, [this]() {
        ui->dataLogView->clear();
        qDebug("clearButton is Clicked !");
    });
    connect(ui->resetRecvCountButton, &QPushButton::clicked, this, [this]() {
        recvCount = 0;
        ui->recvCountBox->setText(QString::number(recvCount));
        qDebug("resetRecvCountButton is Clicked !");
    });
    connect(ui->resetSendCountButton, &QPushButton::clicked, this, [this]() {
        sendCount = 0;
        ui->sendCountBox->setText(QString::number(sendCount));
        qDebug("resetSendCountButton is Clicked !");
    });
    connect(ui->openButton, &QPushButton::clicked, this, [this]() {
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
        qDebug("openButton is Clicked !");
    });
    // connect(recvOptions, SIGNAL(idClicked(int)), this, SLOT(checkRecvFormatting(int)));
    connect(recvOptions, &QButtonGroup::idClicked, this, [this](int buttonId) {
        isRecvConvertToHex = buttonId != 0 ? true : false;
        qDebug("isRecvConvertToHex = 0x%d", isRecvConvertToHex);
    });
    // connect(sendOptions, SIGNAL(idClicked(int)), this, SLOT(checkSendFormatting(int)));
    connect(sendOptions, &QButtonGroup::idClicked, this, [this](int buttonId) {
        isSendConvertToHex                     = buttonId != 0 ? true : false;
        HexStringValidator *hexStringValidator = new HexStringValidator(this);

        if (isSendConvertToHex == true) {
            ui->dataSendEdit->setValidator(hexStringValidator);
        } else {
            ui->dataSendEdit->setValidator(0);
        }

        qDebug("isSendConvertToHex = 0x%d", isSendConvertToHex);
    });
    // connect(ui->isPeriod, SIGNAL(stateChanged(int)), this, SLOT(isSendPeriodChecked(int)));
    connect(ui->isPeriod, &QCheckBox::stateChanged, this, [this](bool isChecked) {
        bool enable = isChecked == 0 ? true : false;
        ui->periodEdit->setEnabled(enable);
        if (enable) {
            sendPeriod = 0;
            timerSendPeriodically->stop();
        } else {
            sendPeriod = ui->periodEdit->text().toInt();
        }
        qDebug("isChecked = 0x%d", isChecked);
    });
    // connect(timerDisplayTime, SIGNAL(timeout()), this, SLOT(showTime()));
    connect(timerDisplayTime, &QTimer::timeout, this, &Widget::showTime);

    connect(timerCheckReceivedMessage, &QTimer::timeout, this, &Widget::recvMessage);
    connect(timerSendPeriodically, &QTimer::timeout, this, &Widget::sendMessage);

    connect(ui->sendButton, &QPushButton::clicked, [this]() {
        QString data = ui->dataSendEdit->text();
        if (isSendConvertToHex == true) {
            ui->dataSendEdit->setText(formatHexData(data));
        }

        sendButton_clicked();
    });
    connect(ui->pushButton, &QPushButton::clicked, [this]() {
        QString data = ui->payloadEdit->text();
        ui->payloadEdit->setText(formatHexData(data));
        const QByteArray payload = QByteArray::fromHex(data.remove(u' ').toLatin1());
    });

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

void Widget::showTime() {
    currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd dddd ap hh:mm:ss");
    ui->currentTimeLabel->setText(currentTime);
}

void Widget::recvMessage() {
    static qint64 prevBytesAvailable  = 0;
    quint8        positionCharOfSpace = 0;
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
    uiTransmitMessage = ui->dataSendEdit->text();

    if (uiTransmitMessage.size() != 0) {
        if (isSendConvertToHex == true) {
            ui->dataLogView->setTextColor(Qt::gray);
            ui->dataLogView->append(timeMessage);
            ui->dataLogView->setTextColor(Qt::darkGreen);
            ui->dataLogView->append(uiTransmitMessage.toUpper());
            serialPort->write(QByteArray::fromHex(uiTransmitMessage.remove(u' ').toLatin1()));
            sendCount++;
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

void Widget::sendButton_clicked() {
    if (serialPort->isOpen() == true) {
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
