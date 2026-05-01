// ============================================================
// 文件名: protocol/ProtocolParser.cpp
// 说明:   协议解析器实现
// 协议格式:
//   [帧头 0x22] [长度 1B] [功能码 1B] [数据 NB] [CRC16_L] [CRC16_H]
//   长度 = 帧头到CRC末尾的总字节数
//   功能码: 主→从偶数(命令)，从→主奇数(响应=命令码+1)
//   CRC: CRC-16/Modbus，低字节在前
// 同时兼容文本行格式("I:12.3", "1,2,3")
// ============================================================
#include "ProtocolParser.h"
#include "PmsmProtocol.h"
#include <QDebug>
#include <QTime>
#include <cstring>

ProtocolParser::ProtocolParser(QObject *parent)
    : QObject(parent)
{}

void ProtocolParser::feed(const QByteArray &data)
{
    m_buf.append(data);
    parseBuffer();
}

void ProtocolParser::reset()
{
    m_buf.clear();
}

// ==================== 主解析循环 ====================
void ProtocolParser::parseBuffer()
{
    // 防止缓冲区无限增长
    if (m_buf.size() >= 4096)
        m_buf = m_buf.right(512);

    // ---- 二进制帧解析 ----
    while (m_buf.size() >= Protocol::MIN_FRAME_LEN) {

        // // 搜索帧头 0x22
        // int headIdx = -1;
        // for (int i = 0; i < m_buf.size(); i++) {
        //     if (static_cast<quint8>(m_buf[i]) == Protocol::FRAME_HEAD) {
        //         headIdx = i;
        //         break;
        //     }
        // }

        // // 无帧头 → 全部当作文本处理
        // if (headIdx < 0) {
        //     QString text = QString::fromUtf8(m_buf);
        //     if (text.contains('\n')) {
        //         QStringList lines = text.split('\n');
        //         for (int i = 0; i < lines.size() - 1; i++)
        //             tryTextLine(lines[i].trimmed());
        //         m_buf = lines.last().toUtf8();
        //     }
        //     break;
        // }

        // // 帧头前有数据 → 尝试文本解析后丢弃
        // if (headIdx > 0) {
        //     QByteArray textPart = m_buf.left(headIdx);
        //     QString text = QString::fromUtf8(textPart);
        //     QStringList lines = text.split('\n');
        //     for (int i = 0; i < lines.size(); i++)
        //         tryTextLine(lines[i].trimmed());
        //     m_buf = m_buf.mid(headIdx);
        // }

        // 至少需要2字节才能读取长度字段
        if (m_buf.size() < 2) break;

        // 读取帧长度
        quint8 frameLen = static_cast<quint8>(m_buf[Protocol::IDX_LEN]);

        // 长度合法性检查
        if (frameLen < Protocol::MIN_FRAME_LEN || frameLen > (Protocol::MAX_FRAME_LEN - 1)) {
            emit logMessage(QString("[协议] 长度非法: %1，跳过").arg(frameLen));
            m_buf = m_buf.mid(1);
            continue;
        }

        // 数据不够一帧，等待更多数据
        if (m_buf.size() < frameLen) break;

        // 提取完整帧并解码
        QByteArray rawFrame = m_buf.left(frameLen);
        PmsmFrame frame = decodeFrame(rawFrame);

        if (frame.valid) {
            dispatchFrame(frame);
            m_buf = m_buf.mid(frameLen);
        } else {
            // CRC 失败或解码异常，跳过帧头继续搜索
            m_buf = m_buf.mid(1);
        }
    }

    // // ---- 文本行解析（换行符驱动） ----
    // while (true) {
    //     int nl = m_buf.indexOf('\n');
    //     if (nl < 0) break;
    //     QByteArray lineBuf = m_buf.left(nl);
    //     // 确认这段没有二进制帧头，避免重复处理
    //     bool hasBinHead = false;
    //     for (int i = 0; i < lineBuf.size(); i++) {
    //         if (static_cast<quint8>(lineBuf[i]) == Protocol::FRAME_HEAD) {
    //             hasBinHead = true;
    //             break;
    //         }
    //     }
    //     // if (!hasBinHead)
    //     //     tryTextLine(QString::fromUtf8(lineBuf).trimmed());
    //     m_buf = m_buf.mid(nl + 1);
    // }
}

