// ============================================================
// 文件名: mainwindow.cpp
// 说明:   主窗口实现
//         负责 UI 事件路由，所有业务逻辑委托给子模块处理
// ============================================================
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QTime>
#include <QNetworkInterface>
#include <QTcpSocket>
#include <QDebug>
#include <ResizableChartView.h>
#include "customdial.h"
// ==================== 构造 / 析构 ====================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->resize(680, 600);
    this->setWindowTitle("STM32H7 PMSM HIL 测试系统 v3.0");

    initSubModules();
    initUi();
    initConnections();

}

MainWindow::~MainWindow()
{
    // 子模块 parent 为 this，Qt 自动析构
    delete ui;
}

// ==================== 初始化：子模块 ====================

void MainWindow::initSubModules()
{
    m_parser = new ProtocolParser(this);
    m_serial = new SerialChannel(this);
    m_tcp    = new TcpChannel(this);
    m_udp    = new UdpChannel(this);
    m_chart  = new ChartManager(this);
    m_demo   = new DemoGenerator(this);
    m_csv    = new CsvManager(this);
}

// ==================== 初始化：UI ====================

void MainWindow::initUi()
{
    // 状态栏
    m_statusLabel = new QLabel("系统就绪", this);
    statusBar()->addWidget(m_statusLabel);
    statusBar()->setSizeGripEnabled(false);

    // 连接模式下拉
    ui->comboBox_connection_method->clear();
    ui->comboBox_connection_method->addItem("串口",MODE_SERIAL);
    ui->comboBox_connection_method->addItem("TCP服务器", MODE_TCP_CLIENT);
    ui->comboBox_connection_method->addItem("UDP", MODE_UDP);
    ui->comboBox_connection_method->addItem("Demo演示",  MODE_DEMO);

    // 波特率
    ui->comboBox_BaudRate->clear();
    ui->comboBox_BaudRate->addItems({"9600","19200","38400","57600",
                                     "115200","230400","460800","921600"});
    ui->comboBox_BaudRate->setCurrentText("115200");

    // 校验位
    ui->comboBox_Parity->clear();
    ui->comboBox_Parity->addItems({"无", "奇校验", "偶校验"});

    // 接收框只读
    ui->textEdit_RecvSerial->setReadOnly(true);

    // 图表
    m_chart->setup(ui->chartView);
    ui->chartView->resize(100,100);
    // 1. 创建序列和图表
    // QLineSeries *series = new QLineSeries();
    // series->append(0, 6);
    // series->append(2, 4);
    // series->append(3, 8);

    // QChart *chart = new QChart();
    // chart->addSeries(series);
    // chart->createDefaultAxes();
    // chart->setTitle("可拖动调整大小的图表");

    // // 2. 直接给 ui->chartView 设置图表（它现在已经是 ResizableChartView 了）
    // ui->chartView->setChart(chart);
    // ui->chartView->setRenderHint(QPainter::Antialiasing); // 抗锯齿，照常设置


    // 刷新串口列表
    refreshPortList();

    // 触发一次模式切换以同步UI状态
    onModeChanged(0);



        ui->dial->setRange(0, 100);
}

// ==================== 初始化：信号槽 ====================

void MainWindow::initConnections()
{
    // ── 协议解析器 ──
    connect(m_parser, &ProtocolParser::dataReady,
            this, &MainWindow::onChannelData);
    connect(m_parser, &ProtocolParser::logMessage,
            ui->textEdit_RecvSerial, &QTextEdit::append);

    // ── 串口 ──
    connect(m_serial, &SerialChannel::dataReceived,  this, &MainWindow::onDataReceived);
    connect(m_serial, &SerialChannel::statusChanged, this, &MainWindow::onCommStatus);
    connect(m_serial, &SerialChannel::errorOccurred, this, &MainWindow::onCommError);
    connect(m_serial, &SerialChannel::portsChanged,  this, &MainWindow::onPortsChanged);
    m_serial->startPortScanning();

    // ── TCP ──
    connect(m_tcp, &TcpChannel::dataReceived,  this, &MainWindow::onDataReceived);
    connect(m_tcp, &TcpChannel::statusChanged, this, &MainWindow::onCommStatus);
    connect(m_tcp, &TcpChannel::errorOccurred, this, &MainWindow::onCommError);

    // ── UDP ──
    connect(m_udp, &UdpChannel::dataReceived,  this, &MainWindow::onDataReceived);
    connect(m_udp, &UdpChannel::statusChanged, this, &MainWindow::onCommStatus);
    connect(m_udp, &UdpChannel::errorOccurred, this, &MainWindow::onCommError);

    // ── Demo ──
    connect(m_demo, &DemoGenerator::sampleReady,
            this, &MainWindow::onChannelData);

    // ── UI 控件 ──
    connect(ui->radioButton_OpenSerial, &QRadioButton::clicked,
            this, &MainWindow::onToggleConnection);

    connect(ui->pushButton_SendSerial, &QPushButton::clicked,
            this, &MainWindow::onSendClicked);

    connect(ui->comboBox_connection_method,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onModeChanged);

    connect(ui->actionExport, &QAction::triggered,
            this, &MainWindow::onExportCsv);

    if (ui->btn_RefreshIP) {
        connect(ui->btn_RefreshIP, &QPushButton::clicked, this, [this]() {
            QString ip = getLocalIPv4Address();
            ui->tcp_ipv4_address->setText(ip.isEmpty() ? "127.0.0.1" : ip);
        });
    }
}

