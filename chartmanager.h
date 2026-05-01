#pragma once

#include "PmsmProtocol.h"
#include <QObject>
#include <QChart>
#include <QLineSeries>
#include <QValueAxis>
#include <QChartView>
#include <QColor>
#include <QString>
#include <QtCharts>

//QT_CHARTS_USE_NAMESPACE
    // ============================================================
    // 文件名: chart/ChartManager.h
    // 说明:   三通道实时波形图表管理器
    //         封装所有 QtCharts 操作，提供简洁的 appendData() 接口
    // ============================================================
    /**
 * @brief ChartManager
 * 管理三通道实时波形图表，封装所有 QtCharts 操作。
 * 调用 setup() 将图表挂载到指定的 QChartView 上。
 */


    class ChartManager : public QObject
{
    Q_OBJECT
public:
    explicit ChartManager(QObject *parent = nullptr);

    // 初始化并挂载到指定 chartView（必须首先调用）
    void setup(QChartView *chartView, int maxPoints = 200);

    // 追加单通道数据（channel: 0-based）
    void appendData(int channel, double value);

    // 清空所有通道数据（切换 Demo 时调用）
    void clearAll();

    // 设置通道可见性
    void setChannelVisible(int channel, bool visible);

private:
    void updateYAxis();

    QChart      *m_chart              = nullptr;
    QLineSeries *m_series[CH_COUNT]   = {nullptr};
    QValueAxis  *m_axisX              = nullptr;
    QValueAxis  *m_axisY              = nullptr;

    int    m_maxPoints  = 200;
    int    m_pointIndex = 0;
    double m_yMin[CH_COUNT];
    double m_yMax[CH_COUNT];

    static const QString s_names[CH_COUNT];
    static const QColor  s_colors[CH_COUNT];
};




// PMSM_HIL/
// ├── main.cpp                          ← 应用入口
// ├── mainwindow.h / .cpp               ← 主窗口（UI协调）
// ├── PMSM_HIL.pro                      ← 项目文件
// ├── protocol/
// │   ├── PmsmProtocol.h                ← 公共类型定义
// │   ├── ProtocolParser.h / .cpp       ← 协议解析器
// ├── comm/
// │   ├── ICommChannel.h                ← 通信抽象接口
// │   ├── SerialChannel.h / .cpp        ← 串口
// │   ├── TcpChannel.h / .cpp           ← TCP服务器
// │   └── UdpChannel.h / .cpp           ← UDP
// ├── chart/
// │   └── ChartManager.h / .cpp         ← 图表管理
// └── data/
//     └── DataManager.h / .cpp          ← Demo发生器 + CSV管理
