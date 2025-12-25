#include "MainWindow.h"
#include "PlaceOrderDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QMenuBar>
#include <QInputDialog>
#include <QPushButton>
#include <QHeaderView>
#include <QMessageBox>
#include <QTimer>
#include <QRandomGenerator>
#include <QActionGroup>
#include <random>     // Для std::shuffle
#include <algorithm>  // Для std::shuffle

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    facade = new TradingServiceFacade();

    resize(1200, 700);
    setWindowTitle("Биржевой Терминал (Курсовой проект)");

    // Настройка таймера
    simulationSpeedFps = 5;
    simulationIntervalMs = 1000 / simulationSpeedFps;
    simulationTimer = new QTimer(this);
    connect(simulationTimer, &QTimer::timeout, this, &MainWindow::onSimulationTick);

    setupMenu();
    setupUI();

    // Инициализация данных
    facade->createAsset("AAPL", "Apple Inc.", 150.0);
    facade->createAsset("TSLA", "Tesla Inc.", 200.0);
    facade->createAsset("GAZP", "Gazprom", 175.5);

    facade->createTrader("Иван Иванов", 10000.0);
    facade->createTrader("Петр Петров", 50000.0);

    // Раздача акций
    facade->addSharesToTrader(1, "AAPL", 10, 145.0);
    facade->addSharesToTrader(1, "TSLA", 5, 190.0);
    facade->addSharesToTrader(2, "GAZP", 20, 170.0);

    refreshTraderCombo();
    refreshAssetTable();

    if (traderCombo->count() > 0) {
        traderCombo->setCurrentIndex(0);
        onCurrentTraderChanged(0);
    }

    // Подключение сигналов
    MatchingEngine* engine = MatchingEngine::getInstance();
    connect(engine, &MatchingEngine::marketUpdate, this, &MainWindow::updateMarketData);
    connect(engine, &MatchingEngine::portfolioUpdated, this, &MainWindow::updatePortfolio);
    connect(engine, &MatchingEngine::globalHistoryUpdated, this, &MainWindow::updateHistory);
}

MainWindow::~MainWindow() {}

void MainWindow::setupMenu() {
    QMenu* adminMenu = menuBar()->addMenu("Администрирование");

    adminActionTrader = new QAction("Добавить трейдера", this);
    connect(adminActionTrader, &QAction::triggered, this, &MainWindow::onAddTraderAction);
    adminMenu->addAction(adminActionTrader);

    adminActionAsset = new QAction("Добавить актив (Компанию)", this);
    connect(adminActionAsset, &QAction::triggered, this, &MainWindow::onAddAssetAction);
    adminMenu->addAction(adminActionAsset);

    QMenu* modeMenu = menuBar()->addMenu("Режим работы");
    QAction* manualMode = new QAction("Ручной режим", this);
    manualMode->setCheckable(true);
    manualMode->setChecked(true);
    QAction* simMode = new QAction("Симуляция рынка", this);
    simMode->setCheckable(true);
    QActionGroup* group = new QActionGroup(this);
    group->addAction(manualMode);
    group->addAction(simMode);

    connect(manualMode, &QAction::triggered, this, [this](){ onSimulationModeChanged(false); });
    connect(simMode, &QAction::triggered, this, [this](){ onSimulationModeChanged(true); });
    modeMenu->addAction(manualMode);
    modeMenu->addAction(simMode);
}

void MainWindow::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // Верхняя панель
    QHBoxLayout* topLayout = new QHBoxLayout();
    topLayout->setContentsMargins(10, 10, 10, 10);
    topLayout->addWidget(new QLabel("Текущий трейдер:"));
    traderCombo = new QComboBox();
    traderCombo->setMinimumWidth(200);
    connect(traderCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onCurrentTraderChanged);
    topLayout->addWidget(traderCombo);

    balanceLabel = new QLabel("Баланс: 0.00 $");
    balanceLabel->setStyleSheet("font-weight: bold; font-size: 14px; margin-left: 20px;");
    topLayout->addWidget(balanceLabel);
    topLayout->addStretch();

    mainLayout->addLayout(topLayout);

    tabs = new QTabWidget();
    setupMarketTab();
    setupPortfolioTab();
    setupHistoryTab();
    mainLayout->addWidget(tabs);
}

