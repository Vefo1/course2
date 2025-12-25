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

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    facade = new TradingServiceFacade();

    resize(1200, 700);
    setWindowTitle("Биржевой Терминал (Курсовой проект)");

    setupMenu();
    setupUI();

    // Тестовые данные
    facade->createAsset("AAPL", "Apple Inc.", 150.0);
    facade->createAsset("TSLA", "Tesla Inc.", 200.0);
    facade->createAsset("GAZP", "Gazprom", 175.5);

    facade->createTrader("Иван Иванов", 10000.0);
    facade->createTrader("Петр Петров", 50000.0);

    refreshTraderCombo();
    refreshAssetTable();

    MatchingEngine* engine = MatchingEngine::getInstance();
    connect(engine, &MatchingEngine::marketUpdate, this, &MainWindow::updateMarketData);
    connect(engine, &MatchingEngine::portfolioUpdated, this, &MainWindow::updatePortfolio);
    connect(engine, &MatchingEngine::globalHistoryUpdated, this, &MainWindow::updateHistory);
}

MainWindow::~MainWindow() {}

void MainWindow::setupMenu() {
    QMenu* adminMenu = menuBar()->addMenu("Администрирование");

    QAction* addTraderAct = new QAction("Добавить трейдера", this);
    connect(addTraderAct, &QAction::triggered, this, &MainWindow::onAddTraderAction);
    adminMenu->addAction(addTraderAct);

    QAction* addAssetAct = new QAction("Добавить актив (Компанию)", this);
    connect(addAssetAct, &QAction::triggered, this, &MainWindow::onAddAssetAction);
    adminMenu->addAction(addAssetAct);
}

void MainWindow::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // --- Верхняя панель ---
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

    // --- Табы ---
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

    // 1. Таблица активов
    assetTableWidget = new QTableWidget();
    assetTableWidget->setColumnCount(3);
    assetTableWidget->setHorizontalHeaderLabels({"Тикер", "Название", "Цена"});
    assetTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    assetTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    assetTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    assetTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(assetTableWidget, &QTableWidget::cellClicked, this, &MainWindow::onAssetTableClicked);

    splitter->addWidget(assetTableWidget);

    // 2. График
    priceChart = new QChart();
    series = new QLineSeries();
    priceChart->addSeries(series);
    priceChart->createDefaultAxes();
    priceChart->setTitle("Выберите актив");
    priceChart->legend()->hide();

    chartView = new QChartView(priceChart);
    chartView->setRenderHint(QPainter::Antialiasing);
    splitter->addWidget(chartView);

    // 3. Стакан
    QWidget* orderPanel = new QWidget();
    QVBoxLayout* orderLayout = new QVBoxLayout(orderPanel);

    selectedAssetLabel = new QLabel("Выберите актив");
    selectedAssetLabel->setAlignment(Qt::AlignCenter);
    selectedAssetLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #555;");
    orderLayout->addWidget(selectedAssetLabel);

    orderLayout->addWidget(new QLabel("Стакан заявок"));
    orderBookTable = new QTableWidget();
    orderBookTable->setColumnCount(3);
    orderBookTable->setHorizontalHeaderLabels({"Цена", "Кол-во", "Тип"});
    orderBookTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    orderLayout->addWidget(orderBookTable);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* btnBuy = new QPushButton("Купить");
    btnBuy->setStyleSheet("background-color: #4CAF50; color: white;");
    QPushButton* btnSell = new QPushButton("Продать");
    btnSell->setStyleSheet("background-color: #F44336; color: white;");

    connect(btnBuy, &QPushButton::clicked, this, &MainWindow::onBuyClicked);
    connect(btnSell, &QPushButton::clicked, this, &MainWindow::onSellClicked);

    btnLayout->addWidget(btnBuy);
    btnLayout->addWidget(btnSell);
    orderLayout->addLayout(btnLayout);

    splitter->addWidget(orderPanel);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 1);

    layout->addWidget(splitter);
    tabs->addTab(marketWidget, "Рынок");
}

void MainWindow::setupPortfolioTab() {
    portfolioTable = new QTableWidget();
    portfolioTable->setColumnCount(3);
    portfolioTable->setHorizontalHeaderLabels({"Тикер", "Количество", "Средняя цена"});
    portfolioTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tabs->addTab(portfolioTable, "Портфель");
}

