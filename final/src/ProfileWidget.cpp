#include "ProfileWidget.h"
#include <QPaintEvent>
#include <cmath>
#include <algorithm>
#include <limits>

ProfileWidget::ProfileWidget(QWidget *parent)
    : QWidget(parent), dataTable(nullptr), currentTimeIndex(0),
      minDistance(0), maxDistance(2000),
      displayMode(Mode_WindSpeed), windowSize(5)
{
    // 只设置最小宽度，不限制高度
    setMinimumWidth(600);
}

void ProfileWidget::setDisplayMode(DisplayMode mode)
{
    if (displayMode != mode) {
        displayMode = mode;
        update();
    }
}

void ProfileWidget::setWindowSize(int size)
{
    if (windowSize != size) {
        windowSize = size;
        if (displayMode == Mode_TurbulenceIntensity) {
            update();
        }
    }
}

double ProfileWidget::calculateTI(int timeIdx, int distIdx)
{
    if (!dataTable || timeIdx < 0 || timeIdx >= dataTable->speed.size()) return 0.0;
    
    const std::vector<double>& ray = dataTable->speed[timeIdx];
    int numGates = ray.size();
    
    if (distIdx < 0 || distIdx >= numGates) return 0.0;
    
    int halfWin = windowSize / 2;
    int start = std::max(0, distIdx - halfWin);
    int end = std::min(numGates - 1, distIdx + halfWin);
    
    double sum = 0.0;
    double sumSq = 0.0;
    int count = 0;
    
    for (int i = start; i <= end; ++i) {
        double val = ray[i];
        if (!std::isnan(val)) {
            sum += val;
            sumSq += val * val;
            count++;
        }
    }
    
    if (count < 2) return 0.0;
    
    double mean = sum / count;
    double variance = (sumSq - (sum * sum) / count) / (count - 1);
    double stdDev = std::sqrt(std::max(0.0, variance));
    
    if (std::abs(mean) < 1e-6) return 0.0;
    
    return stdDev / std::abs(mean);
}

ProfileWidget::~ProfileWidget()
{
}

void ProfileWidget::setData(DATA_TABLE* data, int timeIndex, int minDist, int maxDist)
{
    dataTable = data;
    currentTimeIndex = timeIndex;
    minDistance = minDist;
    maxDistance = maxDist;
}

void ProfileWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 背景
    painter.fillRect(rect(), Qt::white);

    if (dataTable == nullptr || currentTimeIndex >= dataTable->time.size()) {
        painter.setPen(Qt::black);
        painter.drawText(rect(), Qt::AlignCenter, "无数据");
        return;
    }

    drawProfile(painter, width(), height());
}

QPixmap ProfileWidget::renderToPixmap(const QSize& size)
{
    QPixmap pixmap(size);
    pixmap.fill(Qt::white);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    if (dataTable == nullptr || currentTimeIndex >= dataTable->time.size()) {
        painter.setPen(Qt::black);
        painter.drawText(QRect(0, 0, size.width(), size.height()), Qt::AlignCenter, "无数据");
        return pixmap;
    }
    
    drawProfile(painter, size.width(), size.height());
    return pixmap;
}

