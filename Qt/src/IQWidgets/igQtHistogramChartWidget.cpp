#include "IQWidgets/igQtHistogramChartWidget.h"

#include <QBrush>
#include <QColor>
#include <QFrame>
#include <QLineSeries>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QResizeEvent>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <vector>

namespace {
// ParaView 图表调色板（ColorBrewer Set1）实测值，来自 XYChartRepresentation 的 SeriesColor：
//   Points_Magnitude 黑、Points_X (0.89,0.10,0.11)、Points_Y (0.22,0.49,0.72)、Points_Z (0.30,0.69,0.29)、
//   第一条数据数组 v (0.60,0.31,0.64) 紫；bin_values 系列为 (0.89,0.10,0.11) 红。
const QColor kOverlayBarColor(0x99, 0x4F, 0xA3);   // #984EA3：模型数据数组在图表里的系列色
const QColor kDockBarColor(0xE3, 0x1A, 0x1C);      // #E41A1C：bin_values 的系列色
constexpr IGsize kMaxBarCount = 1000;              // 超过这个值改用折线，避免 Qt Charts 卡顿
} // namespace

void igQtHistogramChartWidget::DrawArrayValues(iGame::ArrayObject::Pointer array, int component,
                                               const QString& title) {
    if (array.IsNull()) { return; }
    QChart* chart = m_ChartView->chart();
    if (chart == nullptr) { return; }

    const int dimension = std::max(1, array->GetDimension());
    const IGsize count = array->GetNumberOfValues() / dimension;
    if (count <= 0) { return; }
    const bool magnitude = (component >= dimension);
    auto valueAt = [&](IGsize index) {
        if (magnitude) {
            double squareSum = 0.0;
            for (int c = 0; c < dimension; ++c) {
                const double value = array->GetElementValue(index, c);
                squareSum += value * value;
            }
            return std::sqrt(squareSum);
        }
        const int safeComponent = component < 0 ? 0 : component;
        return array->GetElementValue(index, safeComponent);
    };

    chart->removeAllSeries();
    const auto previousAxes = chart->axes();
    for (QAbstractAxis* axis : previousAxes) {
        chart->removeAxis(axis);
        delete axis;
    }

    std::vector<double> values(static_cast<size_t>(count));
    double minimum = 0.0;
    double maximum = 0.0;
    for (IGsize i = 0; i < count; ++i) {
        values[static_cast<size_t>(i)] = valueAt(i);
        if (i == 0 || values[static_cast<size_t>(i)] < minimum) { minimum = values[static_cast<size_t>(i)]; }
        if (i == 0 || values[static_cast<size_t>(i)] > maximum) { maximum = values[static_cast<size_t>(i)]; }
    }
    const double padding = std::max(0.5, (maximum - minimum) * 0.05);

    if (count <= kMaxBarCount) {
        auto* set = new QBarSet(QString::fromStdString(array->GetName()));
        set->setColor(kOverlayBarColor);
        set->setBorderColor(QColor("#2B2B2B"));
        for (IGsize i = 0; i < count; ++i) { *set << values[static_cast<size_t>(i)]; }
        auto* series = new QBarSeries();
        series->append(set);
        chart->addSeries(series);

        auto* axisX = new QBarCategoryAxis();
        for (IGsize i = 0; i < count; ++i) { axisX->append(QString::number(static_cast<long long>(i))); }
        axisX->setTitleText(QStringLiteral("索引"));
        axisX->setLabelsVisible(count <= 30);
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);

        auto* axisY = new QValueAxis();
        axisY->setRange(minimum - padding, maximum + padding);
        axisY->setTitleText(QString::fromStdString(array->GetName()));
        axisY->setLabelFormat("%g");
        axisY->setTickCount(5);
        axisY->setMinorTickCount(0);
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
        auto styleAxis = [](auto* axis) {
            axis->setLabelsColor(QColor("#C8C8C8"));
            axis->setTitleBrush(QBrush(QColor("#C8C8C8")));
            axis->setGridLineColor(QColor(255, 255, 255, 35));
            axis->setLinePenColor(QColor("#6A6A6A"));
        };
        styleAxis(axisX);
        styleAxis(axisY);
    } else {
        auto* series = new QLineSeries();
        QPen pen(kOverlayBarColor);
        pen.setWidthF(1.0);
        series->setPen(pen);
        for (IGsize i = 0; i < count; ++i) {
            series->append(static_cast<double>(i), values[static_cast<size_t>(i)]);
        }
        chart->addSeries(series);
        auto* axisX = new QValueAxis();
        axisX->setRange(0.0, static_cast<double>(count - 1));
        axisX->setTitleText(QStringLiteral("索引"));
        axisX->setLabelFormat("%g");
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);
        auto* axisY = new QValueAxis();
        axisY->setRange(minimum - padding, maximum + padding);
        axisY->setTitleText(QString::fromStdString(array->GetName()));
        axisY->setLabelFormat("%g");
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
        auto styleAxis = [](auto* axis) {
            axis->setLabelsColor(QColor("#C8C8C8"));
            axis->setTitleBrush(QBrush(QColor("#C8C8C8")));
            axis->setGridLineColor(QColor(255, 255, 255, 35));
            axis->setLinePenColor(QColor("#6A6A6A"));
        };
        styleAxis(axisX);
        styleAxis(axisY);
    }

    chart->setTitle(title);
}

