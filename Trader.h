#ifndef TRADER_H
#define TRADER_H
#include <QString>
#include <QMap>
#include "PortfolioItem.h"

class Trader {
public:
    int id;
    QString name;
    double cashBalance;
    // Тикер -> Элемент портфеля
    QMap<QString, PortfolioItem> portfolio;

    Trader(int _id, QString _name, double _balance)
        : id(_id), name(_name), cashBalance(_balance) {}
};
#endif // TRADER_H
