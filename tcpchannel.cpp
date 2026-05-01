#include "tcpchannel.h"
// ============================================================
// 文件名: comm/TcpChannel.cpp
// 说明:   TCP 服务器通道实现
// ============================================================
#include <QDebug>

TcpChannel::TcpChannel(QObject *parent)
    : ICommChannel(parent)
    , m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection,
            this, &TcpChannel::onNewConnection);
}

TcpChannel::~TcpChannel()
{
    close();
}

void TcpChannel::setConfig(const TcpConfig &cfg)
{
    m_cfg = cfg;
}

bool TcpChannel::open()
{
    if (!m_server->listen(m_cfg.listenAddress, m_cfg.port)) {
        emit errorOccurred("TCP监听失败: " + m_server->errorString());
        return false;
    }
    m_listening = true;
    emit statusChanged(QString("✅ TCP服务已启动，端口: %1").arg(m_cfg.port));
    return true;
}

void TcpChannel::close()
{
    for (QTcpSocket *c : qAsConst(m_clients)) {
        if (c) { c->disconnectFromHost(); c->deleteLater(); }
    }
    m_clients.clear();
    m_server->close();
    m_listening = false;
    emit statusChanged("TCP服务已停止");
}

bool TcpChannel::isOpen() const
{
    return m_listening;
}

bool TcpChannel::send(const QByteArray &data)
{
    bool sent = false;
    for (QTcpSocket *c : qAsConst(m_clients)) {
        if (c && c->state() == QTcpSocket::ConnectedState) {
            c->write(data);
            sent = true;
        }
    }
    return sent;
}

void TcpChannel::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *client = m_server->nextPendingConnection();
        m_clients.append(client);

        emit statusChanged("✅ TCP客户端已连接: " + client->peerAddress().toString());

        connect(client, &QTcpSocket::readyRead, this, [this, client]() {
            emit dataReceived(client->readAll());
        });

        connect(client, &QTcpSocket::disconnected,
                this, &TcpChannel::onClientDisconnected);
    }
}

void TcpChannel::onClientDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    m_clients.removeOne(client);
    client->deleteLater();

    if (m_clients.isEmpty())
        emit statusChanged("❌ 所有TCP客户端已断开");
    else
        emit statusChanged(QString("TCP客户端断开，剩余 %1 个").arg(m_clients.size()));
}

