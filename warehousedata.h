#ifndef WAREHOUSEDATA_H
#define WAREHOUSEDATA_H

#include <QString>
#include <QDateTime>

// 货品结构体
struct Product {
    int id;
    QString code;       // 编号 (唯一)
    QString name;       // 名称
    QString category;   // 分类
    QString unit;       // 单位
    double price;       // 单价
    int quantity;       // 当前库存
    int minStock;       // 警戒库存

    // 用于在 ComboBox 中显示的文本
    QString displayText() const {
        return QString("%1 - %2 (库存: %3)").arg(code, name).arg(quantity);
    }
};

// 出入库记录结构体
struct Record {
    int id;
    int productId;      // 关联的货品ID
    QString productName;// 冗余存一个名称，方便显示（可选）
    int type;           // 1: 入库, 0: 出库
    int count;          // 数量
    QDateTime time;     // 操作时间
    QString remark;     // 备注

    QString typeStr() const {
        return (type == 1) ? "入库" : "出库";
    }
};

#endif // WAREHOUSEDATA_H
