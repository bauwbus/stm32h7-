#ifndef CUSTOMDIAL_H
#define CUSTOMDIAL_H

#include <QDial>
#include <QPainter>
#include <QMainWindow>

class CustomDial : public QDial
{
    Q_OBJECT
public:
    explicit CustomDial(QWidget *parent = nullptr);

protected:
    // 绘制事件
    void paintEvent(QPaintEvent *event) override;
};

#endif // CUSTOMDIAL_H