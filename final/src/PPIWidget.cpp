#include "PPIWidget.h"
#include <QPaintEvent>
#include <QPainterPath>
#include <QLinearGradient>
#include <cmath>
#include <algorithm>
#include <limits>
#include <QToolTip>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PPIWidget::PPIWidget(QWidget *parent)
    : QWidget(parent), dataTable(nullptr), currentTimeIndex(0),
      minDistance(0), maxDistance(2000),
      centerX(0), centerY(0), radius(0),
      displayMode(Mode_WindSpeed), windowSize(5),
      scaleFactor(1.0), offset(0, 0),
      isDragging(false), isHovering(false)
{
    // 只设置最小宽度，不限制高度
    setMinimumWidth(400);
    setMouseTracking(true); // 开启鼠标追踪以支持悬停信息显示
}

void PPIWidget::setDisplayMode(DisplayMode mode)
{
    if (displayMode != mode) {
        displayMode = mode;
        cacheValid = false;
        update();
    }
}

void PPIWidget::setWindowSize(int size)
{
    if (windowSize != size) {
        windowSize = size;
        if (displayMode == Mode_TurbulenceIntensity) {
            cacheValid = false;
            update();
        }
    }
}

double PPIWidget::calculateTI(int timeIdx, int distIdx)
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
    
    if (count < 2) return 0.0; // 样本太少无法计算标准差
    
    double mean = sum / count;
    double variance = (sumSq - (sum * sum) / count) / (count - 1);
    double stdDev = std::sqrt(std::max(0.0, variance));
    
    if (std::abs(mean) < 1e-6) return 0.0; // 避免除以零
    
    return stdDev / std::abs(mean);
}

void PPIWidget::wheelEvent(QWheelEvent *event)
{
    double zoomFactor = 1.1;
    if (event->angleDelta().y() < 0) {
        scaleFactor /= zoomFactor;
    } else {
        scaleFactor *= zoomFactor;
    }
    
    // 限制缩放范围
    if (scaleFactor < 0.5) scaleFactor = 0.5;
    if (scaleFactor > 5.0) scaleFactor = 5.0;
    
    update();
}

void PPIWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        isDragging = true;
        lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void PPIWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (isDragging) {
        QPoint delta = event->pos() - lastMousePos;
        offset += delta;
        lastMousePos = event->pos();
        update();
    } else {
        hoverPos = event->pos();
        isHovering = true;
        update(); // 触发重绘以显示悬停信息
    }
}

void PPIWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        isDragging = false;
        setCursor(Qt::ArrowCursor);
    }
}

void PPIWidget::leaveEvent(QEvent *event)
{
    isHovering = false;
    update();
    QWidget::leaveEvent(event);
}

PPIWidget::~PPIWidget()
{
}

void PPIWidget::setData(DATA_TABLE* data, int timeIndex, int minDist, int maxDist)
{
    // Check if anything actually changed to avoid unnecessary cache invalidation
    if (dataTable != data || currentTimeIndex != timeIndex || 
        minDistance != minDist || maxDistance != maxDist) {
        dataTable = data;
        currentTimeIndex = timeIndex;
        minDistance = minDist;
        maxDistance = maxDist;
        cacheValid = false;
    }
}

void PPIWidget::paintEvent(QPaintEvent *event)
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

    drawContent(painter, width(), height());
}