void MainWindow::setupMarketTab() {
    QWidget* marketWidget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(marketWidget);
    QSplitter* splitter = new QSplitter(Qt::Horizontal);

    // Левая колонка
    QWidget* leftContainer = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(0,0,0,0);

    leftLayout->addWidget(new QLabel("Список компаний"));
    assetTableWidget = new QTableWidget();
    assetTableWidget->setColumnCount(3);
    assetTableWidget->setHorizontalHeaderLabels({"Тикер", "Название", "Цена"});
    assetTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    assetTableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    assetTableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    assetTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    assetTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    assetTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(assetTableWidget, &QTableWidget::cellClicked, this, &MainWindow::onAssetTableClicked);
    leftLayout->addWidget(assetTableWidget);

    leftLayout->addWidget(new QLabel("Стакан заявок"));
    orderBookTable = new QTableWidget();
    orderBookTable->setColumnCount(3);
    orderBookTable->setHorizontalHeaderLabels({"Цена", "Кол-во", "Тип"});
    orderBookTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    leftLayout->addWidget(orderBookTable);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* btnBuy = new QPushButton("Купить");
    btnBuy->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;");
    QPushButton* btnSell = new QPushButton("Продать");
    btnSell->setStyleSheet("background-color: #F44336; color: white; font-weight: bold;");
    QPushButton* btnUndo = new QPushButton("Отмена посл.");
    connect(btnBuy, &QPushButton::clicked, this, &MainWindow::onBuyClicked);
    connect(btnSell, &QPushButton::clicked, this, &MainWindow::onSellClicked);
    connect(btnUndo, &QPushButton::clicked, this, &MainWindow::onUndoClicked);
    btnLayout->addWidget(btnBuy);
    btnLayout->addWidget(btnSell);
    btnLayout->addWidget(btnUndo);
    leftLayout->addLayout(btnLayout);

    splitter->addWidget(leftContainer);

    // Правая колонка
    priceChart = new QChart();
    series = new QLineSeries();
    priceChart->addSeries(series);
    priceChart->setTitle("Динамика цены");
    priceChart->legend()->hide();

    QValueAxis *axisX = new QValueAxis;
    axisX->setTitleText("Сделки (№)");
    axisX->setLabelFormat("%i");
    priceChart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis;
    axisY->setTitleText("Цена ($)");
    priceChart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chartView = new QChartView(priceChart);
    chartView->setRenderHint(QPainter::Antialiasing);
    splitter->addWidget(chartView);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    layout->addWidget(splitter);
    tabs->addTab(marketWidget, "Рынок");
}

void MainWindow::setupPortfolioTab() {
    portfolioTable = new QTableWidget();
    portfolioTable->setColumnCount(3);
    portfolioTable->setHorizontalHeaderLabels({"Тикер", "Количество", "Ср. цена покупки"});
    portfolioTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tabs->addTab(portfolioTable, "Портфель");
}

// --- ИЗМЕНЕННАЯ ФУНКЦИЯ НАСТРОЙКИ ИСТОРИИ (СОРТИРОВКА + ФИЛЬТР) ---
void MainWindow::setupHistoryTab() {
    QWidget* historyWidget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(historyWidget);

    // 1. Панель фильтров
    QHBoxLayout* filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel("Поиск:"));

    historySearchEdit = new QLineEdit();
    historySearchEdit->setPlaceholderText("Введите текст для фильтрации...");
    connect(historySearchEdit, &QLineEdit::textChanged, this, &MainWindow::onHistoryFilterChanged);
    filterLayout->addWidget(historySearchEdit);

    filterLayout->addWidget(new QLabel("Столбец:"));
    historyColumnCombo = new QComboBox();
    historyColumnCombo->addItems({"Все", "Время", "Тикер", "Цена", "Кол-во", "Сумма", "Покупатель", "Продавец"});
    connect(historyColumnCombo, &QComboBox::currentIndexChanged, this, &MainWindow::onHistoryFilterChanged);
    filterLayout->addWidget(historyColumnCombo);

    layout->addLayout(filterLayout);

    // 2. Таблица
    historyTable = new QTableWidget();
    historyTable->setColumnCount(7);
    historyTable->setHorizontalHeaderLabels({"Время", "Тикер", "Цена", "Кол-во", "Сумма", "Покупатель", "Продавец"});
    historyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // ВКЛЮЧАЕМ СОРТИРОВКУ (Клик по заголовку)
    historyTable->setSortingEnabled(true);

    layout->addWidget(historyTable);

    tabs->addTab(historyWidget, "История сделок");
}

// --- СЛОТЫ И ЛОГИКА ---

void MainWindow::onAddTraderAction() {
    bool ok;
    QString name = QInputDialog::getText(this, "Новый трейдер", "Имя:", QLineEdit::Normal, "", &ok);
    if (ok && !name.isEmpty()) {
        double balance = QInputDialog::getDouble(this, "Баланс", "Сумма:", 1000, 0, 1000000, 2, &ok);
        if (ok) {
            facade->createTrader(name, balance);
            refreshTraderCombo();
        }
    }
}

