#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dbmanager.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QFormLayout>
#include <QTimer>
#include <QDialogButtonBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_progressDlg(nullptr)
{
    ui->setupUi(this);

    //初始化数据库
    if (!DbManager::instance().init()) {
        QMessageBox::critical(this, "严重错误", "无法初始化数据库，程序即将退出。");
        QTimer::singleShot(0, qApp, &QCoreApplication::quit);
        return;
    }

    //初始化 Model
    m_productModel = new ProductModel(this);
    m_recordModel = new RecordModel(this);

    //设置 ProxyModel
    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_productModel);
    m_proxyModel->setFilterKeyColumn(2); // 默认搜索"名称"列 (第2列)
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    //绑定 View
    ui->tableStock->setModel(m_proxyModel);
    ui->tableRecord->setModel(m_recordModel);

    //绑定信号槽
    setupUiLogic();

    //初始加载
    m_productModel->reload();
    refreshComboList();

    //状态栏
    ui->statusbar->showMessage("系统就绪");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupUiLogic() {
    //标签页切换
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);

    //库存页
    connect(ui->btnStockAdd, &QPushButton::clicked, this, &MainWindow::onAddProduct);
    connect(ui->btnStockExport, &QPushButton::clicked, this, &MainWindow::onStockExport);
    connect(ui->btnStockImport, &QPushButton::clicked, this, &MainWindow::onStockImport);
    connect(ui->editSearch, &QLineEdit::textChanged, this, &MainWindow::onSearchStock);

    //操作页
    connect(ui->btnSubmit, &QPushButton::clicked, this, &MainWindow::onSubmitOperation);

    //记录页
    connect(ui->btnRecordExport, &QPushButton::clicked, this, &MainWindow::onRecordExport);
    connect(ui->btnRefreshRecord, &QPushButton::clicked, this, &MainWindow::onRefreshRecords);
}

void MainWindow::onTabChanged(int index) {
    //切换到"出入库操作"(index=1)时，刷新一下下拉框，防止刚加了货品这里没显示
    if (index == 1) {
        refreshComboList();
    }
    //切换到"历史记录"(index=2)时，刷新列表
    if (index == 2) {
        m_recordModel->reload();
    }
}

void MainWindow::refreshComboList() {
    ui->comboProduct->clear();
    QList<Product> list = DbManager::instance().getAllProducts();
    for (const Product &p : list) {
        ui->comboProduct->addItem(p.displayText(), QVariant(p.id));
    }
}

void MainWindow::onSearchStock(const QString &text) {
    m_proxyModel->setFilterFixedString(text);
}

//新增货品
void MainWindow::onAddProduct() {
    QDialog dlg(this);
    dlg.setWindowTitle("新增货品");
    QFormLayout *layout = new QFormLayout(&dlg);

    QLineEdit *editCode = new QLineEdit();
    QLineEdit *editName = new QLineEdit();
    QComboBox *comboCat = new QComboBox();
    comboCat->addItems({"电子产品", "办公用品", "原材料", "食品", "其他"});
    comboCat->setEditable(true);
    QLineEdit *editUnit = new QLineEdit("个");
    QDoubleSpinBox *spinPrice = new QDoubleSpinBox();
    spinPrice->setMaximum(999999.99);
    QSpinBox *spinMin = new QSpinBox();
    spinMin->setRange(0, 10000);

    layout->addRow("编号 (唯一)*:", editCode);
    layout->addRow("名称*:", editName);
    layout->addRow("分类:", comboCat);
    layout->addRow("单位:", editUnit);
    layout->addRow("单价:", spinPrice);
    layout->addRow("安全库存预警:", spinMin);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(box);
    connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        QString code = editCode->text().trimmed();
        QString name = editName->text().trimmed();
        if (code.isEmpty() || name.isEmpty()) {
            QMessageBox::warning(this, "警告", "编号和名称不能为空！");
            return;
        }

        if (DbManager::instance().isCodeExists(code)) {
            QMessageBox::warning(this, "错误", "该编号已存在，请更换！");
            return;
        }

        Product p;
        p.code = code;
        p.name = name;
        p.category = comboCat->currentText();
        p.unit = editUnit->text();
        p.price = spinPrice->value();
        p.quantity = 0;
        p.minStock = spinMin->value();

        if (DbManager::instance().addProduct(p)) {
            m_productModel->reload();
            QMessageBox::information(this, "成功", "货品添加成功");
        } else {
            QMessageBox::critical(this, "失败", "数据库写入失败");
        }
    }
}