void ProfileWidget::drawProfile(QPainter& painter, int w, int h)
{
    int margin = 60;
    int gap = 40;

    // 两个子图并排显示
    int plotWidth = (w - 2 * margin - gap) / 2;
    int plotHeight = h - 2 * margin;

    QRect speedPlotRect(margin, margin, plotWidth, plotHeight);
    QRect snrPlotRect(margin + plotWidth + gap, margin, plotWidth, plotHeight);


    // 准备数据
    std::vector<double> distances;
    std::vector<double> values;
    std::vector<double> snrs;

    for (size_t i = 0; i < dataTable->distance.size(); ++i) {
        int dist = dataTable->distance[i];
        if (dist >= minDistance && dist <= maxDistance) {
            double speed = dataTable->speed[currentTimeIndex][i];
            double snr = dataTable->snr[currentTimeIndex][i];

            double val = 0.0;
            bool valValid = false;
            
            if (displayMode == Mode_WindSpeed) {
                val = speed;
                valValid = !std::isnan(val);
            } else {
                val = calculateTI(currentTimeIndex, i);
                valValid = true; // calculateTI returns 0.0 on failure, which is a plottable value
            }

            if (valValid && !std::isnan(snr)) {
                distances.push_back(dist);
                values.push_back(val);
                snrs.push_back(snr);
            }
        }
    }

    if (distances.empty()) {
        painter.drawText(rect(), Qt::AlignCenter, "当前时刻无有效数据");
        return;
    }

    // 设置左侧图表范围和标签
    double minVal, maxVal;
    QString valLabel, valTitle;
    
    if (displayMode == Mode_WindSpeed) {
        minVal = -15.0;
        maxVal = 15.0;
        valLabel = "径向风速 (m/s)";
        valTitle = "距离-风速剖面";
    } else {
        minVal = 0.0;
        maxVal = 1.0;
        valLabel = "湍流强度";
        valTitle = "距离-湍流强度剖面";
    }

    // 固定SNR范围：0 到 80 dB
    double minSNR = 0.0;
    double maxSNR = 80.0;

    // 距离范围
    double minDist = *std::min_element(distances.begin(), distances.end());
    double maxDist = *std::max_element(distances.begin(), distances.end());

    // 绘制左侧图表（风速或TI）
    drawAxes(painter, speedPlotRect, minVal, maxVal, minDist, maxDist,
             valLabel, "距离 (m)", valTitle);

    // 绘制折线
    painter.setPen(QPen(Qt::blue, 2));
    for (size_t i = 0; i + 1 < distances.size(); ++i) {
        double x1 = speedPlotRect.left() + (values[i] - minVal) / (maxVal - minVal) * speedPlotRect.width();
        double y1 = speedPlotRect.bottom() - (distances[i] - minDist) / (maxDist - minDist) * speedPlotRect.height();
        double x2 = speedPlotRect.left() + (values[i+1] - minVal) / (maxVal - minVal) * speedPlotRect.width();
        double y2 = speedPlotRect.bottom() - (distances[i+1] - minDist) / (maxDist - minDist) * speedPlotRect.height();

        // 简单的裁剪处理
        if (x1 < speedPlotRect.left()) x1 = speedPlotRect.left();
        if (x1 > speedPlotRect.right()) x1 = speedPlotRect.right();
        if (x2 < speedPlotRect.left()) x2 = speedPlotRect.left();
        if (x2 > speedPlotRect.right()) x2 = speedPlotRect.right();

        painter.drawLine(QPointF(x1, y1), QPointF(x2, y2));
    }

    // 绘制数据点
    painter.setBrush(Qt::blue);
    for (size_t i = 0; i < distances.size(); ++i) {
        double x = speedPlotRect.left() + (values[i] - minVal) / (maxVal - minVal) * speedPlotRect.width();
        double y = speedPlotRect.bottom() - (distances[i] - minDist) / (maxDist - minDist) * speedPlotRect.height();
        
        if (x >= speedPlotRect.left() && x <= speedPlotRect.right()) {
            painter.drawEllipse(QPointF(x, y), 2, 2);
        }
    }

    // 绘制SNR图
    drawAxes(painter, snrPlotRect, minSNR, maxSNR, minDist, maxDist,
             "信噪比 SNR (dB)", "距离 (m)", "距离-SNR剖面");

    // 绘制SNR折线
    painter.setPen(QPen(Qt::red, 2));
    for (size_t i = 0; i + 1 < distances.size(); ++i) {
        double x1 = snrPlotRect.left() + (snrs[i] - minSNR) / (maxSNR - minSNR) * snrPlotRect.width();
        double y1 = snrPlotRect.bottom() - (distances[i] - minDist) / (maxDist - minDist) * snrPlotRect.height();
        double x2 = snrPlotRect.left() + (snrs[i+1] - minSNR) / (maxSNR - minSNR) * snrPlotRect.width();
        double y2 = snrPlotRect.bottom() - (distances[i+1] - minDist) / (maxDist - minDist) * snrPlotRect.height();

        painter.drawLine(QPointF(x1, y1), QPointF(x2, y2));
    }

    // 绘制数据点
    painter.setBrush(Qt::red);
    for (size_t i = 0; i < distances.size(); ++i) {
        double x = snrPlotRect.left() + (snrs[i] - minSNR) / (maxSNR - minSNR) * snrPlotRect.width();
        double y = snrPlotRect.bottom() - (distances[i] - minDist) / (maxDist - minDist) * snrPlotRect.height();
        painter.drawEllipse(QPointF(x, y), 2, 2);
    }
}

void ProfileWidget::drawAxes(QPainter& painter, const QRect& plotRect,
                              double minX, double maxX, double minY, double maxY,
                              const QString& xLabel, const QString& yLabel, const QString& title)
{
    // 绘制边框
    painter.setPen(QPen(Qt::black, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(plotRect);

    // 绘制网格
    painter.setPen(QPen(Qt::lightGray, 1, Qt::DashLine));
    int numGridLines = 5;

    // 垂直网格线
    for (int i = 1; i < numGridLines; ++i) {
        int x = plotRect.left() + plotRect.width() * i / numGridLines;
        painter.drawLine(x, plotRect.top(), x, plotRect.bottom());
    }

    // 水平网格线
    for (int i = 1; i < numGridLines; ++i) {
        int y = plotRect.top() + plotRect.height() * i / numGridLines;
        painter.drawLine(plotRect.left(), y, plotRect.right(), y);
    }

    // 绘制坐标轴标签
    painter.setPen(Qt::black);
    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);

    // X轴刻度
    int numXTicks = 6;
    for (int i = 0; i <= numXTicks; ++i) {
        double value = minX + (maxX - minX) * i / numXTicks;
        int x = plotRect.left() + plotRect.width() * i / numXTicks;
        painter.drawLine(x, plotRect.bottom(), x, plotRect.bottom() + 5);
        painter.drawText(QPointF(x - 15, plotRect.bottom() + 20), QString::number(value, 'f', 1));
    }

    // Y轴刻度
    int numYTicks = 6;
    for (int i = 0; i <= numYTicks; ++i) {
        double value = minY + (maxY - minY) * i / numYTicks;
        int y = plotRect.bottom() - plotRect.height() * i / numYTicks;
        painter.drawLine(plotRect.left() - 5, y, plotRect.left(), y);
        painter.drawText(QPointF(plotRect.left() - 35, y + 4), QString::number(value, 'f', 0));
    }

    // X轴标签
    painter.drawText(QRect(plotRect.left(), plotRect.bottom() + 35,
                          plotRect.width(), 20), Qt::AlignCenter, xLabel);

    // Y轴标签
    painter.save();
    painter.translate(plotRect.left() - 45, plotRect.top() + plotRect.height() / 2);
    painter.rotate(-90);
    painter.drawText(0, 0, yLabel);
    painter.restore();

    // 标题
    QFont titleFont = font;
    titleFont.setBold(true);
    titleFont.setPointSize(10);
    painter.setFont(titleFont);
    painter.drawText(QRect(plotRect.left(), plotRect.top() - 25,
                          plotRect.width(), 20), Qt::AlignCenter, title);
}