// ==================== 二进制帧解码 ====================
// 帧格式: [0x22] [LEN] [FUNC] [DATA...] [CRC_L] [CRC_H]
PmsmFrame ProtocolParser::decodeFrame(const QByteArray &raw)
{
    PmsmFrame f;
    int len = raw.size();
    if (len < Protocol::MIN_FRAME_LEN) return f;

    // 帧头检查
    if (static_cast<quint8>(raw[Protocol::IDX_HEAD]) != Protocol::FRAME_HEAD)
        return f;

    // 长度一致性检查
    quint8 frameLen = static_cast<quint8>(raw[Protocol::IDX_LEN]);
    if (frameLen != len) return f;

    // 提取 CRC（最后2字节，低字节在前）
    quint8 crcLo = static_cast<quint8>(raw[len - 2]);
    quint8 crcHi = static_cast<quint8>(raw[len - 1]);
    quint16 rxCrc = static_cast<quint16>(crcLo) | (static_cast<quint16>(crcHi) << 8);

    // 计算 CRC（从帧头到数据域末尾，即 raw[0..len-3]）
    quint16 calcCrc = Protocol::crc16(
        reinterpret_cast<const quint8*>(raw.constData()), len - 2);

    if (calcCrc != rxCrc) {
        qDebug() << QString("[协议] CRC失败: 计算=0x%1 收到=0x%2")
                        .arg(calcCrc, 4, 16, QChar('0'))
                        .arg(rxCrc, 4, 16, QChar('0'));
        return f;
    }

    // 解析成功
    f.valid    = true;
    f.funcCode = static_cast<quint8>(raw[Protocol::IDX_FUNC]);
    f.data     = raw.mid(Protocol::IDX_DATA, len - Protocol::MIN_FRAME_LEN);  // 纯数据域

    return f;
}

// ==================== 帧分发 ====================
// 根据功能码将帧数据映射为通道 + 数值，并发射信号
void ProtocolParser::dispatchFrame(const PmsmFrame &frame)
{
    QString timeStr = QTime::currentTime().toString("hh:mm:ss.zzz");
    static const QString chNames[] = {"电压", "电流", "转速", "功率"};

    // 发射完整帧信号（供高级模块使用）
    emit frameReceived(frame);

    switch (frame.funcCode) {

    // ---- 单通道响应：数据域为 4 字节 float(LE) ----
    case Protocol::RSP_READ_VOLTAGE:
    case Protocol::RSP_READ_CURRENT:
    case Protocol::RSP_READ_SPEED:
    case Protocol::RSP_READ_POWER:
    {
        if (frame.data.size() < 4) {
            emit logMessage(QString("[%1][协议] 功能码0x%2 数据长度不足")
                                .arg(timeStr).arg(frame.funcCode, 2, 16, QChar('0')));
            return;
        }
        float val;
        memcpy(&val, frame.data.constData(), sizeof(float));

        // 功能码到通道映射: 0x03→CH_VOLTAGE, 0x05→CH_CURRENT, 0x07→CH_SPEED, 0x09→CH_Power
        int ch = -1;
        switch (frame.funcCode) {
        case Protocol::RSP_READ_VOLTAGE: ch = CH_VOLTAGE; break;
        case Protocol::RSP_READ_CURRENT: ch = CH_CURRENT; break;
        case Protocol::RSP_READ_SPEED:   ch = CH_SPEED;   break;
        case Protocol::RSP_READ_POWER:   ch = CH_Power;   break;
        }

        if (ch >= 0 && ch < CH_COUNT) {
            emit dataReady(ch, static_cast<double>(val));
            emit logMessage(QString("[%1][帧] 功能码0x%2 %3=%4")
                                .arg(timeStr)
                                .arg(frame.funcCode, 2, 16, QChar('0'))
                                .arg(chNames[ch])
                                .arg(static_cast<double>(val), 0, 'f', 4));
        }
        break;
    }

    // ---- 全部通道响应：数据域为 N×4 字节 float(LE) ----
    case Protocol::RSP_READ_ALL:
    {
        int count = frame.data.size() / 4;
        if (count == 0) {
            emit logMessage(QString("[%1][协议] 全通道响应数据为空").arg(timeStr));
            return;
        }
        for (int i = 0; i < qMin(count, static_cast<int>(CH_COUNT)); i++) {
            float val;
            memcpy(&val, frame.data.constData() + i * 4, sizeof(float));
            emit dataReady(i, static_cast<double>(val));
            emit logMessage(QString("[%1][帧] 全通道 %2=%3")
                                .arg(timeStr)
                                .arg(i < CH_COUNT ? chNames[i] : QString::number(i))
                                .arg(static_cast<double>(val), 0, 'f', 4));
        }
        break;
    }

    // ---- 设置参数响应 ----
    case Protocol::RSP_SET_PARAM:
    {
        quint8 result = frame.data.isEmpty() ? 0xFF : static_cast<quint8>(frame.data[0]);
        emit logMessage(QString("[%1][帧] 设置参数响应: %2")
                            .arg(timeStr,result == 0x00 ? "成功" : "失败"));
        break;
    }

    // ---- 心跳响应 ----
    case Protocol::RSP_HEARTBEAT:
    {
        emit logMessage(QString("[%1][帧] 心跳响应").arg(timeStr));
        break;
    }

    default:


        logMessage(QString("[%1][帧] 未知功能码: 0x%2  数据长度: %3")
                            .arg(timeStr)
                            .arg(frame.funcCode, 2, 16, QChar('0'))
                            .arg(frame.data.size()));
        break;
    }
}