QPixmap PPIWidget::renderToPixmap(const QSize& size)
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
    
    // For export, we force a non-cached draw to ensure correct resolution
    // Note: This will use the current 'radius', 'centerX', 'centerY' members
    // which are set by paintEvent/resizeEvent.
    // If 'size' is different from current widget size, we need to temporarily
    // calculate layout for the target size.
    
    // Calculate layout for target size
    int margin = 80;
    int colorBarWidth = 60;
    int plotWidth = size.width() - 2 * margin - colorBarWidth - 20;
    int plotHeight = size.height() - 2 * margin;
    int plotSize = std::min(plotWidth, plotHeight);
    
    double targetRadius = plotSize / 2 - 10;
    double targetCenterX = margin + plotSize / 2;
    double targetCenterY = margin + plotSize / 2;
    
    // Save current state
    double savedRadius = radius;
    double savedCX = centerX;
    double savedCY = centerY;
    
    // Set target state
    radius = targetRadius;
    centerX = targetCenterX;
    centerY = targetCenterY;
    
    // Draw content (without caching logic for simplicity, or we adapt drawContent)
    // Actually drawContent calls drawPPIScan which uses members.
    // We need to bypass the 'cachedScan' drawing in drawContent if we want high res
    
    // Let's modify drawContent to take a flag or handle this.
    // Or simpler: manually call drawing functions here.
    
    // Re-implement simplified drawContent for export:
    
    // 1. Draw PPI Scan (Directly, no cache)
    // We need to temporarily reset offset/scale for "default view" export
    // But renderToPixmap is supposed to respect current view? 
    // User said "save default size image", implying "default view" (no zoom).
    // The previous exportImage implementation already reset transform to 1.0/0,0.
    // So we can assume scale=1.0, offset=0,0 here or pass them.
    // Let's assume we want a clean, unzoomed view fitting the target size.
    
    double savedScale = scaleFactor;
    QPointF savedOffset = offset;
    
    scaleFactor = 1.0;
    offset = QPointF(0, 0);
    
    // Draw Scan
    painter.save();
    painter.translate(centerX, centerY); // Move to center
    // drawPPIScan assumes painter is at center (0,0) and uses 'radius' member
    // But wait, drawPPIScan also uses 'centerX'/'centerY' inside?
    // Let's check drawPPIScan... it saves/restores centerX/centerY and sets them to 0.
    // So it's safe.
    drawPPIScan(painter);
    painter.restore();
    
    // Draw Axes
    drawAxes(painter);
    
    // Draw ColorBar
    // drawColorBar uses 'width()' and 'height()' of widget!
    // We need to refactor drawColorBar to take size or rect.
    drawColorBar(painter, size.width(), size.height());
    
    // Draw Title
    painter.setPen(Qt::black);
    QFont titleFont = painter.font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    
    QString modeStr = (displayMode == Mode_WindSpeed) ? "风速绝对值" : "湍流强度";
    QString title = QString("PPI扫描 - %1 - 仰角: %2° - 时间: %3")
                        .arg(modeStr)
                        .arg(dataTable->ele[currentTimeIndex], 0, 'f', 1)
                        .arg(QString::fromStdString(dataTable->time[currentTimeIndex]));
    painter.drawText(QRect(0, 10, size.width(), 30), Qt::AlignCenter, title);
    
    // Restore state
    radius = savedRadius;
    centerX = savedCX;
    centerY = savedCY;
    scaleFactor = savedScale;
    offset = savedOffset;
    
    return pixmap;
}

void PPIWidget::drawContent(QPainter& painter, int w, int h)
{
    // 计算绘图区域
    int margin = 80;
    int colorBarWidth = 60;
    int plotWidth = w - 2 * margin - colorBarWidth - 20;
    int plotHeight = h - 2 * margin;
    int plotSize = std::min(plotWidth, plotHeight);

    // 如果半径发生变化，缓存也失效
    double newRadius = plotSize / 2 - 10;
    if (std::abs(newRadius - radius) > 1.0) {
        radius = newRadius;
        cacheValid = false;
    }

    centerX = margin + plotSize / 2;
    centerY = margin + plotSize / 2;
    radius = newRadius;

    // 更新缓存（如果需要）
    if (!cacheValid) {
        updateCache();
    }

    // 应用变换（缩放和平移）
    painter.save();
    
    // 以中心点为基准进行缩放
    painter.translate(centerX + offset.x(), centerY + offset.y());
    painter.scale(scaleFactor, scaleFactor);
    
    if (!cachedScan.isNull()) {
        painter.drawPixmap(-cachedScan.width() / 2, -cachedScan.height() / 2, cachedScan);
    }
    
    painter.restore(); // 恢复到Widget坐标系

    // 重新应用变换，但这次是为了绘制坐标轴
    painter.save();
    painter.translate(centerX + offset.x(), centerY + offset.y());
    painter.scale(scaleFactor, scaleFactor);
    painter.translate(-centerX, -centerY); // 回到以(0,0)为原点的Widget坐标系逻辑
    
    drawAxes(painter);
    painter.restore();

    // 绘制颜色条（固定在屏幕上）
    drawColorBar(painter, w, h);

    // 绘制标题
    painter.setPen(Qt::black);
    QFont titleFont = painter.font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    
    QString modeStr = (displayMode == Mode_WindSpeed) ? "风速绝对值" : "湍流强度";
    QString title = QString("PPI扫描 - %1 - 仰角: %2° - 时间: %3")
                        .arg(modeStr)
                        .arg(dataTable->ele[currentTimeIndex], 0, 'f', 1)
                        .arg(QString::fromStdString(dataTable->time[currentTimeIndex]));
    painter.drawText(QRect(0, 10, w, 30), Qt::AlignCenter, title);
    
    // 绘制悬停信息
    if (isHovering) {
        drawHoverInfo(painter);
    }
}

