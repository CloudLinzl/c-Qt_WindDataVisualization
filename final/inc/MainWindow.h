#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QStatusBar>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QTimer>
#include "loader.hpp"
#include "PPIWidget.h"
#include "ProfileWidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openWindSpeedFile();
    void openGimbalFile();
    void loadData();
    void exportData();
    void exportImage();
    void updateSNRThreshold(double value);
    void updateDistanceRange();
    void updateTimeIndex(int value);
    void performVisualizationUpdate();
    void setResolution900x600();
    void setResolution1280x800();
    void setResolution1400x1000();
    void setResolution1600x1200();
    void setResolution1920x1080();
    void updateDisplayMode(int index);
    void updateWindowSize(int size);

private:
    void createMenus();
    void createControlPanel();
    void createVisualizationArea();
    void createStatusBar();
    void updateVisualization();
    void updateStatusBar();

    // 数据成员
    DATA_TABLE* dataTable;
    QString windSpeedFilePath;
    QString gimbalFilePath;
    bool dataLoaded;

    // UI组件
    QWidget* centralWidget;
    QVBoxLayout* mainLayout;

    // 菜单
    QMenu* fileMenu;
    QMenu* exportMenu;
    QMenu* viewMenu;
    QAction* openWindSpeedAction;
    QAction* openGimbalAction;
    QAction* loadDataAction;
    QAction* exportDataAction;
    QAction* exportImageAction;
    QAction* exitAction;

    // 控制面板
    QGroupBox* controlPanel;
    QDoubleSpinBox* snrThresholdSpinBox;
    QSpinBox* minDistanceSpinBox;
    QSpinBox* maxDistanceSpinBox;
    QComboBox* displayModeCombo;
    QSpinBox* windowSizeSpinBox;
    QSlider* timeSlider;
    QLabel* timeLabel;
    QPushButton* loadButton;

    // 可视化区域
    QWidget* visualizationArea;
    PPIWidget* ppiWidget;
    ProfileWidget* profileWidget;

    // 状态栏标签
    QLabel* fileStatusLabel;
    QLabel* scanModeLabel;
    QLabel* dataPointsLabel;

    // 当前参数
    double currentSNRThreshold;
    int currentMinDistance;
    int currentMaxDistance;
    int currentTimeIndex;

    // 更新节流
    QTimer* updateThrottleTimer;
    int pendingTimeIndex;
};

#endif // MAINWINDOW_H
