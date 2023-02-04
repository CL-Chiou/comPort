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

class HexStringValidator : public QValidator {
    Q_OBJECT

   public:
    explicit HexStringValidator(QObject *parent = nullptr);

    QValidator::State validate(QString &input, int &pos) const;
};

class Widget : public QWidget {
    Q_OBJECT

   public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

   private slots:
    void displayTime();
    void receiveMessage();
    void transmitMessage();
    void sendButton_clicked();

   private:
    Ui::Widget *m_ui;

    void initialization();
    void openSerialPort();

    QSerialPort        *m_serialPort          = nullptr;
    QTimer             *m_receiveMessageTimer = nullptr;
    QTimer             *m_repetitionTimer     = nullptr;
    QTimer             *m_displayTimeTimer    = nullptr;
    HexStringValidator *m_hexStringValidator  = nullptr;

    QString m_portName;
    QString m_currentTime;

    bool m_isRecvHexEnabled     = false;
    bool m_isSendHexEnabled     = false;
    int  m_numberPacketReceived = 0;
    int  m_numberPacketWritten  = 0;
    int  m_repetition_ms        = 0;
};

#endif  // WIDGET_H