void PPIWidget::updateCache()
{
    // 创建缓存Pixmap
    // 大小覆盖整个扫描区域 (2*radius)
    // 增加一点边距以防万一
    int size = static_cast<int>(radius * 2 + 10);
    cachedScan = QPixmap(size, size);
    cachedScan.fill(Qt::transparent);

    QPainter painter(&cachedScan);
    painter.setRenderHint(QPainter::Antialiasing);

    // 将绘图原点移动到Pixmap中心
    painter.translate(size / 2.0, size / 2.0);
    
    // 在缓存绘制中使用 0,0 作为中心
    // 我们需要修改 drawPPIScan 或创建一个新的绘制函数
    // 为了最小化修改，我们可以在这里直接实现绘制逻辑，
    // 或者调整 drawPPIScan 让它接受中心点参数
    // 这里我们重构 drawPPIScan 为 drawPPIScanToPainter
    
    drawPPIScan(painter);
    
    cacheValid = true;
}

void PPIWidget::drawPPIScan(QPainter& painter)
{
    // 注意：此函数现在被 updateCache 调用
    // painter 的坐标系原点已经是圆心
    // 所以所有的 centerX, centerY 都应该被视为 0, 0
    // 但是原来的代码使用了成员变量 centerX, centerY
    // 我们需要临时覆盖这些值，或者修改逻辑
    
    double savedCenterX = centerX;
    double savedCenterY = centerY;
    
    // 在绘制到缓存时，中心点是 (0,0)
    centerX = 0;
    centerY = 0;

    if (dataTable == nullptr || currentTimeIndex >= dataTable->time.size()) {
        centerX = savedCenterX;
        centerY = savedCenterY;
        return;
    }

    double currentElevation = dataTable->ele[currentTimeIndex];

    // 收集当前仰角层的所有扫描数据，并按方位角排序
    struct ScanData {
        int idx;
        double azimuth;
    };
    std::vector<ScanData> scanDataList;

    // 搜索参数
    const double ELEVATION_TOLERANCE = 0.5; // 仰角容差

    // 1. 查找当前仰角状态的起始点
    int startIdx = currentTimeIndex;
    while (startIdx > 0) {
        if (std::abs(dataTable->ele[startIdx - 1] - currentElevation) > ELEVATION_TOLERANCE) {
            break;
        }
        startIdx--;
    }

    // 2. 收集从起始点到当前时间点的数据
    for (int i = startIdx; i <= currentTimeIndex; ++i) {
        scanDataList.push_back({i, dataTable->azi[i]});
    }

    // 3. 按方位角排序以进行绘图
    std::sort(scanDataList.begin(), scanDataList.end(),
              [](const ScanData& a, const ScanData& b) { return a.azimuth < b.azimuth; });

    // 定义方位角偏移量：数据0度对应地理方位105度（东偏南15度）
    const double AZIMUTH_OFFSET = 105.0;

    painter.setPen(Qt::NoPen);

    // 对每对相邻的扫描线进行插值绘制
    for (size_t i = 0; i < scanDataList.size(); ++i) {
        int idx1 = scanDataList[i].idx;
        double azi1 = scanDataList[i].azimuth;

        int idx2 = idx1;
        double azi2 = azi1;
        if (i + 1 < scanDataList.size()) {
            idx2 = scanDataList[i + 1].idx;
            azi2 = scanDataList[i + 1].azimuth;
        }

        double deltaAzi = (i + 1 < scanDataList.size()) ? (azi2 - azi1) : 0.5;
        int numInterp = std::max(1, static_cast<int>(deltaAzi / 0.5));

        for (int interpIdx = 0; interpIdx < numInterp; ++interpIdx) {
            double t = static_cast<double>(interpIdx) / numInterp;
            double interpAzi = azi1 + t * (azi2 - azi1);

            // 应用偏移量：将数据角度转换为地理角度
            double realAzimuth = interpAzi + AZIMUTH_OFFSET;
            double screenAngleDeg = 90.0 - realAzimuth;
            double screenAngle = screenAngleDeg * M_PI / 180.0;

            for (size_t distIdx = 0; distIdx < dataTable->distance.size(); distIdx += 1) {
                int dist = dataTable->distance[distIdx];

                if (dist < minDistance || dist > maxDistance) {
                    continue;
                }

                double val1, val2;
                
                if (displayMode == Mode_WindSpeed) {
                    val1 = dataTable->speed[idx1][distIdx];
                    val2 = (i + 1 < scanDataList.size()) ? dataTable->speed[idx2][distIdx] : val1;
                } else {
                    val1 = calculateTI(idx1, distIdx);
                    val2 = (i + 1 < scanDataList.size()) ? calculateTI(idx2, distIdx) : val1;
                }

                if (std::isnan(val1)) val1 = val2;
                if (std::isnan(val2)) val2 = val1;
                if (std::isnan(val1)) continue;

                double interpVal = val1 + t * (val2 - val1);
                double absVal = std::abs(interpVal);

                bool masked = false;
                if (dataTable->mask_table.size() > idx1) {
                    int maskPos = dataTable->mask_table[idx1];
                    if (maskPos >= 0 && distIdx >= maskPos) {
                        masked = true;
                    }
                }

                double normalizedDist = (dist - minDistance) / (double)(maxDistance - minDistance);
                double r = radius * normalizedDist;

                QColor color;
                if (masked) {
                    color = Qt::lightGray;
                } else {
                    double minVal = (displayMode == Mode_WindSpeed) ? SPEED_MIN : TI_MIN;
                    double maxVal = (displayMode == Mode_WindSpeed) ? SPEED_MAX : TI_MAX;
                    color = getColorForValue(absVal, minVal, maxVal);
                }

                painter.setBrush(color);

                double angleSpan = 1.0; 

                size_t nextDistIdx = distIdx + 1;
                if (nextDistIdx >= dataTable->distance.size()) {
                    nextDistIdx = distIdx;
                }
                int dist2 = dataTable->distance[nextDistIdx];
                double normalizedDist2 = (dist2 - minDistance) / (double)(maxDistance - minDistance);
                double r2 = radius * normalizedDist2;

                double innerR = r;
                double outerR = r2;

                if (innerR < 0) innerR = 0;

                QPainterPath path;
                path.moveTo(centerX, centerY);
                path.arcTo(QRectF(centerX - outerR, centerY - outerR, 2 * outerR, 2 * outerR),
                          screenAngleDeg - angleSpan / 2, angleSpan);
                if (innerR > 0) {
                    path.arcTo(QRectF(centerX - innerR, centerY - innerR, 2 * innerR, 2 * innerR),
                              screenAngleDeg + angleSpan / 2, -angleSpan);
                }
                path.closeSubpath();

                painter.drawPath(path);
            }
        }
    }
    
    // 恢复成员变量
    centerX = savedCenterX;
    centerY = savedCenterY;
}

