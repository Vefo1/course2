#ifndef PLACEORDERDIALOG_H
#define PLACEORDERDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include "Enums.h"

class PlaceOrderDialog : public QDialog {
    Q_OBJECT

public:
    PlaceOrderDialog(QWidget* parent, QString ticker, OrderType type, double currentPrice);

    double getPrice() const;
    int getQuantity() const;

private:
    QDoubleSpinBox* priceSpin;
    QSpinBox* qtySpin;
};

#endif // PLACEORDERDIALOG_H
