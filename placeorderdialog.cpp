#include "PlaceOrderDialog.h"

PlaceOrderDialog::PlaceOrderDialog(QWidget* parent, QString ticker, OrderType type, double currentPrice)
    : QDialog(parent)
{
    QString typeStr = (type == OrderType::BUY) ? "Покупка" : "Продажа";
    setWindowTitle(QString("%1: %2").arg(typeStr, ticker));
    setModal(true);
    resize(300, 200);

    QVBoxLayout* layout = new QVBoxLayout(this);

    // Информация
    QLabel* infoLabel = new QLabel(QString("Актив: %1\nТекущая рыночная цена: %2")
                                       .arg(ticker).arg(currentPrice));
    infoLabel->setStyleSheet("font-weight: bold; margin-bottom: 10px;");
    layout->addWidget(infoLabel);

    // Ввод цены
    layout->addWidget(new QLabel("Цена заявки:"));
    priceSpin = new QDoubleSpinBox();
    priceSpin->setRange(0.01, 1000000.0);
    priceSpin->setDecimals(2);
    priceSpin->setValue(currentPrice); // По умолчанию ставим текущую цену
    priceSpin->setSuffix(" $");
    layout->addWidget(priceSpin);

    // Ввод количества
    layout->addWidget(new QLabel("Количество лотов:"));
    qtySpin = new QSpinBox();
    qtySpin->setRange(1, 1000000);
    qtySpin->setValue(1);
    layout->addWidget(qtySpin);

    // Кнопки
    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* btnOk = new QPushButton(typeStr); // "Купить" или "Продать"
    QPushButton* btnCancel = new QPushButton("Отмена");

    // Стилизация кнопки действия
    if (type == OrderType::BUY) {
        btnOk->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;");
    } else {
        btnOk->setStyleSheet("background-color: #F44336; color: white; font-weight: bold;");
    }

    connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    btnLayout->addWidget(btnOk);
    btnLayout->addWidget(btnCancel);
    layout->addLayout(btnLayout);
}

double PlaceOrderDialog::getPrice() const {
    return priceSpin->value();
}

int PlaceOrderDialog::getQuantity() const {
    return qtySpin->value();
}