// ==================== 槽：通用开关按钮 ====================

void MainWindow::onToggleConnection()
{
    int mode = ui->comboBox_connection_method->currentIndex();
    switch(mode)
    {
    case  MODE_SERIAL:
        if (m_serial->isOpen()) {
            m_serial->close();
            m_serial->startPortScanning();
            ui->comboBox_SerialPort->setEnabled(true);
            ui->comboBox_BaudRate->setEnabled(true);
            ui->comboBox_Parity->setEnabled(true);
        } else {
            SerialConfig cfg;
            cfg.portName = ui->comboBox_SerialPort->currentText().split(" ").first();
            cfg.baudRate = ui->comboBox_BaudRate->currentText().toInt();
            cfg.parity   = ui->comboBox_Parity->currentText();
            m_serial->setConfig(cfg);

            if (m_serial->open()) {
                m_parser->reset();
                ui->comboBox_SerialPort->setEnabled(false);
                ui->comboBox_BaudRate->setEnabled(false);
                ui->comboBox_Parity->setEnabled(false);
            }
        }
     break;
    case  MODE_TCP_CLIENT:
        if (m_tcp->isOpen()) {
            m_tcp->close();
            ui->port_number->setEnabled(true);
        } else {
            quint16 port = ui->port_number->text().trimmed().toUShort();
            if (port == 0) { QMessageBox::warning(this, "警告", "请输入有效端口号！"); return; }
            TcpConfig cfg;
            cfg.port = port;
            m_tcp->setConfig(cfg);
            if (m_tcp->open()) {
                m_parser->reset();
                ui->port_number->setEnabled(false);
            }
        }
    break;
    case  MODE_UDP:
        if (m_udp->isOpen()) {
            m_udp->close();
            ui->port_number->setEnabled(true);
            ui->tcp_ipv4_address->setEnabled(true);
        } else {
            quint16 port = ui->port_number->text().trimmed().toUShort();
            UdpConfig cfg;
            cfg.bindPort   = port;
            cfg.targetAddr = QHostAddress(ui->tcp_ipv4_address->text().trimmed());
            cfg.targetPort = port + 1;
            m_udp->setConfig(cfg);
            if (m_udp->open()) {
                m_parser->reset();
                ui->port_number->setEnabled(false);
                ui->tcp_ipv4_address->setEnabled(false);
            }
        }
        break;
    case  MODE_DEMO:
        if (m_demo->isRunning()) {
            m_demo->stop();
            setStatusMessage("Demo 已停止");
        } else {
            m_chart->clearAll();
            m_demo->start();
            setStatusMessage("🎯 Demo 运行中... (电流 / 电压 / 转速)");
        }
        break;
    }

    updateToggleButton();
}

// ==================== 槽：数据接收（原始字节） ====================

void MainWindow::onDataReceived(const QByteArray &raw)
{
    m_parser->feed(raw);
}

// ==================== 槽：单通道数据就绪 ====================

