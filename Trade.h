#ifndef TRADE_H
#define TRADE_H
#include <QString>
#include <QDateTime>

struct Trade {
    int id;
    int buyOrderId;
    int sellOrderId;
    int buyerId;
    int sellerId;
    QString assetTicker;
    double price;
    int quantity;
    QDateTime timestamp;
};
#endif // TRADE_H