//出入库提交 ---
void MainWindow::onSubmitOperation() {
    //获取选中的货品ID
    int index = ui->comboProduct->currentIndex();
    if (index < 0) {
        QMessageBox::warning(this, "提示", "请选择一个货品");
        return;
    }
    int pId = ui->comboProduct->currentData().toInt();

    //获取参数
    int count = ui->spinCount->value();
    QString remark = ui->editRemark->text();
    bool isInbound = ui->radioIn->isChecked();

    //调用核心逻辑
    QString error = DbManager::instance().adjustStock(pId, count, isInbound, remark);

    if (error.isEmpty()) {
        QMessageBox::information(this, "成功", isInbound ? "入库成功！" : "出库成功！");
        ui->editRemark->clear();
        ui->spinCount->setValue(1);
        m_productModel->reload();
        refreshComboList();

    } else {
        QMessageBox::critical(this, "操作失败", error);
    }
}

//显示进度条
void MainWindow::showProgress(const QString &title) {
    if (m_progressDlg) delete m_progressDlg;
    m_progressDlg = new QProgressDialog(title, "取消", 0, 100, this);
    m_progressDlg->setWindowModality(Qt::WindowModal);
    m_progressDlg->setMinimumDuration(0);
    m_progressDlg->setValue(0);
}

void MainWindow::onWorkerProgress(int current, int total) {
    if (m_progressDlg) {
        if (total > 0) {
            m_progressDlg->setMaximum(total);
            m_progressDlg->setValue(current);
        } else {
            m_progressDlg->setMaximum(0);
            m_progressDlg->setValue(0);
        }
    }
}

void MainWindow::onWorkerFinished(bool success, QString msg) {
    if (m_progressDlg) {
        m_progressDlg->close();
        delete m_progressDlg;
        m_progressDlg = nullptr;
    }

    if (success) {
        QMessageBox::information(this, "完成", msg);
        m_productModel->reload();
        m_recordModel->reload();
        refreshComboList();
    } else {
        QMessageBox::critical(this, "错误", msg);
    }
}

//导出库存
void MainWindow::onStockExport() {
    QString path = QFileDialog::getSaveFileName(this, "导出库存", "stocks.csv", "CSV Files (*.csv)");
    if (path.isEmpty()) return;

    showProgress("正在导出库存数据...");

    DataWorker *worker = new DataWorker(this);
    worker->setTask(TaskType::ExportStock, path);

    connect(worker, &DataWorker::progressUpdated, this, &MainWindow::onWorkerProgress);
    connect(worker, &DataWorker::taskFinished, this, &MainWindow::onWorkerFinished);
    connect(worker, &DataWorker::finished, worker, &QObject::deleteLater);

    worker->start();
}

//导入库存
void MainWindow::onStockImport() {
    QString path = QFileDialog::getOpenFileName(this, "选择导入文件", "", "CSV Files (*.csv)");
    if (path.isEmpty()) return;

    if (QMessageBox::question(this, "确认", "批量导入可能需要一些时间，建议先备份数据库。\n确定继续吗？") != QMessageBox::Yes)
        return;

    showProgress("正在批量导入，请稍候...");

    DataWorker *worker = new DataWorker(this);
    worker->setTask(TaskType::ImportStock, path);

    connect(worker, &DataWorker::progressUpdated, this, &MainWindow::onWorkerProgress);
    connect(worker, &DataWorker::taskFinished, this, &MainWindow::onWorkerFinished);
    connect(worker, &DataWorker::finished, worker, &QObject::deleteLater);

    worker->start();
}

//导出记录
void MainWindow::onRecordExport() {
    QString path = QFileDialog::getSaveFileName(this, "导出记录", "records.csv", "CSV Files (*.csv)");
    if (path.isEmpty()) return;

    showProgress("正在导出历史记录...");

    DataWorker *worker = new DataWorker(this);
    worker->setTask(TaskType::ExportRecord, path);

    connect(worker, &DataWorker::progressUpdated, this, &MainWindow::onWorkerProgress);
    connect(worker, &DataWorker::taskFinished, this, &MainWindow::onWorkerFinished);
    connect(worker, &DataWorker::finished, worker, &QObject::deleteLater);

    worker->start();
}

void MainWindow::onRefreshRecords() {
    m_recordModel->reload();
    ui->statusbar->showMessage("记录表已刷新");
}
