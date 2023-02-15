#include "widget.h"

#include "./ui_widget.h"

using namespace Qt::StringLiterals;

Widget::Widget(QWidget *parent)
    : QWidget(parent),
      m_ui(new Ui::Widget),
      m_serialPort(new QSerialPort(this)),
      m_receiveMessageTimer(new QTimer(this)),
      m_repetitionTimer(new QTimer(this)),
      m_displayTimeTimer(new QTimer(this)) {
    m_ui->setupUi(this);
    this->setWindowTitle("Serial Port Exercise");

    // Create a validator
    m_hexStringValidator = new HexStringValidator(this);

    initialization();

    adjustComboBoxViewWidth(m_ui->serialPortComboxBox);
    adjustComboBoxViewWidth(m_ui->flowControlComboBox);
}

Widget::~Widget() {
    delete m_ui;
}

static const QString timeString(QString str, bool en) {
    QString s = QString("[%1 %2]# %3 %4")
                    .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd"))
                    .arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"))
                    .arg(str)
                    .arg(((en == true) ? "HEX" : "ASCII"));
    return s;
}

static const QByteArray insertSpaceBetweenByte(QByteArray input) {
    quint8     cursorSpace = 0;
    QByteArray output      = nullptr;
    for (int index = 0; index < input.toHex().size(); index++) {
        output.append(input.toHex().toUpper().at(index));
        if ((++cursorSpace >= 2) && (index < (input.toHex().size() - 1))) {
            cursorSpace = 0;
            output.append(' ');
        }
    }
    return output;
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
            if (out.at(pos) == u' ') {
                out.insert(pos - 1, u'0');
            }
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

HexStringValidator::HexStringValidator(QObject *parent) : QValidator(parent) {
}

QValidator::State HexStringValidator::validate(QString &input, int &pos) const {
    QRegularExpression regxNonHexdecimal(u"[^[:xdigit:]|^ ]+"_s);
    QString            data = nullptr;

    data = input.replace(regxNonHexdecimal, u""_s);

    data.remove(u' ');

    if (data.isEmpty()) return Intermediate;

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
            input.insert(pos + 1, u' ');
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

void Widget::initialization() {
    m_ui->repetitionLineEdit->setValidator(new QIntValidator(0, INT_FAST16_MAX, this));

    m_ui->recvOptionsButtonGroup->setId(m_ui->isRecvAsciiRadioButton, 0);
    m_ui->recvOptionsButtonGroup->setId(m_ui->isRecvHexRadioButton, 1);
    m_ui->sendOptionsButtonGroup->setId(m_ui->isSendAsciiRadioButton, 0);
    m_ui->sendOptionsButtonGroup->setId(m_ui->isSendHexRadioButton, 1);

    // create connection
    connect(m_ui->refreshPushButton, &QPushButton::clicked, this, [this]() {
        m_ui->serialPortComboxBox->clear();
        // Search all available serial ports
        const auto infos = QSerialPortInfo::availablePorts();
        for (const QSerialPortInfo &info : infos)
            m_ui->serialPortComboxBox->addItem(info.portName() + " #" + info.description());
        adjustComboBoxViewWidth(m_ui->serialPortComboxBox);
        adjustComboBoxViewWidth(m_ui->flowControlComboBox);
        qDebug("refreshPushButton is Clicked !");
    });
    connect(m_ui->clearPushButton, &QPushButton::clicked, this, [this]() {
        m_ui->dataLogTextBrowser->clear();
        qDebug("clearPushButton is Clicked !");
    });
    connect(m_ui->resetRecvCountPushButton, &QPushButton::clicked, this, [this]() {
        m_numberPacketReceived = 0;
        m_ui->recvCount->setText(QString::number(m_numberPacketReceived));
        qDebug("resetRecvCountPushButton is Clicked !");
    });
    connect(m_ui->resetSendCountPushButton, &QPushButton::clicked, this, [this]() {
        m_numberPacketWritten = 0;
        m_ui->sendCount->setText(QString::number(m_numberPacketWritten));
        qDebug("resetSendCountPushButton is Clicked !");
    });
    connect(m_ui->runPushButton, &QPushButton::clicked, this, &Widget::openSerialPort);
    connect(m_ui->recvOptionsButtonGroup, &QButtonGroup::idClicked, this, [this](int buttonId) {
        m_isRecvHexEnabled = ((buttonId == 1) ? true : false);
        qDebug("m_isRecvHexEnabled = 0x%d", m_isRecvHexEnabled);
    });
    connect(m_ui->sendOptionsButtonGroup, &QButtonGroup::idClicked, this, [this](int buttonId) {
        m_isSendHexEnabled = ((buttonId == 1) ? true : false);
        static int prev    = -1;
        if (prev != m_isSendHexEnabled) {
            QString data            = m_ui->dataSendLineEdit->text();
            QString placeholderText = "Hello World !!! :)";
            if (m_isSendHexEnabled == true) {
                m_ui->dataSendLineEdit->setValidator(m_hexStringValidator);
                m_ui->dataSendLineEdit->setText(QByteArray::fromHex(data.toUtf8().toHex().toHex()).toUpper());
                // Hello World!!! -> Hex Ascii
                m_ui->dataSendLineEdit->setPlaceholderText(insertSpaceBetweenByte(placeholderText.toUtf8()));
            } else {
                m_ui->dataSendLineEdit->setValidator(nullptr);
                m_ui->dataSendLineEdit->setText(QByteArray::fromHex(data.remove(u' ').toUtf8()));
                m_ui->dataSendLineEdit->setPlaceholderText(placeholderText);
            }
        }
        prev = m_isSendHexEnabled;
        qDebug("m_isSendHexEnabled = 0x%d", m_isSendHexEnabled);
    });
    connect(m_ui->isPeriodCheckBox, &QCheckBox::stateChanged, this, [this](bool isChecked) {
        bool enable = ((isChecked == false) ? true : false);
        m_ui->repetitionLineEdit->setEnabled(enable);
        if (enable) {
            m_repetition_ms = 0;
            m_repetitionTimer->stop();
        } else {
            m_repetition_ms = m_ui->repetitionLineEdit->text().toInt();
        }
        qDebug("isPeriodCheckBox = 0x%d", isChecked);
    });
    connect(m_ui->sendPushButton, &QPushButton::clicked, [this]() {
        QString data = m_ui->dataSendLineEdit->text();
        if (m_isSendHexEnabled == true) {
            m_ui->dataSendLineEdit->setText(formatHexData(data));
        }
        sendButton_clicked();
    });
    connect(m_ui->endCursorButton, &QPushButton::clicked, [this]() {
        m_ui->dataLogTextBrowser->moveCursor(QTextCursor::End);
        connect(m_ui->dataLogTextBrowser, &QTextBrowser::textChanged,
                [this]() { m_ui->dataLogTextBrowser->moveCursor(QTextCursor::End); });
    });
    connect(m_ui->dataLogTextBrowser->verticalScrollBar(), &QScrollBar::valueChanged, [this](int value) {
        static int temp = value;
        if (value < temp) {
            disconnect(m_ui->dataLogTextBrowser, &QTextBrowser::textChanged, 0, 0);
        }
        temp = value;
    });
    connect(m_ui->freezeWindowsBox, &QCheckBox::clicked, [this](bool isChecked) {
        m_ui->dataLogTextBrowser->setEnabled(!isChecked);
        m_isFreezeWindows = isChecked;
    });

    connect(m_displayTimeTimer, &QTimer::timeout, this, &Widget::displayTime);
    connect(m_receiveMessageTimer, &QTimer::timeout, this, &Widget::receiveMessage);
    connect(m_repetitionTimer, &QTimer::timeout, this, &Widget::transmitMessage);

    displayTime();
    m_displayTimeTimer->start(250);

    // Search all available serial ports
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos)
        m_ui->serialPortComboxBox->addItem(info.portName() + " #" + info.description());

    // Show supported baud rates
    const auto baudRates     = QSerialPortInfo::standardBaudRates();
    quint8     indexBaudrate = 0;
    for (int i = 0; i < baudRates.size(); i++) {
        if (baudRates.at(i) == 115200) {
            indexBaudrate = i;
        }
        m_ui->baudrateComboBox->addItem(QString::number(baudRates.at(i)));
    }
    m_ui->baudrateComboBox->setCurrentIndex(indexBaudrate);

    const QStringList dataBits = {"5 Bits", "6 Bits", "7 Bits", "8 Bits"};
    for (int index = 0; index < dataBits.size(); index++) m_ui->databitsComboBox->addItem(dataBits[index]);
    m_ui->databitsComboBox->setCurrentIndex(3);

    const QStringList stopBits = {"1 Bit", "1.5 Bits", "2 Bits"};
    for (int index = 0; index < stopBits.size(); index++) m_ui->stopbitsComboBox->addItem(stopBits[index]);

    const QStringList parity = {"No Parity", "Even Parity", "Odd Parity", "Mark Parity", "Space Parity"};
    for (int index = 0; index < parity.size(); index++) m_ui->parityComboBox->addItem(parity[index]);

    const QStringList flowControl = {"No FlowControl", "Hardware FlowControl", "Software FlowControl"};
    for (int index = 0; index < flowControl.size(); index++) m_ui->flowControlComboBox->addItem(flowControl[index]);
}

void Widget::displayTime() {
    m_currentTime = QDateTime::currentDateTime().toString("A hh:mm:ss\r\nyyyy-MM-dd ddd");
    m_ui->currentTimeLabel->setText(m_currentTime);
}

void Widget::receiveMessage() {
    static qint64 prev        = 0;
    quint8        cursorSpace = 0;
    QByteArray    data        = nullptr;
    QByteArray    dataAcsii   = nullptr;
    QString       timeMessage = timeString("RECV", m_isRecvHexEnabled);

    if ((m_serialPort->bytesAvailable() == prev) && (prev != 0)) {
        data = m_serialPort->readAll();

        if (m_isRecvHexEnabled == true) {
            dataAcsii = insertSpaceBetweenByte(data);
        } else {
            dataAcsii = data;
        }

        if (m_isFreezeWindows == false) {
            m_ui->dataLogTextBrowser->setTextColor(Qt::gray);
            m_ui->dataLogTextBrowser->append(timeMessage);

            m_ui->dataLogTextBrowser->setTextColor(Qt::blue);
            m_ui->dataLogTextBrowser->append(dataAcsii);
        }

        m_ui->recvCount->setText(QString::number(++m_numberPacketReceived));
    }
    prev = m_serialPort->bytesAvailable();
}

void Widget::openSerialPort() {
    static bool isOpened = false;
    m_portName           = m_ui->serialPortComboxBox->currentText();
    if (isOpened == false) {
        // Serial Port Settings
        if (m_serialPort->isOpen() == false) {
            m_serialPort->setPortName(m_portName.split(" ")[0]);
            m_serialPort->setBaudRate(m_ui->baudrateComboBox->currentText().toInt());

            qint8 configDataBits = m_ui->databitsComboBox->currentIndex();
            switch (configDataBits) {
                case 0:
                    m_serialPort->setDataBits(QSerialPort::Data5);
                    break;
                case 1:
                    m_serialPort->setDataBits(QSerialPort::Data6);
                    break;
                case 2:
                    m_serialPort->setDataBits(QSerialPort::Data7);
                    break;
                case 3:
                default:
                    m_serialPort->setDataBits(QSerialPort::Data8);
                    break;
            }

            qint8 configStopBits = m_ui->stopbitsComboBox->currentIndex();
            switch (configStopBits) {
                case 0:
                default:
                    m_serialPort->setStopBits(QSerialPort::OneStop);
                    break;
                case 1:
                    m_serialPort->setStopBits(QSerialPort::OneAndHalfStop);
                    break;
                case 2:
                    m_serialPort->setStopBits(QSerialPort::TwoStop);
                    break;
            }

            qint8 configParity = m_ui->parityComboBox->currentIndex();
            switch (configParity) {
                case 0:
                default:
                    m_serialPort->setParity(QSerialPort::NoParity);
                    break;
                case 1:
                    m_serialPort->setParity(QSerialPort::EvenParity);
                    break;
                case 2:
                    m_serialPort->setParity(QSerialPort::OddParity);
                    break;
                case 3:
                    m_serialPort->setParity(QSerialPort::MarkParity);
                    break;
                case 4:
                    m_serialPort->setParity(QSerialPort::SpaceParity);
                    break;
            }

            qint8 configFlowControl = m_ui->flowControlComboBox->currentIndex();
            switch (configFlowControl) {
                case 0:
                default:
                    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);
                    break;
                case 1:
                    m_serialPort->setFlowControl(QSerialPort::HardwareControl);
                    break;
                case 2:
                    m_serialPort->setFlowControl(QSerialPort::SoftwareControl);
                    break;
            }
        }

        // Open the serial port reminder box
        m_ui->dataLogTextBrowser->setTextColor(Qt::black);  // Receieved message's color is black.
        if (m_serialPort->open(QIODevice::ReadWrite) == true) {
            QString s = tr("---- Serial port %1 is open ----").arg(m_portName.split(" ")[0]);
            isOpened  = true;
            m_ui->runPushButton->setText("Close");
            m_ui->dataLogTextBrowser->append(s);
            m_ui->serialPortComboxBox->setEnabled(false);
            m_ui->baudrateComboBox->setEnabled(false);
            m_ui->databitsComboBox->setEnabled(false);
            m_ui->stopbitsComboBox->setEnabled(false);
            m_ui->parityComboBox->setEnabled(false);
            m_ui->flowControlComboBox->setEnabled(false);
            m_ui->refreshPushButton->setAttribute(Qt::WA_TransparentForMouseEvents);
            m_receiveMessageTimer->start(2);
        } else {
            QString s = tr("**** Unable to open serial port %1. ****").arg(m_portName.split(" ")[0]);
            m_ui->dataLogTextBrowser->append(s);
        }
    } else {
        isOpened = false;
        m_ui->runPushButton->setText("Open");
        if (m_serialPort->isOpen() == true) {
            QString s = tr("---- Serial port %1 closed ----").arg(m_portName.split(" ")[0]);
            m_ui->dataLogTextBrowser->setTextColor(Qt::black);  // Receieved message's color is black.
            m_ui->dataLogTextBrowser->append(s);
            m_ui->sendPushButton->setText("Send");
            m_ui->dataSendLineEdit->setEnabled(true);
            m_ui->isPeriodCheckBox->setEnabled(true);
            m_ui->serialPortComboxBox->setEnabled(true);
            m_ui->baudrateComboBox->setEnabled(true);
            m_ui->databitsComboBox->setEnabled(true);
            m_ui->stopbitsComboBox->setEnabled(true);
            m_ui->parityComboBox->setEnabled(true);
            m_ui->flowControlComboBox->setEnabled(true);
            m_ui->refreshPushButton->setAttribute(Qt::WA_TransparentForMouseEvents, false);
            m_serialPort->close();
            m_repetitionTimer->stop();
            m_receiveMessageTimer->stop();
        }
    }
    qDebug("runPushButton is Clicked !");
}

