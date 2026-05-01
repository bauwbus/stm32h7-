// ============================================================
// 文件名: comm/UdpChannel.cpp
// 说明:   UDP 通信通道实现
// ============================================================
#include "UdpChannel.h"

UdpChannel::UdpChannel(QObject *parent)
    : ICommChannel(parent)
    , m_socket(new QUdpSocket(this))
{
    connect(m_socket, &QUdpSocket::readyRead,
            this, &UdpChannel::onReadyRead);
}

UdpChannel::~UdpChannel()
{
    close();
}

void UdpChannel::setConfig(const UdpConfig &cfg)
{
    m_cfg = cfg;
}

bool UdpChannel::open()
{
    if (!m_socket->bind(QHostAddress::Any, m_cfg.bindPort)) {
        emit errorOccurred("UDP绑定失败: " + m_socket->errorString());
        return false;
    }
    m_bound = true;
    emit statusChanged(QString("✅ UDP已绑定，端口: %1").arg(m_cfg.bindPort));
    return true;
}

void UdpChannel::close()
{
    m_socket->close();
    m_bound = false;
    emit statusChanged("UDP已关闭");
}

bool UdpChannel::isOpen() const
{
    return m_bound;
}

bool UdpChannel::send(const QByteArray &data)
{
    if (!m_bound) return false;
    qint64 sent = m_socket->writeDatagram(data, m_cfg.targetAddr, m_cfg.targetPort);
    return sent == data.size();
}

void UdpChannel::onReadyRead()
{
    while (m_socket->hasPendingDatagrams()) {
        QByteArray dg;
        dg.resize(int(m_socket->pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort;
        m_socket->readDatagram(dg.data(), dg.size(), &sender, &senderPort);

        emit dataReceived(dg);
        emit statusChanged(QString("[UDP] 收到来自 %1:%2 的数据")
                               .arg(sender.toString()).arg(senderPort));
    }
}