void MainWindow::onAddAssetAction() {
    bool ok;
    QString ticker = QInputDialog::getText(this, "Новый актив", "Тикер:", QLineEdit::Normal, "", &ok);
    if (ok && !ticker.isEmpty()) {
        QString name = QInputDialog::getText(this, "Название", "Компания:", QLineEdit::Normal, "", &ok);
        double price = QInputDialog::getDouble(this, "Цена", "IPO Цена:", 100.0, 0.1, 10000, 2, &ok);
        if (ok) {
            facade->createAsset(ticker, name, price);
            refreshAssetTable();
        }
    }
}

void MainWindow::refreshTraderCombo() {
    traderCombo->clear();
    auto traders = facade->getAllTraders();
    for(auto* t : traders) {
        traderCombo->addItem(t->name, t->id);
    }
}

void MainWindow::onCurrentTraderChanged(int index) {
    if (index < 0) return;
    currentTraderId = traderCombo->currentData().toInt();
    updatePortfolio(currentTraderId);
}

void MainWindow::refreshAssetTable() {
    assetTableWidget->setRowCount(0);
    auto assets = facade->getAllAssets();
    for(const auto& asset : assets) {
        int row = assetTableWidget->rowCount();
        assetTableWidget->insertRow(row);
        assetTableWidget->setItem(row, 0, new QTableWidgetItem(asset.ticker));
        assetTableWidget->setItem(row, 1, new QTableWidgetItem(asset.name));
        assetTableWidget->setItem(row, 2, new QTableWidgetItem(QString::number(asset.currentPrice, 'f', 2)));
    }
}

void MainWindow::onAssetTableClicked(int row, int column) {
    Q_UNUSED(column);
    QString ticker = assetTableWidget->item(row, 0)->text();
    currentAssetTicker = ticker;
    priceChart->setTitle("Динамика цены: " + ticker);
    updateMarketData(ticker);
}

void MainWindow::updateMarketData(QString ticker) {
    refreshAssetTable();
    if (ticker != currentAssetTicker) return;
    refreshOrderBook(ticker);
    refreshChart(ticker);
}

void MainWindow::refreshOrderBook(QString ticker) {
    auto orders = facade->getOrdersForAsset(ticker);
    orderBookTable->setRowCount(0);
    for(const auto& o : orders) {
        int row = orderBookTable->rowCount();
        orderBookTable->insertRow(row);

        // Используем Data(Qt::DisplayRole), чтобы сортировка (если включена) работала корректно
        QTableWidgetItem* priceItem = new QTableWidgetItem();
        priceItem->setData(Qt::DisplayRole, o.price);
        orderBookTable->setItem(row, 0, priceItem);

        QTableWidgetItem* qtyItem = new QTableWidgetItem();
        qtyItem->setData(Qt::DisplayRole, o.quantity);
        orderBookTable->setItem(row, 1, qtyItem);

        QString typeStr = (o.type == OrderType::BUY) ? "BUY" : "SELL";
        QTableWidgetItem* typeItem = new QTableWidgetItem(typeStr);
        if(o.type == OrderType::BUY) typeItem->setBackground(QColor(200, 255, 200));
        else typeItem->setBackground(QColor(255, 200, 200));
        orderBookTable->setItem(row, 2, typeItem);
    }
}

void MainWindow::refreshChart(QString ticker) {
    if (ticker.isEmpty()) return;
    Asset a = facade->getAssetData(ticker);
    series->clear();

    double minPrice = a.currentPrice;
    double maxPrice = a.currentPrice;

    for(int i = 0; i < a.priceHistory.size(); ++i) {
        double p = a.priceHistory[i];
        series->append(i, p);
        if (p < minPrice) minPrice = p;
        if (p > maxPrice) maxPrice = p;
    }

    auto axesX = priceChart->axes(Qt::Horizontal);
    if (!axesX.isEmpty()) {
        static_cast<QValueAxis*>(axesX.first())->setRange(0, a.priceHistory.size() > 10 ? a.priceHistory.size() : 10);
    }

    auto axesY = priceChart->axes(Qt::Vertical);
    if (!axesY.isEmpty()) {
        double margin = (maxPrice - minPrice) * 0.1;
        if (margin == 0) margin = maxPrice * 0.05;
        static_cast<QValueAxis*>(axesY.first())->setRange(minPrice - margin, maxPrice + margin);
    }
}

