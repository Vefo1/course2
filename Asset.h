#ifndef ASSET_H
#define ASSET_H
#include <QString>
#include <QList>

struct Asset {
    QString ticker;
    QString name;
    double currentPrice;
    QList<double> priceHistory;
};
#endif // ASSET_H