igQtHistogramChartWidget::igQtHistogramChartWidget(QWidget* parent, bool overlay)
    : QWidget(parent), m_Overlay(overlay) {
    auto* chart = new QChart();
    // 叠加层要能透出下面的模型，所以背景走半透明
    chart->setBackgroundBrush(QBrush(m_Overlay ? QColor(18, 18, 18, 190) : QColor("#1F1F1F")));
    chart->setBackgroundPen(Qt::NoPen);
    chart->setBackgroundRoundness(0);
    chart->setMargins(QMargins(2, 2, 2, 2));
    chart->setPlotAreaBackgroundVisible(true);
    chart->setPlotAreaBackgroundBrush(QBrush(m_Overlay ? QColor(0, 0, 0, 120) : QColor("#252526")));
    chart->setPlotAreaBackgroundPen(Qt::NoPen);
    chart->setTitleBrush(QBrush(QColor("#E0E0E0")));
    chart->legend()->setVisible(!m_Overlay);  // 叠加层面积小，不显示图例
    chart->legend()->setAlignment(Qt::AlignTop);
    chart->legend()->setLabelColor(QColor("#D0D0D0"));

    m_ChartView = new QChartView(chart, this);
    m_ChartView->setRenderHint(QPainter::Antialiasing, true);
    m_ChartView->setFrameShape(QFrame::NoFrame);
    m_ChartView->setBackgroundBrush(QBrush(Qt::NoBrush));
    m_ChartView->setStyleSheet(QStringLiteral("QChartView { background: transparent; border: 0px; }"));

    if (m_Overlay) {
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAutoFillBackground(false);
        m_CloseButton = new QPushButton(QStringLiteral("✕"), this);
        m_CloseButton->setFixedSize(18, 18);
        m_CloseButton->setCursor(Qt::PointingHandCursor);
        m_CloseButton->setToolTip(QStringLiteral("关闭直方图叠加层"));
        m_CloseButton->setStyleSheet(
                QStringLiteral("QPushButton { color:#D0D0D0; background: rgba(60,60,60,170); border:0px; "
                               "border-radius:9px; font-size:11px; }"
                               "QPushButton:hover { background: rgba(152,78,163,220); color:#FFFFFF; }"));
        connect(m_CloseButton, &QPushButton::clicked, this, [this]() { hide(); });
        m_CloseButton->raise();
    }

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(m_Overlay ? 2 : 4, m_Overlay ? 2 : 4, m_Overlay ? 2 : 4, m_Overlay ? 2 : 4);
    layout->setSpacing(0);
    layout->addWidget(m_ChartView);
    if (!m_Overlay) { setMinimumHeight(200); }
}

void igQtHistogramChartWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (m_CloseButton != nullptr) {
        m_CloseButton->move(width() - m_CloseButton->width() - 4, 4);
        m_CloseButton->raise();
    }
}

void igQtHistogramChartWidget::DrawHistogram(iGame::ArrayObject::Pointer binExtents,
                                             iGame::ArrayObject::Pointer binValues,
                                             const QString& title,
                                             bool normalized) {
    if (binExtents.IsNull() || binValues.IsNull()) { return; }
    const int numberOfBins = static_cast<int>(binValues->GetNumberOfValues());
    if (numberOfBins <= 0 || binExtents->GetNumberOfValues() < numberOfBins) { return; }

    QChart* chart = m_ChartView->chart();
    if (chart == nullptr) { return; }

    // 重新统计时先清掉上一次的系列与坐标轴，否则坐标轴会不断累积
    chart->removeAllSeries();
    const auto previousAxes = chart->axes();
    for (QAbstractAxis* axis : previousAxes) {
        chart->removeAxis(axis);
        delete axis;
    }

    auto* set = new QBarSet(QStringLiteral("bin_values"));
    set->setColor(m_Overlay ? kOverlayBarColor : kDockBarColor);
    set->setBorderColor(QColor("#2B2B2B"));
    double maxValue = 0.0;
    for (int i = 0; i < numberOfBins; ++i) {
        const double value = binValues->GetValue(i);
        if (value > maxValue) { maxValue = value; }
        *set << value;
    }
    auto* series = new QBarSeries();
    series->append(set);
    chart->addSeries(series);

    auto* axisX = new QBarCategoryAxis();
    for (int i = 0; i < numberOfBins; ++i) {
        axisX->append(QString::number(binExtents->GetValue(i), 'g', 6));
    }
    axisX->setTitleText(QStringLiteral("bin 中心"));
    axisX->setLabelsVisible(numberOfBins <= 30);  // 箱数很多时不显示类别标签，避免互相压叠
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto* axisY = new QValueAxis();
    if (normalized) {
        axisY->setRange(0.0, std::max(1e-6, maxValue * 1.1));
        axisY->setTitleText(QStringLiteral("频率"));
        axisY->setLabelFormat("%g");
    } else {
        axisY->setRange(0.0, std::max(1.0, std::ceil(maxValue * 1.1)));
        axisY->setTitleText(QStringLiteral("频次"));
        axisY->setLabelFormat("%d");
    }
    axisY->setTickCount(5);
    axisY->setMinorTickCount(0);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chart->setTitle(title);
    auto applyDarkAxisStyle = [](auto* axis) {
        axis->setLabelsColor(QColor("#C8C8C8"));
        axis->setTitleBrush(QBrush(QColor("#C8C8C8")));
        axis->setGridLineColor(QColor(255, 255, 255, 35));
        axis->setLinePenColor(QColor("#6A6A6A"));
    };
    applyDarkAxisStyle(axisX);
    applyDarkAxisStyle(axisY);
}
