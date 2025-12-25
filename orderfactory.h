#ifndef ORDERFACTORY_H
#define ORDERFACTORY_H
#include "Order.h"

class OrderFactory {
public:
    static Order createOrder(int traderId, QString ticker, OrderType type, double price, int quantity);
};
#endif // ORDERFACTORY_H
