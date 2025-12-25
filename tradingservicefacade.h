#ifndef TRADINGSERVICEFACADE_H
#define TRADINGSERVICEFACADE_H

#include <QString>
#include <QList>
#include "Enums.h"
#include "MatchingEngine.h"

class TradingServiceFacade {
public:
    TradingServiceFacade();

    // Возвращает ID заявки
    int placeOrder(int traderId, QString ticker, OrderType type, double price, int quantity);
    void cancelOrder(int orderId);

    void createTrader(QString name, double balance);
    void createAsset(QString ticker, QString name, double price);
    void addSharesToTrader(int traderId, QString ticker, int quantity, double avgPrice);

    QList<Trader*> getAllTraders();
    QList<Asset> getAllAssets();

    QList<Order> getOrdersForAsset(QString ticker);
    Asset getAssetData(QString ticker);
    double getTraderBalance(int traderId);
    QList<PortfolioItem> getTraderPortfolio(int traderId);

    // Получение имени по ID
    QString getTraderName(int id);
};

#endif // TRADINGSERVICEFACADE_H
