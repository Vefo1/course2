#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QComboBox>
#include <QLabel>
#include <QTableWidget>
#include <QMessageBox>

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

    void updateMarketData(QString ticker);
    void updatePortfolio(int traderId);
    void updateHistory();

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

    QComboBox* traderCombo;
    QLabel* balanceLabel;

    QTabWidget* tabs;

    // Вкладка Рынок
    QTableWidget* assetTableWidget;
    QTableWidget* orderBookTable;
    QLabel* selectedAssetLabel;

    QChart* priceChart;
    QChartView* chartView;
    QLineSeries* series;

    QTableWidget* portfolioTable;
    QTableWidget* historyTable;
};
#endif // MAINWINDOW_H