void MainWindow::onBuyClicked() {
    if(currentTraderId == -1) { QMessageBox::warning(this, "Ошибка", "Выберите трейдера!"); return; }
    if(currentAssetTicker.isEmpty()) { QMessageBox::warning(this, "Ошибка", "Выберите актив!"); return; }

    Asset a = facade->getAssetData(currentAssetTicker);
    PlaceOrderDialog dlg(this, currentAssetTicker, OrderType::BUY, a.currentPrice);

    if (dlg.exec() == QDialog::Accepted) {
        int id = facade->placeOrder(currentTraderId, currentAssetTicker, OrderType::BUY, dlg.getPrice(), dlg.getQuantity());
        if (id == -1) QMessageBox::warning(this, "Ошибка", "Недостаточно средств или встречная заявка!");
        else {
            myOrderHistory.push(id);
            QMessageBox::information(this, "Успех", "Заявка на покупку выставлена!");
        }
    }
}

void MainWindow::onSellClicked() {
    if(currentTraderId == -1) { QMessageBox::warning(this, "Ошибка", "Выберите трейдера!"); return; }
    if(currentAssetTicker.isEmpty()) { QMessageBox::warning(this, "Ошибка", "Выберите актив!"); return; }

    Asset a = facade->getAssetData(currentAssetTicker);
    PlaceOrderDialog dlg(this, currentAssetTicker, OrderType::SELL, a.currentPrice);

    if (dlg.exec() == QDialog::Accepted) {
        int id = facade->placeOrder(currentTraderId, currentAssetTicker, OrderType::SELL, dlg.getPrice(), dlg.getQuantity());
        if (id == -1) QMessageBox::warning(this, "Ошибка", "Недостаточно акций или встречная заявка!");
        else {
            myOrderHistory.push(id);
            QMessageBox::information(this, "Успех", "Заявка на продажу выставлена!");
        }
    }
}

void MainWindow::onUndoClicked() {
    if (myOrderHistory.isEmpty()) { QMessageBox::information(this, "Инфо", "Нет заявок для отмены."); return; }
    int lastId = myOrderHistory.pop();
    facade->cancelOrder(lastId);
    QMessageBox::information(this, "Отмена", "Запрос на отмену отправлен.");
}

void MainWindow::updatePortfolio(int traderId) {
    if (traderId != currentTraderId) return;
    double bal = facade->getTraderBalance(traderId);
    balanceLabel->setText(QString("Баланс: %1 $").arg(QString::number(bal, 'f', 2)));

    auto items = facade->getTraderPortfolio(traderId);
    portfolioTable->setRowCount(0);

    for(const auto& item : items) {
        if(item.quantity >= 0) {
            int row = portfolioTable->rowCount();
            portfolioTable->insertRow(row);
            portfolioTable->setItem(row, 0, new QTableWidgetItem(item.assetTicker));
            portfolioTable->setItem(row, 1, new QTableWidgetItem(QString::number(item.quantity)));
            portfolioTable->setItem(row, 2, new QTableWidgetItem(QString::number(item.averageBuyPrice, 'f', 2)));
        }
    }
}

// --- ОБНОВЛЕНИЕ ИСТОРИИ ---
void MainWindow::updateHistory() {
    auto history = MatchingEngine::getInstance()->getTradeHistory();

    // ВАЖНО: Отключаем сортировку перед заполнением, иначе будет тормозить и прыгать
    historyTable->setSortingEnabled(false);

    historyTable->setRowCount(0);
    for(int i = history.size() - 1; i >= 0; --i) {
        const auto& t = history[i];
        int row = historyTable->rowCount();
        historyTable->insertRow(row);

        // Используем setData для правильной сортировки чисел
        historyTable->setItem(row, 0, new QTableWidgetItem(t.timestamp.toString("HH:mm:ss")));
        historyTable->setItem(row, 1, new QTableWidgetItem(t.assetTicker));

        QTableWidgetItem* priceItem = new QTableWidgetItem();
        priceItem->setData(Qt::DisplayRole, t.price);
        historyTable->setItem(row, 2, priceItem);

        QTableWidgetItem* qtyItem = new QTableWidgetItem();
        qtyItem->setData(Qt::DisplayRole, t.quantity);
        historyTable->setItem(row, 3, qtyItem);

        QTableWidgetItem* sumItem = new QTableWidgetItem();
        sumItem->setData(Qt::DisplayRole, t.price * t.quantity);
        historyTable->setItem(row, 4, sumItem);

        historyTable->setItem(row, 5, new QTableWidgetItem(facade->getTraderName(t.buyerId)));
        historyTable->setItem(row, 6, new QTableWidgetItem(facade->getTraderName(t.sellerId)));
    }

    // Возвращаем сортировку
    historyTable->setSortingEnabled(true);

    // Применяем фильтр (если что-то введено)
    onHistoryFilterChanged();
}

