#ifndef PRODUCTMODEL_H
#define PRODUCTMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include "warehousedata.h"

class ProductModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit ProductModel(QObject *parent = nullptr);

    // 标准 Model 必须重写的函数
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    // 自定义功能
    void reload();          // 从数据库重新加载数据
    Product getProduct(int row); // 获取某一行的数据（用于编辑或出入库选择）

private:
    QList<Product> m_products;
    QStringList m_headers;
};

#endif // PRODUCTMODEL_H
