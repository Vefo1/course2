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

void MatchingEngine::addSharesToTrader(int traderId, QString ticker, int quantity, double avgPrice) {
    Trader* t = traders.value(traderId);
    if (!t) return;

    PortfolioItem& item = t->portfolio[ticker];
    item.assetTicker = ticker;
    double totalCost = item.averageBuyPrice * item.quantity + avgPrice * quantity;
    item.quantity += quantity;
    if (item.quantity > 0)
        item.averageBuyPrice = totalCost / item.quantity;

    emit portfolioUpdated(traderId);
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

bool MatchingEngine::isIPOAvailable(QString ticker) {
    // Пробегаем по всем трейдерам и ищем, есть ли у кого-то этот актив
    for(auto* t : traders) {
        if(t->portfolio.contains(ticker)) {
            if(t->portfolio[ticker].quantity > 0) {
                return false; // Кто-то уже владеет акциями, IPO закончилось, торгуем на вторичке
            }
        }
    }
    return true; // Ни у кого нет акций -> это IPO
}

// ОСНОВНАЯ ЛОГИКА ТОРГОВ (С ПРОВЕРКАМИ)
bool MatchingEngine::processNewOrder(Order newOrder) {
    Trader* t = traders.value(newOrder.traderId);
    if (!t) return false;

    // --- ПРОВЕРКА 1: Проверка средств/акций ---
    if (newOrder.type == OrderType::BUY) {
        double requiredCash = newOrder.price * newOrder.quantity;
        if (t->cashBalance < requiredCash) {
            return false; // Недостаточно денег
        }
    } else { // SELL
        int availableQty = t->portfolio.value(newOrder.assetTicker).quantity;
        if (availableQty < newOrder.quantity) {
            return false; // Недостаточно акций
        }
    }

    // --- ПРОВЕРКА 2: Запрет встречных заявок (чтобы не торговать сам с собой) ---
    for (const auto& o : activeOrders) {
        if (o.traderId == newOrder.traderId &&
            o.assetTicker == newOrder.assetTicker &&
            o.status == OrderStatus::ACTIVE)
        {
            // Если у трейдера уже есть заявка противоположного типа
            if (o.type != newOrder.type) {
                return false; // Нельзя ставить BUY, если уже стоит SELL (и наоборот)
            }
        }
    }

    // Если это ПОКУПКА и никто не владеет акциями (IPO)
    if (newOrder.type == OrderType::BUY && isIPOAvailable(newOrder.assetTicker)) {
        Asset& asset = assets[newOrder.assetTicker];

        // Проверяем цену (не ниже стартовой/текущей)
        if (newOrder.price >= asset.currentPrice) {
            // Исполняем сразу "от лица компании"

            Trade trade;
            trade.id = tradeIdCounter++;
            trade.assetTicker = newOrder.assetTicker;
            trade.price = newOrder.price; // По цене заявки
            trade.quantity = newOrder.quantity;
            trade.timestamp = QDateTime::currentDateTime();
            trade.buyOrderId = newOrder.id;
            trade.sellOrderId = -1; // -1 означает "Система/Эмитент"

            trade.buyerId = newOrder.traderId;
            trade.sellerId = -1;    // Продавец - Система

            tradeHistory.append(trade);
            emit globalHistoryUpdated();

            // Обновляем цену
            asset.currentPrice = trade.price;
            asset.priceHistory.append(trade.price);

            // Списываем деньги у покупателя и даем акции
            t->cashBalance -= trade.price * trade.quantity;
            PortfolioItem& item = t->portfolio[newOrder.assetTicker];
            item.assetTicker = newOrder.assetTicker;
            // Расчет средней
            double totalCost = item.averageBuyPrice * item.quantity + trade.price * trade.quantity;
            item.quantity += trade.quantity;
            item.averageBuyPrice = totalCost / item.quantity;

            emit portfolioUpdated(t->id);
            emit tradeExecuted(trade.assetTicker, trade.price, trade.quantity);
            emit marketUpdate(newOrder.assetTicker);

            return true; // Заявка полностью исполнена
        }
    }

    // Если проверки пройдены, начинаем сведение
    newOrder.status = OrderStatus::ACTIVE;

    QList<Order> counterOrders;
    for (const auto& o : activeOrders) {
        if (o.assetTicker == newOrder.assetTicker &&
            o.type != newOrder.type &&
            o.status == OrderStatus::ACTIVE)
        {
            // Важно: Пропускаем свои же заявки (на случай если проверка выше пропущена)
            if (o.traderId == newOrder.traderId) continue;
            counterOrders.append(o);
        }
    }

    // Сортировка (Покупатели хотят дешевле, Продавцы хотят дороже)
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

            // Обновляем встречную заявку
            for(int i=0; i<activeOrders.size(); ++i) {
                if(activeOrders[i].id == co.id) {
                    activeOrders[i].quantity -= tradeQty;
                    if(activeOrders[i].quantity == 0) activeOrders[i].status = OrderStatus::EXECUTED;
                    break;
                }
            }

            // Создаем сделку
            Trade trade;
            trade.id = tradeIdCounter++;
            trade.assetTicker = newOrder.assetTicker;
            trade.price = tradePrice;
            trade.quantity = tradeQty;
            trade.timestamp = QDateTime::currentDateTime();
            trade.buyOrderId = (newOrder.type == OrderType::BUY) ? newOrder.id : co.id;
            trade.sellOrderId = (newOrder.type == OrderType::SELL) ? newOrder.id : co.id;
            trade.buyerId = (newOrder.type == OrderType::BUY) ? newOrder.traderId : co.traderId;
            trade.sellerId = (newOrder.type == OrderType::SELL) ? newOrder.traderId : co.traderId;

            tradeHistory.append(trade);
            emit globalHistoryUpdated();

            assets[trade.assetTicker].currentPrice = tradePrice;
            assets[trade.assetTicker].priceHistory.append(tradePrice);

            // Обновляем портфели участников
            Trader* buyer = traders.value(trade.buyerId);
            Trader* seller = traders.value(trade.sellerId);

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

                emit portfolioUpdated(buyer->id);
                emit portfolioUpdated(seller->id);
            }
            emit tradeExecuted(trade.assetTicker, tradePrice, tradeQty);
        }
    }

    // Если заявка не исполнилась полностью — добавляем остаток в стакан
    if (newOrder.quantity > 0) {
        activeOrders.append(newOrder);
    } else {
        newOrder.status = OrderStatus::EXECUTED;
    }

    emit marketUpdate(newOrder.assetTicker);
    return true; // Успех
}

void MatchingEngine::cancelOrder(int orderId) {
    for(int i = 0; i < activeOrders.size(); ++i) {
        if(activeOrders[i].id == orderId) {
            activeOrders[i].status = OrderStatus::CANCELED;
            QString ticker = activeOrders[i].assetTicker;
            activeOrders.removeAt(i);
            emit marketUpdate(ticker);
            return;
        }
    }
}