void MainWindow::onChannelData(int channel, double value)
{
    // 更新图表
    m_chart->appendData(channel, value);

    // 暂存 CSV 行数据
    if (channel >= 0 && channel < CH_COUNT) {
        m_pendingRow[channel]   = value;
        m_pendingDirty[channel] = true;
    }

    // 当所有通道均有新数据时，提交一行到 CSV 缓冲
    bool allDirty = true;
    for (int i = 0; i < CH_COUNT; i++) allDirty &= m_pendingDirty[i];
    if (allDirty) {
        m_csv->appendRow(m_pendingRow);
        for (int i = 0; i < CH_COUNT; i++) m_pendingDirty[i] = false;
    }
}

// ==================== 槽：状态变化 ====================

void MainWindow::onCommStatus(const QString &msg)
{
    setStatusMessage(msg);
    updateToggleButton();
}

void MainWindow::onCommError(const QString &err)
{
    setStatusMessage("❌ " + err);
    updateToggleButton();

    // 串口错误时恢复控件可用性
    int mode = ui->comboBox_connection_method->currentIndex();
    if (mode == MODE_SERIAL) {
        ui->comboBox_SerialPort->setEnabled(true);
        ui->comboBox_BaudRate->setEnabled(true);
        ui->comboBox_Parity->setEnabled(true);
        m_serial->startPortScanning();
    }
}

// ==================== 槽：串口列表变化 ====================

void MainWindow::onPortsChanged(const QStringList &ports)
{
    Q_UNUSED(ports);
    refreshPortList();  //刷新
}

// ==================== 槽：模式切换 ====================

void MainWindow::onModeChanged(int index)
{
    if (ui->stackedWidget->count() > index)
        ui->stackedWidget->setCurrentIndex(index);

    bool showNetwork = (index == MODE_TCP_CLIENT || index == MODE_UDP);
    if (ui->tcp_ipv4_address) ui->tcp_ipv4_address->setVisible(showNetwork);
    if (ui->port_number)  ui->port_number->setVisible(showNetwork);

    if (showNetwork) {
        QString ip = getLocalIPv4Address();
        ui->tcp_ipv4_address->setText(ip.isEmpty() ? "127.0.0.1" : ip);  //isEmpty判断ip是否为空
    }

    updateToggleButton();
}

// ==================== 槽：发送 ====================

void MainWindow::onSendClicked()
{
    QString text = ui->lineEdit_SendData->text().trimmed();
    if (text.isEmpty()) return;

    ICommChannel *ch = currentChannel();
    if (!ch || !ch->isOpen()) {
        QMessageBox::warning(this, "警告", "当前没有已连接的通道！");
        return;
    }

    QString ts = QTime::currentTime().toString("hh:mm:ss.zzz");

    // 尝试解析为协议命令格式: "CMD:0x02" 或 "CMD:0x10:数据hex"
    if (text.startsWith("CMD:", Qt::CaseInsensitive)) {
        QStringList parts = text.mid(4).split(':');
        bool okFunc;
        quint8 funcCode = static_cast<quint8>(parts[0].trimmed().toUInt(&okFunc, 16));
        if (!okFunc) {
            QMessageBox::warning(this, "警告", "功能码格式错误！请使用十六进制，如 CMD:0x02");
            return;
        }

        QByteArray payload;
        if (parts.size() > 1) {
            // 解析十六进制数据域，如 "41200000" → 4字节
            payload = QByteArray::fromHex(parts[1].trimmed().toLatin1());
        }

        sendProtocolFrame(funcCode, payload);
        ui->lineEdit_SendData->clear();
        return;
    }

    // 普通文本发送（兼容旧模式）
    QByteArray data = text.toUtf8();
    ch->send(data);
    ui->textEdit_RecvSerial->append(
        QString("[%1][发送] %2").arg(ts).arg(text));
    ui->lineEdit_SendData->clear();
}

// ==================== 槽：导出 CSV ====================

void MainWindow::onExportCsv()
{
    if (m_csv->rowCount() == 0) {
        QMessageBox::information(this, "提示", "暂无数据可导出。");
        return;
    }
    QString path = QFileDialog::getSaveFileName(this, "导出CSV", "", "CSV文件 (*.csv)");
    if (path.isEmpty()) return;

    if (m_csv->exportToFile(path))
        setStatusMessage(QString("✅ CSV已导出：%1（%2行）").arg(path).arg(m_csv->rowCount()));
    else
        QMessageBox::warning(this, "错误", "CSV导出失败！");
}

// ==================== 辅助：状态栏 ====================

void MainWindow::setStatusMessage(const QString &msg)
{
    if (m_statusLabel) m_statusLabel->setText(msg);
    qDebug() << "[状态]" << msg;
}

