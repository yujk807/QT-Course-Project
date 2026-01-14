#ifndef RECORDMODEL_H
#define RECORDMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include "warehousedata.h"

class RecordModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit RecordModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void reload();

private:
    QList<Record> m_records;
    QStringList m_headers;
};

#endif // RECORDMODEL_H
