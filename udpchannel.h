// ============================================================
// 文件名: comm/UdpChannel.h
// 说明:   UDP 通信通道（实现 ICommChannel 接口）
//         绑定本地端口接收，向目标地址/端口发送
// ============================================================
#pragma once
#include "ICommChannel.h"
#include <QUdpSocket>
#include <QHostAddress>

struct UdpConfig {
    quint16      bindPort   = 9000;
    QHostAddress targetAddr = QHostAddress::LocalHost;
    quint16      targetPort = 9001;
};

class UdpChannel : public ICommChannel
{
    Q_OBJECT
public:
    explicit UdpChannel(QObject *parent = nullptr);
    ~UdpChannel() override;

    void setConfig(const UdpConfig &cfg);

    bool    open()  override;
    void    close() override;
    bool    isOpen() const override;
    bool    send(const QByteArray &data) override;
    QString channelName() const override { return "UDP"; }

private slots:
    void onReadyRead();

private:
    QUdpSocket *m_socket;
    UdpConfig   m_cfg;
    bool        m_bound = false;
};