// ==================== 辅助：更新开关按钮文字 ====================

void MainWindow::updateToggleButton()
{
    if (!ui->radioButton_OpenSerial) return;

    int  mode   = ui->comboBox_connection_method->currentIndex();
    bool active = false;

    switch (mode) {
    case MODE_SERIAL:     active = m_serial->isOpen();   break;
    case MODE_TCP_CLIENT: active = m_tcp->isOpen();      break;
    case MODE_UDP:        active = m_udp->isOpen();      break;
    case MODE_DEMO:       active = m_demo->isRunning();  break;
    }

    static const QString openLabels[]  = {"打开串口", "开始监听", "启动UDP", "启动Demo"};
    static const QString closeLabels[] = {"关闭串口", "停止监听", "关闭UDP", "停止Demo"};

    if (mode >= 0 && mode < 4)
        ui->radioButton_OpenSerial->setText(active ? closeLabels[mode] : openLabels[mode]);
}

// ==================== 辅助：刷新串口下拉列表 ====================

void MainWindow::refreshPortList()
{
    if (!ui->comboBox_SerialPort) return;

    QString current = ui->comboBox_SerialPort->currentText().split(" ").first();

    QStringList displayList;
    QStringList nameList;
    for (const auto &info : SerialChannel::availablePorts()) {
        displayList.append(info.portName() + " (" + info.description() + ")");
        nameList.append(info.portName());
    }

    ui->comboBox_SerialPort->clear();
    ui->comboBox_SerialPort->addItems(displayList);

    // 恢复之前选中的串口
    int idx = nameList.indexOf(current);
    if (idx >= 0) ui->comboBox_SerialPort->setCurrentIndex(idx);
}

// ==================== 辅助：获取本机 IPv4 ====================

QString MainWindow::getLocalIPv4Address() const
{
    // 优先通过路由探测外网接口
    QTcpSocket socket;
    socket.connectToHost("8.8.8.8", 53);
    if (socket.waitForConnected(500)) {
        QString ip = socket.localAddress().toString();
        if (ip.startsWith("::ffff:")) ip = ip.mid(7);
        socket.disconnectFromHost();
        return ip;
    }

    // 回退：枚举网卡
    for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
        if (!(iface.flags() & QNetworkInterface::IsUp)) continue;
        if (iface.flags() & QNetworkInterface::IsLoopBack) continue;
        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            if (entry.ip().protocol() != QAbstractSocket::IPv4Protocol) continue;
            QString ip = entry.ip().toString();
            if (ip.startsWith("169.254")) continue;   // APIPA 跳过
            if (ip.startsWith("192.168.") ||
                ip.startsWith("10.")      ||
                ip.startsWith("172."))
                return ip;
        }
    }
    return {};
}

// ==================== 获取当前通信通道 ====================

ICommChannel *MainWindow::currentChannel() const
{
    switch (ui->comboBox_connection_method->currentIndex()) {
    case MODE_SERIAL:     return m_serial;
    case MODE_TCP_CLIENT: return m_tcp;
    case MODE_UDP:        return m_udp;
    default:              return nullptr;
    }
}

// ==================== 协议帧发送 ====================
// 按照新协议格式组帧并发送:
//   [帧头 0x22] [长度] [功能码] [数据...] [CRC16_L] [CRC16_H]
void MainWindow::sendProtocolFrame(quint8 funcCode, const QByteArray &data)
{
    ICommChannel *ch = currentChannel();
    if (!ch || !ch->isOpen()) {
        QMessageBox::warning(this, "警告", "当前没有已连接的通道！");
        return;
    }

    // 使用 Protocol::buildFrame 组帧（自动计算长度和CRC）
    QByteArray frame = Protocol::buildFrame(funcCode, data);
    ch->send(frame);

    // 日志输出
    QString ts = QTime::currentTime().toString("hh:mm:ss.zzz");
    QString hexStr = frame.toHex(' ').toUpper();
    QString funcDesc;
    if (Protocol::isCommand(funcCode))
        funcDesc = "命令";
    else
        funcDesc = "响应";

    ui->textEdit_RecvSerial->append(
        QString("[%1][发送][%2] 功能码=0x%3 数据长度=%4  HEX: %5")
            .arg(ts)
            .arg(funcDesc)
            .arg(funcCode, 2, 16, QChar('0'))
            .arg(data.size())
            .arg(hexStr));
}
