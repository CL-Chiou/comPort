#ifndef WIDGET_H
#define WIDGET_H

#include <QButtonGroup>
#include <QDebug>
#include <QMessageBox>
#include <QTime>
#include <QTimer>
#include <QWidget>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget {
    Q_OBJECT

   public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

   private slots:
    void initialization();
    void on_clearButton_clicked();
    void on_refreshButton_clicked();
    void showTime();
    void recvMessage();
    void sendMessage();
    void on_sendButton_clicked();
    void checkRecvFormatting(int);
    void checkSendFormatting(int);
    void on_openButton_clicked();
    char AsciiToHex(char c);
    void on_resetRecvCountButton_clicked();
    void on_resetSendCountButton_clicked();
    void isSendPeriodChecked(int);

   private:
    Ui::Widget  *ui;
    QSerialPort *serialPort;
    QTimer      *timerCheckReceivedMessage;
    QTimer      *timerSendPeriodically;
    QStringList  comPortName;
    QString      portName;
    QStringList  splitedPortName;
    QString      currentTime;
    QString      uiTransmitMessage;
    bool         isRecvConvertToHex;
    bool         isSendConvertToHex;
    int          recvCount;
    int          sendCount;
    int          sendPeriod;
};

#endif  // WIDGET_H
