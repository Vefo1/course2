#include "TradingServiceFacade.h"
#include "OrderFactory.h"

TradingServiceFacade::TradingServiceFacade() {}

int TradingServiceFacade::placeOrder(int traderId, QString ticker, OrderType type, double price, int quantity) {
    Order o = OrderFactory::createOrder(traderId, ticker, type, price, quantity);

    bool success = MatchingEngine::getInstance()->processNewOrder(o);

    if (success) {
        return o.id; // Успех -> возвращаем ID
    } else {
        return -1;   // Ошибка -> возвращаем -1
    }
}

void TradingServiceFacade::addSharesToTrader(int traderId, QString ticker, int quantity, double avgPrice) {
    MatchingEngine::getInstance()->addSharesToTrader(traderId, ticker, quantity, avgPrice);
}

void TradingServiceFacade::cancelOrder(int orderId) {
    MatchingEngine::getInstance()->cancelOrder(orderId);
}

void TradingServiceFacade::createTrader(QString name, double balance) {
    MatchingEngine::getInstance()->addTrader(name, balance);
}

void TradingServiceFacade::createAsset(QString ticker, QString name, double price) {
    MatchingEngine::getInstance()->addAsset(ticker, name, price);
}

QList<Trader*> TradingServiceFacade::getAllTraders() {
    return MatchingEngine::getInstance()->getAllTraders();
}

QList<Asset> TradingServiceFacade::getAllAssets() {
    return MatchingEngine::getInstance()->getAllAssets();
}

QList<Order> TradingServiceFacade::getOrdersForAsset(QString ticker) {
    return MatchingEngine::getInstance()->getOrderBook(ticker);
}

Asset TradingServiceFacade::getAssetData(QString ticker) {
    return MatchingEngine::getInstance()->getAsset(ticker);
}

double TradingServiceFacade::getTraderBalance(int traderId) {
    Trader* t = MatchingEngine::getInstance()->getTrader(traderId);
    return t ? t->cashBalance : 0.0;
}

QList<PortfolioItem> TradingServiceFacade::getTraderPortfolio(int traderId) {
    Trader* t = MatchingEngine::getInstance()->getTrader(traderId);
    return t ? t->portfolio.values() : QList<PortfolioItem>();
}

QString TradingServiceFacade::getTraderName(int id) {
    Trader* t = MatchingEngine::getInstance()->getTrader(id);
    return t ? t->name : "System";
}
