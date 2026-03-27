#ifndef PPIWIDGET_H
#define PPIWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QColor>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPixmap>
#include <vector>
#include "loader.hpp"

class PPIWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PPIWidget(QWidget *parent = nullptr);
    ~PPIWidget();

    void setData(DATA_TABLE* data, int timeIndex, int minDist, int maxDist);

    enum DisplayMode {
        Mode_WindSpeed,
        Mode_TurbulenceIntensity
    };

    void setDisplayMode(DisplayMode mode);
    void setWindowSize(int size);
    
    // View Control
    double getScaleFactor() const { return scaleFactor; }
    QPointF getOffset() const { return offset; }
    void setTransform(double scale, QPointF off) {
        scaleFactor = scale;
        offset = off;
        update();
    }
    
    // Export Support
    QPixmap renderToPixmap(const QSize& size);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void drawContent(QPainter& painter, int w, int h);
    void drawPPIScan(QPainter& painter);
    void drawAxes(QPainter& painter);
    void drawColorBar(QPainter& painter, int w, int h);
    void drawHoverInfo(QPainter& painter);
    QColor getColorForValue(double value, double minVal, double maxVal);

    // TI Calculation Helper
    double calculateTI(int timeIdx, int rangeIdx);

    // 数据成员
    DATA_TABLE* dataTable;
    int currentTimeIndex;
    int minDistance;
    int maxDistance;
    DisplayMode displayMode; // New
    int windowSize;          // New

    // 可视化参数
    double centerX;
    double centerY;
    double radius;

    // Interaction
    double scaleFactor;
    QPointF offset;
    QPoint lastMousePos;
    bool isDragging;
    QPointF hoverPos;     // Mouse position for hover
    bool isHovering;      // Is mouse over widget?

    // Caching for Performance
    QPixmap cachedScan;
    bool cacheValid;
    void updateCache();

    // 颜色映射范围（固定）
    const double SPEED_MIN = 0.0;
    const double SPEED_MAX = 10.0;
    const double TI_MIN = 0.0;
    const double TI_MAX = 0.5;
};

#endif // PPIWIDGET_H
