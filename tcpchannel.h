// ============================================================
// 文件名: comm/TcpChannel.h
// 说明:   TCP 服务器通道（实现 ICommChannel 接口）
//         支持多客户端同时连接，广播发送
// ============================================================
#pragma once
#include "ICommChannel.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>

struct TcpConfig {
    QHostAddress listenAddress = QHostAddress::Any;
    quint16      port          = 8080;
};

class TcpChannel : public ICommChannel
{
    Q_OBJECT
public:
    explicit TcpChannel(QObject *parent = nullptr);
    ~TcpChannel() override;

    void setConfig(const TcpConfig &cfg);

    bool    open()  override;
    void    close() override;
    bool    isOpen() const override;
    bool    send(const QByteArray &data) override;
    QString channelName() const override { return "TCP服务"; }

    int clientCount() const { return m_clients.size(); }

private slots:
    void onNewConnection();
    void onClientDisconnected();

private:
    QTcpServer         *m_server;
    QList<QTcpSocket*>  m_clients;
    TcpConfig           m_cfg;
    bool                m_listening = false;
};

