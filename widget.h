#ifndef WIDGET_H
#define WIDGET_H

#include <QButtonGroup>
#include <QDebug>
#include <QMessageBox>
#include <QTime>
#include <QTimer>
#include <QValidator>
#include <QWidget>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

#include "qvalidator.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class HexIntegerValidator : public QValidator {
    Q_OBJECT
   public:
    explicit HexIntegerValidator(QObject *parent = nullptr);

    QValidator::State validate(QString &input, int &) const;

    void setMaximum(uint maximum);

   private:
    uint m_maximum = 0;
};

class HexStringValidator : public QValidator {
    Q_OBJECT

   public:
    explicit HexStringValidator(QObject *parent = nullptr);

    QValidator::State validate(QString &input, int &pos) const;

    void setMaxLength(int maxLength);

   private:
    int m_maxLength = 0;
};

class Widget : public QWidget {
    Q_OBJECT

   public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

   private slots:
    void showTime();
    void recvMessage();
    void sendMessage();
    void sendButton_clicked();

   private:
    Ui::Widget *ui;

    void initialization();
    char AsciiToHex(char c);

    QSerialPort *serialPort;
    QTimer      *timerCheckReceivedMessage;
    QTimer      *timerSendPeriodically;
    QStringList  comPortName;
    QString      portName;
    QStringList  splitedPortName;
    QString      currentTime;
    QString      uiTransmitMessage;
    bool         isRecvConvertToHex = false;
    bool         isSendConvertToHex = false;
    int          recvCount          = 0;
    int          sendCount          = 0;
    int          sendPeriod         = 0;
};

#endif  // WIDGET_H
