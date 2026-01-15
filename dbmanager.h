#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QSqlDatabase>
#include <QList>
#include <QMutex>
#include "warehousedata.h"

class DbManager
{
public:
    static DbManager& instance();

    bool init(); // 初始化数据库和表

    // --- 货品管理 (CRUD) ---
    bool addProduct(const Product &p);
    bool updateProduct(const Product &p);
    bool deleteProduct(int id);
    QList<Product> getAllProducts();
    Product getProductById(int id);
    bool isCodeExists(const QString &code); // 检查编号是否重复

    // --- 核心业务：出入库操作 ---
    // 返回值: 空字符串表示成功，非空字符串表示具体的错误信息（如"库存不足"）
    QString adjustStock(int productId, int count, bool isInbound, const QString &remark);

    // --- 记录查询 ---
    QList<Record> getAllRecords();
    QList<Record> getRecordsByDateRange(const QDateTime &start, const QDateTime &end);

private:
    DbManager();
    ~DbManager();
    DbManager(const DbManager&) = delete;
    DbManager& operator=(const DbManager&) = delete;

    QSqlDatabase m_db;
};

#endif // DBMANAGER_H

