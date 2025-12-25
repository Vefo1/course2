#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QComboBox>
#include <QLabel>
#include <QTableWidget>
#include <QMessageBox>
#include <QStack>
#include <QTimer>
#include <QRandomGenerator>

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChart>

#include "TradingServiceFacade.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAddTraderAction();
    void onAddAssetAction();

    void onCurrentTraderChanged(int index);
    void onAssetTableClicked(int row, int column);
    void onBuyClicked();
    void onSellClicked();
    void onUndoClicked(); // Слот отмены

    void updateMarketData(QString ticker);
    void updatePortfolio(int traderId);
    void updateHistory();

    void onSimulationModeChanged(bool enable);
    void onSimulationTick();

    void onHistoryFilterChanged();

private:
    void setupUI();
    void setupMenu();
    void setupMarketTab();
    void setupPortfolioTab();
    void setupHistoryTab();

    void refreshTraderCombo();
    void refreshAssetTable();
    void refreshOrderBook(QString ticker);
    void refreshChart(QString ticker);

    TradingServiceFacade* facade;
    int currentTraderId = -1;
    QString currentAssetTicker;

    QStack<int> myOrderHistory; // Стек ID заявок

    QComboBox* traderCombo;
    QLabel* balanceLabel;

    QTimer* simulationTimer;
    int simulationSpeedFps = 5;
    int simulationIntervalMs;

    const double ARBITRAGE_MARGIN = 0.05;
    const double SPREAD_THRESHOLD = 0.05;

    QAction* adminActionTrader;
    QAction* adminActionAsset;

    QTabWidget* tabs;

    // Вкладка Рынок
    QTableWidget* assetTableWidget;
    QTableWidget* orderBookTable;

    // График
    QChart* priceChart;
    QChartView* chartView;
    QLineSeries* series;

    QTableWidget* portfolioTable;
    QTableWidget* historyTable;
    QLineEdit* historySearchEdit;
    QComboBox* historyColumnCombo;
};
#endif // MAINWINDOW_H
