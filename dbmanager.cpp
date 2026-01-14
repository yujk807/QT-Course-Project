#include "dbmanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QCoreApplication>
#include <QStandardPaths>

DbManager::DbManager() {}

DbManager::~DbManager() {
    if (m_db.isOpen()) m_db.close();
}

DbManager& DbManager::instance() {
    static DbManager instance;
    return instance;
}

bool DbManager::init() {
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    QString dbPath = QCoreApplication::applicationDirPath() + "/warehouse.db";
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qDebug() << "DB Connect Error:" << m_db.lastError().text();
        return false;
    }

    QSqlQuery query;

    //创建货品表
    bool t1 = query.exec("CREATE TABLE IF NOT EXISTS products ("
                         "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                         "code TEXT UNIQUE, "
                         "name TEXT, "
                         "category TEXT, "
                         "unit TEXT, "
                         "price REAL, "
                         "quantity INTEGER DEFAULT 0, "
                         "min_stock INTEGER DEFAULT 0)");
    if (!t1) qDebug() << "Create Products Table Error:" << query.lastError();

    //创建记录表
    bool t2 = query.exec("CREATE TABLE IF NOT EXISTS records ("
                         "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                         "product_id INTEGER, "
                         "type INTEGER, "
                         "count INTEGER, "
                         "timestamp INTEGER, "
                         "remark TEXT)");
    if (!t2) qDebug() << "Create Records Table Error:" << query.lastError();

    return t1 && t2;
}

//货品管理实现

bool DbManager::addProduct(const Product &p) {
    QSqlQuery query;
    query.prepare("INSERT INTO products (code, name, category, unit, price, quantity, min_stock) "
                  "VALUES (:code, :name, :cat, :unit, :price, :qty, :min)");
    query.bindValue(":code", p.code);
    query.bindValue(":name", p.name);
    query.bindValue(":cat", p.category);
    query.bindValue(":unit", p.unit);
    query.bindValue(":price", p.price);
    query.bindValue(":qty", p.quantity);
    query.bindValue(":min", p.minStock);
    return query.exec();
}

bool DbManager::updateProduct(const Product &p) {
    QSqlQuery query;
    query.prepare("UPDATE products SET code=:code, name=:name, category=:cat, "
                  "unit=:unit, price=:price, min_stock=:min WHERE id=:id");
    query.bindValue(":code", p.code);
    query.bindValue(":name", p.name);
    query.bindValue(":cat", p.category);
    query.bindValue(":unit", p.unit);
    query.bindValue(":price", p.price);
    query.bindValue(":min", p.minStock);
    query.bindValue(":id", p.id);
    return query.exec();
}

bool DbManager::deleteProduct(int id) {
    QSqlQuery query;
    query.prepare("DELETE FROM products WHERE id = :id");
    query.bindValue(":id", id);
    return query.exec();
}

bool DbManager::isCodeExists(const QString &code) {
    QSqlQuery query;
    query.prepare("SELECT id FROM products WHERE code = :code");
    query.bindValue(":code", code);
    if (query.exec() && query.next()) return true;
    return false;
}

QList<Product> DbManager::getAllProducts() {
    QList<Product> list;
    QSqlQuery query("SELECT * FROM products ORDER BY id DESC");
    while (query.next()) {
        Product p;
        p.id = query.value("id").toInt();
        p.code = query.value("code").toString();
        p.name = query.value("name").toString();
        p.category = query.value("category").toString();
        p.unit = query.value("unit").toString();
        p.price = query.value("price").toDouble();
        p.quantity = query.value("quantity").toInt();
        p.minStock = query.value("min_stock").toInt();
        list.append(p);
    }
    return list;
}

Product DbManager::getProductById(int id) {
    QSqlQuery query;
    query.prepare("SELECT * FROM products WHERE id = :id");
    query.bindValue(":id", id);
    Product p;
    p.id = -1;
    if (query.exec() && query.next()) {
        p.id = query.value("id").toInt();
        p.code = query.value("code").toString();
        p.name = query.value("name").toString();
        p.category = query.value("category").toString();
        p.unit = query.value("unit").toString();
        p.price = query.value("price").toDouble();
        p.quantity = query.value("quantity").toInt();
        p.minStock = query.value("min_stock").toInt();
    }
    return p;
}

//事务处理出入库
QString DbManager::adjustStock(int productId, int count, bool isInbound, const QString &remark) {
    if (count <= 0) return "数量必须大于0";

    //开启事务
    m_db.transaction();

    //查询当前库存
    Product p = getProductById(productId);
    if (p.id == -1) {
        m_db.rollback();
        return "货品不存在";
    }

    int newQuantity = p.quantity;
    if (isInbound) {
        newQuantity += count;
    } else {
        //出库校验
        if (p.quantity < count) {
            m_db.rollback();
            return QString("库存不足！当前库存: %1, 申请出库: %2").arg(p.quantity).arg(count);
        }
        newQuantity -= count;
    }

    //更新库存表
    QSqlQuery updateQuery;
    updateQuery.prepare("UPDATE products SET quantity = :qty WHERE id = :id");
    updateQuery.bindValue(":qty", newQuantity);
    updateQuery.bindValue(":id", productId);

    if (!updateQuery.exec()) {
        m_db.rollback();
        return "更新库存失败: " + updateQuery.lastError().text();
    }

    //插入记录表
    QSqlQuery recordQuery;
    recordQuery.prepare("INSERT INTO records (product_id, type, count, timestamp, remark) "
                        "VALUES (:pid, :type, :count, :time, :remark)");
    recordQuery.bindValue(":pid", productId);
    recordQuery.bindValue(":type", isInbound ? 1 : 0);
    recordQuery.bindValue(":count", count);
    recordQuery.bindValue(":time", QDateTime::currentDateTime().toSecsSinceEpoch());
    recordQuery.bindValue(":remark", remark);

    if (!recordQuery.exec()) {
        m_db.rollback();
        return "写入记录失败: " + recordQuery.lastError().text();
    }

    //提交事务
    if (m_db.commit()) {
        return "";
    } else {
        m_db.rollback();
        return "事务提交失败";
    }
}

QList<Record> DbManager::getAllRecords() {
    QList<Record> list;
    QSqlQuery query("SELECT r.*, p.name as p_name FROM records r "
                    "LEFT JOIN products p ON r.product_id = p.id "
                    "ORDER BY r.timestamp DESC");

    while (query.next()) {
        Record r;
        r.id = query.value("id").toInt();
        r.productId = query.value("product_id").toInt();
        r.productName = query.value("p_name").toString(); // 这里的别名 p_name
        r.type = query.value("type").toInt();
        r.count = query.value("count").toInt();
        r.time = QDateTime::fromSecsSinceEpoch(query.value("timestamp").toLongLong());
        r.remark = query.value("remark").toString();
        list.append(r);
    }
    return list;
}
