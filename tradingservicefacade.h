#ifndef TRADINGSERVICEFACADE_H
#define TRADINGSERVICEFACADE_H

#include <QString>
#include <QList>
#include "Enums.h"
#include "MatchingEngine.h"

class TradingServiceFacade {
public:
    TradingServiceFacade();

    // Основные действия
    void placeOrder(int traderId, QString ticker, OrderType type, double price, int quantity);
    void createTrader(QString name, double balance);
    void createAsset(QString ticker, QString name, double price);

    // --- ВОТ ЭТИХ МЕТОДОВ НЕ ХВАТАЛО ---
    QList<Trader*> getAllTraders();
    QList<Asset> getAllAssets();

    QList<Order> getOrdersForAsset(QString ticker);
    Asset getAssetData(QString ticker);
    double getTraderBalance(int traderId);
    QList<PortfolioItem> getTraderPortfolio(int traderId);
    // -----------------------------------
};

#endif // TRADINGSERVICEFACADE_H