// --- ЛОГИКА ФИЛЬТРАЦИИ ---
void MainWindow::onHistoryFilterChanged() {
    QString filterText = historySearchEdit->text();
    int columnIdx = historyColumnCombo->currentIndex() - 1; // -1, т.к. 0 это "Все"

    for(int i = 0; i < historyTable->rowCount(); ++i) {
        bool match = false;

        if (columnIdx < 0) {
            // Ищем во всех колонках
            for(int j = 0; j < historyTable->columnCount(); ++j) {
                if(historyTable->item(i, j)->text().contains(filterText, Qt::CaseInsensitive)) {
                    match = true;
                    break;
                }
            }
        } else {
            // Ищем в конкретной колонке
            if(historyTable->item(i, columnIdx)->text().contains(filterText, Qt::CaseInsensitive)) {
                match = true;
            }
        }

        historyTable->setRowHidden(i, !match);
    }
}

// --- ЛОГИКА СИМУЛЯЦИИ ---
void MainWindow::onSimulationModeChanged(bool enable) {
    if (enable) {
        adminActionTrader->setEnabled(false);
        adminActionAsset->setEnabled(false);
        simulationTimer->start(simulationIntervalMs);
    } else {
        simulationTimer->stop();
        adminActionTrader->setEnabled(true);
        adminActionAsset->setEnabled(true);
    }
}

void MainWindow::onSimulationTick() {
    auto traders = facade->getAllTraders();
    auto assets = facade->getAllAssets();

    if (traders.isEmpty() || assets.isEmpty()) return;

    const double ARBITRAGE_THRESHOLD = 0.03;
    const double VOLATILITY = 0.03;

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(assets.begin(), assets.end(), g);

    for (const auto& a : assets) {
        auto orders = facade->getOrdersForAsset(a.ticker);
        double bestBid = -1.0;
        double bestAsk = 1e9;
        bool hasBid = false;
        bool hasAsk = false;

        for (const auto& o : orders) {
            if (o.type == OrderType::BUY) {
                if (o.price > bestBid) { bestBid = o.price; hasBid = true; }
            } else {
                if (o.price < bestAsk) { bestAsk = o.price; hasAsk = true; }
            }
        }

        double marketPrice = a.currentPrice;
        int randTIdx = QRandomGenerator::global()->bounded(traders.size());
        Trader* t = traders[randTIdx];

        OrderType type;
        bool arbitrageAction = false;

        if (hasBid && bestBid > marketPrice * (1.0 + ARBITRAGE_THRESHOLD)) {
            type = OrderType::SELL;
            arbitrageAction = true;
        }
        else if (hasAsk && bestAsk < marketPrice * (1.0 - ARBITRAGE_THRESHOLD)) {
            type = OrderType::BUY;
            arbitrageAction = true;
        }
        else {
            if (!hasAsk) type = OrderType::SELL;
            else if (!hasBid) type = OrderType::BUY;
            else {
                int myQty = t->portfolio.value(a.ticker).quantity;
                if (myQty > 50) type = OrderType::SELL;
                else {
                    if (hasBid && bestBid > marketPrice) type = OrderType::SELL;
                    else if (hasAsk && bestAsk < marketPrice) type = OrderType::BUY;
                    else type = (QRandomGenerator::global()->bounded(2) == 0) ? OrderType::BUY : OrderType::SELL;
                }
            }
        }

        if (type == OrderType::BUY) {
            if (t->cashBalance < marketPrice * 5) {
                t->cashBalance += 5000.0;
                if (t->id == currentTraderId) updatePortfolio(t->id);
            }
        } else {
            if (!t->portfolio.contains(a.ticker) || t->portfolio[a.ticker].quantity < 1) {
                facade->addSharesToTrader(t->id, a.ticker, 10, marketPrice);
                if (t->id == currentTraderId) updatePortfolio(t->id);
            }
        }

        double price;
        if (arbitrageAction) {
            price = (type == OrderType::BUY) ? bestAsk : bestBid;
        } else {
            double noise = (QRandomGenerator::global()->bounded(201) - 100) / 100.0 * VOLATILITY;
            price = marketPrice * (1.0 + noise);

            if (QRandomGenerator::global()->bounded(100) < 30) {
                if (type == OrderType::BUY && hasAsk) price = bestAsk;
                else if (type == OrderType::SELL && hasBid) price = bestBid;
            }
        }

        if (price < 1.0) price = 1.0;
        int qty = QRandomGenerator::global()->bounded(1, 4);

        facade->placeOrder(t->id, a.ticker, type, price, qty);
    }
}
