#include "TradingServiceFacade.h"
#include "OrderFactory.h"

TradingServiceFacade::TradingServiceFacade() {}

void TradingServiceFacade::placeOrder(int traderId, QString ticker, OrderType type, double price, int quantity) {
    // ИСПРАВЛЕНИЕ: Передаем 5 аргументов (без ID), так как ID генерируется внутри фабрики
    Order o = OrderFactory::createOrder(traderId, ticker, type, price, quantity);
    MatchingEngine::getInstance()->processNewOrder(o);
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
