#ifndef PROFILEWIDGET_H
#define PROFILEWIDGET_H

#include <QWidget>
#include <QPainter>
#include <vector>
#include "loader.hpp"

class ProfileWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProfileWidget(QWidget *parent = nullptr);
    ~ProfileWidget();

    void setData(DATA_TABLE* data, int timeIndex, int minDist, int maxDist);

    enum DisplayMode {
        Mode_WindSpeed,
        Mode_TurbulenceIntensity
    };

    void setDisplayMode(DisplayMode mode);
    void setWindowSize(int size);

    // Export Support
    QPixmap renderToPixmap(const QSize& size);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void drawProfile(QPainter& painter, int w, int h);
    double calculateTI(int timeIdx, int distIdx);
    void drawAxes(QPainter& painter, const QRect& plotRect,
                  double minX, double maxX, double minY, double maxY,
                  const QString& xLabel, const QString& yLabel, const QString& title);

    // 数据成员
    DATA_TABLE* dataTable;
    int currentTimeIndex;
    int minDistance;
    int maxDistance;
    DisplayMode displayMode;
    int windowSize;
};

#endif // PROFILEWIDGET_H
