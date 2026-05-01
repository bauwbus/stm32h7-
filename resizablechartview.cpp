#include "ResizableChartView.h"
#include <QMouseEvent>
#include <QDebug>

ResizableChartView::ResizableChartView(QWidget *parent)
    : QChartView(parent), m_isDragging(false)
{
    qDebug() << "=== 自定义控件加载成功 ===";
    setMouseTracking(true);

    // 关键：设置一个明显的边框，方便确认控件范围
    setFrameStyle(QFrame::Box | QFrame::Plain);
    setLineWidth(2);

    // 强制设置初始大小，证明控件生效
    setMinimumSize(300, 300);
    resize(400, 400);
}

void ResizableChartView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        qDebug() << ">>> 鼠标按下";
        m_isDragging = true;
        m_dragStartPos = event->globalPos();
        m_initialSize = size();
        event->accept();
        return;
    }
    QChartView::mousePressEvent(event);
}

void ResizableChartView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDragging) {
        QPoint delta = event->globalPos() - m_dragStartPos;
        QSize newSize = m_initialSize + QSize(delta.x(), delta.y());

        qDebug() << ">>> 拖动中，新大小:" << newSize;
        resize(newSize); 
        event->accept();
        return;
    }
    QChartView::mouseMoveEvent(event);
}

void ResizableChartView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        qDebug() << ">>> 鼠标松开";
        m_isDragging = false;
        event->accept();
    }
    QChartView::mouseReleaseEvent(event);
}