void PPIWidget::drawAxes(QPainter& painter)
{
    painter.save();
    painter.setPen(QPen(Qt::gray, 1, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);

    // 绘制距离圈
    int numRings = 5;
    QFont font = painter.font();
    font.setPointSize(7); // Set font size to 7 as requested
    painter.setFont(font);
    
    for (int i = 1; i <= numRings; ++i) {
        double r = radius * i / numRings;
        painter.drawEllipse(QPointF(centerX, centerY), r, r);
        
        int dist = minDistance + (maxDistance - minDistance) * i / numRings;
        painter.drawText(centerX + 5, centerY - r + 5, QString::number(dist));
    }

    // 绘制方位线
    int numLines = 8; // 45 degrees step
    for (int i = 0; i < numLines; ++i) {
        double angleDeg = i * (360.0 / numLines);
        double angleRad = angleDeg * M_PI / 180.0;
        
        double x = centerX + radius * std::cos(angleRad);
        double y = centerY - radius * std::sin(angleRad); // y轴向下
        
        painter.drawLine(QPointF(centerX, centerY), QPointF(x, y));
        
        // Label
        double labelR = radius + 15;
        double labelX = centerX + labelR * std::cos(angleRad);
        double labelY = centerY - labelR * std::sin(angleRad);
        
        double az = 90.0 - angleDeg;
        if (az < 0) az += 360.0;
        if (az >= 360.0) az -= 360.0;
        
        painter.drawText(QRectF(labelX - 15, labelY - 10, 30, 20), Qt::AlignCenter, QString::number(az, 'f', 0));
    }
    
    painter.restore();
}

void PPIWidget::drawColorBar(QPainter& painter, int w, int h)
{
    int barWidth = 20;
    int barHeight = 200;
    int rightMargin = 20;
    int topMargin = 50;

    int x = w - rightMargin - barWidth;
    int y = topMargin;

    // Draw gradient bar
    QLinearGradient gradient(x, y + barHeight, x, y);
    gradient.setColorAt(0.0, Qt::blue);
    gradient.setColorAt(0.25, Qt::cyan);
    gradient.setColorAt(0.5, Qt::green);
    gradient.setColorAt(0.75, Qt::yellow);
    gradient.setColorAt(1.0, Qt::red);

    painter.fillRect(x, y, barWidth, barHeight, gradient);
    painter.setPen(Qt::black);
    painter.drawRect(x, y, barWidth, barHeight);

    // Draw labels
    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);

    double minVal = 0.0;
    double maxVal = (displayMode == Mode_WindSpeed) ? 10.0 : 0.5; // TI max changed to 0.5

    for (int i = 0; i <= 5; ++i) {
        double val = minVal + (maxVal - minVal) * i / 5.0;
        int ly = y + barHeight - (barHeight * i / 5);
        painter.drawText(x + barWidth + 5, ly + 5, QString::number(val, 'f', 1));
    }

    // Title
    painter.save();
    painter.translate(x + barWidth + 35, y + barHeight / 2);
    painter.rotate(-90);
    painter.drawText(0, 0, (displayMode == Mode_WindSpeed) ? "风速 (m/s)" : "湍流强度");
    painter.restore();
}

