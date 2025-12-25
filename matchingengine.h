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
    MatchingEngine();

    QMap<int, Trader*> traders;
    QMap<QString, Asset> assets;
    QList<Order> activeOrders;
    QList<Trade> tradeHistory;

    int orderIdCounter;
    int tradeIdCounter;

    bool isIPOAvailable(QString ticker);

public:
    static MatchingEngine* getInstance();

    void addTrader(QString name, double balance);
    void addAsset(QString ticker, QString name, double startPrice);

    // НОВЫЙ МЕТОД: Раздача акций при старте
    void addSharesToTrader(int traderId, QString ticker, int quantity, double avgPrice);

    QList<Trader*> getAllTraders();
    QList<Asset> getAllAssets();
    Trader* getTrader(int id);
    Asset getAsset(QString ticker);
    QList<Order> getOrderBook(QString ticker);
    QList<Trade> getTradeHistory();

    // ИЗМЕНЕНИЕ: Возвращает bool (успех/неудача)
    bool processNewOrder(Order order);
    void cancelOrder(int orderId);

signals:
    void marketUpdate(QString ticker);
    void tradeExecuted(QString ticker, double price, int qty);
    void portfolioUpdated(int traderId);
    void globalHistoryUpdated();
};

#endif // MATCHINGENGINE_H
