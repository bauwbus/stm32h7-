// ============================================================
// 文件名: mainwindow.h
// 说明:   主窗口头文件
//         职责：UI事件响应 + 各子模块的生命周期管理与信号连接
//         业务逻辑均委托给子模块，本类只做UI协调
// ============================================================
#pragma once
#include <QMainWindow>
#include <QLabel>

#include "PmsmProtocol.h"
#include "ProtocolParser.h"
#include "ICommChannel.h"
#include "SerialChannel.h"
#include "TcpChannel.h"
#include "UdpChannel.h"
#include "chartmanager.h"
#include "dataManager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// 连接模式枚举（与 comboBox_connection_method 下标对应）
enum ConnectMode {
    MODE_SERIAL     = 0,
    MODE_TCP_CLIENT = 1,
    MODE_UDP        = 2,
    MODE_DEMO       = 3,
};

/**
 * @brief MainWindow
 *
 * 职责：UI 事件响应 + 各子模块的生命周期管理与信号连接。
 * 业务逻辑（协议解析、图表渲染、数据缓冲）均委托给子模块。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    // 通用开关按钮（根据当前模式自动路由）
    void onToggleConnection();

    // 发送按钮
    void onSendClicked();

    // 子模块回调
    void onDataReceived(const QByteArray &raw);
    void onChannelData(int channel, double value);
    void onCommStatus(const QString &msg);
    void onCommError(const QString &err);

    // 串口列表变化
    void onPortsChanged(const QStringList &ports);

    // 连接模式切换
    void onModeChanged(int index);

    // CSV 导出
    void onExportCsv();

private:
    // ── 初始化 ──
    void initUi();
    void initConnections();
    void initSubModules();

    // ── 辅助 ──
    void setStatusMessage(const QString &msg);
    void updateToggleButton();
    void refreshPortList();
    QString getLocalIPv4Address() const;
    ICommChannel *currentChannel() const;

    // ── UI ──
    Ui::MainWindow *ui;
    QLabel         *m_statusLabel = nullptr;

    // ── 子模块 ──
    ProtocolParser *m_parser = nullptr;
    SerialChannel  *m_serial = nullptr;
    TcpChannel     *m_tcp    = nullptr;
    UdpChannel     *m_udp    = nullptr;
    ChartManager   *m_chart  = nullptr;
    DemoGenerator  *m_demo   = nullptr;
    CsvManager     *m_csv    = nullptr;

        // ── 协议帧发送辅助 ──
    void sendProtocolFrame(quint8 funcCode, const QByteArray &data = {});

    // ── CSV行暂存（等所有通道都就绪再提交） ──
    double m_pendingRow[CH_COUNT]   = {};
    bool   m_pendingDirty[CH_COUNT] = {};
};

