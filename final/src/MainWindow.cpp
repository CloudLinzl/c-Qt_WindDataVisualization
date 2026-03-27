#include "MainWindow.h"
#include <QSplitter>
#include <QDateTime>
#include <QDebug>
#include <QPainter>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), dataTable(nullptr), dataLoaded(false),
      currentSNRThreshold(-5.0), currentMinDistance(0),
      currentMaxDistance(2000), currentTimeIndex(0), pendingTimeIndex(-1)
{
    setWindowTitle("测风数据分析与可视化平台");
    resize(1400, 1000);

    // 创建更新节流定时器（50ms延迟，减少卡顿）
    updateThrottleTimer = new QTimer(this);
    updateThrottleTimer->setSingleShot(true);
    updateThrottleTimer->setInterval(50);
    connect(updateThrottleTimer, &QTimer::timeout, this, &MainWindow::performVisualizationUpdate);

    // 创建中央部件
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    mainLayout = new QVBoxLayout(centralWidget);

    // 创建各个部分
    createMenus();
    createControlPanel();
    createVisualizationArea();
    createStatusBar();

    // 初始状态
    updateStatusBar();
}

MainWindow::~MainWindow()
{
    if (dataTable != nullptr) {
        delete dataTable;
    }
}

void MainWindow::createMenus()
{
    // 文件菜单
    fileMenu = menuBar()->addMenu("文件(&F)");

    openWindSpeedAction = new QAction("打开风速数据...", this);
    connect(openWindSpeedAction, &QAction::triggered, this, &MainWindow::openWindSpeedFile);
    fileMenu->addAction(openWindSpeedAction);

    openGimbalAction = new QAction("打开云台角度数据...", this);
    connect(openGimbalAction, &QAction::triggered, this, &MainWindow::openGimbalFile);
    fileMenu->addAction(openGimbalAction);

    loadDataAction = new QAction("加载数据", this);
    loadDataAction->setShortcut(QKeySequence("Ctrl+L"));
    connect(loadDataAction, &QAction::triggered, this, &MainWindow::loadData);
    fileMenu->addAction(loadDataAction);

    fileMenu->addSeparator();

    exitAction = new QAction("退出(&X)", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(exitAction);

    // 导出菜单
    exportMenu = menuBar()->addMenu("导出(&E)");

    exportDataAction = new QAction("导出数据...", this);
    connect(exportDataAction, &QAction::triggered, this, &MainWindow::exportData);
    exportMenu->addAction(exportDataAction);

    exportImageAction = new QAction("导出图像...", this);
    connect(exportImageAction, &QAction::triggered, this, &MainWindow::exportImage);
    exportMenu->addAction(exportImageAction);

    // 视图菜单
    viewMenu = menuBar()->addMenu("视图(&V)");

    QAction* res900x600Action = new QAction("900 x 600 (小屏幕)", this);
    connect(res900x600Action, &QAction::triggered, this, &MainWindow::setResolution900x600);
    viewMenu->addAction(res900x600Action);

    QAction* res1280x800Action = new QAction("1280 x 800", this);
    connect(res1280x800Action, &QAction::triggered, this, &MainWindow::setResolution1280x800);
    viewMenu->addAction(res1280x800Action);

    QAction* res1400x1000Action = new QAction("1400 x 1000 (默认)", this);
    connect(res1400x1000Action, &QAction::triggered, this, &MainWindow::setResolution1400x1000);
    viewMenu->addAction(res1400x1000Action);

    QAction* res1600x1200Action = new QAction("1600 x 1200", this);
    connect(res1600x1200Action, &QAction::triggered, this, &MainWindow::setResolution1600x1200);
    viewMenu->addAction(res1600x1200Action);

    QAction* res1920x1080Action = new QAction("1920 x 1080 (全高清)", this);
    connect(res1920x1080Action, &QAction::triggered, this, &MainWindow::setResolution1920x1080);
    viewMenu->addAction(res1920x1080Action);
}

void MainWindow::createControlPanel()
{
    controlPanel = new QGroupBox("控制面板", centralWidget);
    QHBoxLayout* controlLayout = new QHBoxLayout(controlPanel);

    // SNR阈值控制
    QLabel* snrLabel = new QLabel("SNR阈值(dB):", controlPanel);
    snrThresholdSpinBox = new QDoubleSpinBox(controlPanel);
    snrThresholdSpinBox->setRange(-30.0, 30.0);
    snrThresholdSpinBox->setValue(-5.0);
    snrThresholdSpinBox->setSingleStep(0.5);
    connect(snrThresholdSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::updateSNRThreshold);

    // 距离范围控制
    QLabel* minDistLabel = new QLabel("最小距离(m):", controlPanel);
    minDistanceSpinBox = new QSpinBox(controlPanel);
    minDistanceSpinBox->setRange(0, 5000);
    minDistanceSpinBox->setValue(0);
    minDistanceSpinBox->setSingleStep(100);
    connect(minDistanceSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::updateDistanceRange);

    QLabel* maxDistLabel = new QLabel("最大距离(m):", controlPanel);
    maxDistanceSpinBox = new QSpinBox(controlPanel);
    maxDistanceSpinBox->setRange(100, 5000);
    maxDistanceSpinBox->setValue(2000);
    maxDistanceSpinBox->setSingleStep(100);
    connect(maxDistanceSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::updateDistanceRange);

    // 时间滑块
    QLabel* timeSliderLabel = new QLabel("时间:", controlPanel);
    timeSlider = new QSlider(Qt::Horizontal, controlPanel);
    timeSlider->setMinimum(0);
    timeSlider->setMaximum(0);
    timeSlider->setValue(0);
    connect(timeSlider, &QSlider::valueChanged, this, &MainWindow::updateTimeIndex);

    timeLabel = new QLabel("0/0", controlPanel);

    // 显示模式
    QLabel* modeLabel = new QLabel("显示模式:", controlPanel);
    displayModeCombo = new QComboBox(controlPanel);
    displayModeCombo->addItem("风速绝对值");
    displayModeCombo->addItem("湍流强度");
    connect(displayModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::updateDisplayMode);

    // TI窗口大小
    QLabel* winSizeLabel = new QLabel("TI窗口:", controlPanel);
    windowSizeSpinBox = new QSpinBox(controlPanel);
    windowSizeSpinBox->setRange(3, 21);
    windowSizeSpinBox->setSingleStep(2);
    windowSizeSpinBox->setValue(5);
    windowSizeSpinBox->setToolTip("湍流强度计算的滑动窗口大小（距离门数）");
    connect(windowSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::updateWindowSize);

    // 加载按钮
    loadButton = new QPushButton("加载数据", controlPanel);
    connect(loadButton, &QPushButton::clicked, this, &MainWindow::loadData);

    // 布局
    controlLayout->addWidget(snrLabel);
    controlLayout->addWidget(snrThresholdSpinBox);
    controlLayout->addSpacing(20);
    controlLayout->addWidget(minDistLabel);
    controlLayout->addWidget(minDistanceSpinBox);
    controlLayout->addSpacing(10);
    controlLayout->addWidget(maxDistLabel);
    controlLayout->addWidget(maxDistanceSpinBox);
    controlLayout->addSpacing(20);
    controlLayout->addWidget(modeLabel);
    controlLayout->addWidget(displayModeCombo);
    controlLayout->addSpacing(10);
    controlLayout->addWidget(winSizeLabel);
    controlLayout->addWidget(windowSizeSpinBox);
    controlLayout->addSpacing(20);
    controlLayout->addWidget(timeSliderLabel);
    controlLayout->addWidget(timeSlider, 1);
    controlLayout->addWidget(timeLabel);
    controlLayout->addSpacing(20);
    controlLayout->addWidget(loadButton);

    mainLayout->addWidget(controlPanel);
}

void MainWindow::createVisualizationArea()
{
    visualizationArea = new QWidget(centralWidget);
    QVBoxLayout* visLayout = new QVBoxLayout(visualizationArea);

    // PPI扫描热力图
    ppiWidget = new PPIWidget(visualizationArea);

    // 距离-风速/SNR折线图
    profileWidget = new ProfileWidget(visualizationArea);

    visLayout->addWidget(ppiWidget, 1);
    visLayout->addWidget(profileWidget, 1);

    mainLayout->addWidget(visualizationArea, 1);
}

void MainWindow::createStatusBar()
{
    fileStatusLabel = new QLabel("未加载数据", this);
    scanModeLabel = new QLabel("扫描模式: PPI", this);
    dataPointsLabel = new QLabel("数据点: 0", this);

    statusBar()->addWidget(fileStatusLabel, 1);
    statusBar()->addWidget(scanModeLabel);
    statusBar()->addWidget(dataPointsLabel);
}

void MainWindow::openWindSpeedFile()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "打开风速数据文件",
        "",
        "CSV Files (*.csv);;All Files (*)"
    );

    if (!fileName.isEmpty()) {
        windSpeedFilePath = fileName;
        updateStatusBar();
    }
}

