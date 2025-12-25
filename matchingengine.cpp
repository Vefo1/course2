#include "MatchingEngine.h"
#include <algorithm>

MatchingEngine* MatchingEngine::instance = nullptr;

MatchingEngine::MatchingEngine() {
    orderIdCounter = 1;
    tradeIdCounter = 1;
}

MatchingEngine* MatchingEngine::getInstance() {
    if (!instance) instance = new MatchingEngine();
    return instance;
}

void MatchingEngine::addTrader(QString name, double balance) {
    int newId = traders.size() + 1;
    traders.insert(newId, new Trader(newId, name, balance));
}

void MatchingEngine::addAsset(QString ticker, QString name, double startPrice) {
    Asset a;
    a.ticker = ticker;
    a.name = name;
    a.currentPrice = startPrice;
    a.priceHistory.append(startPrice);
    assets.insert(ticker, a);
}

QList<Trader*> MatchingEngine::getAllTraders() { return traders.values(); }
QList<Asset> MatchingEngine::getAllAssets() { return assets.values(); }
Trader* MatchingEngine::getTrader(int id) { return traders.value(id, nullptr); }
Asset MatchingEngine::getAsset(QString ticker) { return assets.value(ticker); }
QList<Trade> MatchingEngine::getTradeHistory() { return tradeHistory; }

QList<Order> MatchingEngine::getOrderBook(QString ticker) {
    QList<Order> result;
    for(const auto& o : activeOrders) {
        if(o.assetTicker == ticker && o.status == OrderStatus::ACTIVE)
            result.append(o);
    }
    return result;
}

void MatchingEngine::processNewOrder(Order newOrder) {
    newOrder.status = OrderStatus::ACTIVE;
    Trader* t = traders.value(newOrder.traderId);
    if (!t) return;

    // Списываем/блокируем средства или акции (упрощенно - только проверка)
    // В реальной бирже здесь нужно заморозить средства (hold)

    QList<Order> counterOrders;
    for (const auto& o : activeOrders) {
        if (o.assetTicker == newOrder.assetTicker && o.type != newOrder.type && o.status == OrderStatus::ACTIVE) {
            counterOrders.append(o);
        }
    }

    std::sort(counterOrders.begin(), counterOrders.end(), [&](const Order& a, const Order& b){
        if (newOrder.type == OrderType::BUY) return a.price < b.price;
        else return a.price > b.price;
    });

    for (auto& co : counterOrders) {
        if (newOrder.quantity <= 0) break;

        bool priceMatch = (newOrder.type == OrderType::BUY && co.price <= newOrder.price) ||
                          (newOrder.type == OrderType::SELL && co.price >= newOrder.price);

        if (priceMatch) {
            int tradeQty = std::min(newOrder.quantity, co.quantity);
            double tradePrice = co.price;

            newOrder.quantity -= tradeQty;

            // Обновляем встречную заявку в основном списке
            for(int i=0; i<activeOrders.size(); ++i) {
                if(activeOrders[i].id == co.id) {
                    activeOrders[i].quantity -= tradeQty;
                    if(activeOrders[i].quantity == 0) activeOrders[i].status = OrderStatus::EXECUTED;
                    break;
                }
            }

            Trade trade;
            trade.id = tradeIdCounter++;
            trade.assetTicker = newOrder.assetTicker;
            trade.price = tradePrice;
            trade.quantity = tradeQty;
            trade.timestamp = QDateTime::currentDateTime();
            trade.buyOrderId = (newOrder.type == OrderType::BUY) ? newOrder.id : co.id;
            trade.sellOrderId = (newOrder.type == OrderType::SELL) ? newOrder.id : co.id;
            tradeHistory.append(trade);
            emit globalHistoryUpdated(); // Уведомляем вкладку истории

            // Обновляем цену актива
            assets[trade.assetTicker].currentPrice = tradePrice;
            assets[trade.assetTicker].priceHistory.append(tradePrice);

            // Обновляем балансы трейдеров
            Trader* buyer = traders.value(trade.buyOrderId);
            Trader* seller = traders.value(trade.sellOrderId);

            if(buyer && seller) {
                buyer->cashBalance -= tradePrice * tradeQty;
                PortfolioItem& itemB = buyer->portfolio[trade.assetTicker];
                itemB.assetTicker = trade.assetTicker;
                double currentCost = itemB.averageBuyPrice * itemB.quantity;
                itemB.quantity += tradeQty;
                itemB.averageBuyPrice = (currentCost + tradePrice * tradeQty) / itemB.quantity;

                seller->cashBalance += tradePrice * tradeQty;
                PortfolioItem& itemS = seller->portfolio[trade.assetTicker];
                itemS.quantity -= tradeQty;
                // Если < 0, значит ошибка логики, но пока оставим так

                emit portfolioUpdated(buyer->id);
                emit portfolioUpdated(seller->id);
            }
            emit tradeExecuted(trade.assetTicker, tradePrice, tradeQty);
        }
    }

    if (newOrder.quantity > 0) {
        activeOrders.append(newOrder);
    } else {
        newOrder.status = OrderStatus::EXECUTED;
    }

    emit marketUpdate(newOrder.assetTicker);
}
