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
    void on_clearButton_clicked();
    void on_refreshButton_clicked();
    void showTime();
    void recvMessage();
    void sendMessage();
    void on_sendButton_clicked();
    void recvOptions(int);
    void sendOptions(int);
    void on_openButton_clicked();
    char AsciiToHex(char c);
    void on_clearButton_recvCount_clicked();
    void on_clearButton_sendCount_clicked();
    void isSendPeriodChecked(int);

   private:
    Ui::Widget  *ui;
    QSerialPort *serialPort;
    QTimer      *timerDisplayTime;
    QTimer      *timerCheckReceivedMessage;
    QTimer      *timerSendPeriodically;
    QStringList  comPortName;
    QString      portName;
    QStringList  splitedPortName;
    QString      currentTime;
    bool         isRecvConvertToHex;
    bool         isSendConvertToHex;
    int          recvCount;
    int          sendCount;
};

#endif  // WIDGET_H
