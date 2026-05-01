// ============================================================
// 文件名: protocol/PmsmProtocol.h
// 说明:   公共类型定义（通道枚举、协议帧结构、CSV行结构）
//         所有模块共享此头文件
// ============================================================
#pragma once
#include <QtGlobal>
#include <QByteArray>
#include <QVector>

// ==================== 通道枚举 ====================
enum ChannelIndex {
    CH_VOLTAGE = 0,
    CH_CURRENT,              // 电流
    CH_SPEED,
    CH_Power,                // 功率
    CH_COUNT,
};

// ==================== 协议帧常量 ====================
// 帧结构: [帧头 1B] [长度 1B] [功能码 1B] [数据 NB] [校验码 2B]
//   帧头:   固定 0x22
//   长度:   从帧头到校验码的所有字节总数（含自身）
//   功能码: 主→从 偶数（命令），从→主 奇数（响应 = 命令码+1）
//   校验码: CRC-16/Modbus，低字节在前
// 最小帧长 = 帧头(1) + 长度(1) + 功能码(1) + 校验码(2) = 5（无数据）
namespace Protocol {

static constexpr quint8  FRAME_HEAD    = 0x22;   // 帧头固定值
static constexpr quint8     MIN_FRAME_LEN = 5;      // 最小帧长（无数据域）
static constexpr quint8     MAX_FRAME_LEN = 256;    // 最大帧长
static constexpr quint8     IDX_HEAD      = 0;      // 帧头偏移
static constexpr quint8     IDX_LEN       = 1;      // 长度偏移
static constexpr quint8     IDX_FUNC      = 2;      // 功能码偏移
static constexpr quint8     IDX_DATA      = 3;      // 数据域起始偏移

// ==================== 功能码定义 ====================
// 主机→从机（命令）：偶数
// 从机→主机（响应）：奇数 = 命令码 + 1

static constexpr quint8 CMD_READ_VOLTAGE  = 0x30;  // 读电压
static constexpr quint8 RSP_READ_VOLTAGE  = 0x31;  // 读电压响应

static constexpr quint8 CMD_READ_CURRENT  = 0x32;  // 读电流
static constexpr quint8 RSP_READ_CURRENT  = 0x33;  // 读电流响应

static constexpr quint8 CMD_READ_SPEED    = 0x34;  // 读转速
static constexpr quint8 RSP_READ_SPEED    = 0x35;  // 读转速响应

static constexpr quint8 CMD_READ_POWER    = 0x36;  // 读功率
static constexpr quint8 RSP_READ_POWER    = 0x37;  // 读功率响应

static constexpr quint8 CMD_READ_ALL      = 0x38;  // 读全部通道
static constexpr quint8 RSP_READ_ALL      = 0x39;  // 读全部通道响应

static constexpr quint8 CMD_SET_PARAM     = 0x3A;  // 设置参数
static constexpr quint8 RSP_SET_PARAM     = 0x3B;  // 设置参数响应

static constexpr quint8 CMD_HEARTBEAT     = 0x3C;  // 心跳
static constexpr quint8 RSP_HEARTBEAT     = 0x3D;  // 心跳响应

static constexpr quint8 CMD_Basic         = 0x3E;  // 窗口里中间状态的全部数据
static constexpr quint8 RSP_Basic         = 0x3F;


// ==================== CRC-16/Modbus ====================
inline quint16 crc16(const quint8 *data, int len)
{
    quint16 crc = 0xFFFF;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;   // 低字节在前
}

// ==================== 辅助函数 ====================

// 判断功能码是否为命令（偶数）
inline bool isCommand(quint8 func)  { return (func & 0x01) == 0; }
// 判断功能码是否为响应（奇数）
inline bool isResponse(quint8 func) { return (func & 0x01) == 1; }
// 从命令码获取对应响应码
inline quint8 toResponse(quint8 cmd) { return cmd | 0x01; }
// 从响应码获取对应命令码
inline quint8 toCommand(quint8 rsp)  { return rsp & 0xFE; }

// ==================== 组帧函数 ====================
// 构建一帧完整数据（自动计算长度和CRC）
inline QByteArray buildFrame(quint8 funcCode, const QByteArray &data = {})
{
    // 总长度 = 帧头(1) + 长度(1) + 功能码(1) + 数据(N) + CRC(2)
    int totalLen = 3 + data.size() + 2;
    QByteArray frame;
    frame.reserve(totalLen);
    frame.append(static_cast<char>(FRAME_HEAD));
    frame.append(static_cast<char>(totalLen & 0xFF));
    frame.append(static_cast<char>(funcCode));
    frame.append(data);

    // 计算 CRC（从帧头到数据域末尾，不含CRC本身）
    quint16 crc = crc16(reinterpret_cast<const quint8*>(frame.constData()), frame.size());
    frame.append(static_cast<char>(crc & 0xFF));         // CRC 低字节
    frame.append(static_cast<char>((crc >> 8) & 0xFF));  // CRC 高字节

    return frame;
}

}  // namespace Protocol

// ==================== 解码结果 ====================
struct PmsmFrame {
    bool       valid    = false;
    quint8     funcCode = 0;       // 功能码
    QByteArray data;               // 数据域（不含帧头、长度、功能码、CRC）
    // 便捷访问
    qint8      channel  = -1;      // 解析后的通道号（0-based），-1表示未识别
    double     value    = 0.0;     // 解析后的数值
};

// ==================== CSV 行 ====================
struct CsvRow {
    qint64 timestamp          = 0;
    double values[CH_COUNT]   = {0, 0, 0, 0};
};
