#pragma once
#include <QChartView>
#include <QWidget>

class ResizableChartView : public QChartView
{
    Q_OBJECT

public:
    explicit ResizableChartView(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool m_isDragging;
    QPoint m_dragStartPos;
    QSize m_initialSize;
};