void MainWindow::openGimbalFile()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "打开云台角度数据文件",
        "",
        "CSV Files (*.csv);;All Files (*)"
    );

    if (!fileName.isEmpty()) {
        gimbalFilePath = fileName;
        updateStatusBar();
    }
}

void MainWindow::loadData()
{
    if (windSpeedFilePath.isEmpty() || gimbalFilePath.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先选择风速数据和云台角度数据文件！");
        return;
    }

    try {
        // 删除旧数据
        if (dataTable != nullptr) {
            delete dataTable;
        }

        // 加载新数据
        dataTable = new DATA_TABLE(
            windSpeedFilePath.toStdString(),
            gimbalFilePath.toStdString()
        );

        // 预处理数据
        dataTable->pre_process(3, 5, currentSNRThreshold);

        dataLoaded = true;

        // 调试信息
        qDebug() << "数据加载完成:";
        qDebug() << "  时间点数量:" << dataTable->time.size();
        qDebug() << "  距离门数量:" << dataTable->distance.size();
        qDebug() << "  当前时间索引:" << currentTimeIndex;

        // 更新时间滑块
        if (dataTable->time.size() > 0) {
            timeSlider->setMaximum(dataTable->time.size() - 1);
            currentTimeIndex = 0;
            pendingTimeIndex = 0;
            timeSlider->setValue(0);
        }

        // 更新距离范围
        if (dataTable->distance.size() > 0) {
            int maxDist = dataTable->distance.back();
            maxDistanceSpinBox->setMaximum(maxDist);
            maxDistanceSpinBox->setValue(maxDist);
            currentMaxDistance = maxDist;
        }

        // 立即更新可视化（不使用节流机制）
        updateVisualization();
        updateStatusBar();

        QMessageBox::information(this, "成功", "数据加载成功！");

    } catch (const std::exception& e) {
        QMessageBox::critical(this, "错误", QString("数据加载失败: %1").arg(e.what()));
    } catch (const char* msg) {
        QMessageBox::critical(this, "错误", QString("数据加载失败: %1").arg(msg));
    }
}