QColor PPIWidget::getColorForValue(double value, double minVal, double maxVal)
{
    // 限制值在范围内
    if (value < minVal) value = minVal;
    if (value > maxVal) value = maxVal;

    // 归一化到0-1
    double normalized = (value - minVal) / (maxVal - minVal);

    // 简化的渐变色谱：深蓝 -> 蓝 -> 青 -> 绿 -> 黄 -> 橙 -> 红
    int r, g, b;

    if (normalized < 0.167) {
        // 深蓝(0,0,139) 到 蓝(0,0,255)
        double t = normalized / 0.167;
        r = 0;
        g = 0;
        b = static_cast<int>(139 + (255 - 139) * t);
    } else if (normalized < 0.333) {
        // 蓝(0,0,255) 到 青(0,255,255)
        double t = (normalized - 0.167) / 0.166;
        r = 0;
        g = static_cast<int>(255 * t);
        b = 255;
    } else if (normalized < 0.5) {
        // 青(0,255,255) 到 绿(0,255,0)
        double t = (normalized - 0.333) / 0.167;
        r = 0;
        g = 255;
        b = static_cast<int>(255 * (1 - t));
    } else if (normalized < 0.667) {
        // 绿(0,255,0) 到 黄(255,255,0)
        double t = (normalized - 0.5) / 0.167;
        r = static_cast<int>(255 * t);
        g = 255;
        b = 0;
    } else if (normalized < 0.833) {
        // 黄(255,255,0) 到 橙(255,165,0)
        double t = (normalized - 0.667) / 0.166;
        r = 255;
        g = static_cast<int>(255 - 90 * t);
        b = 0;
    } else {
        // 橙(255,165,0) 到 红(255,0,0)
        double t = (normalized - 0.833) / 0.167;
        r = 255;
        g = static_cast<int>(165 * (1 - t));
        b = 0;
    }

    return QColor(r, g, b);
}

