#ifndef PORTFOLIOITEM_H
#define PORTFOLIOITEM_H
#include <QString>

struct PortfolioItem {
    QString assetTicker;
    int quantity = 0;
    double averageBuyPrice = 0.0;
};
#endif // PORTFOLIOITEM_H
