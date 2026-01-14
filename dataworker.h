#ifndef DATAWORKER_H
#define DATAWORKER_H

#include <QThread>
#include <QString>

// 定义任务类型
enum class TaskType {
    ExportStock,   // 导出库存
    ExportRecord,  // 导出记录
    ImportStock    // 导入库存
};

class DataWorker : public QThread
{
    Q_OBJECT
public:
    explicit DataWorker(QObject *parent = nullptr);

    // 设置任务参数
    void setTask(TaskType type, const QString &filePath);

protected:
    void run() override; // 线程入口函数

signals:
    // 报告进度 (current / total)
    void progressUpdated(int current, int total);
    // 任务结束 (成功/失败，以及消息)
    void taskFinished(bool success, QString message);

private:
    TaskType m_type;
    QString m_filePath;

    // 内部处理函数
    void doExportStock();
    void doExportRecord();
    void doImportStock();
};

#endif // DATAWORKER_H