void MainWindow::exportData()
{
    if (!dataLoaded || dataTable == nullptr) {
        QMessageBox::warning(this, "警告", "没有可导出的数据！");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(
        this,
        "导出数据",
        "processed_data.csv",
        "CSV Files (*.csv)"
    );

    if (!fileName.isEmpty()) {
        try {
            dataTable->to_csv(fileName.toStdString());
            QMessageBox::information(this, "成功", "数据导出成功！");
        } catch (const std::exception& e) {
            QMessageBox::critical(this, "错误", QString("数据导出失败: %1").arg(e.what()));
        }
    }
}

void MainWindow::exportImage()
{
    if (!dataLoaded || dataTable == nullptr) {
        QMessageBox::warning(this, "警告", "没有可导出的图像！");
        return;
    }

    // 1. 生成默认文件名 (基于当前时间)
    QString currentTime = QString::fromStdString(dataTable->time[currentTimeIndex]);
    // 替换非法字符: "2025-11-18 13:01:15" -> "2025-11-18_13-01-15"
    QString defaultName = currentTime.replace(":", "-").replace(" ", "_") + ".png";

    QString fileName = QFileDialog::getSaveFileName(
        this,
        "导出图像",
        defaultName,
        "PNG Files (*.png);;JPEG Files (*.jpg);;All Files (*)"
    );

    if (!fileName.isEmpty()) {
        // 4. 获取两个Widget的截图
        // 使用 renderToPixmap 替代 grab()，支持按需大小渲染
        
        // 设置一个合理的目标宽度，例如 1200 像素
        int targetWidth = 1200;
        int ppiHeight = 600; // PPI 区域高度
        int profileHeight = 400; // Profile 区域高度
        
        QSize ppiSize(targetWidth, ppiHeight);
        QSize profileSize(targetWidth, profileHeight);

        QPixmap ppiPix = ppiWidget->renderToPixmap(ppiSize);
        QPixmap profilePix = profileWidget->renderToPixmap(profileSize);

        // 5. 组合图片 (垂直排列)
        // 直接使用 targetWidth，不需要 max 计算，因为我们强制了宽度一致
        int width = targetWidth;
        int height = ppiPix.height() + profilePix.height();

        QPixmap compositePix(width, height);
        compositePix.fill(Qt::white); // 白色背景

        QPainter painter(&compositePix);
        // 绘制PPI图 (左上角对齐，因为宽度已经铺满)
        painter.drawPixmap(0, 0, ppiPix);

        // 绘制折线图 (位于PPI下方，左上角对齐)
        painter.drawPixmap(0, ppiPix.height(), profilePix);
        painter.end();

        // 7. 保存
        if (compositePix.save(fileName)) {
            QMessageBox::information(this, "成功", "图像导出成功！");
        } else {
            QMessageBox::critical(this, "错误", "图像导出失败！");
        }
    }
}

void MainWindow::updateSNRThreshold(double value)
{
    currentSNRThreshold = value;
    if (dataLoaded && dataTable != nullptr) {
        dataTable->SNR_mask(currentSNRThreshold);
        updateVisualization();
    }
}

void MainWindow::updateDistanceRange()
{
    currentMinDistance = minDistanceSpinBox->value();
    currentMaxDistance = maxDistanceSpinBox->value();

    if (currentMinDistance >= currentMaxDistance) {
        QMessageBox::warning(this, "警告", "最小距离必须小于最大距离！");
        return;
    }

    if (dataLoaded) {
        updateVisualization();
    }
}

void MainWindow::updateTimeIndex(int value)
{
    pendingTimeIndex = value;

    if (dataLoaded && dataTable != nullptr) {
        timeLabel->setText(QString("%1/%2").arg(value + 1).arg(dataTable->time.size()));
        // 暂时禁用节流，强制更新以满足用户"对每一个时间点都绘制"的需求
        currentTimeIndex = pendingTimeIndex;
        updateVisualization();
    }
}

void MainWindow::performVisualizationUpdate()
{
    if (pendingTimeIndex >= 0 && pendingTimeIndex != currentTimeIndex) {
        currentTimeIndex = pendingTimeIndex;
        updateVisualization();
    }
}

void MainWindow::updateVisualization()
{
    if (!dataLoaded || dataTable == nullptr || currentTimeIndex >= dataTable->time.size()) {
        return;
    }

    // 更新PPI热力图
    ppiWidget->setData(dataTable, currentTimeIndex, currentMinDistance, currentMaxDistance);
    ppiWidget->update();

    // 更新折线图
    profileWidget->setData(dataTable, currentTimeIndex, currentMinDistance, currentMaxDistance);
    profileWidget->update();
}

void MainWindow::updateStatusBar()
{
    QString fileStatus;
    if (dataLoaded && dataTable != nullptr) {
        fileStatus = QString("已加载: 风速数据和云台角度数据");
        dataPointsLabel->setText(QString("数据点: %1").arg(dataTable->time.size()));
    } else if (!windSpeedFilePath.isEmpty() && !gimbalFilePath.isEmpty()) {
        fileStatus = "已选择文件，请点击加载";
        dataPointsLabel->setText("数据点: 0");
    } else {
        fileStatus = "未加载数据";
        dataPointsLabel->setText("数据点: 0");
    }

    fileStatusLabel->setText(fileStatus);
}

void MainWindow::setResolution900x600()
{
    resize(900, 600);
}

void MainWindow::setResolution1280x800()
{
    resize(1280, 800);
}

void MainWindow::setResolution1400x1000()
{
    resize(1400, 1000);
}

void MainWindow::setResolution1600x1200()
{
    resize(1600, 1200);
}

void MainWindow::setResolution1920x1080()
{
    resize(1920, 1080);
}

void MainWindow::updateDisplayMode(int index)
{
    // 0: 风速, 1: 湍流强度
    PPIWidget::DisplayMode mode = (index == 0) ? PPIWidget::Mode_WindSpeed : PPIWidget::Mode_TurbulenceIntensity;
    ppiWidget->setDisplayMode(mode);
    
    ProfileWidget::DisplayMode pMode = (index == 0) ? ProfileWidget::Mode_WindSpeed : ProfileWidget::Mode_TurbulenceIntensity;
    profileWidget->setDisplayMode(pMode);
    
    // 更新状态栏
    scanModeLabel->setText(QString("扫描模式: %1").arg(index == 0 ? "PPI (风速)" : "PPI (湍流强度)"));
}

void MainWindow::updateWindowSize(int size)
{
    // 确保窗口大小为奇数
    if (size % 2 == 0) {
        size++;
        windowSizeSpinBox->blockSignals(true);
        windowSizeSpinBox->setValue(size);
        windowSizeSpinBox->blockSignals(false);
    }
    
    ppiWidget->setWindowSize(size);
    profileWidget->setWindowSize(size);
}
