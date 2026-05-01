// ============================================================
// 文件名: protocol/ProtocolParser.h
// 说明:   协议解析器（二进制帧 + 文本行双模式）
//         从原始字节流解析数据，通过信号通知上层
// ============================================================
// 协议格式:
//   [帧头 0x22] [长度 1B] [功能码 1B] [数据 NB] [CRC16 2B]
//   长度 = 从帧头到CRC的所有字节总数
//   主→从 功能码为偶数（命令），从→主 功能码为奇数（响应=命令码+1）
//   CRC: CRC-16/Modbus，低字节在前
// ============================================================
#pragma once
#include "PmsmProtocol.h"
#include <QByteArray>
#include <QObject>

class ProtocolParser : public QObject
{
    Q_OBJECT
public:
    explicit ProtocolParser(QObject *parent = nullptr);

    // 向缓冲区追加新数据并触发解析
    void feed(const QByteArray &data);

    // 清空缓冲区（切换连接时调用）
    void reset();

signals:
    // 成功解析到一个通道数据（channel: 0-based）
    void dataReady(int channel, double value);

    // 收到完整帧（供需要原始帧的模块使用）
    void frameReceived(const PmsmFrame &frame);

    // 调试/日志信息（可直接连接到 textEdit_RecvSerial）
    void logMessage(const QString &msg);

private:
    void      parseBuffer();                           // 主解析循环
    PmsmFrame decodeFrame(const QByteArray &raw);      // 二进制帧解码
    void      dispatchFrame(const PmsmFrame &frame);   // 分发已解码帧

    QByteArray m_buf;
};
