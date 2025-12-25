#ifndef ORDER_H
#define ORDER_H
#include <QString>
#include <QDateTime>
#include "Enums.h"

struct Order {
    int id;
    int traderId;
    QString assetTicker;
    OrderType type;
    double price;
    int quantity;
    OrderStatus status;
    QDateTime timestamp;
};
#endif // ORDER_H
