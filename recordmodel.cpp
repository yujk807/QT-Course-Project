#include "recordmodel.h"
#include "dbmanager.h"
#include <QColor>
#include <QBrush>

RecordModel::RecordModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    m_headers << "时间" << "类型" << "货品名称" << "变动数量" << "备注";
}

void RecordModel::reload() {
    beginResetModel();
    m_records = DbManager::instance().getAllRecords();
    endResetModel();
}

int RecordModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_records.size();
}

int RecordModel::columnCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_headers.size();
}

QVariant RecordModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_records.size())
        return QVariant();

    const Record &r = m_records.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0: return r.time.toString("yyyy-MM-dd HH:mm:ss");
        case 1: return r.typeStr();
        case 2: return r.productName;
        case 3: return r.count;
        case 4: return r.remark;
        }
    }
    else if (role == Qt::ForegroundRole) {
        if (index.column() == 1) {
            return (r.type == 1) ? QBrush(Qt::darkGreen) : QBrush(Qt::darkBlue);
        }
    }
    else if (role == Qt::TextAlignmentRole) {
        return QVariant(Qt::AlignCenter);
    }

    return QVariant();
}

QVariant RecordModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return m_headers.value(section);
    }
    return QVariant();
}
