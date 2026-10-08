#pragma once

#include <IQCore/igQtExportModule.h>

#include <iGameArrayObject.h>

#include <QWidget>

#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>
#include <string>

QT_CHARTS_USE_NAMESPACE

class QPushButton;

/**
 * @class   igQtHistogramChartWidget
 * @brief   属性直方图的柱状图控件。横轴 = bin 中心（HistogramData::GetBinExtents()），
 *          纵轴 = 频次（HistogramData::GetBinValues()，归一化时是频率）。
 *
 * 两种用法：
 *  - c overlay = false：停靠在主窗口底部的 Dock 里（深色背景、蓝色柱）；
 *  - c overlay = true ：作为 3D 视图（igQtRenderWidget，QOpenGLWidget）的子控件叠加在原模型上
 *    （半透明背景、紫色柱、右上角 ✕ 关闭），对应 ParaView 运行 Histogram 后在模型视图上出现的那张图。
 */
class IG_QT_MODULE_EXPORT igQtHistogramChartWidget : public QWidget {
    Q_OBJECT

public:
    explicit igQtHistogramChartWidget(QWidget* parent = nullptr, bool overlay = false);

    /** p normalized 为 true 时纵轴是归一化后的频率（bin_values 是 double 列），否则是整数频次。 */
    void DrawHistogram(iGame::ArrayObject::Pointer binExtents,
                       iGame::ArrayObject::Pointer binValues,
                       const QString& title,
                       bool normalized = false);

    /**
     * 画「数组数值 vs 索引」的柱状图（ParaView 图表视图里显示模型时的样子：UseIndexForXAxis = 1，
     * 纵轴就是所选数组本身，第一条数据数组的系列色为 ColorBrewer Set1 的紫色 #984EA3）。
     * p component 等于数组维数时按模长统计；超过 1000 个值时退化为折线以免 Qt Charts 卡顿。
     */
    void DrawArrayValues(iGame::ArrayObject::Pointer array, int component, const QString& title);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    QChartView* m_ChartView{nullptr};
    QPushButton* m_CloseButton{nullptr};
    bool m_Overlay{false};
};
