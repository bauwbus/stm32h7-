// ============================================================
// 文件名: chart/ChartManager.cpp
// 说明:   三通道实时波形图表管理器实现
//         Y轴自适应、X轴滑动窗口、通道颜色定义
// ============================================================
#include "chartmanager.h"
#include <QPainter>
#include <QtMath>

//QT_CHARTS_USE_NAMESPACE

    // ==================== 通道元数据 ====================
const QString ChartManager::s_names[CH_COUNT]  = {"电流 (A)", "电压 (V)", "转速 (RPM)"};
const QColor  ChartManager::s_colors[CH_COUNT] = {
    QColor("#E74C3C"),   // 电流 红
    QColor("#2ECC71"),   // 电压 绿
    QColor("#3498DB"),   // 转速 蓝
};

ChartManager::ChartManager(QObject *parent)
    : QObject(parent)
{
    for (int i = 0; i < CH_COUNT; i++) {
        m_yMin[i] =  1e9;
        m_yMax[i] = -1e9;
    }
}

void ChartManager::setup(QChartView *chartView, int maxPoints)
{
    m_maxPoints = maxPoints;

    m_chart = new QChart();
    m_chart->legend()->show();
    m_chart->setTitle("PMSM 实时波形");
    m_chart->setBackgroundBrush(QBrush(QColor("#1C1C2E")));
    m_chart->setTitleBrush(QBrush(Qt::white));
    m_chart->setAnimationOptions(QChart::NoAnimation);   // 实时场景关闭动画

    for (int i = 0; i < CH_COUNT; i++) {
        m_series[i] = new QLineSeries();
        m_series[i]->setName(s_names[i]);
        m_series[i]->setColor(s_colors[i]);
        m_series[i]->setOpacity(0.9);
        m_chart->addSeries(m_series[i]);
    }

    m_axisX = new QValueAxis();
    m_axisY = new QValueAxis();

    m_axisX->setRange(0, m_maxPoints);
    m_axisY->setRange(-20, 20);
    m_axisX->setLabelFormat("%d");
    m_axisY->setLabelFormat("%.2f");
    m_axisX->setLabelsColor(Qt::white);
    m_axisY->setLabelsColor(Qt::white);
    m_axisX->setGridLineColor(QColor("#333355"));
    m_axisY->setGridLineColor(QColor("#333355"));
    m_axisX->setTitleText("采样点");
    m_axisX->setTitleBrush(QBrush(Qt::white));
    m_axisY->setTitleText("数值");
    m_axisY->setTitleBrush(QBrush(Qt::white));

    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);

    for (int i = 0; i < CH_COUNT; i++) {
        m_series[i]->attachAxis(m_axisX);
        m_series[i]->attachAxis(m_axisY);
    }

    chartView->setChart(m_chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setBackgroundBrush(QBrush(QColor("#1C1C2E")));
}

void ChartManager::appendData(int channel, double value)
{
    if (channel < 0 || channel >= CH_COUNT) return;
    if (!m_series[channel] || !m_axisX) return;

    m_series[channel]->append(m_pointIndex, value);

    // 滑动窗口：超出 maxPoints 时删除最旧数据
    if (m_series[channel]->count() > m_maxPoints)
        m_series[channel]->removePoints(0, m_series[channel]->count() - m_maxPoints);

    m_pointIndex++;

    // X轴滚动
    m_axisX->setRange(qMax(0, m_pointIndex - m_maxPoints), m_pointIndex + 5);

    // Y轴自适应
    m_yMin[channel] = qMin(m_yMin[channel], value);
    m_yMax[channel] = qMax(m_yMax[channel], value);
    updateYAxis();
}

void ChartManager::clearAll()
{
    for (int i = 0; i < CH_COUNT; i++) {
        if (m_series[i]) m_series[i]->clear();
        m_yMin[i] =  1e9;
        m_yMax[i] = -1e9;
    }
    m_pointIndex = 0;
    if (m_axisX) m_axisX->setRange(0, m_maxPoints);
    if (m_axisY) m_axisY->setRange(-20, 20);
}

void ChartManager::setChannelVisible(int channel, bool visible)
{
    if (channel >= 0 && channel < CH_COUNT && m_series[channel])
        m_series[channel]->setVisible(visible);
}

void ChartManager::updateYAxis()
{
    double gMin =  1e9;
    double gMax = -1e9;
    for (int i = 0; i < CH_COUNT; i++) {
        if (m_series[i] && m_series[i]->count() > 0) {
            gMin = qMin(gMin, m_yMin[i]);
            gMax = qMax(gMax, m_yMax[i]);
        }
    }
    if (gMin >= gMax) { gMin = -10; gMax = 10; }
    double margin = (gMax - gMin) * 0.15;
    m_axisY->setRange(gMin - margin, gMax + margin);
    m_axisY->applyNiceNumbers();
}

