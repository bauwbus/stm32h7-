 // ============================================================
// 文件名: data/DataManager.h
// 说明:   数据管理模块
//         DemoGenerator  - 仿真信号发生器（20Hz，三通道）
//         CsvManager     - CSV 环形缓冲与文件导出
// ============================================================
#pragma once
#include "PmsmProtocol.h"
#include <QObject>
#include <QVector>
#include <QTimer>
#include <QString>

// ==================== Demo 信号发生器 ====================

/**
 * @brief DemoGenerator
 * 产生仿真信号（电流/电压/转速），20Hz，供 Demo 模式使用。
 * 无需真实硬件即可演示系统功能。
 */
class DemoGenerator : public QObject
{
    Q_OBJECT
public:
    explicit DemoGenerator(QObject *parent = nullptr);

    void start();
    void stop();
    bool isRunning() const;

signals:
    // channel: 0-based
    void sampleReady(int channel, double value);

private slots:
    void onTick();

private:
    QTimer *m_timer;
    double  m_phase[CH_COUNT] = {0, 0, 0};
};

// ==================== CSV 管理器 ====================

/**
 * @brief CsvManager
 * 环形缓冲，最多保留 MAX_ROWS 行数据。
 * 支持随时调用 exportToFile() 导出为 CSV。
 */
class CsvManager : public QObject
{
    Q_OBJECT
public:
    explicit CsvManager(QObject *parent = nullptr);

    // 追加一行（全通道值）
    void appendRow(const double values[CH_COUNT]);

    // 导出到文件
    bool exportToFile(const QString &path) const;

    int  rowCount() const { return m_buffer.size(); }
    void clear()          { m_buffer.clear(); }

private:
    static constexpr int MAX_ROWS = 100000;
    QVector<CsvRow> m_buffer;
};
