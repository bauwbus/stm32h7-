// ============================================================
// 文件名: comm/SerialChannel.cpp
// 说明:   串口通信通道实现
// ============================================================
#include "SerialChannel.h"
#include <QDebug>

SerialChannel::SerialChannel(QObject *parent)
    : ICommChannel(parent)
    , m_port(new QSerialPort(this))
    , m_scanTimer(new QTimer(this))
{
    connect(m_port, &QSerialPort::readyRead,
            this, &SerialChannel::onReadyRead);
    connect(m_port, &QSerialPort::errorOccurred,
            this, &SerialChannel::onError);
    connect(m_scanTimer, &QTimer::timeout,
            this, &SerialChannel::onScanTimer);
}

SerialChannel::~SerialChannel()
{
    close();
}

void SerialChannel::setConfig(const SerialConfig &cfg)
{
    m_cfg = cfg;
}

bool SerialChannel::open()
{
    if (m_cfg.portName.isEmpty()) {
        emit errorOccurred("未选择串口");
        return false;
    }

    m_port->setPortName(m_cfg.portName);
    m_port->setBaudRate(m_cfg.baudRate);
    m_port->setDataBits(QSerialPort::Data8);
    m_port->setStopBits(QSerialPort::OneStop);
    m_port->setFlowControl(QSerialPort::NoFlowControl);

    if (m_cfg.parity == "奇校验")
        m_port->setParity(QSerialPort::OddParity);
    else if (m_cfg.parity == "偶校验")
        m_port->setParity(QSerialPort::EvenParity);
    else
        m_port->setParity(QSerialPort::NoParity);

    if (!m_port->open(QIODevice::ReadWrite)) {
        emit errorOccurred("串口打开失败: " + m_port->errorString());
        return false;
    }

    stopPortScanning();
    emit statusChanged("✅ 串口已打开: " + m_cfg.portName);
    return true;
}

void SerialChannel::close()
{
    if (m_port->isOpen()) {
        m_port->close();
        emit statusChanged("串口已关闭");
    }
}

bool SerialChannel::isOpen() const
{
    return m_port->isOpen();
}

bool SerialChannel::send(const QByteArray &data)
{
    if (!m_port->isOpen()) return false;
    return m_port->write(data) == data.size();
}

QList<QSerialPortInfo> SerialChannel::availablePorts()
{
    return QSerialPortInfo::availablePorts();
}

void SerialChannel::startPortScanning(int intervalMs)
{
    m_scanTimer->start(intervalMs);
}

void SerialChannel::stopPortScanning()
{
    m_scanTimer->stop();
}

void SerialChannel::onReadyRead()
{
    emit dataReceived(m_port->readAll());
}

void SerialChannel::onError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) return;
    qDebug() << "[串口错误]" << m_port->errorString();
    close();
    emit errorOccurred("串口错误: " + m_port->errorString());
}

void SerialChannel::onScanTimer()
{
    QStringList ports;
    for (const auto &info : QSerialPortInfo::availablePorts())
        ports.append(info.portName());

    if (ports != m_lastPorts) {
        m_lastPorts = ports;
        emit portsChanged(ports);
    }
}
