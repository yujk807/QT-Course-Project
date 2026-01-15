#include "productmodel.h"
#include "dbmanager.h"
#include <QColor>
#include <QBrush>

ProductModel::ProductModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    //定义表头
    m_headers << "ID" << "编号" << "名称" << "分类" << "单位" << "单价" << "当前库存" << "安全库存";
}

void ProductModel::reload() {
    beginResetModel();
    m_products = DbManager::instance().getAllProducts();
    endResetModel();
}

int ProductModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_products.size();
}

int ProductModel::columnCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_headers.size();
}

QVariant ProductModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_products.size())
        return QVariant();

    const Product &p = m_products.at(index.row());

    //文本显示
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0: return p.id;
        case 1: return p.code;
        case 2: return p.name;
        case 3: return p.category;
        case 4: return p.unit;
        case 5: return QString::number(p.price, 'f', 2); // 保留2位小数
        case 6: return p.quantity;
        case 7: return p.minStock;
        }
    }
    //库存预警颜色
    else if (role == Qt::ForegroundRole) {
        // 如果当前库存 < 安全库存，整行文字变红
        if (p.quantity < p.minStock) {
            return QBrush(Qt::red);
        }
    }
    else if (role == Qt::TextAlignmentRole) {
        if (index.column() >= 5)
            return QVariant(Qt::AlignCenter);
        return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
    }

    return QVariant();
}

QVariant ProductModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return m_headers.value(section);
    }
    return QVariant();
}

Product ProductModel::getProduct(int row) {
    if (row >= 0 && row < m_products.size())
        return m_products.at(row);
    return Product();
}

