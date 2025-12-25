#include "OrderFactory.h"

Order OrderFactory::createOrder(int traderId, QString ticker, OrderType type, double price, int quantity) {
    static int idGen = 100;
    Order o;
    o.id = idGen++;
    o.traderId = traderId;
    o.assetTicker = ticker;
    o.type = type;
    o.price = price;
    o.quantity = quantity;
    o.status = OrderStatus::ACTIVE;
    o.timestamp = QDateTime::currentDateTime();
    return o;
}
