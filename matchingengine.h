#ifndef MATCHINGENGINE_H
#define MATCHINGENGINE_H

#include <QObject>
#include <QMap>
#include <QList>
#include "Trader.h"
#include "Asset.h"
#include "Order.h"
#include "Trade.h"

class MatchingEngine : public QObject {
    Q_OBJECT
private:
    static MatchingEngine* instance;
    MatchingEngine(); // Private Constructor

    QMap<int, Trader*> traders;
    QMap<QString, Asset> assets;
    QList<Order> activeOrders;
    QList<Trade> tradeHistory;

    int orderIdCounter;
    int tradeIdCounter;

public:
    static MatchingEngine* getInstance();

    // Методы управления данными
    void addTrader(QString name, double balance);
    void addAsset(QString ticker, QString name, double startPrice);

    // Getters
    QList<Trader*> getAllTraders();
    QList<Asset> getAllAssets();
    Trader* getTrader(int id);
    Asset getAsset(QString ticker);
    QList<Order> getOrderBook(QString ticker);
    QList<Trade> getTradeHistory();

    // Core Logic
    void processNewOrder(Order order);

signals:
    void marketUpdate(QString ticker);
    void tradeExecuted(QString ticker, double price, int qty);
    void portfolioUpdated(int traderId);
    void globalHistoryUpdated(); // Сигнал для вкладки "История"
};

#endif // MATCHINGENGINE_H