void Widget::transmitMessage() {
    QString timeMessage = timeString("SEND", m_isSendHexEnabled);
    QString data        = m_ui->dataSendLineEdit->text();

    if (data.size() != 0) {
        if (m_isSendHexEnabled == true) {
            m_ui->dataLogTextBrowser->setTextColor(Qt::gray);
            m_ui->dataLogTextBrowser->append(timeMessage);
            m_ui->dataLogTextBrowser->setTextColor(Qt::darkGreen);
            m_ui->dataLogTextBrowser->append(data.toUpper());
            m_serialPort->write(QByteArray::fromHex(data.remove(u' ').toLatin1()));
        } else {
            m_ui->dataLogTextBrowser->setTextColor(Qt::gray);
            m_ui->dataLogTextBrowser->append(timeMessage);
            m_ui->dataLogTextBrowser->setTextColor(Qt::darkGreen);
            m_ui->dataLogTextBrowser->append(data);
            m_serialPort->write(data.toUtf8());
        }
        m_ui->sendCount->setText(QString::number(++m_numberPacketWritten));
    } else {
        m_ui->sendPushButton->setText("Send");
        m_ui->dataSendLineEdit->setEnabled(true);
        m_ui->isPeriodCheckBox->setEnabled(true);
        m_repetitionTimer->stop();
        QMessageBox::information(this, "Hint", "Data Send field cannot be blank");
    }
}

