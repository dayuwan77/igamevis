#include <IQWidgets/igQtCharts.h>
#include <iGameFlatArray.h>

#include <QApplication>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <cmath>
#include <iostream>
#include <vector>

namespace {
bool Near(double lhs, double rhs) { return std::abs(lhs - rhs) <= 1.0e-8; }

bool Check(bool condition, const char* message) {
    if (!condition) std::cerr << "FAILED: " << message << '\n';
    return condition;
}
}

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);

    auto data = iGame::DoubleArray::New();
    data->SetName("Velocity");
    data->SetDimension(2);
    data->AddElement2(0.0, 10.0);
    data->AddElement2(1.0, 20.0);
    data->AddElement2(2.0, 30.0);
    const std::vector<double> distances{0.0, 0.5, 2.0};

    igQtCharts dialog;
    dialog.drawLineChart(data, distances, 1, QStringLiteral("沿线距离"));
    auto* chart = dialog.getChartView()->chart();
    bool ok = Check(chart->series().size() == 1, "one line series is created");
    auto* series = chart->series().empty() ? nullptr : qobject_cast<QLineSeries*>(chart->series().front());
    ok &= Check(series && series->count() == 3, "one point is plotted for every line sample");
    if (series && series->count() == 3) {
        ok &= Check(Near(series->at(0).x(), 0.0) && Near(series->at(0).y(), 10.0),
                    "first distance and selected component are plotted");
        ok &= Check(Near(series->at(2).x(), 2.0) && Near(series->at(2).y(), 30.0),
                    "last distance and selected component are plotted");
    }

    const auto horizontalAxes = chart->axes(Qt::Horizontal);
    const auto verticalAxes = chart->axes(Qt::Vertical);
    auto* axisX = horizontalAxes.empty() ? nullptr : qobject_cast<QValueAxis*>(horizontalAxes.front());
    auto* axisY = verticalAxes.empty() ? nullptr : qobject_cast<QValueAxis*>(verticalAxes.front());
    ok &= Check(axisX && axisX->titleText() == QStringLiteral("沿线距离"),
                "horizontal axis represents line distance");
    ok &= Check(axisY && axisY->titleText() == QStringLiteral("Velocity [1]"),
                "vertical axis identifies the selected array component");

    if (!ok) return 1;
    std::cout << "Point line chart acceptance tests passed.\n";
    return 0;
}