void MainWindow::setupHistoryTab() {
    historyTable = new QTableWidget();
    historyTable->setColumnCount(5);
    historyTable->setHorizontalHeaderLabels({"Время", "Тикер", "Цена", "Кол-во", "Сумма"});
    historyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tabs->addTab(historyTable, "История сделок");
}

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

    selectedAssetLabel->setText("Выбран актив: " + ticker);
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
        orderBookTable->setItem(row, 0, new QTableWidgetItem(QString::number(o.price, 'f', 2)));
        orderBookTable->setItem(row, 1, new QTableWidgetItem(QString::number(o.quantity)));
        QString typeStr = (o.type == OrderType::BUY) ? "BUY" : "SELL";
        QTableWidgetItem* typeItem = new QTableWidgetItem(typeStr);
        if(o.type == OrderType::BUY) typeItem->setBackground(QColor(200, 255, 200));
        else typeItem->setBackground(QColor(255, 200, 200));
        orderBookTable->setItem(row, 2, typeItem);
    }
}

void MainWindow::refreshChart(QString ticker) {
    Asset a = facade->getAssetData(ticker);
    series->clear();
    int x = 0;
    for(double p : a.priceHistory) {
        series->append(x++, p);
    }
    priceChart->createDefaultAxes();
}

void MainWindow::onBuyClicked() {
    if(currentTraderId == -1) {
        QMessageBox::warning(this, "Ошибка", "Выберите трейдера сверху!");
        return;
    }
    if(currentAssetTicker.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Выберите актив слева!");
        return;
    }
    Asset a = facade->getAssetData(currentAssetTicker);
    PlaceOrderDialog dlg(this, currentAssetTicker, OrderType::BUY, a.currentPrice);
    if (dlg.exec() == QDialog::Accepted) {
        facade->placeOrder(currentTraderId, currentAssetTicker, OrderType::BUY, dlg.getPrice(), dlg.getQuantity());
        QMessageBox::information(this, "Успех", "Заявка на покупку выставлена!");
    }
}

void MainWindow::onSellClicked() {
    if(currentTraderId == -1 || currentAssetTicker.isEmpty()) return;
    Asset a = facade->getAssetData(currentAssetTicker);
    PlaceOrderDialog dlg(this, currentAssetTicker, OrderType::SELL, a.currentPrice);
    if (dlg.exec() == QDialog::Accepted) {
        facade->placeOrder(currentTraderId, currentAssetTicker, OrderType::SELL, dlg.getPrice(), dlg.getQuantity());
        QMessageBox::information(this, "Успех", "Заявка на продажу выставлена!");
    }
}

void MainWindow::updatePortfolio(int traderId) {
    if (traderId != currentTraderId) return;

    double bal = facade->getTraderBalance(traderId);
    balanceLabel->setText(QString("Баланс: %1 $").arg(QString::number(bal, 'f', 2)));

    auto items = facade->getTraderPortfolio(traderId);
    portfolioTable->setRowCount(0);

    for(const auto& item : items) {
        if(item.quantity > 0) {
            int row = portfolioTable->rowCount();
            portfolioTable->insertRow(row);
            portfolioTable->setItem(row, 0, new QTableWidgetItem(item.assetTicker));
            portfolioTable->setItem(row, 1, new QTableWidgetItem(QString::number(item.quantity)));
            portfolioTable->setItem(row, 2, new QTableWidgetItem(QString::number(item.averageBuyPrice, 'f', 2)));
        }
    }
}

void MainWindow::updateHistory() {
    auto history = MatchingEngine::getInstance()->getTradeHistory();
    historyTable->setRowCount(0);
    for(int i = history.size() - 1; i >= 0; --i) {
        const auto& t = history[i];
        int row = historyTable->rowCount();
        historyTable->insertRow(row);
        historyTable->setItem(row, 0, new QTableWidgetItem(t.timestamp.toString("HH:mm:ss")));
        historyTable->setItem(row, 1, new QTableWidgetItem(t.assetTicker));
        historyTable->setItem(row, 2, new QTableWidgetItem(QString::number(t.price, 'f', 2)));
        historyTable->setItem(row, 3, new QTableWidgetItem(QString::number(t.quantity)));
        historyTable->setItem(row, 4, new QTableWidgetItem(QString::number(t.price * t.quantity, 'f', 2)));
    }
}
