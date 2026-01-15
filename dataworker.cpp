#include "dataworker.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QDebug>
#include <QDateTime>

DataWorker::DataWorker(QObject *parent) : QThread(parent) {}

void DataWorker::setTask(TaskType type, const QString &filePath) {
    m_type = type;
    m_filePath = filePath;
}

void DataWorker::run() {
    //在线程内部建立独立的数据库连接
    const QString connName = "WorkerConnection";

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(QCoreApplication::applicationDirPath() + "/warehouse.db");

        if (!db.open()) {
            emit taskFinished(false, "后台线程无法连接数据库");
            return;
        }

        switch (m_type) {
        case TaskType::ExportStock:
            doExportStock();
            break;
        case TaskType::ExportRecord:
            doExportRecord();
            break;
        case TaskType::ImportStock:
            doImportStock();
            break;
        }

        db.close();
    }
    QSqlDatabase::removeDatabase(connName);
}

//导出库存逻辑
void DataWorker::doExportStock() {
    QSqlDatabase db = QSqlDatabase::database("WorkerConnection");
    QSqlQuery query(db);

    //先查询总数
    if (!query.exec("SELECT count(*) FROM products")) {
        emit taskFinished(false, "查询失败");
        return;
    }
    query.next();
    int total = query.value(0).toInt();

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit taskFinished(false, "无法创建文件");
        return;
    }

    QTextStream out(&file);
    //写入BOM防止Excel中文乱码
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    out.setEncoding(QStringConverter::Utf8);
#else
    out.setCodec("UTF-8");
#endif
    out << QString::fromLocal8Bit("\xEF\xBB\xBF");

    //表头
    out << "ID,编号,名称,分类,单位,单价,库存数量,预警阈值\n";

    if (!query.exec("SELECT * FROM products")) {
        emit taskFinished(false, "查询数据失败");
        return;
    }

    int current = 0;
    while (query.next()) {
        out << query.value("id").toString() << ","
            << query.value("code").toString() << ","
            << query.value("name").toString() << ","
            << query.value("category").toString() << ","
            << query.value("unit").toString() << ","
            << query.value("price").toString() << ","
            << query.value("quantity").toString() << ","
            << query.value("min_stock").toString() << "\n";

        current++;
        //每10条或者是最后一条时更新一下进度
        if (current % 10 == 0 || current == total) {
            emit progressUpdated(current, total);
        }
        //模拟一点耗时，让你能看清进度条（实际项目可去掉）
        msleep(2);
    }

    file.close();
    emit taskFinished(true, QString("成功导出 %1 条库存数据").arg(current));
}

//导出记录逻辑
void DataWorker::doExportRecord() {
    QSqlDatabase db = QSqlDatabase::database("WorkerConnection");
    QSqlQuery query(db);

    //查询记录总数
    query.exec("SELECT count(*) FROM records");
    query.next();
    int total = query.value(0).toInt();

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit taskFinished(false, "无法打开文件");
        return;
    }

    QTextStream out(&file);
    out << QString::fromLocal8Bit("\xEF\xBB\xBF"); // BOM
    //表头
    out << "ID,货品ID,类型(1入0出),数量,时间,备注\n";

    query.exec("SELECT * FROM records ORDER BY timestamp DESC");

    int current = 0;
    while (query.next()) {
        QString timeStr = QDateTime::fromSecsSinceEpoch(query.value("timestamp").toLongLong())
        .toString("yyyy-MM-dd HH:mm:ss");

        out << query.value("id").toString() << ","
            << query.value("product_id").toString() << ","
            << (query.value("type").toInt() == 1 ? "入库" : "出库") << ","
            << query.value("count").toString() << ","
            << timeStr << ","
            << query.value("remark").toString() << "\n";

        current++;
        if (current % 10 == 0) emit progressUpdated(current, total);
    }

    emit progressUpdated(total, total);
    file.close();
    emit taskFinished(true, QString("成功导出 %1 条出入库记录").arg(current));
}

//导入库存逻辑
void DataWorker::doImportStock() {
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit taskFinished(false, "无法打开导入文件");
        return;
    }

    QTextStream in(&file);

    QSqlDatabase db = QSqlDatabase::database("WorkerConnection");
    db.transaction();

    QSqlQuery query(db);

    query.prepare("INSERT INTO products (code, name, category, unit, price, quantity, min_stock) "
                  "VALUES (:code, :name, :cat, :unit, :price, :qty, :min)");

    int successCount = 0;
    int lineIndex = 0;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        lineIndex++;

        if (line.isEmpty()) continue;
        if (lineIndex == 1 && (line.contains("ID") || line.contains("编号"))) continue;

        QStringList parts = line.split(",");



        if (parts.size() < 7) continue;


        QString code = parts[1].trimmed();
        QString name = parts[2].trimmed();
        QString cat  = parts[3].trimmed();
        QString unit = parts[4].trimmed();
        double price = parts[5].toDouble();
        int qty      = parts[6].toInt();
        int min      = parts[7].toInt();

        query.bindValue(":code", code);
        query.bindValue(":name", name);
        query.bindValue(":cat", cat);
        query.bindValue(":unit", unit);
        query.bindValue(":price", price);
        query.bindValue(":qty", qty);
        query.bindValue(":min", min);

        if (query.exec()) {
            successCount++;
        }

        if (successCount % 50 == 0) {
            emit progressUpdated(successCount, 0);
        }
    }

    //提交事务
    if (db.commit()) {
        file.close();
        emit taskFinished(true, QString("批量导入完成，成功插入 %1 个货品").arg(successCount));
    } else {
        db.rollback();
        file.close();
        emit taskFinished(false, "数据库提交事务失败，导入回滚");
    }
}

