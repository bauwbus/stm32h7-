// ============================================================
// 文件名: comm/SerialChannel.h
// 说明:   串口通信通道（实现 ICommChannel 接口）
//         包含自动串口扫描、波特率/校验位配置
// ============================================================
#pragma once
#include "ICommChannel.h"
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>

struct SerialConfig {
    QString portName;
    qint32  baudRate = 115200;
    QString parity   = "无";   // "无" / "奇校验" / "偶校验"
};

class SerialChannel : public ICommChannel
{
    Q_OBJECT
public:
    explicit SerialChannel(QObject *parent = nullptr);
    ~SerialChannel() override;

    void setConfig(const SerialConfig &cfg);

    bool    open()  override;
    void    close() override;
    bool    isOpen() const override;
    bool    send(const QByteArray &data) override;
    QString channelName() const override { return "串口"; }

    // 供 UI 使用：获取当前可用串口列表
    static QList<QSerialPortInfo> availablePorts();

    void startPortScanning(int intervalMs = 1000);
    void stopPortScanning();

signals:
    // 可用串口列表发生变化时触发
    void portsChanged(const QStringList &ports);

private slots:
    void onReadyRead();
    void onError(QSerialPort::SerialPortError error);
    void onScanTimer();

private:
    QSerialPort  *m_port;
    QTimer       *m_scanTimer;
    SerialConfig  m_cfg;
    QStringList   m_lastPorts;
};

