#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //设置全局样式
    a.setStyleSheet(R"(
        QWidget { font-size: 14px; font-family: "Segoe UI", "Microsoft YaHei"; }
        QHeaderView::section {
            background-color: #f0f0f0;
            padding: 4px;
            border: 1px solid #d0d0d0;
            font-weight: bold;
        }
        QTableView {
            selection-background-color: #0078d7;
            selection-color: white;
            alternate-background-color: #f9f9f9;
        }
        QPushButton {
            padding: 5px 10px;
            border: 1px solid #999;
            border-radius: 3px;
            background-color: #fff;
        }
        QPushButton:hover { background-color: #e6f7ff; border-color: #0078d7; }
        QLineEdit, QComboBox, QSpinBox {
            padding: 4px;
            border: 1px solid #ccc;
            border-radius: 2px;
        }
    )");

    MainWindow w;
    w.show();
    return a.exec();
}

