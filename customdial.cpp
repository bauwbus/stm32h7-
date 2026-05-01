#include "CustomDial.h"
#include <QtMath>

CustomDial::CustomDial(QWidget *parent) : QDial(parent)
{
    // 禁用父类的默认外部刻度线
    setNotchesVisible(false);
}

void CustomDial::paintEvent(QPaintEvent *event)
{
    // 1. 先绘制父类的QDial（只画旋钮背景和手柄，不画刻度）
    QDial::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::black);

    // 2. 获取基本参数
    const int min = minimum();
    const int max = maximum();
    const int range = max - min;

    // 3. 计算绘制区域
    const QRectF dialRect = rect().adjusted(30, 30, -30, -30);
    const qreal centerX = dialRect.center().x();
    const qreal centerY = dialRect.center().y();
    const qreal outerRadius = dialRect.width() / 2; // 旋钮最外圈半径

    // --- 4. 手动绘制【内部】刻度线 ---
    const qreal notchInnerRadius = outerRadius * 0.6; // 刻度线内端点半径（离中心近）
    const qreal notchOuterRadius = outerRadius * 0.8; // 刻度线外端点半径（离中心远，在内部）
    const int notchStep = 10;

    painter.setPen(QPen(Qt::black, 2)); // 设置刻度线粗细
    //绘制数字（调整到刻度线外侧，仍在旋钮内部/边缘） ---
    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);
    painter.setPen(Qt::darkGray);
    const qreal textRadius = outerRadius * 1.05; // 数字所在半径（紧贴旋钮边缘）


    for (int val = min; val <= max; val += notchStep)
    {
        const qreal t = (val - min) / static_cast<qreal>(range) +15 ;
        const qreal angle = -135 + t * 270; // QDial默认角度范围 -135° 到 135°
        const qreal rad = qDegreesToRadians(angle);

        // 计算刻度线的两个端点（都在旋钮内部）
        const qreal x1 = centerX + notchInnerRadius * qCos(rad);
        const qreal y1 = centerY - notchInnerRadius * qSin(rad);
        const qreal x2 = centerX + notchOuterRadius * qCos(rad);
        const qreal y2 = centerY - notchOuterRadius * qSin(rad);

        const qreal x = centerX + textRadius * qCos(rad);
        const qreal y = centerY - textRadius * qSin(rad);
        painter.drawLine(QPointF(x1, y1), QPointF(x2, y2));
        QRectF textRect(x - 15, y - 10, 30, 20);
        painter.drawText(textRect, Qt::AlignCenter, QString::number(val));
    }
}