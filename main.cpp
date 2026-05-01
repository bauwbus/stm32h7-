#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();

    for (const QString &locale : uiLanguages) {
        const QString baseName = "PMSM_HIL_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    MainWindow w;
    w.show();
    return a.exec();
}


// // ============================================================
// // 文件名: main.cpp
// // 说明:   应用程序入口
// //         设置全局编码、高DPI支持，创建主窗口
// // ============================================================
// #include "mainwindow.h"
// #include <QApplication>
// #include <QTextCodec>

// int main(int argc, char *argv[])
// {
//     QApplication a(argc, argv);

//     // 全局 UTF-8 编码（中文显示必须）
//     QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

//     // 高 DPI 支持
//     a.setAttribute(Qt::AA_EnableHighDpiScaling);
//     a.setAttribute(Qt::AA_UseHighDpiPixmaps);

//     MainWindow w;
//     w.show();
//     return a.exec();
// }
