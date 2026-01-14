#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSortFilterProxyModel>
#include <QProgressDialog>
#include "productmodel.h"
#include "recordmodel.h"
#include "dataworker.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // --- 界面交互槽函数 ---
    void onTabChanged(int index);   // 切换标签页
    void onSearchStock(const QString &text); // 搜索库存

    // --- 按钮点击槽函数 ---
    void onAddProduct();            // 新增货品
    void onStockExport();           // 导出库存
    void onStockImport();           // 导入库存
    void onRecordExport();          // 导出记录
    void onSubmitOperation();       // 提交出入库
    void onRefreshRecords();        // 刷新记录表

    // --- 后台线程回调 ---
    void onWorkerProgress(int current, int total);
    void onWorkerFinished(bool success, QString msg);

private:
    Ui::MainWindow *ui;

    // 模型对象
    ProductModel *m_productModel;
    QSortFilterProxyModel *m_proxyModel; // 用于库存表的搜索过滤
    RecordModel *m_recordModel;

    // 辅助功能
    void setupUiLogic();
    void refreshComboList(); // 刷新出入库页面的下拉框
    void showProgress(const QString &title); // 显示进度条

    QProgressDialog *m_progressDlg; // 进度条对话框
};

#endif // MAINWINDOW_H