void PPIWidget::drawHoverInfo(QPainter& painter)
{
    if (dataTable == nullptr || currentTimeIndex >= dataTable->time.size()) return;

    // 1. 反变换坐标以查找数据
    // P_screen = scale * (P_world - center) + center + offset
    // P_world - center = (P_screen - center - offset) / scale
    double dx = (hoverPos.x() - (centerX + offset.x())) / scaleFactor;
    double dy = (hoverPos.y() - (centerY + offset.y())) / scaleFactor;
    
    // 计算距离和角度
    double distFromCenter = std::sqrt(dx * dx + dy * dy);
    double r_max = radius;
    
    // 如果在扫描区域外，不显示详细信息
    if (distFromCenter > r_max * 1.1) return; // 允许一点容差
    
    double normalizedDist = distFromCenter / r_max;
    double actualDist = minDistance + normalizedDist * (maxDistance - minDistance);
    
    // 计算屏幕角度（弧度）
    // dy是向下为正
    double angleRad = std::atan2(-dy, dx);
    double screenAngleDeg = angleRad * 180.0 / M_PI;
    double compassAzimuth = 90.0 - screenAngleDeg;
    if (compassAzimuth < 0) compassAzimuth += 360.0;
    if (compassAzimuth >= 360.0) compassAzimuth -= 360.0;

    // 定义方位角偏移量：数据0度对应地理方位105度（东偏南15度）
    const double AZIMUTH_OFFSET = 105.0;
    
    // 计算数据方位角 (Data Azimuth = Compass Azimuth - Offset)
    double radarAzimuth = compassAzimuth - AZIMUTH_OFFSET;
    // 归一化到 [0, 360) 范围，因为数据里的方位角通常是正数
    // 但是这里要注意，如果数据本身只是 0-360 的循环，我们应该使用模运算
    // 假设数据方位角也是 0-360 定义的
    while (radarAzimuth < 0) radarAzimuth += 360.0;
    while (radarAzimuth >= 360.0) radarAzimuth -= 360.0;
    
    // 2. 查找最近的数据点
    double currentElevation = dataTable->ele[currentTimeIndex];
    int startIdx = currentTimeIndex;
    const double ELEVATION_TOLERANCE = 0.5;
    while (startIdx > 0) {
        if (std::abs(dataTable->ele[startIdx - 1] - currentElevation) > ELEVATION_TOLERANCE) {
            break;
        }
        startIdx--;
    }
    
    int bestTimeIdx = -1;
    double minAngleDiff = 1000.0;
    
    double minAziInRange = 360.0;
    double maxAziInRange = 0.0;
    bool hasData = false;

    // Increased tolerance to 20.0 degrees to make it easier to hit data
    for (int i = startIdx; i <= currentTimeIndex; ++i) {
        double azi = dataTable->azi[i];
        
        if (!hasData) {
            minAziInRange = azi;
            maxAziInRange = azi;
            hasData = true;
        } else {
            if (azi < minAziInRange) minAziInRange = azi;
            if (azi > maxAziInRange) maxAziInRange = azi;
        }

        double diff = std::abs(azi - radarAzimuth);
        if (diff > 180.0) diff = 360.0 - diff;
        
        if (diff < minAngleDiff) {
            minAngleDiff = diff;
            bestTimeIdx = i;
        }
    }

    // Debug/Status Info: Always show current cursor position AND search results
    /*
    painter.save();
    painter.setPen(Qt::black);
    painter.setBrush(QColor(255, 255, 255, 200));
    painter.drawRect(5, 5, 400, 85); // Increased size
    painter.drawText(10, 20, QString("Cursor: %1° (Data: %2°) / %3 m").arg(compassAzimuth, 0, 'f', 1).arg(radarAzimuth, 0, 'f', 1).arg(actualDist, 0, 'f', 0));
    painter.drawText(10, 40, QString("Search Idx: [%1, %2] (Elev: %3)").arg(startIdx).arg(currentTimeIndex).arg(currentElevation, 0, 'f', 1));
    if (hasData) {
         painter.drawText(10, 60, QString("Scan Azi Range: [%1°, %2°]").arg(minAziInRange, 0, 'f', 1).arg(maxAziInRange, 0, 'f', 1));
    } else {
         painter.drawText(10, 60, "Scan Azi Range: N/A");
    }

    if (bestTimeIdx != -1) {
        QString matchStatus = (minAngleDiff <= 20.0) ? "MATCH" : "TOO FAR";
        painter.drawText(10, 80, QString("Best: Idx %1, Azi %2°, Diff %3° [%4]").arg(bestTimeIdx).arg(dataTable->azi[bestTimeIdx], 0, 'f', 1).arg(minAngleDiff, 0, 'f', 1).arg(matchStatus));
    } else {
        painter.drawText(10, 80, "No data in current scan range");
    }
    painter.restore();
    */
    
    // Threshold check - adjusted to be more permissible for debugging
    if (bestTimeIdx == -1 || minAngleDiff > 20.0) return; // Increased to 20 degrees
    
    // 查找最近的距离门
    int bestDistIdx = -1;
    double minDistDiff = 100000.0;
    
    for (size_t i = 0; i < dataTable->distance.size(); ++i) {
        double d = dataTable->distance[i];
        double diff = std::abs(d - actualDist);
        if (diff < minDistDiff) {
            minDistDiff = diff;
            bestDistIdx = i;
        }
    }
    
    if (bestDistIdx == -1) return;
    
    // 获取数据
    double speed = dataTable->speed[bestTimeIdx][bestDistIdx];
    double ti = calculateTI(bestTimeIdx, bestDistIdx);
    
    // 3. 绘制高亮圈（准确位置）
    double r = radius * (dataTable->distance[bestDistIdx] - minDistance) / (double)(maxDistance - minDistance);
    double az = dataTable->azi[bestTimeIdx];
    // 使用带偏移量的角度进行绘制
    double screenAng = (90.0 - (az + AZIMUTH_OFFSET)) * M_PI / 180.0;
    double hx = centerX + r * std::cos(screenAng);
    double hy = centerY - r * std::sin(screenAng);
    
    double sx = scaleFactor * (hx - centerX) + centerX + offset.x();
    double sy = scaleFactor * (hy - centerY) + centerY + offset.y();
    
    painter.setPen(QPen(Qt::black, 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QPointF(sx, sy), 6, 6);
    painter.setPen(QPen(Qt::white, 1));
    painter.drawEllipse(QPointF(sx, sy), 4, 4);

    // 4. 绘制悬停信息框
    QStringList info;
    
    // 计算并格式化地理方位角
    double rawAz = dataTable->azi[bestTimeIdx];
    double geoAz = rawAz + AZIMUTH_OFFSET;
    while (geoAz >= 360.0) geoAz -= 360.0;
    while (geoAz < 0.0) geoAz += 360.0;
    
    QString dirStr;
    double dirAngle;
    
    if (geoAz >= 0 && geoAz < 90) {
        dirStr = "北偏东";
        dirAngle = geoAz;
    } else if (geoAz >= 90 && geoAz < 180) {
        dirStr = "东偏南";
        dirAngle = geoAz - 90.0;
    } else if (geoAz >= 180 && geoAz < 270) {
        dirStr = "南偏西";
        dirAngle = geoAz - 180.0;
    } else {
        dirStr = "西偏北";
        dirAngle = geoAz - 270.0;
    }
    
    info << QString("方位: %1%2°").arg(dirStr).arg(dirAngle, 0, 'f', 1);
    info << QString("距离: %1 m").arg(dataTable->distance[bestDistIdx]);
    info << QString("风速: %1 m/s").arg(speed, 0, 'f', 2);
    info << QString("TI: %1").arg(ti, 0, 'f', 3);
    
    QPoint tooltipPos = hoverPos.toPoint() + QPoint(20, 20);
    
    QFontMetrics fm(painter.font());
    int maxWidth = 0;
    int totalHeight = 0;
    int lineHeight = fm.height() + 4;
    for (const QString& str : info) {
        maxWidth = std::max(maxWidth, fm.horizontalAdvance(str));
        totalHeight += lineHeight;
    }
    
    QRect tooltipRect(tooltipPos.x(), tooltipPos.y(), maxWidth + 10, totalHeight + 10);
    
    // 边界检查
    if (tooltipRect.right() > width()) tooltipRect.moveRight(hoverPos.x() - 10);
    if (tooltipRect.bottom() > height()) tooltipRect.moveBottom(hoverPos.y() - 10);
    
    painter.setBrush(QColor(0, 0, 0, 200));
    painter.setPen(Qt::white);
    painter.drawRect(tooltipRect);
    
    int y = tooltipRect.top() + 5 + fm.ascent();
    for (const QString& str : info) {
        painter.drawText(tooltipRect.left() + 5, y, str);
        y += lineHeight;
    }
}