void Widget::sendButton_clicked() {
    if (m_serialPort->isOpen() == true) {
        if (m_ui->isPeriodCheckBox->isChecked() == true) {
            if (m_repetitionTimer->isActive() == false) {
                if (m_repetition_ms > 0) {
                    m_ui->sendPushButton->setText("Stop");
                    m_ui->dataSendLineEdit->setEnabled(false);
                    m_ui->isPeriodCheckBox->setEnabled(false);
                    m_repetitionTimer->start(m_repetition_ms);
                } else {
                    m_ui->isPeriodCheckBox->setChecked(false);
                }
                transmitMessage();
            } else {
                m_ui->sendPushButton->setText("Send");
                m_ui->dataSendLineEdit->setEnabled(true);
                m_ui->isPeriodCheckBox->setEnabled(true);
                m_repetitionTimer->stop();
            }
        } else {
            if (m_ui->dataSendLineEdit->isEnabled() == false) {
                m_ui->dataSendLineEdit->setEnabled(true);
                m_ui->sendPushButton->setText("Send");
            }
            transmitMessage();
        }

    } else {
        QString s  = tr("**** The serial port %1 has not been opened. ****").arg(m_portName.split(" ")[0]);
        m_portName = m_ui->serialPortComboxBox->currentText();
        QMessageBox::information(this, "Hint", s);
    }
}

void Widget::adjustComboBoxViewWidth(QComboBox *combox) {
    QFontMetrics fm(combox->font());
    QRect        rect;
    int          maxLen = 0;
    for (size_t i = 0; i < combox->count(); i++) {
        rect = fm.boundingRect(combox->itemText(i));
        if (maxLen < rect.width()) {
            maxLen = rect.width();
        }
    }
    maxLen *= 1.2;
    int w = qMax(maxLen, combox->width());
    combox->view()->setFixedWidth(w);
}
