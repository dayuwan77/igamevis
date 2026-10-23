#include <iGameRegionPlaneStyle.h>
#include <IQWidgets/igQtExtractLocationWidget.h>
#include "IQCore/igQtMainWindow.h"
#include "Selection/igameextractselectionfilter.h"
#include <IQWidgets/igQtProbeWidget.h>
#include <map>
#include <QCheckBox>
#include <QSignalBlocker>
//
// Created by m_ky on 2024/4/10.
//

#include "MeshMetrics/iGameVolumeMeshMetricsFilter.h"
#include "DataProcessing/OverlappingCellsDetector/iGameOverlappingCellsDetectorFilter.h"
#include "ForceStaticMesh/iGameForceStaticMeshFilter.h"
#include "GhostCell/iGameGhostCellFilter.h"
#include "TextureMapToSphere/iGameTextureMapToSphereFilter.h"
#include "GenerateSurfaceTangents/iGameGenerateSurfaceTangentsFilter.h"
#include "MeshMetrics/iGameCellMeshMetricsFilter.h"
#include "ModelSurface/iGameMultiBlockGeometryFilter.h"
#include "MyFilter/iGameValidateCellsFilter.h"
#include "AppendAttributes/iGameAppendAttributesFilter.h"
#include "AppendDatasets/iGameAppendDatasetsFilter.h"
#include "AppendArcLength/iGameAppendArcLengthFilter.h"
#include "Histogram/iGameHistogramFilter.h"
#include "DataProcessing/ExtractLocation/iGameExtractLocationFilter.h"
#include "DataProcessing/iGameRandomAttributesFilter.h"
#include "FeatureExtraction/iGameDeflectNormalsFilter.h"
#include "MyFilter/iGameExtractCellsByTypeFilter.h"
#include "PassArrays/iGamePassArraysFilter.h"
#include "Selection/iGameExtractCellsByRegionFilter.h"
#include "Transformation/iGameTransformFilter.h"
#include "VolumeOfRevolution/iGameVolumeOfRevolutionFilter.h"
#include "Connectivity/iGameConnectivityFilter.h"
#include "ReverseSense/iGameReverseSenseFilter.h"
#include "WarpByScalar/iGameWarpByScalarFilter.h"
#include "Deformation/iGameStressDeformationFilterCode.h"
#include "ExtractEnclosedPoints/iGameExtractEnclosedPointsFilter.h"
#include "PointPlaneInterpolator/iGamePointPlaneInterpolatorFilter.h"

#include "DataProcessing/Tests/iGameGradient.h"
#include "FeatureExtraction/iGameAdvancedGradientFilter.h"
#include "DataProcessing/Tests/iGameSimplification2.h"
#include "DataProcessing/Tests/iGameSurfaceSimplification.h"
#include "DataProcessing/Tests/meshsimplifier/meshsimplifier.h"
#include "DataProcessing/Tests/simplifier.h"
#include "DataProcessing/iGameMeshSimplificationFilter.h"
#include "DataProcessing/iGameMeshSimplificationFilterPro.h"
#include "DataProcessing/iGameDecimatePolylineFilter.h"
#include "DataProcessing/iGameMeshTriangulationFilter.h"
#include "DataProcessing/iGameSurfaceMeshTopologyChecker.h"
#include "DataProcessing/Simplification/iGameMeshSaliency.h"
#include "DataProcessing/Simplification/iGameMeshSimplificationWithAttributes.h"
#include "DataProcessing/iGameVolumeMeshSimplification.h"
#include "DataProcessing/iGameMeshTetrahedralize.h"

#include "Convert/iGameConvertPolyhedralCellsFilter.h"
#include "Convert/iGameConvertToCellDataFilter.h"
#include "Convert/iGameConvertToLagrangeUnstructuredMeshFilter.h"
#include "Convert/iGameConvertToPointCloudFilter.h"
#include "Convert/iGameConvertToPointDataFilter.h"
#include "Convert/iGameConvertToSurfaceMeshFilter.h"
#include "Convert/iGameConvertToVolumeMeshFilter.h"
#include "DataProcessing/iGameRandomAttributesFilter.h"
#include "FeatureExtraction/iGameDeflectNormalsFilter.h"
#include "Convert/iGamePointSetToOctreeFilter.h"
#include "Convert/iGameResampleToImageFilter.h"
#include <IQWidgets/igQtResampleToImageWidget.h>
#include "MeshQuality/iGameMeshQualityFilter.h"

#include "Transformation/iGameTransformFilter.h"
#include "TimeSeries/iGameTemporalShiftScaleFilter.h"

#include "FeatureExtraction/iGameFeatureEdgesFilter.h"
#include "FeatureExtraction/iGamePlaneSamplingFilter.h"
#include "Selection/iGameExtractCellsByRegionFilter.h"
#include "TimeSeries/iGameExtractTimeStepsFilter.h"
#include "MyFilter/iGameExtractCellsByTypeFilter.h"

#include "Convert/iGameConvertToPointCloudFilter.h"
#include "Convert/iGameConvertToPointDataFilter.h"
#include "Convert/iGameConvertToSurfaceMeshFilter.h"
#include "Convert/iGameConvertToVolumeMeshFilter.h"
#include "Interactor/iGameInteractor.h"
#include "Interactor/iGameBoxStyle.h"
#include "Interactor/iGameRadialStyle.h"
#include "iGameSelectionParameter.h"

#include "CellCenter/iGameCellCenterFilter.h"
#include "MyFilter/iGameCleanToGridFilter.h"

#include "Interactor/iGameInteractor.h"

#include "Tests/iGameARAPTest.h"

#include "iGameFileIO.h"
#include "iGameFilterIncludes.h"
#include <IQComponents/igQtFilterDialogDockWidget.h>
#include <IQComponents/igQtModelDialogWidget.h>
#include <IQComponents/igQtProgressBarWidget.h>
#include <IQCore/igQtFileLoader.h>
#include <IQCore/igQtOpenGLWidgetManager.h>
#include <IQCore/igQtRemoteModelLibrary.h>
#include <IQWidgets/ColorManager/igQtColorManagerWidget.h>
#include <IQWidgets/igQtAiChat/igQtAiChatWidget.h>
#include <IQWidgets/igQtAiChat/igQtCommandManager.h>
#include <IQWidgets/igQtCharts.h>
#include <IQWidgets/igQtDeformationWidget.h>
#include <IQWidgets/igQtElevationFilterPanel.h>
#include <SurfaceNormals/iGameSurfaceNormalsFilter.h>
#include <IQWidgets/igQtExtractCellsByTypeWidget.h>
#include <IQWidgets/igQtExtractTimeStepsWidget.h>
#include <IQWidgets/igQtExtractComponentWidget.h>
#include <IQWidgets/igQtPointSetToOctreeWidget.h>
#include <IQWidgets/igQtTemporalStatisticsWidget.h>
#include <IQWidgets/igQtPointVolumeInterpolatorWidget.h>
#include <IQWidgets/igQtExtractLocationWidget.h>
#include <IQWidgets/igQtHistogramChartWidget.h>
#include <IQWidgets/igQtGlobalIdWidget.h>
#include <IQWidgets/igQtSmoothWidget.h>
#include <IQWidgets/igQtLinearExtrusionWidget.h>
#include <IQWidgets/igQtAxisAlignedReflectionWidget.h>
#include <IQWidgets/igQtMergeVectorComponentsWidget.h>
#include <IQWidgets/igQtMedianFilterWidget.h>
#include <IQWidgets/igQtModelClipWidget.h>
#include <IQWidgets/igQtModelDrawWidget.h>
#include <IQWidgets/igQtModelInformationWidget.h>
#include <IQWidgets/igQtParallelCoordinatesWidget.h>
#include <IQWidgets/igQtTensorWidget.h>
#include <IQWidgets/igQtTextureMapToPlaneWidget.h>
#include <IQWidgets/igQtVariableCorrelationWidget.h>
#include <IQWidgets/igQtPartFocusWidget.h>
#include <IQWidgets/igQtAttributeSelectWidget.h>
#include <IQWidgets/igQtTransformBoxController.h>
#include <IQComponents/Dialog/igQtBoxSettingDialog.h>
#include <IQComponents/Dialog/igQtChromeFramelessDialog.h>
#include <iGameBlockMapping.h>
#include <P3SAM/iGameP3SAMSegmenter.h>
#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <limits>
#include <QButtonGroup>
#include <QCheckBox>
#include <QDebug>
#include <QLabel>
#include <QMessageBox>
#include <QSplitter>
#include <QStyleFactory>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>
#include <QPushButton>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRegion>
#include <memory>
#include <QScrollArea>
#include <Sources/iGameLineTypePointsSourceFilter.h>
#include <Tests/iGameVolumeMeshFilterTest.h>
#include <VolumeMeshAlgorithm/iGameVolumeMeshClipper.h>
#include <fcntl.h>
#include <iGameBoxStyle.h>
#include <iGameCtxPresObjData.h>
#include <iGameDataSource.h>
#include <iGameDynamicBox.h>
#include <iGamePointFinder.h>
#include <iGameSelection.h>
#include <iGameSelectionParameter.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>
#include <include/IQComponents/Dialog/igQtChangeBackGroundDialog.h>
#include <include/IQComponents/Dialog/igQtMeshCodecDialog.h>
#include <include/IQComponents/Dialog/igQtDarkFramelessMessage.h>
#include <include/IQComponents/Dialog/igQtScreenShotOptionDialog.h>
#include <BuildAdjacencyRelation/iGameBuildAdjacencyRelationFilter.h>
#include <meshoptimizer.h>
#include <stdio.h>

#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QSplitter>
#include <QPointer>
#include <QTimer>
#include <QTextStream>
#include <QGuiApplication>
#include <QScreen>
#include <QWindow>
#include <QApplication>
#include <IQWidgets/igQtRenderWidget.h>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QStyle>
#include <QFontMetrics>
#include <QSettings>
#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QStringList>
#include <QFile>
#include <limits>
#include <QMenu>
#include <QAction>

#include <cmath>


#include "ui_igQtVariableCorrelationWidget.h"
#include <IQWidgets/igQtRoundedCornerHelper.h>

#include "GroupDatasets/iGameGroupDatasetsFilter.h"

#include <set>

// GroupDatasets 结果节点的唯一命名：
// 扫描场景中形如 "GroupDatasets_<数字>" 的模型名，取【最小空缺序号】（删除后可复用）。
// 两处判定要点：
//   1) 必须以 "GroupDatasets_" 【开头】—— 用 rfind(prefix, 0) == 0 表达，
//      避免把 "X_GroupDatasets_1" 这类名字误算进来；
//   2) 后缀必须【全是数字】—— 用 find_first_not_of("0123456789") == npos 表达，
//      避免把用户自定义的 "GroupDatasets_test" 拿去 std::stoi 抛异常。
static std::string UniqueGroupDatasetsName() {
    const std::string prefix = "GroupDatasets_";
    std::set<int> used;

    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    if (scene != nullptr) {
        auto modelList = scene->GetModelList();
        if (modelList != nullptr) {
            for (auto it = modelList->Begin(); it != modelList->End(); ++it) {
                if (it->second == nullptr || it->second->GetDataObject() == nullptr) { continue; }
                const std::string& name = it->second->GetDataObject()->GetName();

                if (name.rfind(prefix, 0) != 0) { continue; } // 必须以 prefix 开头
                const std::string suffix = name.substr(prefix.size());
                if (suffix.empty() ||
                    suffix.find_first_not_of("0123456789") != std::string::npos) {
                    continue; // 后缀必须全是数字
                }
                used.insert(std::stoi(suffix));
            }
        }
    }

    int n = 1;
    while (used.count(n) != 0) { ++n; }
    return prefix + std::to_string(n);
}

namespace {
const QColor kFloatingCoverColor(0x1E, 0x1E, 0x1E);

//
static void applyMainWindowRoundedMask(QWidget* w, int radius) {
    if (!w || radius <= 0) return;
    const QSize sz = w->size();
    if (sz.isEmpty()) return;

    const int ss = 4;
    QImage big(sz.width() * ss, sz.height() * ss, QImage::Format_ARGB32_Premultiplied);
    if (big.isNull()) return;
    big.fill(Qt::transparent);
    {
        QPainter p(&big);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::black);
        p.drawRoundedRect(QRectF(0, 0, big.width(), big.height()), qreal(radius) * ss, qreal(radius) * ss);
    }
    const QImage small = big.scaled(sz, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    w->setMask(QRegion(QBitmap::fromImage(small.createAlphaMask())));
}

// ---------------------------------------------------------------------------
// 「数据转换」辅助：把一个数据对象就地转换，并回传统计信息。
//   1) 用核心既有的「就地转换」filter（不改动核心代码）；转换后单元属性会变成同名点属性，
//      必须强制重算可绘制数据，否则画面/色条还是旧的；
//   2) filter 在“该方向没有属性”时会空转并返回 true，所以这里先自己数候选属性，
//      否则“什么都没做”会被当成转换成功（那就会出现点了菜单毫无反应、也不报错）；
//   3) 只处理自身带网格数据的对象：PVD 这类复合模型的父容器只是空壳，
//      调用方（数据转换）会把**当前帧挂载的**对象逐个传进来。
struct ConvertApplyResult {
    int candidateAttrs{0};    // 命中方向的属性数（转点数据=单元属性；转单元数据=点属性）
    int convertedAttrs{0};    // 真正转换成功的属性数
    bool filterFailed{false}; // filter 执行失败
};

ConvertApplyResult ConvertDataObjectInPlace(iGame::DataObject::Pointer object, bool toPointData) {
    using namespace iGame;
    ConvertApplyResult result;
    if (object == nullptr) { return result; }
    if (object->GetPoints() == nullptr || object->GetCellArray() == nullptr) { return result; }
    auto attrs = object->GetAttributeSet();
    if (attrs == nullptr) { return result; }

    for (int i = 0; i < attrs->GetNumberOfAttributes(); ++i) {
        auto& attr = attrs->GetAttribute(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        if (toPointData ? (attr.attachmentType == IG_CELL) : (attr.attachmentType == IG_POINT)) {
            ++result.candidateAttrs;
        }
    }
    if (result.candidateAttrs == 0) { return result; } // 该方向没有属性：空转不算转换

    bool ok = false;
    if (toPointData) {
        ConvertToPointDataFilter::Pointer filter = ConvertToPointDataFilter::New();
        filter->SetInput(object);
        ok = filter->Execute();
    } else {
        ConvertToCellDataFilter::Pointer filter = ConvertToCellDataFilter::New();
        filter->SetInput(object);
        ok = filter->Execute();
    }
    if (!ok) {
        result.filterFailed = true;
        qDebug() << "Convert filter failed on object" << QString::fromStdString(object->GetName())
                 << (toPointData ? "ToPointData" : "ToCellData");
        return result;
    }
    result.convertedAttrs = result.candidateAttrs;
    if (auto drawObject = DynamicCast<DrawObject>(object)) {
        drawObject->ForceReConvertToDrawableData();
    }
    return result;
}

struct ToolbarSpacingMetrics {
    int btnGap;
    int edgeMargin;
    int buttonPadding;
    int verticalGap;
    int bottomMargin;
    int groupGap;
    int rowGap;
};

ToolbarSpacingMetrics metricsForIconSize(int iconSize) {
    iconSize = qMax(24, iconSize);
    ToolbarSpacingMetrics metrics;
    metrics.btnGap = qMax(2, iconSize / 12);
    metrics.edgeMargin = qMax(1, iconSize / 22);
    metrics.buttonPadding = qMax(1, iconSize / 20);
    metrics.verticalGap = qMax(4, iconSize / 5);
    metrics.bottomMargin = qMax(8, iconSize / 3);
    metrics.groupGap = qMax(6, iconSize / 6);
    metrics.rowGap = qMax(2, iconSize / 8);
    return metrics;
}

// 工具栏标题（每个 toolbar 下方那行小灰字，"文件与输出" 之类）字号 → iconSize 的映射；
// 让文字大小随着窗口/屏幕响应式变化。
int titlePointSizeForIcon(int iconSize) {
    return qBound(8, iconSize / 4, 14); // 32->8, 40->10, 46->11, 50->12, 52->13
}

int resolveToolbarIconSize(int availableWidth, qreal dpiScale) {
    int iconSize = 52;
    if (availableWidth <= 1366) {
        iconSize = 32;
    } else if (availableWidth <= 1600) {
        iconSize = 36;
    } else if (availableWidth <= 1920) {
        iconSize = 40;
    } else if (availableWidth <= 2560) {
        iconSize = 46;
    } else if (availableWidth <= 2880) {
        iconSize = 50;
    }

    const qreal scale = qMax<qreal>(1.0, dpiScale);
    return qBound(24, static_cast<int>(qRound(static_cast<qreal>(iconSize) / scale)), 52);
}

int resolveToolbarIconSizeForWidget(const QWidget* widget) {
    QScreen* screen = widget ? widget->screen() : nullptr;
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    const qreal dpiScale = screen ? screen->devicePixelRatio() : 1.0;
    // 响应式：优先按窗口自身可视宽度分档，而不是整个屏幕；
    // 窗口还没显示（width==0）或过窄时才回退到屏幕宽度。
    int refWidth = widget ? widget->width() : 0;
    if (refWidth < 400) {
        refWidth = screen ? screen->availableGeometry().width() : 1920;
    }
    return resolveToolbarIconSize(refWidth, dpiScale);
}

// 工具栏单排适配的可调参数
constexpr int kToolbarIconMin = 24;             // 图标尺寸下限（与 resolveToolbarIconSize 的 clamp 下限一致）
constexpr int kToolbarIconMax = 56;             // 图标尺寸上限（放大填充时允许略超分档表，让宽屏更饱满）
constexpr double kToolbarFillRatio = 0.92;      // 单排填充目标：工具栏总宽达到可用宽度的 92% 左右即停止放大
constexpr int kToolbarButtonTextMinWidth = 48;
constexpr int kToolbarButtonTextMaxLines = 3;

// 按钮字号（逻辑像素）随图标尺寸联动。
// 注意：必须用像素单位而非 pt，否则在高 DPI 缩放下文字宽度会随 DPI 放大，导致换行/压宽度失效。
int toolbarButtonFontPixel(int iconSize) {
    if (iconSize >= 46) return 12;
    if (iconSize >= 38) return 11;
    if (iconSize >= 30) return 10;
    if (iconSize >= 26) return 9;
    return 8;
}

const char* kGlobalSpinBoxDarkQss = R"(
QSpinBox, QDoubleSpinBox {
    background-color: #252526;
    color: #D4D4D4;
    border: 1px solid #3C3C3C;
    border-radius: 4px;
    padding: 4px 24px 4px 8px;
    selection-background-color: #094771;
}
QSpinBox:hover, QDoubleSpinBox:hover {
    border: 1px solid #4A4A4A;
}
QSpinBox:focus, QDoubleSpinBox:focus {
    border: 1px solid #0E639C;
}
QSpinBox::up-button, QDoubleSpinBox::up-button {
    subcontrol-origin: border;
    subcontrol-position: top right;
    width: 18px;
    border-left: 1px solid #3C3C3C;
    border-top-right-radius: 4px;
    background-color: #2D2D30;
}
QSpinBox::down-button, QDoubleSpinBox::down-button {
    subcontrol-origin: border;
    subcontrol-position: bottom right;
    width: 18px;
    border-left: 1px solid #3C3C3C;
    border-top: 1px solid #3C3C3C;
    border-bottom-right-radius: 4px;
    background-color: #2D2D30;
}
QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover,
QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover {
    background-color: #3A3A3D;
}
QSpinBox::up-button:pressed, QDoubleSpinBox::up-button:pressed,
QSpinBox::down-button:pressed, QDoubleSpinBox::down-button:pressed {
    background-color: #45454A;
}
QSpinBox::up-arrow, QDoubleSpinBox::up-arrow {
    image: url(:/Ticon/Icons/spin_up_silver.svg);
    width: 9px;
    height: 9px;
}
QSpinBox::down-arrow, QDoubleSpinBox::down-arrow {
    image: url(:/Ticon/Icons/spin_down_silver.svg);
    width: 9px;
    height: 9px;
}
QComboBox::drop-down {
    border-left: 1px solid #3C3C3C;
    width: 20px;
}
QComboBox::down-arrow {
    image: url(:/Ticon/Icons/spin_down_silver.svg);
    width: 10px;
    height: 10px;
}
QComboBox QAbstractItemView {
    background-color: #252526;
    color: #CCCCCC;
    border: 1px solid #3C3C3C;
    outline: 0;
    selection-background-color: #3A3A3A;
    selection-color: #FFFFFF;
}
QScrollBar:vertical {
    background-color: #1B1B1B;
    border: none;
    width: 12px;
    margin: 0;
}
QScrollBar::handle:vertical {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                stop:0 #9E9E9E, stop:0.5 #BEBEBE, stop:1 #989898);
    border: none;
    border-radius: 6px;
    min-height: 20px;
}
QScrollBar::handle:vertical:hover {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                stop:0 #ABABAB, stop:0.5 #CBCBCB, stop:1 #A5A5A5);
}
QScrollBar::handle:vertical:pressed {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                stop:0 #8B8B8B, stop:0.5 #A9A9A9, stop:1 #858585);
}
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
    background-color: #242424;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}
QScrollBar:horizontal {
    background-color: #1B1B1B;
    border: none;
    height: 12px;
    margin: 0;
}
QScrollBar::handle:horizontal {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                stop:0 #9E9E9E, stop:0.5 #BEBEBE, stop:1 #989898);
    border: none;
    border-radius: 6px;
    min-width: 20px;
}
QScrollBar::handle:horizontal:hover {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                stop:0 #ABABAB, stop:0.5 #CBCBCB, stop:1 #A5A5A5);
}
QScrollBar::handle:horizontal:pressed {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                stop:0 #8B8B8B, stop:0.5 #A9A9A9, stop:1 #858585);
}
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
    background-color: #242424;
}
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0;
}
)";

constexpr int kFallbackStyleMode = 12;

bool isStyleModeAvailable(int mode) {
    return mode >= 12 && mode <= 15;
}

int normalizeStyleMode(int mode) {
    return isStyleModeAvailable(mode) ? mode : kFallbackStyleMode;
}


bool isFloatingCardStyle(int mode) { return mode >= 12 && mode <= 15; }

bool isLightStyle(int mode) { return mode == 2 || mode == 13; }

bool isModernDenseStyle(int mode) { return mode >= 9; }

int styleColorFamily(int mode) {
    switch (mode) {
        case 2:  case 13: return 2;
        case 9:  case 14: return 9;
        case 10: case 15: return 10;
        case 11: return 11;
        default: return 12;
    }
}

QString loadQssResource(const QString& path) {
    QFile qssFile(path);
    if (!qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "iGameVis: failed to load style sheet" << path;
        return QString();
    }
    return QString::fromUtf8(qssFile.readAll());
}

struct FloatingCardPalette {
    const char* backdrop;
    const char* cardBg;
    const char* cardBorder;
};

FloatingCardPalette floatingCardPalette(int mode) {
    switch (styleColorFamily(mode)) {
        case 2:  return { "#EAEEF3", "#F1F3F7", "#CBD2DC" };
        case 9:  return { "#121316", "#22262C", "#31363D" };
        case 10: return { "#14161A", "#20242A", "#2C3038" };
        default: return { "#1E1E1E", "#1E1E1E", "#2D2D30" };
    }
}
}

// 「数据转换」：**就地**转换当前帧挂载的数据，转换完模型树里仍然只有这一个模型——
// 不新增行，而是把该模型改名成转换后的名字（`原名[_fN]_PointData/_CellData`）。
// 说明：
//   - 目标：复合模型（PVD 等）= 当前挂载的子块（即当前帧，不是第一帧）；普通模型 = 自身；
//   - 转换是就地的，所以原来那份数据本身已经变成点/单元数据（不是独立副本）；
//   - 转换后同步刷新属性行的挂载类型图标与画面（否则看起来像"没反应"）。
int igQtMainWindow::createConvertedFrameModel(bool toPointData, QString& reason, QStringList& createdNames) {
    using namespace iGame;
    reason.clear();
    createdNames.clear();

    if (rendererWidget == nullptr || modelTreeWidget == nullptr) {
        reason = QStringLiteral("界面尚未就绪");
        return 0;
    }
    auto scene = rendererWidget->GetScene();
    auto model = scene ? scene->GetCurrentModel() : nullptr;
    if (model == nullptr || model->GetDataObject() == nullptr) {
        reason = QStringLiteral("请先加载模型");
        return 0;
    }
    auto top = model->GetDataObject();

    // 1) 目标：复合模型（PVD 等）→ 当前挂载的子块，也就是“当前帧”；普通模型 → 它自己
    std::vector<DataObject::Pointer> targets;
    if (top->HasSubDataObject()) {
        for (auto it = top->SubDataObjectIteratorBegin();
             it != top->SubDataObjectIteratorEnd(); ++it) {
            auto sub = it->second;
            if (sub == nullptr) { continue; }
            if (sub->GetPoints() == nullptr || sub->GetCellArray() == nullptr) { continue; }
            targets.push_back(sub);
        }
    } else if (top->GetPoints() != nullptr && top->GetCellArray() != nullptr) {
        targets.push_back(top);
    }
    if (targets.empty()) {
        reason = QStringLiteral("当前帧没有带网格数据的对象（子块可能尚未加载完成），无法转换。");
        return 0;
    }

    // Invalidate the C/S snapshot before editing shared mesh arrays.
    // Keep the visible model; only its reusable remote cache entry expires.
    if (fileLoader) {
        fileLoader->InvalidateRemoteMemoryCache(QStringLiteral("In-place data conversion"));
    }

    // 2) 就地转换（共享模式下，转换结果就属于这份数据本身）
    int convertedAttrs = 0;
    int convertFailed = 0;
    int noCandidates = 0;
    for (auto& target : targets) {
        const auto res = ConvertDataObjectInPlace(target, toPointData);
        if (res.convertedAttrs <= 0) {
            if (res.filterFailed) { ++convertFailed; } else { ++noCandidates; }
            continue;
        }
        convertedAttrs += res.convertedAttrs;
    }
    if (convertedAttrs <= 0) {
        if (convertFailed > 0) {
            reason = QStringLiteral("转换 filter 执行失败，详见日志。");
        } else if (noCandidates > 0) {
            reason = toPointData ? QStringLiteral("当前帧没有单元属性（该帧只有点属性），无需转换。")
                                 : QStringLiteral("当前帧没有点属性，无需转换。");
        } else {
            reason = QStringLiteral("当前帧没有可转换的数据。");
        }
        return 0;
    }

    // 3) 复合模型：父容器上的属性是各子块属性的“占位登记”，同步挂载类型并重算值域
    if (top->HasSubDataObject()) {
        if (auto parentAttrs = top->GetAttributeSet()) {
            for (int i = 0; i < parentAttrs->GetNumberOfAttributes(); ++i) {
                auto& parentAttr = parentAttrs->GetAttribute(i);
                if (parentAttr.isDeleted || parentAttr.pointer == nullptr) { continue; }
                const std::string name = parentAttr.pointer->GetName();
                for (auto it = top->SubDataObjectIteratorBegin();
                     it != top->SubDataObjectIteratorEnd(); ++it) {
                    auto sub = it->second;
                    if (sub == nullptr) { continue; }
                    auto subAttrs = sub->GetAttributeSet();
                    if (subAttrs == nullptr) { continue; }
                    const int subIndex = subAttrs->GetAttributeIndex(name);
                    if (subIndex < 0) { continue; }
                    parentAttr.attachmentType = subAttrs->GetAttribute(subIndex).attachmentType;
                    break;
                }
            }
        }
        top->ReCollectSubDataObjectDataRange();
        top->UpdateSubDataObjectDataRange();
        if (auto drawObject = DynamicCast<DrawObject>(top)) {
            drawObject->ForceReConvertToDrawableData();
        }
    }
    // 原行的属性图标/提示按新挂载类型就地刷新（只改图标与提示，不重建行，不会丢子块行）
    modelTreeWidget->refreshAttributeBadges(top);

    // 4) 模型树里只保留这一个模型：把该模型改名成转换后的名字（含发生转换的帧号）
    int frameIndex = 0;
    int frameCount = 1;
    if (auto frames = top->PeekTimeFrames()) {
        frameCount = static_cast<int>(frames->GetTimeNum());
        if (frameCount > 0) {
            frameIndex = ui->widget_Animation != nullptr ? ui->widget_Animation->currentFrameIndex() : 0;
            frameIndex = std::max(0, std::min(frameIndex, frameCount - 1));
        }
    }
    // 名字规则：基准名 + [_f<帧号>] + _PointData/_CellData。
    // 之前已经转换过的话，名字里已经带了这两种后缀，这里先剥干净再拼，否则每转一次都会
    // 多叠一段（`1_f1_PointData` → `1_f1_f12_CellData` → …）。
    QString baseName = QString::fromStdString(top->GetName());
    bool hadDataTypeSuffix = false;
    for (const auto& suffix : {QStringLiteral("_PointData"), QStringLiteral("_CellData")}) {
        if (baseName.endsWith(suffix)) {
            baseName.chop(suffix.size());
            hadDataTypeSuffix = true;
            break;
        }
    }
    // 只有名字确实带过我们加的数据类型后缀时才剪掉结尾的 `_f<数字>`，
    // 免得误伤本来就叫 `xxx_f12` 的文件名。
    if (hadDataTypeSuffix) {
        const int underscore = baseName.lastIndexOf(QLatin1Char('_'));
        if (underscore > 0 && underscore + 2 < baseName.size() &&
            baseName.at(underscore + 1) == QLatin1Char('f')) {
            bool digitsOnly = true;
            for (int i = underscore + 2; i < baseName.size(); ++i) {
                if (!baseName.at(i).isDigit()) {
                    digitsOnly = false;
                    break;
                }
            }
            if (digitsOnly) { baseName.chop(baseName.size() - underscore); }
        }
    }
    if (baseName.isEmpty()) { baseName = QStringLiteral("Model"); }
    QString convertedName = baseName;
    if (frameCount > 1) { convertedName += QStringLiteral("_f%1").arg(frameIndex + 1); }
    convertedName += toPointData ? QStringLiteral("_PointData") : QStringLiteral("_CellData");

    const QString finalName = modelTreeWidget->renameModelRow(top, convertedName);
    createdNames << finalName;
    qDebug() << "Convert in place, model renamed to:" << finalName << "converted attrs:" << convertedAttrs
             << (toPointData ? "ToPointData" : "ToCellData");
    rendererWidget->update();
    return static_cast<int>(createdNames.size());
}

igQtMainWindow::igQtMainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    qApp->setStyleSheet(qApp->styleSheet() + QString::fromUtf8(kGlobalSpinBoxDarkQss));

    m_originalStyleSheet = this->styleSheet();

    QSettings settings(QStringLiteral("iGame"), QStringLiteral("iGameVis"));
    const int savedMode = settings.value(QStringLiteral("ui/styleMode"), kFallbackStyleMode).toInt();
    m_styleMode = normalizeStyleMode(qBound(0, savedMode, 15));
    igQtRenderWidget::setGlobalStyleMode(m_styleMode);
    const QString initialQss = styleSheetForMode(m_styleMode);
    if (!initialQss.isEmpty()) {
        this->setStyleSheet(initialQss);
    } else {
        m_styleMode = kFallbackStyleMode;
    }

    // 设置窗口标题为iGameVis
    this->setWindowTitle("iGameVis");
    // 使用无边框窗口并自定义标题栏
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::Window | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint);
    initCustomTitleBar();
    initAllUnDefinedComponents();
    if (isFloatingCardStyle(m_styleMode)) applyFloatingCards(true);
    UpdateIcons();
    initAllComponents();
    initAllFilters();

    initAllSources();
    initAllInteractor();
    updateRecentFilePaths();
    initToolbarComponent();  // 内部会重建 toolBar_4 的 3×2 轴网格 + 4 组「按钮行+标题」容器 + 单排宽度拟合

    updateRecentFilePaths();


    connect(modelTreeWidget, &igQtModelDialogWidget::Update, rendererWidget, &igQtRenderWidget::update);

    connect(modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged, this, [this]() {
        if (!m_projectChip) return;
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        auto model = scene ? scene->GetCurrentModel() : nullptr;
        if (model && model->GetDataObject()) {
            std::string name = model->GetDataObject()->GetName();
            m_projectChip->setText(QString::fromStdString(name.empty() ? "模型" : name));
        } else {
            m_projectChip->setText(QStringLiteral("iGameVis"));
        }
    });

    // 初始化命令管理器并建立与 MCP Tool Server 的连接
    commandManager = new igQtCommandManager(this);
    if (!commandManager->startConnection("localhost", 12345)) {
        qWarning() << "iGameVis 与 MCP Tool Server 连接失败！";
    }

    ThreadPool::Instance();
}

void igQtMainWindow::initCustomTitleBar() {
    if (m_titleBar) return;

    m_titleBar = new QWidget(this);
    m_titleBar->setObjectName("CustomTitleBar");
    m_titleBar->setAttribute(Qt::WA_StyledBackground, true);
    m_titleBar->setFixedHeight(50);
    // 标题栏 QSS 见 iGameQtMainWindow.ui 中 MainWindow.styleSheet（QWidget#CustomTitleBar 等）

    auto* mainLayout = new QVBoxLayout(m_titleBar);
    mainLayout->setContentsMargins(8, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 顶部一行：图标 + 标题 + 按钮
    QWidget* topRow = new QWidget(m_titleBar);
    auto* topLayout = new QHBoxLayout(topRow);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(4);

    m_logoIconLabel = new QLabel(topRow);
    m_logoIconLabel->setObjectName(QStringLiteral("AppLogoLabel"));
    m_logoIconLabel->setFixedSize(28, 28);
    m_logoIconLabel->setAttribute(Qt::WA_StyledBackground, true);
    QPixmap pm = windowIcon().pixmap(18, 18);
    m_logoIconLabel->setPixmap(pm);
    m_logoIconLabel->setScaledContents(true);
    m_logoIconLabel->setAlignment(Qt::AlignCenter);

    m_brandBox = new QWidget(topRow);
    m_brandBox->setObjectName(QStringLiteral("TitleBrandBox"));
    m_brandBox->setAttribute(Qt::WA_StyledBackground, true);
    auto* brandLayout = new QHBoxLayout(m_brandBox);
    brandLayout->setContentsMargins(5, 2, 10, 2);
    brandLayout->setSpacing(6);
    brandLayout->addWidget(m_logoIconLabel);

    m_titleLabel = new QLabel(m_brandBox);
    m_titleLabel->setObjectName(QStringLiteral("CustomTitleLabel"));
    m_titleLabel->setText(this->windowTitle());
    brandLayout->addWidget(m_titleLabel);

    topLayout->addWidget(m_brandBox, 0, Qt::AlignVCenter);

    m_topMenuLayout = topLayout;
    if (ui->menuBar) {
        ui->menuBar->setParent(topRow);
        // A reparented QMenuBar can keep the designer's full-window size hint.
        // Reserve just enough title-bar space for the four top-level menus;
        // otherwise Qt compresses every entry into the barely visible » menu.
        ui->menuBar->setFixedWidth(360);
        ui->menuBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        ui->menuBar->setFixedHeight(28);
        ui->menuBar->show();
        m_topMenuLayout->insertWidget(1, ui->menuBar, 0, Qt::AlignVCenter);
    }

    topLayout->addStretch(1);

    m_projectChip = new QLabel(QStringLiteral("iGameVis"), topRow);
    m_projectChip->setObjectName(QStringLiteral("ProjectChip"));
    m_projectChip->setAttribute(Qt::WA_StyledBackground, true);
    m_projectChip->setFixedHeight(30);
    m_projectChip->setAlignment(Qt::AlignCenter);
    topLayout->addWidget(m_projectChip, 0);

    // 按钮区域（尺寸与 Windows 标题栏按钮比例相近：较宽、易点）
    m_btnMinimize = new QPushButton(topRow);
    m_btnMinimize->setObjectName(QStringLiteral("MinimizeButton"));
    m_btnMaximize = new QPushButton(topRow);
    m_btnMaximize->setObjectName(QStringLiteral("MaximizeButton"));
    m_btnClose = new QPushButton(QStringLiteral("×"), topRow);
    m_btnClose->setObjectName(QStringLiteral("CloseButton"));

    m_styleToggleButton = new QPushButton(topRow);
    m_styleToggleButton->setObjectName(QStringLiteral("StyleToggleButton"));
    m_styleToggleButton->setCursor(Qt::PointingHandCursor);
    m_styleToggleButton->setToolTip(QStringLiteral("切换界面风格"));

    const QSize captionBtnSize(46, 30);
    const QSize styleToggleSize(116, 30);
    m_btnMinimize->setFixedSize(captionBtnSize);
    m_btnMaximize->setFixedSize(captionBtnSize);
    m_btnClose->setFixedSize(captionBtnSize);
    m_styleToggleButton->setFixedSize(styleToggleSize);

    m_btnMinimize->setIcon(QIcon(QStringLiteral(":/Ticon/Icons/window_minimize_white.svg")));
    m_btnMinimize->setIconSize(QSize(12, 12));
    m_btnMaximize->setIconSize(QSize(12, 12));
    m_btnMaximize->setText(QString());
    m_btnMaximize->setFlat(true);
    m_btnMinimize->setFlat(true);
    m_btnClose->setFlat(true);

    topLayout->addWidget(m_styleToggleButton, 0);

    m_rightDivider = new QFrame(topRow);
    m_rightDivider->setObjectName(QStringLiteral("RightDivider"));
    m_rightDivider->setFixedSize(1, 24);
    m_rightDivider->setFrameShape(QFrame::NoFrame);
    m_rightDivider->setAttribute(Qt::WA_StyledBackground, true);
    topLayout->addWidget(m_rightDivider, 0);

    topLayout->addWidget(m_btnMinimize, 0);
    topLayout->addWidget(m_btnMaximize, 0);
    topLayout->addWidget(m_btnClose, 0);

    // 添加顶部行到主布局
    mainLayout->addWidget(topRow, 0);

    m_titleAccentLine = new QFrame(m_titleBar);
    m_titleAccentLine->setObjectName(QStringLiteral("TitleBarAccentLine"));
    m_titleAccentLine->setFixedHeight(2);
    m_titleAccentLine->setFrameShape(QFrame::NoFrame);
    m_titleAccentLine->setAttribute(Qt::WA_StyledBackground, true);
    mainLayout->addWidget(m_titleAccentLine, 0);

    // 放到 QMainWindow 的菜单栏区域，相当于自定义标题栏
    this->setMenuWidget(m_titleBar);

    // 拖动事件用 eventFilter 处理（只对标题栏整体和标题文本生效，不干扰按钮点击）
    m_titleBar->installEventFilter(this);
    m_titleLabel->installEventFilter(this);

    // 按钮功能
    connect(m_btnMinimize, &QPushButton::clicked, this, [this]() {
        minimizeWithAnimation();
    });

    connect(m_btnMaximize, &QPushButton::clicked, this, [this]() {
        toggleMaximizeRestore();
    });

    connect(m_btnClose, &QPushButton::clicked, this, [this]() {
        this->close();
    });

    createStyleMenu();

    applyStyleMode(m_styleMode);

    // 监听全局鼠标释放，防止拖动状态在某些场景下卡住
    qApp->installEventFilter(this);
    updateMaximizeButtonIcon();
}

bool igQtMainWindow::eventFilter(QObject* watched, QEvent* event) {
    if (!m_titleBar) return QMainWindow::eventFilter(watched, event);

    if (isFloatingCardStyle(m_styleMode) && event->type() == QEvent::Resize) {
        if (watched == rendererWidget) {
            applyMainWindowRoundedMask(rendererWidget, 8);
        } else if (watched == m_floatingTreeDock) {
            const int radius = (modelTreeWidget && modelTreeWidget->isTreeDockCollapsed()) ? 12 : 8;
            applyMainWindowRoundedMask(m_floatingTreeDock, radius);
        }
    }

    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease) {
        const QVariant btnProp = watched->property("igToolbarButton");
        if (btnProp.isValid()) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                if (event->type() == QEvent::MouseButtonRelease) {
                    auto* btn = qobject_cast<QToolButton*>(btnProp.value<QObject*>());
                    auto* host = qobject_cast<QWidget*>(watched);
                    if (btn && host && host->rect().contains(me->pos())) { btn->click(); }
                }
                return true;
            }
        }
    }

    // 全局兜底：只要左键释放就结束拖动，避免窗口“黏在鼠标上”
    if (m_titleBarDragging) {
        if (event->type() == QEvent::MouseButtonRelease) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                m_titleBarDragging = false;
            }
        } else if (event->type() == QEvent::WindowDeactivate) {
            m_titleBarDragging = false;
        }
    }

    // 按钮自身的事件交给 Qt 处理，保证 clicked() 能正常触发
    if (qobject_cast<QPushButton*>(watched)) {
        return QMainWindow::eventFilter(watched, event);
    }

    // 只对标题栏本身或标题文本处理拖动，不拦截按钮
    if (watched == m_titleBar || watched == m_titleLabel) {
        switch (event->type()) {
            case QEvent::MouseButtonPress: {
                auto* me = static_cast<QMouseEvent*>(event);
                if (me->button() == Qt::LeftButton) {
                    m_titleBarDragging = true;
                    m_dragOffset = me->globalPos() - frameGeometry().topLeft();
                    return true;
                }
                break;
            }
            case QEvent::MouseMove: {
                auto* me = static_cast<QMouseEvent*>(event);
                if (m_titleBarDragging && (me->buttons() & Qt::LeftButton)) {
                    if (isMaximized()) {
                        const qreal ratioX = qBound<qreal>(0.0, static_cast<qreal>(me->pos().x()) / qMax(1, m_titleBar->width()), 1.0);
                        showNormal();
                        const int newX = me->globalPos().x() - static_cast<int>(width() * ratioX);
                        const int newY = me->globalPos().y() - m_titleBar->height() / 2;
                        m_dragOffset = me->globalPos() - QPoint(newX, newY);
                        move(newX, newY);
                        return true;
                    }
                    move(me->globalPos() - m_dragOffset);
                    return true;
                }
                if (!(me->buttons() & Qt::LeftButton)) {
                    m_titleBarDragging = false;
                }
                break;
            }
            case QEvent::MouseButtonDblClick: {
                auto* me = static_cast<QMouseEvent*>(event);
                if (me->button() == Qt::LeftButton) {
                    toggleMaximizeRestore();
                    return true;
                }
                break;
            }
            case QEvent::MouseButtonRelease: {
                auto* me = static_cast<QMouseEvent*>(event);
                if (me->button() == Qt::LeftButton) {
                    m_titleBarDragging = false;
                    return true;
                }
                break;
            }
            default:
                break;
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void igQtMainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        updateMaximizeButtonIcon();
    }
    QMainWindow::changeEvent(event);
}

void igQtMainWindow::minimizeWithAnimation() {
    if (m_isMinimizing || isMinimized()) {
        return;
    }

    m_isMinimizing = true;
    m_geometryBeforeMinimize = geometry();

    const QRect startRect = m_geometryBeforeMinimize;
    const QPoint center = startRect.center();
    const int endW = qMax(20, startRect.width() / 8);
    const int endH = qMax(20, startRect.height() / 8);
    const QRect endRect(center.x() - endW / 2, center.y() - endH / 2, endW, endH);

    auto* anim = new QPropertyAnimation(this, "geometry");
    anim->setDuration(160);
    anim->setStartValue(startRect);
    anim->setEndValue(endRect);
    anim->setEasingCurve(QEasingCurve::InCubic);

    connect(anim, &QPropertyAnimation::finished, this, [this, anim]() {
        this->showMinimized();
        this->setGeometry(m_geometryBeforeMinimize);
        m_isMinimizing = false;
        anim->deleteLater();
    });

    anim->start();
}

void igQtMainWindow::toggleMaximizeRestore() {
    if (isMaximized()) {
        if (m_isRestoringFromMaximized) {
            return;
        }

        m_isRestoringFromMaximized = true;
        QRect targetRect = m_normalGeometry;
        if (!targetRect.isValid() || targetRect.width() < 100 || targetRect.height() < 100) {
            QRect workArea = QGuiApplication::primaryScreen()->availableGeometry();
            targetRect = QRect(workArea.x() + workArea.width() / 10,
                               workArea.y() + workArea.height() / 10,
                               workArea.width() * 8 / 10,
                               workArea.height() * 8 / 10);
        }

        QRect startRect = QGuiApplication::primaryScreen()->availableGeometry();
        if (windowHandle() && windowHandle()->screen()) {
            startRect = windowHandle()->screen()->availableGeometry();
        }

        showNormal();
        setGeometry(startRect);

        auto* anim = new QPropertyAnimation(this, "geometry");
        anim->setDuration(170);
        anim->setStartValue(startRect);
        anim->setEndValue(targetRect);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        connect(anim, &QPropertyAnimation::finished, this, [this, anim]() {
            m_isRestoringFromMaximized = false;
            updateMaximizeButtonIcon();
            anim->deleteLater();
        });
        anim->start();
    } else {
        m_normalGeometry = geometry();
        showMaximized();
        updateMaximizeButtonIcon();
    }
}

void igQtMainWindow::updateMaximizeButtonIcon() {
    if (!m_btnMaximize) return;
    const bool light = isLightStyle(m_styleMode);
    const QString restoreIcon = light ? QStringLiteral(":/Ticon/Icons/window_restore_dark.svg")
                                      : QStringLiteral(":/Ticon/Icons/window_restore_white.svg");
    const QString maximizeIcon = light ? QStringLiteral(":/Ticon/Icons/window_maximize_dark.svg")
                                       : QStringLiteral(":/Ticon/Icons/window_maximize_white.svg");
    m_btnMaximize->setIcon(QIcon(isMaximized() ? restoreIcon : maximizeIcon));
    m_btnMaximize->setIconSize(isMaximized() ? QSize(15, 15) : QSize(12, 12));
    m_btnMaximize->setText(QString());
}

void igQtMainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    // 立刻决定要不要换行（视觉上跟手）；同时 100ms 防抖跑一次全量重排（含 iconSize 重算）
    relayoutToolbarWrappers();
    if (m_ResizeDebounceTimer) { m_ResizeDebounceTimer->start(100); }
    updateViewRailPosition();
}

igQtMainWindow::~igQtMainWindow() {
    // Release independent cache ownership before QObject destroys the renderer.
    if (fileLoader) {
        fileLoader->CancelRemotePackage();
        fileLoader->InvalidateRemoteMemoryCache(QStringLiteral("Client shutdown"));
    }
    // 清理命令管理器
    if (commandManager) {
        commandManager->stopConnection();
        delete commandManager;
        commandManager = nullptr;
    }
}
void igQtMainWindow::initArgs(const QStringList& args) {
    int argc = args.size();
    QString remotePackage;
    QString remoteHost = QStringLiteral("127.0.0.1");
    quint16 remotePort = 34567;
    QString remoteCache = QStringLiteral("D:/iGameVis-cs-cache");
    int cacheRepeat = 1;
    QString cacheBenchmarkOutput;
    const bool cpuPreload = args.contains(QStringLiteral("--remote-cpu-preload-first")) ||
                            args.contains(QStringLiteral("--remote-cpu-preload-only"));
    const bool residentCache = !args.contains(QStringLiteral("--remote-no-memory-cache"));
    if (cpuPreload && (!residentCache || args.contains(QStringLiteral("--remote-cache-retain-gpu")))) {
        igError("CPU preload requires memory caching and cannot be combined with --remote-cache-retain-gpu");
        return;
    }
    fileLoader->SetRemoteMemoryCacheEnabled(residentCache);
    fileLoader->SetRemoteCpuOnlyCacheEnabled(!args.contains(QStringLiteral("--remote-cache-retain-gpu")));
    fileLoader->SetRemoteCacheStrictValidation(
            args.contains(QStringLiteral("--remote-cache-repeat")) ||
            args.contains(QStringLiteral("--remote-cache-benchmark-json")));
    for (int i = 1; i < argc; ++i) {
        const QString& cur_arg = args[i].toLower();
        if (cur_arg == "--filepath" && ++i < argc) {
            const QString& filePath = args[i];
            const QByteArray utf8Path = filePath.toUtf8();
            fileLoader->OpenFile(std::string(utf8Path.constData(), static_cast<std::size_t>(utf8Path.size())));
        } else if (cur_arg == "--remote-package" && ++i < argc) {
            remotePackage = args[i];
        } else if (cur_arg == "--remote-host" && ++i < argc) {
            remoteHost = args[i];
        } else if (cur_arg == "--remote-port" && ++i < argc) {
            bool ok = false;
            const uint value = args[i].toUInt(&ok);
            if (ok && value > 0 && value <= 65535) {
                remotePort = static_cast<quint16>(value);
            } else {
                igError("[PackageTransfer] Invalid --remote-port value: {}",
                        args[i].toStdString());
                return;
            }
        } else if (cur_arg == "--remote-cache" && ++i < argc) {
            remoteCache = args[i];
        } else if (cur_arg == "--remote-cpu-cache-limit-gib" && ++i < argc) {
            bool ok = false;
            const quint64 gib = args[i].toULongLong(&ok);
            if (!ok || gib < 1 || gib > 4096) { igError("Invalid CPU cache limit (1..4096 GiB)"); return; }
            fileLoader->SetRemoteMemoryCacheLimitBytes(gib * 1024ull * 1024 * 1024);
        } else if (cur_arg == "--remote-cache-repeat" && ++i < argc) {
            bool ok = false;
            cacheRepeat = args[i].toInt(&ok);
            if (!ok || cacheRepeat < 1 || cacheRepeat > 20) {
                igError("[RemoteOpenBenchmark] --remote-cache-repeat must be in [1,20]");
                return;
            }
        } else if (cur_arg == "--remote-cache-benchmark-json" && ++i < argc) {
            cacheBenchmarkOutput = args[i];
        }
    }
    if (residentCache) {
        // Static-model mode is conservative around application editing tools.
        // Raw-pointer edits outside these paths must explicitly invalidate the cache.
        connect(ui->menu_filters, &QMenu::triggered, this, [this](QAction*) {
            fileLoader->InvalidateRemoteMemoryCache(QStringLiteral("Algorithm action entered"));
        });
        connect(ui->menu_clip, &QMenu::triggered, this, [this](QAction*) {
            fileLoader->InvalidateRemoteMemoryCache(QStringLiteral("Clipping action entered"));
        });
        for (QAction* action : {ui->action_deformation, ui->action_SelectView,
                                ui->action_AiChat,
                                ui->action_ExportAnimation}) {
            connect(action, &QAction::triggered, this, [this, action]() {
                fileLoader->InvalidateRemoteMemoryCache(QStringLiteral("Editing/tool action: ") + action->objectName());
            });
        }
    }
    if (!remotePackage.isEmpty()) {
        if (residentCache && (cpuPreload || cacheRepeat > 1 || !cacheBenchmarkOutput.isEmpty())) {
            ConfigureRemoteCacheBenchmark(remotePackage, remoteHost, remotePort, remoteCache,
                                           cacheRepeat, cacheBenchmarkOutput);
        } else {
            fileLoader->OpenRemotePackage(remoteHost, remotePort, remotePackage, remoteCache);
        }
    }
}
void igQtMainWindow::initAllUnDefinedComponents() {
    rendererWidget = new igQtModelDrawWidget(this);
    igQtOpenGLManager::Instance()->setQtRenderWidget(rendererWidget);
    //    rendererWidget->setParent(this);
    fileLoader = new igQtFileLoader(this);
    fileLoader->SetRemoteCacheRenderWidget(rendererWidget);
    remoteModelLibrary = new igQtRemoteModelLibrary(fileLoader, this);
    remoteModelLibrary->hide();
    this->setCentralWidget(rendererWidget);
    this->ColorManagerWidget = new igQtColorManagerWidget;
    ColorManagerWidget->setGeometry(400, 500, 780, 1000);

    // 初始化AI聊天DockWidget
    aiChatDockWidget = new QDockWidget(this);
    aiChatDockWidget->setWindowTitle("AI聊天助手");
    aiChatWidget = new igQtAiChatWidget(aiChatDockWidget, this);
    aiChatDockWidget->setWidget(aiChatWidget);
    aiChatDockWidget->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    // AI 聊天窗口：不允许拖动/悬浮（只保留可关闭）
    aiChatDockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    aiChatDockWidget->hide(); // 初始隐藏
    this->addDockWidget(Qt::RightDockWidgetArea, aiChatDockWidget);

    // 设置DockWidget的默认大小
    aiChatDockWidget->resize(400, 600);

    // 将原本右侧的 dockwidget 移到左侧（后续统一加入左侧 tab 组）
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_ScalarField);
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_VectorField);
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_FlowField);
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_TensorField);
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_ParallelCoordinatesField);
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_VariableCorrelationField);
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_VariableDensityField);
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_DataChangeField);
    // SelectionField 改為停靠在左側，並放在 Properties 視窗上方
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_SelectionField);
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_ContextPreservingShowField);
    this->addDockWidget(Qt::RightDockWidgetArea, ui->dockWidget_SearchInfo);
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_QualityDetection);
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_EditMode);
    this->addDockWidget(Qt::BottomDockWidgetArea, ui->dockWidget_Animation);
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_ModelList);
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_ContourExtract);
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_MedianFilter);

    // 禁止所有 dock 悬浮：去掉 DockWidgetFloatable
    // 同时为了防止“拖拽标题栏就被扯成系统浮动窗”，这里也把 Movable 去掉（只保留可关闭）。
    // 如果你仍希望允许在 dock 区域内重新排列位置，可以把 DockWidgetMovable 加回去，但必须保持不包含 DockWidgetFloatable。
    ui->dockWidget_ScalarField->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_VectorField->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_FlowField->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_TensorField->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_ParallelCoordinatesField->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_VariableCorrelationField->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_VariableDensityField->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_DataChangeField->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_SelectionField->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_ContextPreservingShowField->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_SearchInfo->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_QualityDetection->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_EditMode->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_Animation->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_ModelList->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_ContourExtract->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_MedianFilter->setFeatures(QDockWidget::DockWidgetClosable);

    QDockWidget* dockWidget_null = new QDockWidget("", this);
    this->addDockWidget(Qt::RightDockWidgetArea, dockWidget_null);
    dockWidget_null->hide();
    ui->dockWidget_ScalarField->hide();
    ui->dockWidget_VectorField->hide();
    ui->dockWidget_FlowField->hide();
    ui->dockWidget_TensorField->hide();
    ui->dockWidget_ParallelCoordinatesField->hide();
    ui->dockWidget_VariableCorrelationField->hide();
    ui->dockWidget_VariableDensityField->hide();
    ui->dockWidget_DataChangeField->hide();
    ui->dockWidget_SelectionField->hide();
    ui->dockWidget_ContextPreservingShowField->hide();
    ui->dockWidget_SearchInfo->hide();
    ui->dockWidget_QualityDetection->hide();
    ui->dockWidget_EditMode->hide();
    ui->dockWidget_Animation->hide();
    ui->dockWidget_ModelList->hide();
    ui->dockWidget_ContourExtract->hide();
    ui->dockWidget_MedianFilter->hide();
    // Setup default GUI layout.
    // 启用左侧区域的 tab 功能，使左侧 dockwidget 可以通过 tab 切换
    this->setTabPosition(Qt::LeftDockWidgetArea, QTabWidget::North);
    this->setTabPosition(Qt::RightDockWidgetArea, QTabWidget::North);
    //this->setTabPosition(Qt::BottomDockWidgetArea, QTabWidget::North);
    // Set up the dock window corners to give the vertical docks more room.
    this->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    this->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    modelTreeWidget = new igQtModelDialogWidget(this);

    // 初始化表面平滑参数面板
    SmoothDockWidget = igQtSmoothWidget::createDockWidget(this);
    SmoothWidget = qobject_cast<igQtSmoothWidget*>(SmoothDockWidget->widget());
    this->addDockWidget(Qt::RightDockWidgetArea, SmoothDockWidget);
    SmoothDockWidget->resize(400, 340);
    SmoothDockWidget->hide();
    connect(SmoothWidget, &igQtSmoothWidget::cancelRequested, SmoothDockWidget, &QDockWidget::hide);
    connect(SmoothWidget, &igQtSmoothWidget::smoothGenerated, this,
            [this](iGame::DataObject::Pointer output) {
                if (!output) return;
                output->SetName(QStringLiteral("Smooth_%1").arg(++m_smoothCount).toStdString());
                modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
                rendererWidget->update();
            });

    auto makeWidgetScrollable = [&](QWidget* content, QWidget* parent) -> QWidget* {
        if (!content) return nullptr;
        if (qobject_cast<QScrollArea*>(content)) return content;
        content->setMinimumHeight(0);
        content->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        auto* scroll = new QScrollArea(parent);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scroll->setWidget(content);
        return scroll;
    };

    auto makeDockWidgetScrollable = [&](QDockWidget* dock) {
        if (!dock) return;
        QWidget* content = dock->widget();
        if (!content || qobject_cast<QScrollArea*>(content)) return;
        dock->setWidget(makeWidgetScrollable(content, dock));
    };

    // 左侧「工具面板」：QTabWidget 内按需加入各面板；下方 Properties 常驻
    m_leftFieldDock = new QDockWidget(this);
    m_leftFieldDock->setObjectName("LeftFieldDock");
    m_leftFieldDock->setWindowTitle(QStringLiteral("工具面板"));
    m_leftFieldDock->setAllowedAreas(Qt::LeftDockWidgetArea);
    m_leftFieldDock->setFeatures(QDockWidget::DockWidgetClosable);
    m_leftFieldTabs = new QTabWidget(m_leftFieldDock);
    m_leftFieldTabs->setObjectName("LeftFieldTabs");
    m_leftFieldTabs->setTabPosition(QTabWidget::North);
    m_leftFieldTabs->setDocumentMode(true);
    m_leftFieldTabs->setTabsClosable(true);
    connect(m_leftFieldTabs, &QTabWidget::tabCloseRequested, this, &igQtMainWindow::onLeftToolTabCloseRequested);
    m_leftFieldDock->setWidget(m_leftFieldTabs);
    this->addDockWidget(Qt::LeftDockWidgetArea, m_leftFieldDock);

    // 属性窗口停靠在左侧（常驻），图层树悬浮在 OpenGL 右下角
    this->addDockWidget(Qt::LeftDockWidgetArea, modelTreeWidget->getPropertiesDock());
    // 上方为工具 Tab，下方为 Properties
    this->splitDockWidget(m_leftFieldDock,
                          modelTreeWidget->getPropertiesDock(),
                          Qt::Vertical);
    modelTreeWidget->getPropertiesDock()->show();
    // 无 Tab 时不占工具区（仅 Properties 可见）
    m_leftFieldDock->hide();
    // 将其他 dockwidget 以 SelectionField 为基准组织成上方的 tab 组
    this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_ParallelCoordinatesField);
    this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_VariableCorrelationField);
    this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_VariableDensityField);
    this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_DataChangeField);
    this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_ContextPreservingShowField);
    this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_QualityDetection);
    this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_EditMode);
    this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_ModelList);
    // 轮廓提取 / 网格切面 / 结构形变 改由左侧「工具面板」Tab 按需打开，不再叠在 Selection 组

    // 左侧扩展面板统一采用可滚动内容，避免 dock 过多时撑高主窗口
    makeDockWidgetScrollable(ui->dockWidget_SelectionField);
    makeDockWidgetScrollable(ui->dockWidget_ParallelCoordinatesField);
    makeDockWidgetScrollable(ui->dockWidget_VariableCorrelationField);
    makeDockWidgetScrollable(ui->dockWidget_VariableDensityField);
    makeDockWidgetScrollable(ui->dockWidget_DataChangeField);
    makeDockWidgetScrollable(ui->dockWidget_ContextPreservingShowField);
    makeDockWidgetScrollable(ui->dockWidget_QualityDetection);
    makeDockWidgetScrollable(ui->dockWidget_EditMode);
    makeDockWidgetScrollable(ui->dockWidget_ModelList);
    makeDockWidgetScrollable(ui->dockWidget_ContourExtract);
    makeDockWidgetScrollable(modelTreeWidget->getPropertiesDock());

    // 设置左侧 dock 区域的初始宽度（不锁死，用户仍可拖拽调整）
    QTimer::singleShot(0, this, [this]() {
        if (m_leftFieldDock) {
            const int curW = m_leftFieldDock->width();
            const int targetW = qMax(curW + 40, 320);
            this->resizeDocks({m_leftFieldDock}, {targetW}, Qt::Horizontal);
        }
    });

    QTimer::singleShot(100, this, [this]() {
        if (rendererWidget && modelTreeWidget) {
            if (m_styleMode == 6) {
                applyWorkspaceLayout(true);
            } else if (m_styleMode == 7 || m_styleMode == 8) {
                applyViewRail(true);
                modelTreeWidget->positionTreeDockToRendererCorner(rendererWidget);
            } else {
                modelTreeWidget->positionTreeDockToRendererCorner(rendererWidget);
            }
        }
    });


    SliceDockWidget = new QDockWidget(this);
    SliceDockWidget->setObjectName("dockWidget_Slice");
    SliceDockWidget->setWindowTitle("网格切割");
    SliceWidget = new igQtModelClipWidget(nullptr);
    SliceWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    SliceWidget->setMinimumWidth(300);
    SliceDockWidget->setWidget(SliceWidget);
    SliceDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea);
    SliceDockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    this->addDockWidget(Qt::LeftDockWidgetArea, SliceDockWidget);
    makeDockWidgetScrollable(SliceDockWidget);
    SliceDockWidget->hide();

    DeformationDockWidget = new QDockWidget(this);
    DeformationDockWidget->setWindowTitle("结构形变");
    DeformationWidget = new igQtDeformationWidget(DeformationDockWidget);
    DeformationDockWidget->setWidget(DeformationWidget);
    DeformationDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    DeformationDockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    DeformationDockWidget->hide();
    this->addDockWidget(Qt::RightDockWidgetArea, DeformationDockWidget);
  
    // 保留指定时间步：左侧工具面板
    m_extractTimeStepsShell = new QDockWidget(this);
    m_extractTimeStepsShell->setObjectName("dockWidget_ExtractTimeSteps");
    m_extractTimeStepsShell->setWindowTitle(QStringLiteral("保留指定时间步"));
    m_extractTimeStepsWidget = new igQtExtractTimeStepsWidget(nullptr);
    m_extractTimeStepsWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_extractTimeStepsWidget->setMinimumWidth(280);
    m_extractTimeStepsShell->setWidget(m_extractTimeStepsWidget);
    m_extractTimeStepsShell->setAllowedAreas(Qt::LeftDockWidgetArea);
    m_extractTimeStepsShell->setFeatures(QDockWidget::DockWidgetClosable);
    this->addDockWidget(Qt::LeftDockWidgetArea, m_extractTimeStepsShell);
    makeDockWidgetScrollable(m_extractTimeStepsShell);
    m_extractTimeStepsShell->hide();
    // 高程 (Elevation) 实时参数面板：入口对话框首次执行后 BindSession 绑定并显示，
    // 面板内可实时调整低/高点、按轴铺满包围盒、修改标量范围并应用（交互对齐 ParaView）
    ElevationFilterPanel = new igQtElevationFilterPanel(this);
    this->addDockWidget(Qt::RightDockWidgetArea, ElevationFilterPanel);
    ElevationFilterPanel->hide();
    // 「应用」成功后就地刷新：滤波器复用同一输出对象，模型树不堆叠新节点
    connect(ElevationFilterPanel, &igQtElevationFilterPanel::elevationApplied, this,
            [this](iGame::DataObject::Pointer output) {
        if (!output) return;
        modelTreeWidget->updateAllAttriubute(output);
        if (auto drawObj = DynamicCast<DrawObject>(output)) { drawObj->ForceReConvertToDrawableData(); }
        rendererWidget->update();
    });
    // 参数校验/执行失败时弹提示（与菜单入口一致的暗色无边框提示框）
    connect(ElevationFilterPanel, &igQtElevationFilterPanel::applyFailed, this, [this](const QString& reason) {
        showDarkFramelessMessage(QStringLiteral("高程 (elevation)"), reason);
    });

}
void igQtMainWindow::initToolbarComponent() {
    // 用 QToolButton 行+标题替代 QToolBar（避免 QToolBar 进 layout 导致图标不渲染）
    const int preferredIcon = resolveToolbarIconSizeForWidget(this);
    rebuildToolbarRow(preferredIcon);
    // 宽度拟合 + 换行兜底
    relayoutToolbarWrappers();
}

void igQtMainWindow::initAllComponents() {
    auto* remoteLibraryAction = new QAction(
            ui->action_LoadFile->icon(), QStringLiteral("Remote Model Library..."), this);
    remoteLibraryAction->setObjectName(QStringLiteral("action_RemoteModelLibrary"));
    ui->menu_file->insertAction(ui->menu_RecentFiles->menuAction(), remoteLibraryAction);
    connect(remoteLibraryAction, &QAction::triggered, this, [this]() {
        remoteModelLibrary->show();
        remoteModelLibrary->raise();
        remoteModelLibrary->activateWindow();
    });
    auto* clearRemoteMemory = new QAction(QStringLiteral("Clear C/S Memory Cache"), this);
    clearRemoteMemory->setObjectName(QStringLiteral("action_ClearRemoteMemoryCache"));
    clearRemoteMemory->setToolTip(QStringLiteral(
            "Release the cached dataset; visible models and disk cache files are kept."));
    ui->menu_file->insertAction(ui->menu_RecentFiles->menuAction(), clearRemoteMemory);
    connect(fileLoader, &igQtFileLoader::RemotePackageRunningChanged, clearRemoteMemory,
            [clearRemoteMemory](bool running) { clearRemoteMemory->setEnabled(!running); });
    connect(clearRemoteMemory, &QAction::triggered, this, [this]() {
        fileLoader->InvalidateRemoteMemoryCache(QStringLiteral("User cleared C/S memory cache"));
        statusBar()->showMessage(QStringLiteral(
                "C/S memory cache cleared. Visible models and disk cache files are unchanged."), 7000);
    });

    connect(ui->action_ShowOrientationAxes, &QAction::triggered, this, [&](bool checked){
        iGame::SceneManager::Instance()->GetCurrentScene()->ToggleAxes();
        iGame::SceneManager::Instance()->GetCurrentScene()->Update();
   });
    ui->action_ShowOrientationAxes->setCheckable(true);
    ui->action_ShowOrientationAxes->setChecked(true);
    connect(ui->action_ChangeBackground, &QAction::triggered, this, [&]() {
        igQtChangeBackGroundDialog dialog(this);
        dialog.setWindowTitle("Change BackGround Color.");
      int R = 0, G = 0, B = 0;
      if (dialog.exec() == QDialog::Accepted) {
          auto input = dialog.getInput();
          R = input[0], G = input[1], B = input[2];
          iGame::SceneManager::Instance()->GetCurrentScene()->SetBackGround(R, G, B);
      }
    });
    connect(ui->action_VolumeRendering, &QAction::triggered, this,
            [&](bool toggled) { iGame::SceneManager::Instance()->GetCurrentScene()->SetVolumeRendering(toggled); });
    // init ProgressBar
    progressBarWidget = new igQtProgressBarWidget(this);
    this->statusBar()->addPermanentWidget(progressBarWidget);

    // vortexMetricsLabel
    vortexMetricsLabel = new QLabel(rendererWidget);
    vortexMetricsLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    vortexMetricsLabel->setStyleSheet(
        "QLabel { color: rgb(230,230,230); font-size: 20px; "
        "background: rgba(30,30,30,150); padding: 8px 12px; border-radius: 6px; }");
	    vortexMetricsLabel->hide();

	    connect(ui->action_compress, &QAction::triggered, this, [&](bool checked) {
	        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return false;
	        auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
	        // 支持两种情况：
	        // 1) 单块：当前对象本身是可压缩的 PointSet
	        // 2) 多块：根对象为容器（HasSubDataObject()==true），由 MeshCodecDialog 自动切换到 IGCM + IGC
	        if (!DynamicCast<PointSet>(obj) && !obj->HasSubDataObject()) return false;

	        igQtMeshCodecDialog* d = new igQtMeshCodecDialog(this, obj);
	        d->exec();

	        return true;
	    });

    connect(ui->action_LoadFile, &QAction::triggered, fileLoader, &igQtFileLoader::LoadFile);
    // connect(ui->action_CS, &QAction::triggered, fileLoader, &igQtFileLoader::LoadOnlineS);
    // connect(ui->action_C, &QAction::triggered, fileLoader, &igQtFileLoader::LoadOnlineC);
    connect(ui->action_SaveMeshAs, &QAction::triggered, fileLoader, &igQtFileLoader::SaveFileAs);
    connect(ui->action_SaveMesh, &QAction::triggered, fileLoader, &igQtFileLoader::SaveFileAs);

    //// 添加按钮：将当前标量场移到第一个位置并另存为
    //QAction* action_MoveScalarToFirstAndSave = new QAction("将标量场移到首位并另存为", this);
    //action_MoveScalarToFirstAndSave->setShortcut(QKeySequence()); // 可以设置快捷键
    //ui->menu_help->addAction(action_MoveScalarToFirstAndSave);
    //connect(action_MoveScalarToFirstAndSave, &QAction::triggered, this, [&]() {
    //    // 获取当前场景的当前模型
    //    auto scene = rendererWidget->GetScene();
    //    if (!scene) {
    //        QMessageBox::warning(this, "警告", "当前没有活动场景");
    //        return;
    //    }
    //    auto model = scene->GetCurrentModel();
    //    if (!model) {
    //        QMessageBox::warning(this, "警告", "当前没有活动模型");
    //        return;
    //    }
    //    auto dataObject = model->GetDataObject();
    //    if (!dataObject) {
    //        QMessageBox::warning(this, "警告", "无法获取数据对象");
    //        return;
    //    }
    //
    //    // 获取当前选择的标量场索引
    //    int currentAttributeIndex = dataObject->GetAttributeIndex();
    //    if (currentAttributeIndex < 0) {
    //        QMessageBox::warning(this, "警告", "当前未选择任何标量场");
    //        return;
    //    }
    //
    //    // 获取属性集
    //    auto attributeSet = dataObject->GetAttributeSet();
    //    if (!attributeSet) {
    //        QMessageBox::warning(this, "警告", "无法获取属性集");
    //        return;
    //    }
    //
    //    // 获取所有属性
    //    auto allAttributes = attributeSet->GetAllAttributes();
    //    if (!allAttributes || allAttributes->GetNumberOfElements() == 0) {
    //        QMessageBox::warning(this, "警告", "属性集为空");
    //        return;
    //    }
    //
    //    // 检查索引是否有效
    //    if (currentAttributeIndex >= allAttributes->GetNumberOfElements()) {
    //        QMessageBox::warning(this, "警告", "当前属性索引无效");
    //        return;
    //    }
    //
    //    // 如果已经在第一个位置，直接另存为
    //    if (currentAttributeIndex == 0) {
    //        fileLoader->SaveFileAs();
    //        return;
    //    }
    //
    //    // 创建新的属性数组，将当前属性移到第一个位置
    //    auto newAttributes = ElementArray<AttributeSet::Attribute>::New();
    //    newAttributes->Reserve(allAttributes->GetNumberOfElements());
    //
    //    // 首先添加当前选择的属性
    //    newAttributes->AddElement(allAttributes->GetElement(currentAttributeIndex));
    //
    //    // 然后添加其他属性（跳过当前属性）
    //    for (int i = 0; i < allAttributes->GetNumberOfElements(); i++) {
    //        if (i != currentAttributeIndex) {
    //            newAttributes->AddElement(allAttributes->GetElement(i));
    //        }
    //    }
    //
    //    // 保存当前属性维度
    //    int currentDimension = dataObject->GetAttributeDimension();
    //
    //    // 设置新的属性数组
    //    attributeSet->SetAllAttributes(newAttributes);
    //
    //    // 标记数据对象已修改
    //    dataObject->Modified();
    //
    //    // 如果是 DrawObject，使用 ViewCloudPicture 方法设置新的属性索引为0
    //    auto drawObject = DynamicCast<DrawObject>(dataObject);
    //    if (drawObject) {
    //        drawObject->ViewCloudPicture(scene, 0, currentDimension);
    //    }
    //
    //    // 更新模型树和渲染
    //    modelTreeWidget->updateAllAttriubute(dataObject);
    //    rendererWidget->update();
    //
    //    // 自动触发另存为
    //    fileLoader->SaveFileAs();
    //});
    connect(ui->action_UseOrthographic, &QAction::triggered, this, [&](bool checked) {
        if (ui->action_UseOrthographic->isChecked()) {
            SceneManager::Instance()->GetCurrentScene()->ChangeCameraType(Camera::Type::ORTHOGRAPHIC);
        } else {
            SceneManager::Instance()->GetCurrentScene()->ChangeCameraType(Camera::Type::PERSPECTIVE);
        }
        rendererWidget->update();
    });
    connect(ui->action_ResetCameraView, &QAction::triggered, this, [&]() {
        SceneManager::Instance()->GetCurrentScene()->ResetCameraView();
        rendererWidget->update();
    });

    connect(ui->action_setViewToPositiveX, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->ResetCameraViewToPositiveX();
        rendererWidget->update();
    });
    connect(ui->action_setViewToNegativeX, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->ResetCameraViewToNegativeX();
        rendererWidget->update();
    });
    connect(ui->action_setViewToPositiveY, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->ResetCameraViewToPositiveY();
        rendererWidget->update();
    });
    connect(ui->action_setViewToNegativeY, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->ResetCameraViewToNegativeY();
        rendererWidget->update();
    });
    connect(ui->action_setViewToPositiveZ, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->ResetCameraViewToPositiveZ();
        rendererWidget->update();
    });
    connect(ui->action_setViewToNegativeZ, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->ResetCameraViewToNegativeZ();
        rendererWidget->update();
    });
    connect(ui->action_setViewToIsometric, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->ResetCameraViewToIsometric();
        rendererWidget->update();
    });
    connect(ui->action_ResetViewByBoundingBox, &QAction::triggered, this, [&](bool checked) {
        auto scene = rendererWidget->GetScene();
        if (scene == nullptr) return;
        auto interactor = scene->GetInteractor();
        if (interactor == nullptr) return;
        auto basicStyle = interactor->GetSpecialInteractor("SelectBox");
        if (basicStyle == nullptr) return;
        auto boxStyle = DynamicCast<iGame::BoxStyle>(basicStyle);
        auto box = boxStyle->GetBox();
        auto minMaxP = box->GetExtremePoint();
        auto boundingBox = BoundingBox(minMaxP.first, minMaxP.second);
        scene->ResetCameraView(boundingBox);
        rendererWidget->update();
    });
    connect(ui->action_rotateNinetyClockwise, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->RotateNinetyClockwise();
        rendererWidget->update();
    });
    connect(ui->action_rotateNinetyCounterClockwise, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->RotateNinetyCounterClockwise();
        rendererWidget->update();
    });


    connect(ui->action_ShowCenter, &QAction::toggled, this, [&](bool checked) {
        /*qDebug() << "Toggle state:" << checked;*/

        rendererWidget->GetScene()->ToggleCenterAxes();
        ui->action_ShowCenter->setChecked(checked);

        rendererWidget->update();
    });

    connect(ui->action_PickCenter, &QAction::toggled, this, [&](bool checked) {
        //拖拽
        if (checked) {
            // 显示坐标轴并进入拖拽模式
            rendererWidget->GetScene()->GetCenterAxesModel()->SetVisibility(true);
            rendererWidget->ChangeInteractorStyle(Interactor::DragCenterStyle);
            //rendererWidget->setCursor(Qt::CrossCursor);
        } else {
            // 退出选择模式
            rendererWidget->setCursor(Qt::ArrowCursor);
            rendererWidget->ChangeInteractorStyle(Interactor::BasicStyle);
        }
        ui->action_PickCenter->setChecked(checked);
        rendererWidget->update();
    });


    connect(ui->action_SaveScreenShot, &QAction::triggered, this, [&]() {
        QString path =
                QFileDialog::getSaveFileName(nullptr, "Save Screen shot", "", "PNG Images(*.png);;BMP Images(*.bmp)");
        igQtScreenShotOptionDialog dialog(this);
        dialog.setDialogTitle(QStringLiteral("Save Screenshot option"));
        int oldwidth = rendererWidget->width(), oldheight = rendererWidget->height();
        int ratio_pixel = rendererWidget->devicePixelRatio();
        int width = 1920, height = 1080;
        if (dialog.exec() == QDialog::Accepted) {
            auto input = dialog.getInput();
            width = input.first, height = input.second;
        }

        width /= ratio_pixel, height /= ratio_pixel;
        rendererWidget->resize(width, height);
        QImage saved_image = rendererWidget->grabFramebuffer();
        rendererWidget->resize(oldwidth, oldheight);
        const bool savedOk = saved_image.save(path, "BMP");
        showDarkFramelessMessage(QStringLiteral("截图结果"),
                                 savedOk ? QStringLiteral("保存成功") : QStringLiteral("保存失败"), savedOk);
    });

    connect(ui->action_SaveAnimation, &QAction::triggered, this, [&]() { ui->widget_Animation->saveAnimation(); });

    connect(ui->action_SetThreadNum, &QAction::triggered, this, [&](bool checked) {
        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("设置并行线程数"));
        // 获取当前线程池的默认线程数
        int currentThreadCount = iGame::ThreadPool::GetDefaultThreadCount();
        int maxThreads = std::thread::hardware_concurrency();
        QString recommendedThreads = QString::number(maxThreads / 2);
        dialog->setFilterDescription(QString("当前并行线程数: %1<br>"
                                             "硬件支持的最大线程数: %2<br>"
                                             "推荐线程数: %3<br>"
                                             "注意: 设置并行线程数会影响程序的性能。<br>"
                                             "建议根据硬件配置合理设置线程数。")
                                             .arg(currentThreadCount)
                                             .arg(maxThreads)
                                             .arg(recommendedThreads));

        // 添加参数：线程数输入框
        int id1 = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "并行线程数",
                                       QString::number(currentThreadCount));
        // 显示对话框
        dialog->show();
        // 设置应用按钮的回调函数
        dialog->setApplyFunctor([=, this]() {
            bool ok;
            // 获取用户输入的线程数
            int newThreadCount = dialog->getInt(id1, ok);
            // 检查输入是否有效
            if (ok && newThreadCount > 0) {
                // 检查线程数是否超过硬件支持的最大值
                /*if (newThreadCount > maxThreads) {
					QMessageBox::warning(this, "错误", QString("线程数不能超过硬件支持的最大值: %1").arg(maxThreads));
					return;
				}*/
                // 设置新的线程数
                iGame::ThreadPool::SetDefaultThreadCount(newThreadCount);
                showDarkFramelessMessage(QStringLiteral("成功"),
                                         QStringLiteral("并行线程数已设置为: %1").arg(newThreadCount), true);
                dialog->close();
            } else {
                showDarkFramelessMessage(QStringLiteral("错误"), QStringLiteral("请输入有效的线程数（大于0的整数）。"));
            }
        });
    });

    // AI聊天助手
    connect(ui->action_AiChat, &QAction::triggered, this, [&](bool checked) {
        if (aiChatDockWidget->isVisible()) {
            aiChatDockWidget->hide();
        } else {
            aiChatDockWidget->show();
        }
    });

    connect(ui->action_StrucDeformation, &QAction::triggered, this,
            [this](bool) { openLeftToolPanel(LeftToolPanelId::Deformation); });
    connect(ui->action_StreamLine, &QAction::triggered, this,
            [this](bool) { openLeftToolPanel(LeftToolPanelId::Flow); });


    initAllDockWidgetConnectWithAction();
    initAllMySignalConnections();
}

void igQtMainWindow::updateVortexMetricsLabelPos()
{
    if (!vortexMetricsLabel || !vortexMetricsLabel->isVisible()) return;

    vortexMetricsLabel->adjustSize();

    const int margin = 20;
    int x = rendererWidget->width()  - vortexMetricsLabel->width()  - margin;
    int y = rendererWidget->height() - vortexMetricsLabel->height() - margin;

    vortexMetricsLabel->move(x, y);
    vortexMetricsLabel->raise();
}

void igQtMainWindow::showDarkFramelessMessage(const QString& title, const QString& text, bool useInformationIcon) {
    igQtShowDarkFramelessMessage(this, title, text, useInformationIcon);
}

struct QuartileReportRow {
    QString name;
    QString attach;
    double  min, q1, median, q3, max;
};

void showQuartilesReportDialog(QWidget* parent, const QVector<QuartileReportRow>& rows) {
    QString report;
    for (const auto& r : rows) {
        report += QStringLiteral("%1 (%2): Min=%3  Q1=%4  Median=%5  Q3=%6  Max=%7\n")
                          .arg(r.name)
                          .arg(r.attach)
                          .arg(r.min)
                          .arg(r.q1)
                          .arg(r.median)
                          .arg(r.q3)
                          .arg(r.max);
    }
    igQtDarkFramelessMessageDialog dlg(parent, QStringLiteral("四分位数统计结果"), report.trimmed(), true);
    dlg.exec();
}

void igQtMainWindow::initAllFilters() {
    /* Data Processing 前两档：加宽以容纳较长参数标签，并关闭参数区滚动条（内容较少无需滚动） */
    auto tuneMeshSimplifyFilterDialog = [](igQtFilterDialogDockWidget* d) {
        constexpr int kDialogWidth = 360;
        d->setFixedWidth(kDialogWidth);
        if (auto* sa = d->findChild<QScrollArea*>(QStringLiteral("scrollArea"))) {
            sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            sa->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    };

    auto showSurfaceTopologyReport = [this](const SurfaceMesh::Pointer& mesh, const QString& stage) {
        const auto report = SurfaceMeshTopologyChecker::Check(mesh);
        auto formatIds = [](const std::vector<igIndex>& ids) {
            QString text;
            const int displayCount = std::min<int>(8, static_cast<int>(ids.size()));
            for (int i = 0; i < displayCount; ++i) {
                if (i != 0) { text += QStringLiteral(", "); }
                text += QString::number(ids[i]);
            }
            if (static_cast<int>(ids.size()) > displayCount) { text += QStringLiteral(" ..."); }
            return text;
        };

        QString text = QStringLiteral("检查阶段：%1\n点数：%2  边数：%3  面数：%4\n\n")
                               .arg(stage)
                               .arg(report.pointCount)
                               .arg(report.edgeCount)
                               .arg(report.faceCount);
        auto appendIssue = [&](const QString& name, const std::vector<igIndex>& ids) {
            text += QStringLiteral("%1：%2").arg(name).arg(ids.size());
            if (!ids.empty()) { text += QStringLiteral("（示例 ID：%1）").arg(formatIds(ids)); }
            text += QLatin1Char('\n');
        };

        appendIssue(QStringLiteral("非三角形面"), report.nonTriangleFaceIds);
        appendIssue(QStringLiteral("越界索引面"), report.invalidIndexFaceIds);
        appendIssue(QStringLiteral("退化三角形"), report.degenerateFaceIds);
        appendIssue(QStringLiteral("零面积三角形"), report.zeroAreaFaceIds);
        appendIssue(QStringLiteral("重复三角形"), report.duplicateFaceIds);
        appendIssue(QStringLiteral("无效边"), report.invalidEdgeIds);
        appendIssue(QStringLiteral("孤立边"), report.isolatedEdgeIds);
        appendIssue(QStringLiteral("非流形边"), report.nonManifoldEdgeIds);
        appendIssue(QStringLiteral("无效边—面邻接"), report.invalidAdjacencyEdgeIds);

        text += report.IsValid()
                        ? QStringLiteral("\n结论：拓扑检查通过，可进行传统表面网格简化。")
                        : QStringLiteral("\n结论：拓扑检查未通过。请根据示例 ID 定位并清理异常单元。 ");
        showDarkFramelessMessage(report.IsValid() ? QStringLiteral("拓扑检查通过")
                                                  : QStringLiteral("拓扑检查失败"),
                                     text, report.IsValid());
    };

    // ParaView 风格的标准 Filter 目录：同一个 QAction 同时出现在“常用”、
    // “按名称”和功能分类中。后续接入算法时只需替换这一处 triggered 回调。
    enum class StandardFilterCategory {
        DataAttributes,
        Geometry,
        Extraction,
        Sampling,
        Transform,
        Composite,
        MeshQuality
    };
    struct StandardFilterEntry {
        const char* id;
        const char* chineseName;
        StandardFilterCategory category;
        bool common;
    };
  
    // 保持按 id 的字母顺序，既方便“按名称”浏览，也方便检查清单是否完整。
    static const StandardFilterEntry standardFilterEntries[] = {
        {"angular_periodic", "角度周期", StandardFilterCategory::Transform, false},
        {"append_arc_length", "附加弧长", StandardFilterCategory::DataAttributes, false},
        {"append_attributes", "附加属性", StandardFilterCategory::DataAttributes, false},
        {"append_datasets", "附加数据集", StandardFilterCategory::Composite, false},
        {"append_location_attributes", "附加位置属性", StandardFilterCategory::DataAttributes, false},
        {"append_reduce", "网格合并去重", StandardFilterCategory::Composite, false},
        {"axis_aligned_reflection", "轴对齐反射", StandardFilterCategory::Transform, false},
        {"axis_aligned_transform", "轴对齐变换", StandardFilterCategory::Transform, false},
        {"boundary_mesh_quality", "边界网格质量", StandardFilterCategory::MeshQuality, false},
        {"cell_centers", "单元中心", StandardFilterCategory::Geometry, false},
        {"cell_quality", "单元质量", StandardFilterCategory::MeshQuality, true},
        {"cell_size", "单元尺寸", StandardFilterCategory::Geometry, false},
        {"clean_cells_to_grid", "清理单元为网格", StandardFilterCategory::Geometry, false},
        {"clean_poly_data", "清理多边形数据", StandardFilterCategory::Geometry, false},
        {"clean_to_grid", "清理为网格", StandardFilterCategory::Geometry, true},
        {"compute_connected_surface_properties", "计算连通表面属性", StandardFilterCategory::DataAttributes, false},
        {"compute_quartiles", "计算四分位数", StandardFilterCategory::DataAttributes, false},
        {"connectivity", "连通性", StandardFilterCategory::Extraction, false},
        {"convert_to_vertex", "转换为顶点", StandardFilterCategory::Geometry, false},
        {"coordinates", "坐标", StandardFilterCategory::DataAttributes, false},
        {"count_cell_faces", "单元面数统计", StandardFilterCategory::Geometry, false},
        {"count_cell_vertices", "单元顶点数统计", StandardFilterCategory::Geometry, false},
        {"decimate_polyline", "折线简化", StandardFilterCategory::Geometry, false},
        {"deflect_normals", "偏转法向量", StandardFilterCategory::DataAttributes, false},
        {"elevation", "高程", StandardFilterCategory::DataAttributes, true},
        {"extract_block", "提取数据块", StandardFilterCategory::Composite, false},
        {"extract_cells_by_region", "按区域提取单元", StandardFilterCategory::Extraction, false},
        {"extract_cells_by_type", "按类型提取单元", StandardFilterCategory::Extraction, false},
        {"extract_component", "提取分量", StandardFilterCategory::DataAttributes, false},
        {"extract_edges", "提取边", StandardFilterCategory::Geometry, true},
        {"extract_enclosed_points", "提取包围点", StandardFilterCategory::Extraction, false},
        {"extract_location", "提取位置", StandardFilterCategory::Extraction, false},
        {"extract_selection", "提取选择", StandardFilterCategory::Extraction, false},
        {"extract_subset", "提取子集", StandardFilterCategory::Extraction, true},
        {"extract_time_steps", "提取时间步", StandardFilterCategory::Extraction, false},
        {"feature_edges", "特征边", StandardFilterCategory::Geometry, true},
        {"feature_edges_region_ids", "特征边区域标识符", StandardFilterCategory::DataAttributes, false},
        {"force_static_mesh", "强制静态网格", StandardFilterCategory::Geometry, false},
        {"generate_ids", "生成标识符", StandardFilterCategory::DataAttributes, false},
        {"generate_surface_tangents", "生成表面切向量", StandardFilterCategory::DataAttributes, false},
        {"ghost_cells", "幽灵单元", StandardFilterCategory::DataAttributes, false},
        {"global_point_and_cell_ids", "全局点与单元标识符", StandardFilterCategory::DataAttributes, false},
        {"group_datasets", "数据集分组", StandardFilterCategory::Composite, false},
        {"histogram", "直方图", StandardFilterCategory::DataAttributes, false},
        {"integrate_variables", "变量积分", StandardFilterCategory::DataAttributes, false},
        {"iso_volume", "等值体", StandardFilterCategory::Extraction, true},
        {"linear_extrusion", "线性挤出", StandardFilterCategory::Geometry, false},
        {"mask", "掩码", StandardFilterCategory::Extraction, false},
        {"mask_points", "点掩码", StandardFilterCategory::Extraction, false},
        {"median", "中位数", StandardFilterCategory::DataAttributes, false},
        {"merge_vector_components", "合并向量分量", StandardFilterCategory::DataAttributes, false},
        {"mesh_quality", "网格质量", StandardFilterCategory::MeshQuality, true},
        {"multiblock_surface_as_multiblock", "多块表面保留多块结构", StandardFilterCategory::Composite, false},
        {"outline", "轮廓", StandardFilterCategory::Extraction, false},
        {"outline_corners", "轮廓角", StandardFilterCategory::Extraction, false},
        {"overlapping_cells_detector", "重叠单元检测", StandardFilterCategory::Geometry, false},
        {"pass_arrays", "传递数组", StandardFilterCategory::DataAttributes, true},
        {"perlin_noise", "柏林噪声", StandardFilterCategory::DataAttributes, false},
        {"point_and_cell_ids", "点与单元标识符", StandardFilterCategory::DataAttributes, false},
        {"point_line_interpolator", "点线插值器", StandardFilterCategory::Sampling, false},
        {"point_plane_interpolator", "点平面插值器", StandardFilterCategory::Sampling, false},
        {"point_set_to_octree_image", "点集转八叉树图像", StandardFilterCategory::Geometry, false},
        {"point_volume_interpolator", "点体积插值器", StandardFilterCategory::Sampling, false},
        {"probe", "探测", StandardFilterCategory::Sampling, true},
        {"probe_location", "位置探测", StandardFilterCategory::Sampling, false},
        {"process_ids", "进程标识符", StandardFilterCategory::DataAttributes, false},
        {"random_attributes", "随机属性", StandardFilterCategory::DataAttributes, false},
        {"random_vectors", "随机向量", StandardFilterCategory::DataAttributes, false},
        {"reflect", "反射", StandardFilterCategory::Transform, false},
        {"remove_ghost_information", "移除幽灵信息", StandardFilterCategory::DataAttributes, false},
        {"rename_arrays", "重命名数组", StandardFilterCategory::DataAttributes, false},
        {"resample_to_image", "重采样到图像", StandardFilterCategory::Sampling, true},
        {"resample_to_line", "重采样到直线", StandardFilterCategory::Sampling, false},
        {"resample_with_dataset", "重采样至数据集", StandardFilterCategory::Sampling, false},
        {"reverse_sense", "反转方向", StandardFilterCategory::Geometry, false},
        {"ribbon", "带状面", StandardFilterCategory::Geometry, false},
        {"shrink", "收缩", StandardFilterCategory::Geometry, true},
        {"slice_with_plane", "平面切片", StandardFilterCategory::Extraction, true},
        {"smooth", "平滑", StandardFilterCategory::Geometry, false},
        {"subdivide", "细分", StandardFilterCategory::Geometry, false},
        {"surface_normals", "表面法向量", StandardFilterCategory::DataAttributes, true},
        {"tensor_principal_invariants", "张量主值与主方向", StandardFilterCategory::DataAttributes, false},
        {"surface_vectors", "表面向量", StandardFilterCategory::DataAttributes, false},
        {"temporal_shift_scale", "时间平移与缩放", StandardFilterCategory::Transform, false},
        {"temporal_statistics", "时间统计", StandardFilterCategory::DataAttributes, false},
        {"texture_map_to_cylinder", "圆柱纹理映射", StandardFilterCategory::DataAttributes, false},
        {"texture_map_to_plane", "平面纹理映射", StandardFilterCategory::DataAttributes, false},
        {"texture_map_to_sphere", "球面纹理映射", StandardFilterCategory::DataAttributes, false},
        {"threshold", "阈值", StandardFilterCategory::Extraction, true},
        {"transform", "变换", StandardFilterCategory::Transform, true},
        {"triangle_strips", "三角形条带", StandardFilterCategory::Geometry, false},
        {"tube", "管线", StandardFilterCategory::Geometry, false},
        {"validate_cells", "验证单元", StandardFilterCategory::Geometry, false},
        {"volume_of_revolution", "旋转体", StandardFilterCategory::Geometry, false},
        {"warp_by_scalar", "按标量扭曲", StandardFilterCategory::Transform, false},
        {"warp_by_vector", "按向量扭曲", StandardFilterCategory::Transform, false},
        {"yield_criteria", "屈服准则", StandardFilterCategory::DataAttributes, false},
    };

    QMenu* standardFilters =
            ui->menu_filters->addMenu(QStringLiteral("标准过滤器（Standard Filters）"));

    QMenu* commonFilters = standardFilters->addMenu(QStringLiteral("Common（常用）"));
    QMenu* alphabeticalFilters = standardFilters->addMenu(QStringLiteral("Alphabetical（按名称）"));
    standardFilters->addSeparator();

    QMenu* dataAttributeFilters =
            standardFilters->addMenu(QStringLiteral("Data Attributes & IDs（数据属性与标识）"));
    QMenu* geometryFilters =
            standardFilters->addMenu(QStringLiteral("Geometry & Mesh（几何与网格）"));
    QMenu* extractionFilters =
            standardFilters->addMenu(QStringLiteral("Extraction & Selection（提取与选择）"));
    QMenu* samplingFilters =
            standardFilters->addMenu(QStringLiteral("Sampling & Interpolation（采样与插值）"));
    QMenu* transformFilters =
            standardFilters->addMenu(QStringLiteral("Transform（变换）"));
    QMenu* compositeFilters =
            standardFilters->addMenu(QStringLiteral("Composite Data（复合数据）"));
    QMenu* meshQualityFilters =
            standardFilters->addMenu(QStringLiteral("Mesh Quality（网格质量）"));

    // 项目已有的 Filter 入口放在标准过滤器目录底部，执行逻辑保持不变。
    standardFilters->addSeparator();
    QMenu* mesh_processing =
            standardFilters->addMenu(QStringLiteral("Data Processing（数据处理）"));
    QMenu* convert = standardFilters->addMenu(QStringLiteral("Convert（数据转换）"));
    QMenu* view = standardFilters->addMenu(QStringLiteral("Feature Extraction（特征提取）"));

    // 全局下拉菜单为 12pt；标准 Filter 数量较多，局部缩小到 10pt，
    // 保留其他菜单的原有字号与样式。
    standardFilters->setStyleSheet(QStringLiteral("QMenu { font-size: 10pt; }"));

    auto categoryMenu = [&](StandardFilterCategory category) -> QMenu* {
        switch (category) {
            case StandardFilterCategory::DataAttributes: return dataAttributeFilters;
            case StandardFilterCategory::Geometry: return geometryFilters;
            case StandardFilterCategory::Extraction: return extractionFilters;
            case StandardFilterCategory::Sampling: return samplingFilters;
            case StandardFilterCategory::Transform: return transformFilters;
            case StandardFilterCategory::Composite: return compositeFilters;
            case StandardFilterCategory::MeshQuality: return meshQualityFilters;
        }
        return standardFilters;
    };

    auto currentFilterInput = [this](const QString& title) -> DataObject::Pointer {
        // P2-2：优先取模型树【当前选中】的数据对象 —— 选中多块装配体的子块时返回子块本身，
        // 无任何选中时回落为当前模型的根 DataObject（见 igQtModelDialogWidget::GetCurrentDataObject）。
        auto obj = modelTreeWidget->GetCurrentDataObject();
        if (!obj) {
            showDarkFramelessMessage(title, QStringLiteral("请先加载并选择一个模型。"));
            return nullptr;
        }
        return obj;
    };

    auto refreshFilterResult = [this](DataObject::Pointer input, DataObject::Pointer output,
                                      const QString& title, bool inPlace = false) {
        if (!output) {
            showDarkFramelessMessage(title, QStringLiteral("Filter 未产生有效输出。"));
            return;
        }

        if (inPlace || output.GetPointer() == input.GetPointer()) {
            modelTreeWidget->updateAllAttriubute(output);
            if (auto drawObj = DynamicCast<DrawObject>(output)) { drawObj->ForceReConvertToDrawableData(); }
        } else {
            modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
        }
        rendererWidget->update();
        showDarkFramelessMessage(title, QStringLiteral("Filter 执行完成。"), true);
    };

    auto showSmoothPanel = [this]() {
        auto scene = rendererWidget->GetScene();
        auto model = scene ? scene->GetCurrentModel() : nullptr;
        if (!model || !modelTreeWidget->getItemFromObject(model->GetDataObject())) {
            showDarkFramelessMessage(QStringLiteral("表面平滑"), QStringLiteral("请先选择一个模型。"));
            return;
        }
        SmoothWidget->resetForm();
        SmoothWidget->setCurrentModel(model);
        SmoothDockWidget->show();
        SmoothDockWidget->raise();
        SmoothWidget->setFocus(Qt::OtherFocusReason);
    };

    auto connectStandardFilterAction = [&](QAction* action, const QString& filterId) -> bool {
        // Standard actions own their callbacks; no development-menu lookup is required.
        if (filterId == QStringLiteral("append_arc_length")) {
            connect(action, &QAction::triggered, this, [this](bool checked) {
        auto* scene = rendererWidget->GetScene();
        auto model = scene ? scene->GetCurrentModel() : nullptr;
        auto data = model ? model->GetDataObject() : nullptr;
        if (data == nullptr) {
            showDarkFramelessMessage(QStringLiteral("No Model Available"),
                QStringLiteral("Please load and select a model first."));
            return;
        }
        AppendArcLengthFilter::Pointer filter = AppendArcLengthFilter::New();
        filter->SetInput(data);
        if (!filter->Execute()) {
            std::string message = filter->GetMessage();
            if (message.empty()) message = "AppendArcLengthFilter execute failed";
            showDarkFramelessMessage(QStringLiteral("Warning"), QString::fromStdString(message));
            return;
        }
        // Register a distinct pipeline result, retaining the original source model.
        auto sourceItem = modelTreeWidget->getItemFromObject(data);
        data = filter->GetOutput();
        static int polylineLengthNumber = 0;
        const QString outputName = QStringLiteral("PolylineLength%1").arg(++polylineLengthNumber);
        data->SetName(outputName.toStdString());
        const int outputModelId = modelTreeWidget->addDataObjectToModelTree(data, ItemSource::Algorithm);
        modelTreeWidget->renameModelRow(data, outputName);
        model = scene->GetModelById(outputModelId);
        if (sourceItem) { sourceItem->changeVisibility(false); }
        auto attributeSet = data->GetAttributeSet();
        int attrIndex = attributeSet ? attributeSet->GetAttributeIndex(AppendArcLengthFilter::GetArrayName()) : -1;
        IGsize valueCount = 0;
        if (attributeSet && attrIndex >= 0) {
            auto& attribute = attributeSet->GetAttribute(attrIndex);
            if (attribute.pointer) { valueCount = attribute.pointer->GetNumberOfValues(); }
        }
        auto item = modelTreeWidget->getItemFromObject(data);
        if (item && attrIndex >= 0) {
            item->setExpanded(true);
            for (int i = 0; i < item->childCount(); ++i) {
                auto child = dynamic_cast<AttribTreeWidgetItem*>(item->child(i));
                if (!child || child->attributeIndex() != attrIndex) { continue; }
                item->setCurrentChild(child);
                item->setSelected(false);
                item->viewAttribute(attrIndex, -1);
                child->setSelected(true);
                modelTreeWidget->setCurrentItem(child);
                break;
            }
        }
        if (ui->dockWidget_SearchInfo && ui->dockWidget_SearchInfo->isVisible()) {
            ui->widget_SearchInfo->setCurrentModel(model);
        }
        showDarkFramelessMessage(QStringLiteral("Success"),
            QStringLiteral("Arc length appended: %1 point(s), %2 polyline(s).")
            .arg(static_cast<qulonglong>(valueCount))
            .arg(static_cast<qulonglong>(filter->GetNumberOfProcessedLines())),
            true);
        });
            return true;
        }
        if (filterId == QStringLiteral("cell_quality")) {
            connect(action, &QAction::triggered, this, [this](bool checked) {
        // 1. 创建对话框
        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("单元质量评估 (Cell Quality)"));
        dialog->setFilterDescription(
                QStringLiteral("对齐 ParaView Cell Quality：选择一个统一指标作用于模型内所有单元；\n"
                               "不适用的单元填入「不支持值」，执行后汇总报告跳过数量。\n"
                               "适用范围：四面体、六面体、三角形、四边形；\n"
                               "各指标对单元类型的适用性按 ParaView Cell Quality 对齐。"));

        // 2. 统一指标下拉框（与 ParaView 一致的交互，不再区分四面体/六面体专用算法）
        //    20 项按 ParaView Cell Quality 的 Quality Measure 列表原样排列
        std::vector<QString> metricNames = {
                QStringLiteral("面积 (Area)"),
                QStringLiteral("纵横比 (Aspect Ratio)"),
                QStringLiteral("条件数 (Condition)"),
                QStringLiteral("畸变度 (Distortion)"),
                QStringLiteral("边长比 (Edge Ratio)"),
                QStringLiteral("等角歪斜 (Equiangle Skew)"),
                QStringLiteral("雅可比行列式 (Jacobian)"),
                QStringLiteral("最小内角 (Minimum Angle)"),
                QStringLiteral("Oddy"),
                QStringLiteral("半径比 (Radius Ratio)"),
                QStringLiteral("相对尺寸平方 (Relative Size Squared)"),
                QStringLiteral("归一化雅可比 (Scaled Jacobian)"),
                QStringLiteral("形状 (Shape)"),
                QStringLiteral("形状与尺寸 (Shape and Size)"),
                QStringLiteral("剪切 (Shear)"),
                QStringLiteral("剪切与尺寸 (Shear and Size)"),
                QStringLiteral("歪斜度 (Skew)"),
                QStringLiteral("伸展度 (Stretch)"),
                QStringLiteral("锥度 (Taper)"),
                QStringLiteral("翘曲度 (Warpage)")};

        std::vector<iGame::CellMeshMetricsFilter::CellQualityMetric> metricEnums = {
                iGame::CellMeshMetricsFilter::QUALITY_AREA,
                iGame::CellMeshMetricsFilter::QUALITY_ASPECT_RATIO,
                iGame::CellMeshMetricsFilter::QUALITY_CONDITION,
                iGame::CellMeshMetricsFilter::QUALITY_DISTORTION,
                iGame::CellMeshMetricsFilter::QUALITY_EDGE_RATIO,
                iGame::CellMeshMetricsFilter::QUALITY_EQUIANGLE_SKEW,
                iGame::CellMeshMetricsFilter::QUALITY_JACOBIAN,
                iGame::CellMeshMetricsFilter::QUALITY_MIN_ANGLE,
                iGame::CellMeshMetricsFilter::QUALITY_ODDY,
                iGame::CellMeshMetricsFilter::QUALITY_RADIUS_RATIO,
                iGame::CellMeshMetricsFilter::QUALITY_RELATIVE_SIZE_SQUARED,
                iGame::CellMeshMetricsFilter::QUALITY_SCALED_JACOBIAN,
                iGame::CellMeshMetricsFilter::QUALITY_SHAPE,
                iGame::CellMeshMetricsFilter::QUALITY_SHAPE_AND_SIZE,
                iGame::CellMeshMetricsFilter::QUALITY_SHEAR,
                iGame::CellMeshMetricsFilter::QUALITY_SHEAR_AND_SIZE,
                iGame::CellMeshMetricsFilter::QUALITY_SKEW,
                iGame::CellMeshMetricsFilter::QUALITY_STRETCH,
                iGame::CellMeshMetricsFilter::QUALITY_TAPER,
                iGame::CellMeshMetricsFilter::QUALITY_WARPAGE};

        int comboID =
                dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX, QStringLiteral("评估指标"), metricNames);

        // 3. 不支持/无效值（对齐 ParaView 的 UnsupportedGeometry，默认 -1.0）
        int unsupportedId =
                dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("不支持/无效值"), "-1.0");

        dialog->show();

        // 4. 确认回调逻辑
        dialog->setApplyFunctor([=, this]() {
            // 4.1 前置校验：必须有选中模型
            auto scene = rendererWidget ? rendererWidget->GetScene() : nullptr;
            if (scene == nullptr || scene->GetCurrentModel() == nullptr) {
                showDarkFramelessMessage(QStringLiteral("无可用模型"),
                                         QStringLiteral("请先在模型树中选中需要评估的体网格模型。"));
                dialog->close();
                return;
            }
            auto obj = scene->GetCurrentModel()->GetDataObject();
            if (obj == nullptr) {
                showDarkFramelessMessage(QStringLiteral("无可用模型"), QStringLiteral("当前模型数据对象为空。"));
                dialog->close();
                return;
            }

            // 4.2 读取指标
            bool ok = false;
            int selectedIndex = dialog->getComboIndex(comboID, ok);
            if (!ok || selectedIndex < 0 || selectedIndex >= (int) metricEnums.size()) {
                showDarkFramelessMessage(QStringLiteral("无效选择"), QStringLiteral("请选择一个有效的评估指标。"));
                dialog->close();
                return;
            }
            auto metric = metricEnums[selectedIndex];

            // 4.3 读取"不支持值"，非法输入则回退到 -1.0
            double unsupportedValue = -1.0;
            bool okValue = false;
            double parsed = dialog->getDouble(unsupportedId, okValue);
            if (okValue) {
                unsupportedValue = parsed;
            } else {
                showDarkFramelessMessage(QStringLiteral("参数提示"),
                                         QStringLiteral("「不支持/无效值」不是合法数字，已回退为默认值 -1.0。"));
            }

            // 4.4 执行 filter
            auto filter = iGame::CellMeshMetricsFilter::New();
            filter->setMetric(metric);
            filter->setUnsupportedValue(unsupportedValue);
            filter->SetInput(obj);
            if (!filter->Execute()) {
                showDarkFramelessMessage(
                        QStringLiteral("计算失败"),
                        QStringLiteral("单元质量评估执行失败，请检查输入是否为可识别的体网格。"));
                dialog->close();
                return;
            }

            // 4.5 输出独立结果节点（不修改原模型）
            auto qualityObj = filter->GetOutput();
            if (qualityObj == nullptr) {
                showDarkFramelessMessage(QStringLiteral("计算失败"), QStringLiteral("未产生有效的输出结果。"));
                dialog->close();
                return;
            }
            qualityObj->SetName(obj->GetName() + "_CellQuality");
            modelTreeWidget->addDataObjectToModelTree(qualityObj, Algorithm);
            rendererWidget->update();

            // 4.6 汇总报告：支持数量 / 跳过数量 / 使用的指标与不支持值
            const int supported = filter->GetSupportedCount();
            const int unsupported = filter->GetUnsupportedCount();
            const int skipped = filter->GetSkippedBlockCount();
            QString report = QStringLiteral("单元质量评估完成。\n\n"
                                            "• 评估指标：%1\n"
                                            "• 成功计算单元：%2 个\n"
                                            "• 跳过不适用单元：%3 个（已标记为 %4）\n"
                                            "• 无法处理的网格块：%5 个\n"
                                            "• 结果已生成为独立节点：%6")
                                     .arg(metricNames[selectedIndex])
                                     .arg(supported)
                                     .arg(unsupported)
                                     .arg(unsupportedValue)
                                     .arg(skipped)
                                     .arg(QString::fromStdString(qualityObj->GetName()));
            if (supported == 0 && unsupported > 0) {
                report += QStringLiteral(
                        "\n\n提示：当前指标对该模型中的所有单元均不适用。\n"
                        "请确认单元类型（四面体/六面体/三角形/四边形），或更换评估指标 ——\n"
                        "不同指标对单元类型的适用范围与 ParaView Cell Quality 一致。");
            }
            showDarkFramelessMessage(QStringLiteral("评估完成"), report, true);
        });
    });
            return true;
        }
        if (filterId == QStringLiteral("connectivity")) {
            connect(action, &QAction::triggered, this, [this](bool) {
                auto scene = rendererWidget ? rendererWidget->GetScene() : nullptr;
                auto currentModel = scene ? scene->GetCurrentModel() : nullptr;
                if (!currentModel) {
                    showDarkFramelessMessage(QStringLiteral("连通区域"), QStringLiteral("请先选择一个模型。"));
                    return;
                }
                auto object = currentModel->GetDataObject();
                if (iGame::DynamicCast<iGame::SurfaceMesh>(object).IsNull()) {
                    showDarkFramelessMessage(QStringLiteral("连通区域"),
                                             QStringLiteral("当前模型不是曲面网格（SurfaceMesh）"));
                    return;
                }

                auto* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(QStringLiteral("连通区域"));
                dialog->setFilterDescription(
                        QStringLiteral("按共享点连通性给区域编号或抽取区域"));
                const int modeId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_COMBO_BOX, QStringLiteral("抽取模式"),
                        std::vector<QString>{QStringLiteral("点种子"),
                                             QStringLiteral("单元种子"),
                                             QStringLiteral("指定区域"),
                                             QStringLiteral("最大区域"),
                                             QStringLiteral("全部区域"),
                                             QStringLiteral("最近点")});
                const int colorId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_CHECK_BOX, QStringLiteral("按区域编号着色"), "true");
                const int assignId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_COMBO_BOX, QStringLiteral("编号方式"),
                        std::vector<QString>{QStringLiteral("遍历顺序"), QStringLiteral("单元数升序"),
                                             QStringLiteral("单元数降序")});
                const int seedsId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("种子编号"), "0");
                const int specId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("区域编号"), "0");
                const int closestId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("最近点坐标 (x, y, z)"), "0,0,0");
                dialog->setParameterLayoutVertical();
                dialog->setMinimumWidth(440);
                dialog->resize(qMax(480, dialog->fontMetrics().horizontalAdvance(
                        QStringLiteral("最近点坐标 (x, y, z)")) + 120), 520);
                for (int id : {seedsId, specId}) {
                    auto* edit = qobject_cast<QLineEdit*>(dialog->getWidget(id));
                    edit->setPlaceholderText(QStringLiteral("例如：0, 2, 5（编号从 0 开始）"));
                    edit->setToolTip(QStringLiteral("多个编号用英文逗号或空格分隔，编号从 0 开始。"));
                }
                auto* closestEdit = qobject_cast<QLineEdit*>(dialog->getWidget(closestId));
                closestEdit->setPlaceholderText(QStringLiteral("例如：1.5, 0, -2.0"));
                closestEdit->setToolTip(QStringLiteral("输入模型坐标中的 x、y、z，用英文逗号或空格分隔。"));
                if (auto* scrollArea = dialog->findChild<QScrollArea*>(QStringLiteral("scrollArea"))) {
                    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                }
                // 默认「全部区域」：一次即可看到所有连通区域的编号着色。
                if (auto* modeCombo = qobject_cast<QComboBox*>(dialog->getWidget(modeId))) {
                    modeCombo->setCurrentIndex(4);
                }
                auto* inputError = new QLabel(dialog);
                inputError->setObjectName(QStringLiteral("ConnectivityInputError"));
                inputError->setWordWrap(true);
                inputError->setStyleSheet(QStringLiteral("color: #ffb4ab;"));
                dialog->addRowWidget(inputError);
                inputError->hide();
                for (int id : {seedsId, specId, closestId}) {
                    connect(qobject_cast<QLineEdit*>(dialog->getWidget(id)), &QLineEdit::textChanged,
                            inputError, &QWidget::hide);
                }
                auto* modeCombo = qobject_cast<QComboBox*>(dialog->getWidget(modeId));
                auto updateModeParameters = [=](int mode) {
                    inputError->hide();
                    dialog->setParameterVisible(seedsId, mode == 0 || mode == 1);
                    dialog->setParameterVisible(specId, mode == 2);
                    dialog->setParameterVisible(closestId, mode == 5);
                    const QString descriptions[] = {
                        QStringLiteral("抽取包含指定点的连通区域。填写点编号，多个编号用逗号分隔。"),
                        QStringLiteral("抽取包含指定单元的连通区域。填写单元编号，多个编号用逗号分隔。"),
                        QStringLiteral("抽取指定编号的区域。区域编号从 0 开始，由编号方式决定。"),
                        QStringLiteral("仅保留单元数最多的连通区域，无需额外参数。"),
                        QStringLiteral("保留全部连通区域，可按区域编号着色，无需额外参数。"),
                        QStringLiteral("抽取距离指定坐标最近的连通区域。填写模型坐标 x、y、z。")};
                    dialog->setFilterDescription(descriptions[mode]);
                    if (mode == 0 || mode == 1) {
                        auto* seeds = qobject_cast<QLineEdit*>(dialog->getWidget(seedsId));
                        seeds->setToolTip(mode == 0 ? QStringLiteral("填写点编号，多个编号用逗号或空格分隔。")
                                                   : QStringLiteral("填写单元编号，多个编号用逗号或空格分隔。"));
                    }
                };
                connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                        dialog, updateModeParameters);
                updateModeParameters(modeCombo->currentIndex());
                dialog->show();

                dialog->setApplyFunctor([=, this]() {
                    bool ok = false;
                    const int mode = dialog->getComboIndex(modeId, ok);
                    auto filter = iGame::ConnectivityFilter::New();
                    switch (mode) {
                        case 0: filter->SetExtractionMode(iGame::ConnectivityFilter::POINT_SEEDED_REGIONS); break;
                        case 1: filter->SetExtractionMode(iGame::ConnectivityFilter::CELL_SEEDED_REGIONS); break;
                        case 2: filter->SetExtractionMode(iGame::ConnectivityFilter::SPECIFIED_REGIONS); break;
                        case 3: filter->SetExtractionMode(iGame::ConnectivityFilter::LARGEST_REGION); break;
                        case 4: filter->SetExtractionMode(iGame::ConnectivityFilter::ALL_REGIONS); break;
                        default: filter->SetExtractionMode(iGame::ConnectivityFilter::CLOSEST_POINT_REGION); break;
                    }
                    filter->SetColorRegions(dialog->getChecked(colorId, ok));
                    const int assign = dialog->getComboIndex(assignId, ok);
                    if (assign == 1) {
                        filter->SetRegionIdAssignmentMode(iGame::ConnectivityFilter::CELL_COUNT_ASCENDING);
                    } else if (assign == 2) {
                        filter->SetRegionIdAssignmentMode(iGame::ConnectivityFilter::CELL_COUNT_DESCENDING);
                    } else {
                        filter->SetRegionIdAssignmentMode(iGame::ConnectivityFilter::UNSPECIFIED);
                    }

                    auto parseDoubles = [](const QString& text, std::vector<double>& out) -> bool {
                        out.clear();
                        QString t = text;
                        t.replace(',', ' ');
                        const auto parts = t.simplified().split(' ', Qt::SkipEmptyParts);
                        for (const auto& p : parts) {
                            bool c = false;
                            const double v = p.toDouble(&c);
                            if (!c || !std::isfinite(v)) { return false; }
                            out.push_back(v);
                        }
                        return !out.empty();
                    };

                    auto rejectInput = [=](int id, const QString& message) {
                        inputError->setText(message);
                        inputError->show();
                        if (auto* edit = qobject_cast<QLineEdit*>(dialog->getWidget(id))) {
                            edit->setFocus();
                            edit->selectAll();
                        }
                    };
                    auto parseIds = [&](int id, qlonglong maxId, std::vector<igIndex>& values) {
                        auto* edit = qobject_cast<QLineEdit*>(dialog->getWidget(id));
                        QString text = edit->text();
                        text.replace(',', ' ');
                        const auto parts = text.simplified().split(' ', Qt::SkipEmptyParts);
                        values.clear();
                        for (const auto& part : parts) {
                            bool valid = false;
                            const qlonglong value = part.toLongLong(&valid);
                            if (!valid || value < 0 || value > maxId ||
                                static_cast<unsigned long long>(value) > static_cast<unsigned long long>(std::numeric_limits<igIndex>::max())) {
                                rejectInput(id, QStringLiteral("请输入 0 到 %1 范围内的整数编号，多个编号用逗号或空格分隔。").arg(maxId));
                                return false;
                            }
                            values.push_back(static_cast<igIndex>(value));
                        }
                        if (values.empty()) {
                            rejectInput(id, QStringLiteral("请至少输入一个整数编号。"));
                            return false;
                        }
                        return true;
                    };
                    inputError->hide();
                    if (mode == 0 || mode == 1) {
                        auto mesh = iGame::DynamicCast<iGame::SurfaceMesh>(object);
                        const qlonglong count = static_cast<qlonglong>(mode == 0 ? mesh->GetNumberOfPoints() : mesh->GetNumberOfFaces());
                        std::vector<igIndex> values;
                        if (!parseIds(seedsId, count - 1, values)) return;
                        filter->InitializeSeedList();
                        for (igIndex value : values) filter->AddSeed(value);
                    } else if (mode == 2) {
                        std::vector<igIndex> values;
                        if (!parseIds(specId, std::numeric_limits<int>::max(), values)) return;
                        filter->InitializeSpecifiedRegionList();
                        for (igIndex value : values) filter->AddSpecifiedRegion(static_cast<int>(value));
                    } else if (mode == 5) {
                        auto* edit = qobject_cast<QLineEdit*>(dialog->getWidget(closestId));
                        std::vector<double> values;
                        if (!parseDoubles(edit->text(), values) || values.size() != 3) {
                            rejectInput(closestId, QStringLiteral("请输入三个有限坐标值 x, y, z，例如：1.5, 0, -2.0。"));
                            return;
                        }
                        filter->SetClosestPoint(values[0], values[1], values[2]);
                    }

                    filter->SetInput(object);
                    if (!filter->Execute()) {
                        if (mode == 2 && (filter->GetMessage() == "Specified region ID is outside the valid range" ||
                                          filter->GetMessage() == "Extraction produced an empty mesh")) {
                            rejectInput(specId, QStringLiteral("请填写 0 到 %1 范围内的区域编号（按当前编号方式排序）。")
                                    .arg(filter->GetNumberOfExtractedRegions() - 1));
                            return;
                        }
                        showDarkFramelessMessage(
                                QStringLiteral("连通区域"),
                                QStringLiteral("执行失败：%1").arg(QString::fromStdString(filter->GetMessage())));
                        return;
                    }
                    auto output = filter->GetOutput();
                    if (!output) {
                        showDarkFramelessMessage(QStringLiteral("连通区域"), QStringLiteral("输出对象为空"));
                        return;
                    }
                    output->SetName(object->GetName() + "_Connectivity");

                    auto inputDraw = iGame::DynamicCast<iGame::DrawObject>(object);
                    auto outDraw = iGame::DynamicCast<iGame::DrawObject>(output);
                    if (inputDraw && outDraw) {
                        // 只继承视图样式；不继承颜色映射表——RegionId 是新增属性，需用输出自身的
                        // 颜色映射（range 由 RegionId 数据范围确定），否则会出现只有一种颜色。
                        outDraw->SetViewStyle(static_cast<IGenum>(inputDraw->GetViewStyle()));
                    }

                    dialog->setFilterDescription(
                            QStringLiteral("共 %1 个连通区域").arg(filter->GetNumberOfExtractedRegions()));
                    modelTreeWidget->addDataObjectToModelTree(output, ItemSource::Algorithm);

                    // 优先按 RegionId（点标量）上色；否则退回第一个标量点/单元属性。
                    if (auto attrSet = output->GetAttributeSet(); attrSet && filter->GetColorRegions()) {
                        int activeIdx = attrSet->GetAttributeIndex(iGame::ConnectivityFilter::RegionIdName);
                        if (activeIdx < 0) {
                            int pointAttrIdx = -1;
                            int cellAttrIdx = -1;
                            for (int i = 0; i < static_cast<int>(attrSet->GetNumberOfAttributes()); ++i) {
                                auto& attr = attrSet->GetAttribute(i);
                                if (attr.IsNone() || attr.isDeleted || attr.type != IG_SCALAR) { continue; }
                                if (attr.attachmentType == IG_POINT && pointAttrIdx < 0) {
                                    pointAttrIdx = i;
                                } else if (attr.attachmentType == IG_CELL && cellAttrIdx < 0) {
                                    cellAttrIdx = i;
                                }
                            }
                            activeIdx = (pointAttrIdx >= 0) ? pointAttrIdx : cellAttrIdx;
                        }
                        if (activeIdx >= 0 && outDraw) {
                            // 显式按该属性的数据范围设置色表 range，保证热力图映射正确
                            // （不依赖 SetAttributeWithPointData 内部的 MTime 条件）。
                            auto& attr = attrSet->GetAttribute(activeIdx);
                            if (auto range = attr.GetDataRange()) {
                                double lo = range->GetValue(2);
                                double hi = range->GetValue(3);
                                if (hi <= lo) { hi = lo + 1.0; } // 单值退化范围，避免所有值落入同一点
                                if (auto cm = outDraw->GetColorMapper()) {
                                    cm->SetRangeStable(false);
                                    cm->SetRange(lo, hi);
                                }
                            }
                            if (auto scene2 = rendererWidget->GetScene()) {
                                outDraw->ViewCloudPicture(scene2, activeIdx, 0);
                            }
                        }
                    }

                    modelTreeWidget->updateCloudPicture();
                    rendererWidget->update();
                    dialog->close();
                });
            });
            return true;
        }
        if (filterId == QStringLiteral("decimate_polyline")) {
            connect(action, &QAction::triggered, this, [this](bool) {
                auto scene = rendererWidget ? rendererWidget->GetScene() : nullptr;
                auto currentModel = scene ? scene->GetCurrentModel() : nullptr;
                auto input = currentModel ? currentModel->GetDataObject() : nullptr;
                if (!DecimatePolylineFilter::CanProcessInput(input)) {
                    showDarkFramelessMessage(
                            QStringLiteral("折线简化失败"),
                            QStringLiteral("当前数据不支持该算法。"));
                    return;
                }

                auto* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(QStringLiteral("折线简化 (Decimate Polyline)"));
                dialog->setMinimumWidth(520);
                dialog->setFilterDescription(
                        QStringLiteral("与 ParaView 一致，支持角度、自定义点字段和距离三种策略；"
                                       "每条折线独立简化，端点始终保留。"));
                const int reductionId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT,
                        QStringLiteral("目标缩减率 (0..1)"), QStringLiteral("0.9"));
                const int maximumErrorId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT,
                        QStringLiteral("最大误差 (Maximum Error)"),
                        QStringLiteral("1.7976931348623157e+308"));
                const int strategyId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_COMBO_BOX,
                        QStringLiteral("简化策略 (Decimation Strategy)"),
                        std::vector<QString>{QStringLiteral("角度策略 (Angle)"),
                                             QStringLiteral("自定义字段策略 (Custom Field)"),
                                             QStringLiteral("距离策略 (Distance)")});
                std::vector<QString> customFieldNames;
                auto inputPointSet = DynamicCast<PointSet>(input);
                auto attributes = input->GetAttributeSet();
                auto allAttributes = attributes ? attributes->GetAllAttributes() : nullptr;
                if (allAttributes) {
                    for (IGsize i = 0; i < allAttributes->GetNumberOfElements(); ++i) {
                        const auto& attribute = allAttributes->GetElement(i);
                        if (attribute.isDeleted || attribute.attachmentType != IG_POINT ||
                            !attribute.pointer || attribute.pointer->GetDimension() < 1 ||
                            attribute.pointer->GetName().empty() ||
                            !inputPointSet || attribute.pointer->GetNumberOfElements() <
                                                      inputPointSet->GetNumberOfPoints()) {
                            continue;
                        }
                        switch (attribute.pointer->GetArrayType()) {
                        case IG_FloatArray:
                        case IG_DoubleArray:
                        case IG_IntArray:
                        case IG_UnsignedIntArray:
                        case IG_CharArray:
                        case IG_UnsignedCharArray:
                        case IG_ShortArray:
                        case IG_UnsignedShortArray:
                        case IG_LongLongArray:
                        case IG_UnsignedLongLongArray:
                            customFieldNames.push_back(
                                    QString::fromStdString(attribute.pointer->GetName()));
                            break;
                        default: break;
                        }
                    }
                }
                const int customFieldId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_COMBO_BOX,
                        QStringLiteral("点字段 (Field Name)"),
                        customFieldNames.empty()
                                ? std::vector<QString>{QStringLiteral("<无可用点字段>")}
                                : customFieldNames);
                auto* strategyCombo = qobject_cast<QComboBox*>(dialog->getWidget(strategyId));
                auto* customFieldCombo = qobject_cast<QComboBox*>(dialog->getWidget(customFieldId));
                if (strategyCombo) strategyCombo->setCurrentIndex(2);

                // ParaView exposes FieldName as a child property of the custom-field
                // strategy. Keep the whole row hidden for Angle/Distance and reveal it
                // only when Custom Field is selected.
                auto findRowLabel = [](QWidget* value) -> QLabel* {
                    if (!value || !value->parentWidget()) return nullptr;
                    const auto grids = value->parentWidget()->findChildren<QGridLayout*>();
                    for (auto* grid : grids) {
                        const int itemIndex = grid->indexOf(value);
                        if (itemIndex < 0) continue;
                        int row = 0, column = 0, rowSpan = 0, columnSpan = 0;
                        grid->getItemPosition(itemIndex, &row, &column, &rowSpan, &columnSpan);
                        for (int c = 0; c < grid->columnCount(); ++c) {
                            auto* item = grid->itemAtPosition(row, c);
                            if (!item || item->widget() == value) continue;
                            if (auto* label = qobject_cast<QLabel*>(item->widget())) return label;
                        }
                    }
                    return nullptr;
                };
                auto* customFieldLabel = findRowLabel(customFieldCombo);
                auto setCustomFieldVisible = [customFieldCombo, customFieldLabel](bool visible) {
                    if (customFieldLabel) customFieldLabel->setVisible(visible);
                    if (customFieldCombo) customFieldCombo->setVisible(visible);
                };
                setCustomFieldVisible(false);
                if (strategyCombo && customFieldCombo) {
                    connect(strategyCombo, qOverload<int>(&QComboBox::currentIndexChanged), dialog,
                            [setCustomFieldVisible](int index) {
                                setCustomFieldVisible(index == 1);
                            });
                }
                dialog->show();

                dialog->setApplyFunctor([=, this]() {
                    bool reductionOk = false;
                    bool maximumErrorOk = false;
                    bool strategyOk = false;
                    const double targetReduction = dialog->getDouble(reductionId, reductionOk);
                    const double maximumError = dialog->getDouble(maximumErrorId, maximumErrorOk);
                    const int strategy = dialog->getComboIndex(strategyId, strategyOk);
                    if (!reductionOk || !std::isfinite(targetReduction) ||
                        targetReduction < 0.0 || targetReduction > 1.0) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("目标缩减率必须是 0 到 1 之间的数值。"));
                        return;
                    }
                    if (!maximumErrorOk || !std::isfinite(maximumError) || maximumError < 0.0) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("最大误差必须是非负数值。"));
                        return;
                    }
                    if (!strategyOk || strategy < 0 || strategy > 2) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("请选择有效的简化策略。"));
                        return;
                    }
                    if (strategy == 1 && customFieldNames.empty()) {
                        showDarkFramelessMessage(
                                QStringLiteral("参数错误"),
                                QStringLiteral("自定义字段策略需要至少一个数值型点数据数组。"));
                        return;
                    }

                    auto filter = DecimatePolylineFilter::New();
                    filter->SetInput(input);
                    filter->SetTargetReduction(targetReduction);
                    filter->SetMaximumError(maximumError);
                    if (strategy == 0) {
                        filter->SetDecimationStrategy(
                                DecimatePolylineFilter::DecimationStrategy::Angle);
                    } else if (strategy == 1) {
                        filter->SetDecimationStrategy(
                                DecimatePolylineFilter::DecimationStrategy::CustomField);
                        if (customFieldCombo) {
                            filter->SetCustomFieldName(
                                    customFieldCombo->currentText().toStdString());
                        }
                    } else {
                        filter->SetDecimationStrategy(
                                DecimatePolylineFilter::DecimationStrategy::Distance);
                    }
                    if (!filter->Execute()) {
                        showDarkFramelessMessage(
                                QStringLiteral("执行出错"),
                                QStringLiteral("Poly Data 必须包含显式 LINES；自定义字段策略还要求"
                                               "选择有效的数值型点数据数组。"));
                        return;
                    }

                    auto output = filter->GetOutput();
                    output->SetName(input->GetName() + "_DecimatePolyline");
                    if (auto outputDrawObject = DynamicCast<DrawObject>(output)) {
                        // A line-only result is clearest when both retained vertices and segments are shown.
                        outputDrawObject->SetViewStyle(IG_POINTS | IG_WIREFRAME);
                        outputDrawObject->SetPointSize(7.0f);
                        outputDrawObject->SetLineWidth(3.0f);
                    }
                    // ParaView hides the source when a filter result is applied. The eye icon can
                    // still be used to show it again for an overlay comparison.
                    if (auto* inputItem = modelTreeWidget->getItemFromObject(input)) {
                        inputItem->changeVisibility(false);
                    }
                    modelTreeWidget->addDataObjectToModelTree(output, ItemSource::Algorithm);
                    rendererWidget->update();
                    dialog->close();
                });
            });
            return true;
        }
        if (filterId == QStringLiteral("integrate_variables")) {
            connect(action, &QAction::triggered, this, [this](bool) {
        auto* scene = rendererWidget->GetScene();
        auto model = scene ? scene->GetCurrentModel() : nullptr;
        auto data = model ? model->GetDataObject() : nullptr;
        if (data == nullptr) {
            showDarkFramelessMessage(QStringLiteral("No Model Available"),
                                     QStringLiteral("Please load and select a model first."));
            return;
        }
        const auto dtype = data->GetDataObjectType();
        if (dtype != IG_SURFACE_MESH && dtype != IG_VOLUME_MESH &&
            dtype != IG_UNSTRUCTURED_MESH && dtype != IG_STRUCTURED_MESH) {
            showDarkFramelessMessage(
                    QStringLiteral("Unsupported Model"),
                    QStringLiteral("Integrate Variables supports surface, volume, unstructured, and structured meshes."));
            return;
        }

        auto* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("积分变量 (Integrate Variables)"));
        dialog->setFilterDescription(QStringLiteral(
                "对最高维度单元的点属性和单元属性进行积分。输出为一个点，并保存积分结果及总长度、总面积或总体积。"));

        dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX,
                             QStringLiteral("积分策略"),
                             std::vector<QString>{QStringLiteral("Linear Strategy")});
        const int divideCellDataId = dialog->addParameter(
                igQtFilterDialogDockWidget::QT_CHECK_BOX,
                QStringLiteral("单元数据除以总测度"),
                QStringLiteral("false"));

        dialog->setApplyFunctor([this, dialog, data, divideCellDataId]() {
            bool optionOk = false;
            const bool divideCellData = dialog->getChecked(divideCellDataId, optionOk);
            if (!optionOk) {
                showDarkFramelessMessage(QStringLiteral("Warning"),
                                         QStringLiteral("无法读取积分选项。"));
                return;
            }

            IntegrateVariablesFilter::Pointer filter = IntegrateVariablesFilter::New();
            filter->SetInput(data);
            filter->SetDivideAllCellDataByMeasure(divideCellData);
            if (!filter->Execute()) {
                std::string message = filter->GetMessage();
                if (message.empty()) message = "IntegrateVariablesFilter execute failed";
                showDarkFramelessMessage(QStringLiteral("Warning"),
                                         QString::fromStdString(message));
                return;
            }

            auto output = filter->GetOutput();
            if (!output) {
                showDarkFramelessMessage(QStringLiteral("Warning"),
                                         QStringLiteral("积分算法没有生成有效输出。"));
                return;
            }

            modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
            if (auto item = modelTreeWidget->getItemFromObject(output)) item->setExpanded(true);
            rendererWidget->update();
            dialog->hide();
            dialog->deleteLater();

            ui->dockWidget_SearchInfo->show();
            ui->dockWidget_SearchInfo->raise();
            ui->widget_SearchInfo->setCurrentModel(rendererWidget->GetScene()->GetCurrentModel());
        });
        dialog->resize(480, 320);
        dialog->show();
    });
            return true;
        }
        if (filterId == QStringLiteral("multiblock_surface_as_multiblock")) {
            connect(action, &QAction::triggered, this, [this, currentFilterInput, refreshFilterResult]() {
        const QString title = QStringLiteral("多块模型表面提取");
        auto input = currentFilterInput(title);
        if (!input) return;

        auto multiBlockFilter = MultiBlockGeometryFilter::New();
        multiBlockFilter->SetInput(input);
        if (!multiBlockFilter->Execute()) {
            QString errorMsg = QString::fromStdString(multiBlockFilter->GetMessage());
            if (errorMsg.isEmpty()) { errorMsg = QStringLiteral("多块模型表面提取失败。"); }
            showDarkFramelessMessage(title, errorMsg);
            return;
        }

        auto multiBlockObj = multiBlockFilter->GetOutput();
        if (multiBlockObj) {
            multiBlockObj->SetName(input->GetName() + "_MultiBlockSurface");
        }
        refreshFilterResult(input, multiBlockObj, title);

        // 子构件层面的异常信息汇总展示（避免静默丢块）
        const auto& failedBlocks = multiBlockFilter->GetFailedBlocks();
        if (!failedBlocks.empty()) {
            QString warningMsg = QStringLiteral("多块表面提取已完成，但以下 %1 个子构件处理异常（已为您原样保留）：\n")
                                     .arg(failedBlocks.size());
            for (const auto& block : failedBlocks) {
                warningMsg += QStringLiteral("• [%1]: %2\n")
                                  .arg(QString::fromStdString(block.path), QString::fromStdString(block.reason));
            }
            showDarkFramelessMessage(QStringLiteral("部分子构件提示"), warningMsg);
        }
    });
            return true;
        }
        if (filterId == QStringLiteral("ribbon")) {
            connect(action, &QAction::triggered, this, [this](bool checked) {
                auto* scene = rendererWidget->GetScene();
                if (scene == nullptr || scene->GetCurrentModel() == nullptr) {
                    showDarkFramelessMessage(QStringLiteral("无可用模型"),
                                             QStringLiteral("请先加载并选择一个网格模型。"));
                    return;
                }
                auto model = scene->GetCurrentModel();
                auto obj = model->GetDataObject();
                if (obj == nullptr) {
                    showDarkFramelessMessage(QStringLiteral("无可用数据"),
                                             QStringLiteral("当前模型没有可用的网格数据。"));
                    return;
                }

                igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(QStringLiteral("带状面 (Ribbon)"));
                dialog->setFilterDescription(QStringLiteral(
                    "把线/折线沿“切向 × 线法向”构成的框架扫成有向带状面。<br>"
                    "半宽决定带宽：带宽 = 2 × 半宽；偏转角相对线法向旋转带面（0° 时带面垂直于线法向与切向）。<br>"
                    "输入需要包含线单元（IG_LINE / IG_POLY_LINE），其他单元类型会被忽略并在结果提示中说明。"));

                const int widthId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                         QStringLiteral("半宽 (Width)"), "0.5");
                const int angleId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                         QStringLiteral("偏转角/度 (Angle)"), "0");
                const int useDefId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_CHECK_BOX,
                        QStringLiteral("使用默认线法向 (Use Default Normal)"), "true");
                const int nxId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("默认法向 X (Normal X)"), "0");
                const int nyId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("默认法向 Y (Normal Y)"), "0");
                const int nzId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("默认法向 Z (Normal Z)"), "1");

                // 加宽面板，避免说明文字与标签过度换行
                dialog->setFixedWidth(660);

                auto* useDefCheck = qobject_cast<QCheckBox*>(dialog->getWidget(useDefId));
                if (useDefCheck != nullptr) {
                    useDefCheck->setToolTip(QStringLiteral(
                        "勾选时全部点使用下方默认法向；取消勾选则优先取输入的点法向（IG_NORMAL），"
                        "输入没有点法向时回退到默认法向并在结果提示中说明。"));
                }
                const QString normalEditTip = QStringLiteral(
                    "仅在勾选上方“使用默认线法向”后作为带面的定向依据；切线与该法向平行的那条折线会被跳过。");
                for (int id: {nxId, nyId, nzId}) {
                    if (QWidget* editor = dialog->getWidget(id)) { editor->setToolTip(normalEditTip); }
                }
                if (QWidget* angleEdit = dialog->getWidget(angleId)) {
                    angleEdit->setToolTip(QStringLiteral("单位度，取值 0–360；超出范围会被夹到区间内（与 VTK 一致）。"));
                }

                dialog->setApplyFunctor([this, dialog, obj, widthId, angleId, useDefId, nxId, nyId, nzId]() {
                    bool wok = false;
                    bool aok = false;
                    bool uok = false;
                    bool xok = false;
                    bool yok = false;
                    bool zok = false;

                    const double width = dialog->getDouble(widthId, wok);
                    const double angle = dialog->getDouble(angleId, aok);
                    const bool useDefaultNormal = dialog->getChecked(useDefId, uok);
                    const double nx = dialog->getDouble(nxId, xok);
                    const double ny = dialog->getDouble(nyId, yok);
                    const double nz = dialog->getDouble(nzId, zok);

                    if (!wok || width <= 0.0) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("半宽必须是大于 0 的数字（带宽 = 2 × 半宽）。"));
                        return;
                    }
                    if (!aok) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("偏转角不是有效数字。"));
                        return;
                    }
                    if (xok && yok && zok && nx == 0.0 && ny == 0.0 && nz == 0.0) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("默认法向不能是零向量。"));
                        return;
                    }

                    RibbonFilter::Pointer filter = RibbonFilter::New();
                    filter->SetInput(obj);
                    filter->SetWidth(width);
                    filter->SetAngle(angle);
                    filter->SetUseDefaultNormal(uok ? useDefaultNormal : true);
                    if (xok && yok && zok) { filter->SetDefaultNormal(Vector3d(nx, ny, nz)); }

                    if (!filter->Execute()) {
                        // 失败原因（无输入 / 无线单元 / 全被跳过 / 零宽度或零法向）由 filter 写在 m_Message 里
                        const std::string message = filter->GetMessage();
                        showDarkFramelessMessage(
                                QStringLiteral("带状面生成失败"),
                                message.empty() ? QStringLiteral("请检查输入是否包含线/折线单元（IG_LINE / IG_POLY_LINE）。")
                                                : QString::fromStdString(message));
                        return;
                    }

                    auto outMesh = DynamicCast<SurfaceMesh>(filter->GetOutput());
                    if (!outMesh) {
                        showDarkFramelessMessage(QStringLiteral("带状面生成失败"),
                                                 QStringLiteral("算法未产生有效结果。"));
                        return;
                    }

                    modelTreeWidget->addDataObjectToModelTree(outMesh, ItemSource::Algorithm);
                    rendererWidget->update();
                    // 非致命提示（忽略的非线单元、被跳过的折线）只在确实发生时弹一次，
                    // 与 Tube 一致：先弹模态提示再关闭面板，避免 close dock 触发布局重建后再开模态对话框
                    if (!filter->GetMessage().empty()) {
                        showDarkFramelessMessage(QStringLiteral("带状面生成完成"),
                                                 QString::fromStdString(filter->GetMessage()), true);
                    }
                    dialog->close();
                });
                dialog->show();
            });
            return true;
        }
        if (filterId == QStringLiteral("transform") || filterId == QStringLiteral("axis_aligned_transform")) {
            connect(action, &QAction::triggered, this, [=, this]() {
        const QString title = QStringLiteral("通用几何变换");
        // 面板已打开：只把它提到最前
        if (m_transformController != nullptr) {
            m_transformController->FocusPanel();
            return;
        }

        auto input = currentFilterInput(title);
        if (!input) return;

        const IGenum inputType = input->GetDataObjectType();
        if (inputType != IG_SURFACE_MESH && inputType != IG_VOLUME_MESH &&
            inputType != IG_STRUCTURED_MESH && inputType != IG_UNSTRUCTURED_MESH &&
            inputType != IG_POINT_SET) {
            showDarkFramelessMessage(title,
                                     QStringLiteral("请在模型树中选择网格或点云（多帧容器暂不支持交互变换）。"));
            return;
        }

        auto* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(title);
        dialog->setFilterDescription(QStringLiteral(
                "三维盒子交互：按住鼠标<b>中键</b>拖动。<br>"
                "拖棱或中心：平移<br>"
                "拖面手柄：缩放<br>"
                "拖面：旋转"));

        std::array<int, igQtTransformBoxController::kParamCount> parameterIds{};
        parameterIds[0] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("平移 X"), "0");
        parameterIds[1] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("平移 Y"), "0");
        parameterIds[2] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("平移 Z"), "0");
        parameterIds[3] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("旋转 X (度)"), "0");
        parameterIds[4] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("旋转 Y (度)"), "0");
        parameterIds[5] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("旋转 Z (度)"), "0");
        parameterIds[6] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("缩放 X"), "1");
        parameterIds[7] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("缩放 Y"), "1");
        parameterIds[8] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("缩放 Z"), "1");

        // 「重置」按钮：参数回到单位变换、盒子回到初始包围盒（模型不变，需要应用才生效）
        auto* resetRow = new QWidget(dialog);
        auto* resetLayout = new QHBoxLayout(resetRow);
        resetLayout->setContentsMargins(0, 0, 0, 0);
        auto* resetButton = new QPushButton(QStringLiteral("重置"), resetRow);
        resetLayout->addStretch();
        resetLayout->addWidget(resetButton);
        dialog->addRowWidget(resetRow);
        dialog->show();

        m_transformController =
                new igQtTransformBoxController(input, rendererWidget, dialog, parameterIds, this);
        connect(resetButton, &QPushButton::clicked, this, [this]() {
            if (m_transformController != nullptr) { m_transformController->Reset(); }
        });
        connect(m_transformController, &igQtTransformBoxController::Applied, this,
                [=, this](DataObject::Pointer output) {
                    refreshFilterResult(input, output, title);
                    if (auto* item = modelTreeWidget->getItemFromObject(input)) {
                        item->changeVisibility(false);
                        rendererWidget->update();
                    }
                });
        connect(m_transformController, &igQtTransformBoxController::Failed, this,
                [this, title]() { showDarkFramelessMessage(title, QStringLiteral("几何变换失败。")); });
        connect(m_transformController, &igQtTransformBoxController::Closed, this, [this]() {
            if (m_transformController == nullptr) { return; }
            m_transformController->deleteLater();
            m_transformController = nullptr;
        });
        if (!m_transformController->HasBox()) {
            showDarkFramelessMessage(title, QStringLiteral("交互盒子创建失败，仍可手动输入参数后应用。"));
        }

        dialog->setApplyFunctor([this]() {
            if (m_transformController != nullptr) { m_transformController->Apply(); }
        });
    });
            return true;
        }
        if (filterId == QStringLiteral("warp_by_vector")) {
            connect(action, &QAction::triggered, this, [this](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        if (!obj) return;

        // 收集可作为形变向量的数组:优先点关联的三维向量
        std::vector<QString> vectorItems;
        std::vector<std::string> vectorNames;
        if (auto attrs = obj->GetAttributeSet()) {
            for (IGsize i = 0; i < static_cast<IGsize>(attrs->GetNumberOfAttributes()); ++i) {
                auto& attr = attrs->GetAttribute(i);
                if (attr.isDeleted || !attr.pointer) continue;
                if (attr.attachmentType != IG_POINT) continue;
                if (attr.type != IG_VECTOR && attr.pointer->GetDimension() < 3) continue;
                const std::string name = attr.pointer->GetName();
                vectorNames.push_back(name);
                vectorItems.push_back(QStringLiteral("%1 (dim=%2)")
                                              .arg(QString::fromStdString(name))
                                              .arg(attr.pointer->GetDimension()));
            }
        }
        if (vectorItems.empty()) {
            showDarkFramelessMessage(QStringLiteral("Warning"),
                                     QStringLiteral("当前模型没有点关联的三维向量数组，无法进行向量形变。"));
            return;
        }

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("按向量变形 (Warp By Vector)"));
        int vectorId = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX, QStringLiteral("向量 (Vectors)"),
                                            vectorItems);
        int scaleId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                           QStringLiteral("缩放系数 (Scale Factor)"), "1");
        int autoScaleId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                               QStringLiteral("自动缩放(按模型尺度)"), "false");
        int limitId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                           QStringLiteral("最大位移比例 (0 = 不限)"), "0");
        dialog->show();
        dialog->setApplyFunctor([=, this]() {
            bool ok = false;
            const int choice = dialog->getComboIndex(vectorId, ok);
            if (!ok || choice < 0 || choice >= static_cast<int>(vectorNames.size())) {
                showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("请选择有效的向量数组。"));
                return;
            }
            const double scale = dialog->getDouble(scaleId, ok);
            const bool autoScale = dialog->getChecked(autoScaleId, ok);
            const double limit = dialog->getDouble(limitId, ok);

            auto filter = WarpByVectorFilter::New();
            filter->SetInput(obj);
            filter->SetVectorArrayName(vectorNames[static_cast<size_t>(choice)]);
            filter->SetScaleFactor(scale);
            filter->SetAutoScale(autoScale);
            filter->SetMaxDisplacementRatio(limit > 0.0 ? limit : 0.0);
            if (!filter->Execute()) {
                showDarkFramelessMessage(QStringLiteral("Warning"),
                                         QStringLiteral("向量形变失败，请检查向量数组与缩放系数。"));
                return;
            }

            // 输出为独立的新数据对象,原模型保持不变
            auto warpOutput = filter->GetOutput();
            if (!warpOutput) {
                showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("向量形变未产生有效结果。"));
                return;
            }
            warpOutput->SetName(obj->GetName() + "_warp");
            modelTreeWidget->addDataObjectToModelTree(warpOutput, Algorithm);
            rendererWidget->update();
            dialog->close();
        });
    });
            return true;
        }
        if (filterId == QStringLiteral("generate_surface_tangents")) {
            connect(action, &QAction::triggered, this, [this](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        auto data = model->GetDataObject();
        if (data == nullptr || data->GetPoints() == nullptr) {
            showDarkFramelessMessage(QStringLiteral("Warning"),
                                     QStringLiteral("当前模型没有点数据，无法计算表面切向量。"));
            return;
        }

        // 先把模型里的纹理坐标点属性（IG_TCOORD，至少 2 个分量）列出来给用户选
        auto attrs = data->GetAttributeSet();
        std::vector<QString> texLabels;
        std::vector<std::string> texNames;
        if (attrs) {
            for (IGsize i = 0; i < static_cast<IGsize>(attrs->GetNumberOfAttributes()); ++i) {
                auto& attr = attrs->GetAttribute(i);
                if (attr.isDeleted || !attr.pointer) continue;
                if (attr.attachmentType != IG_POINT || attr.type != IG_TCOORD) continue;
                if (attr.pointer->GetDimension() < 2) continue;
                texNames.push_back(attr.pointer->GetName());
                texLabels.push_back(QStringLiteral("%1（%2 个分量）")
                                            .arg(QString::fromStdString(attr.pointer->GetName()))
                                            .arg(attr.pointer->GetDimension()));
            }
        }
        if (texLabels.empty()) {
            showDarkFramelessMessage(
                    QStringLiteral("Warning"),
                    QStringLiteral("当前模型没有纹理坐标（IG_TCOORD 点属性），<br>"
                                   "切向量必须由纹理坐标算出来。<br>"
                                   "请先做一次「球面纹理坐标 (Texture Map To Sphere)」之类的 filter。"));
            return;
        }

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("表面切向量 (Generate Surface Tangents)"));
        int texId =
                dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX, QStringLiteral("纹理坐标"), texLabels);
        int pointId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                           QStringLiteral("输出点切向量 Tangents"), "true");
        int cellId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                          QStringLiteral("输出单元切向量（不归一化）"), "false");
        int bitangentId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                               QStringLiteral("输出副切向量 Bitangents"), "false");
        dialog->setApplyFunctor([=, this]() {
            bool okTex = false, okPoint = false, okCell = false, okBitangent = false;
            const int texIndex = dialog->getComboIndex(texId, okTex);
            const bool computePoint = dialog->getChecked(pointId, okPoint);
            const bool computeCell = dialog->getChecked(cellId, okCell);
            const bool computeBitangent = dialog->getChecked(bitangentId, okBitangent);
            if (!okPoint || !okCell || !okBitangent) {
                showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("参数读取失败"));
                return;
            }
            if (!computePoint && !computeCell && !computeBitangent) {
                showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("至少勾选一种切向量输出"));
                return;
            }

            auto filter = iGame::GenerateSurfaceTangentsFilter::New();
            if (okTex && texIndex >= 0 && texIndex < static_cast<int>(texNames.size())) {
                filter->SetTextureCoordinatesArrayName(texNames[texIndex]);
            }
            filter->SetComputePointTangents(computePoint);
            filter->SetComputeCellTangents(computeCell);
            filter->SetComputeBitangents(computeBitangent);
            filter->SetInput(0, data);
            if (!filter->Execute()) {
                showDarkFramelessMessage(QStringLiteral("Warning"),
                                         QStringLiteral("表面切向量执行失败：<br>"
                                                        "模型需要有纹理坐标，以及三角形 / 四边形 / 多边形单元"
                                                        "（体网格请先「转换为表面网格」）。<br>"
                                                        "细节见日志。"));
                return;
            }
            auto outObj = filter->GetOutput(); // 独立输出，原模型不变
            if (auto drawObject = iGame::DynamicCast<iGame::DrawObject>(outObj)) {
                drawObject->ForceReConvertToDrawableData();
            }
            modelTreeWidget->addDataObjectToModelTree(outObj, Algorithm);
            // 把新节点选成当前模型
            if (auto item = modelTreeWidget->getItemFromObject(outObj)) { modelTreeWidget->setCurrentItem(item); }
            rendererWidget->update();
            showDarkFramelessMessage(
                    QStringLiteral("表面切向量"),
                    QStringLiteral("已生成点属性 Tangents（单位向量，方向沿纹理坐标 u 增大的方向）。<br>"), true);
            dialog->close();
        });
        dialog->show();
    });
            return true;
        }
        if (filterId == QStringLiteral("group_datasets")) {
            connect(action, &QAction::triggered, this, [this](bool) {
                // 1) 读取模型树中当前选中的数据对象（支持多选，也支持多块装配体的子块）
                auto inputs = modelTreeWidget->GetSelectedDataObjects();
                if (inputs.empty()) {
                    showDarkFramelessMessage(
                            QStringLiteral("合并数据集"),
                            QStringLiteral("请先在左侧模型树中选中对象。\n\n"
                                           "• 多选：按住 Ctrl 或 Shift 点击\n"
                                           "• 支持选中多块装配体的子块"));
                    return;
                }

                // 2) 唯一命名。必须在注册进场景【之前】算，因为它扫描的是场景的模型列表。
                const std::string groupName = UniqueGroupDatasetsName();

                // 3) 执行组合（只做容器组合，不合并几何）
                auto filter = iGame::GroupDatasetsFilter::New();
                filter->SetInputs(inputs);
                filter->SetOutputName(groupName);
                if (!filter->Execute()) {
                    showDarkFramelessMessage(QStringLiteral("合并数据集失败"),
                                             QString::fromStdString(filter->GetMessage()));
                    return;
                }

                auto group = filter->GetOutput(0);
                if (!group) {
                    showDarkFramelessMessage(QStringLiteral("合并数据集失败"),
                                             QStringLiteral("未生成结果节点。"));
                    return;
                }

                // 4) 挂到场景 + 模型树。
                //    注意：addDataObjectToModelTree 内部已经调用 scene->AddModel()，
                //    这里不要再单独 AddModel，否则会重复注册出冗余节点。
                modelTreeWidget->addDataObjectToModelTree(group, ItemSource::Algorithm);

                // 5) 【不隐藏原始输入】。
                //
                //    组的每个子块都是原模型的【深拷贝克隆】（几何 / 属性 / 显隐样式各持一份），
                //    隐藏原模型不会影响组的内容，隐藏组内的块也不会影响原模型 —— 新旧完全独立。
                //    因此原模型保持原状即可；这里仅刷新全树眼睛图标，让每处图标都反映
                //    各自对象的真实显隐状态。
                modelTreeWidget->RefreshModelTreeVisibilityIcons();

                rendererWidget->update();

                // 6) 汇报结果（子块按序号列出，重名也能区分）
                QString blockList;
                const auto& blockNames = filter->GetBlockNames();
                for (size_t i = 0; i < blockNames.size(); ++i) {
                    blockList += QStringLiteral("%1. %2\n")
                                         .arg(static_cast<int>(i) + 1)
                                         .arg(QString::fromStdString(blockNames[i]));
                }
                const QString report = QStringLiteral("已合并 %1 个对象 → %2\n%3")
                                               .arg(filter->GetGroupedCount())
                                               .arg(QString::fromStdString(groupName))
                                               .arg(blockList);
                showDarkFramelessMessage(QStringLiteral("合并数据集"), report, true);
            });
            return true;
        }
        if (filterId == QStringLiteral("median")) {
            connect(action, &QAction::triggered, this, [this](bool) {
        openLeftToolPanel(LeftToolPanelId::MedianFilter);
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        if (!scene) return;
        auto CurrentModel = scene->GetCurrentModel();
        if (!CurrentModel) return;
        auto dataObject = CurrentModel->GetDataObject();
        if (!dataObject) return;
        ui->widget_MedianFilter->SetOriginDataObject(dataObject);
    });
            return true;
        }
        if (filterId == QStringLiteral("pass_arrays")) {
            connect(action, &QAction::triggered, this, [=, this]() {
        const QString title = QStringLiteral("传递过滤数据数组");
        auto input = currentFilterInput(title);
        if (!input) return;

        auto attrSet = input->GetAttributeSet();
        if (attrSet == nullptr) {
            showDarkFramelessMessage(title, QStringLiteral("当前模型没有属性集。"));
            return;
        }

        // 收集属性名称：点数据与单元数据分开收集，后续也分开记录勾选结果
        QStringList pointNames;
        QStringList cellNames;
        auto collectNames = [](auto attrs, QStringList& out) {
            if (!attrs) return;
            for (IGsize i = 0; i < attrs->GetNumberOfElements(); ++i) {
                auto& attr = attrs->GetElement(i);
                if (attr.IsNone() || !attr.pointer) continue;
                out.append(QString::fromStdString(attr.pointer->GetName()));
            }
        };
        collectNames(attrSet->GetAllPointAttributes(), pointNames);
        collectNames(attrSet->GetAllCellAttributes(), cellNames);
        if (pointNames.isEmpty() && cellNames.isEmpty()) {
            showDarkFramelessMessage(title, QStringLiteral("当前模型没有任何属性可传递。"));
            return;
        }

        // ---------- 属性多选框对话框 ----------
        igQtChromeFramelessDialog* dlg = new igQtChromeFramelessDialog(this);
        dlg->setDialogTitle(QStringLiteral("选择要传递的属性"));
        dlg->setMaximizeEnabled(false);
        dlg->setAttribute(Qt::WA_DeleteOnClose);

        QWidget* content = new QWidget(dlg->contentHost());
        content->setStyleSheet(
                "QWidget { background-color: #252526; color: #D4D4D4; }"
                "QCheckBox { color: #D4D4D4; spacing: 6px; }"
                "QCheckBox::indicator { width: 16px; height: 16px; }"
                "QCheckBox::indicator:unchecked { border: 1px solid #6A6A6A; background-color: #3A3A3A; }"
                "QCheckBox::indicator:checked { border: 1px solid #0E639C; background-color: #0E639C; }"
                "QCheckBox::indicator:unchecked:hover { border: 1px solid #9A9A9A; }"
                "QCheckBox::indicator:checked:hover { background-color: #1177BB; }"
                "QPushButton { background-color: #3A3A3A; color: #D4D4D4; border: 1px solid #4A4A4A; "
                "              padding: 6px 12px; border-radius: 4px; }"
                "QPushButton:hover { background-color: #4A4A4A; border-color: #5A5A5A; }"
                "QPushButton:pressed { background-color: #2A2A2A; }"
                "QScrollArea { background-color: #1E1E1E; border: none; }"
                "QScrollBar:vertical { background: #1E1E1E; width: 10px; margin: 0; }"
                "QScrollBar::handle:vertical { background: #4A4A4A; border-radius: 5px; min-height: 20px; }"
                "QScrollBar::handle:vertical:hover { background: #5A5A5A; }"
                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }");
        QVBoxLayout* mainLayout = new QVBoxLayout(content);
        mainLayout->setContentsMargins(12, 12, 12, 12);
        mainLayout->setSpacing(8);

        QLabel* tip = new QLabel(
                QStringLiteral("勾选需要传递到输出的属性，未勾选的属性会被丢弃。"),
                content);
        tip->setWordWrap(true);
        mainLayout->addWidget(tip);

        QScrollArea* scrollArea = new QScrollArea(content);
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        QWidget* listWidget = new QWidget(scrollArea);
        QVBoxLayout* listLayout = new QVBoxLayout(listWidget);
        listLayout->setSpacing(4);
        listLayout->setContentsMargins(0, 0, 0, 0);

        QList<QCheckBox*> pointBoxes;
        QList<QCheckBox*> cellBoxes;
        auto addSection = [&](const QString& header, const QStringList& names, const QString& emptyTip,
                              QList<QCheckBox*>& boxes) {
            QLabel* headerLabel = new QLabel(header, listWidget);
            QFont headerFont = headerLabel->font();
            headerFont.setBold(true);
            headerLabel->setFont(headerFont);
            listLayout->addWidget(headerLabel);

            if (names.isEmpty()) {
                listLayout->addWidget(new QLabel(emptyTip, listWidget));
                return;
            }

            QHBoxLayout* btnRow = new QHBoxLayout();
            btnRow->setContentsMargins(0, 0, 0, 0);
            QPushButton* selectAllBtn = new QPushButton(QStringLiteral("全选"), listWidget);
            QPushButton* deselectAllBtn = new QPushButton(QStringLiteral("全不选"), listWidget);
            btnRow->addWidget(selectAllBtn);
            btnRow->addWidget(deselectAllBtn);
            btnRow->addStretch();
            listLayout->addLayout(btnRow);

            for (const QString& name: names) {
                QCheckBox* cb = new QCheckBox(name, listWidget);
                cb->setChecked(true); // 默认全部选中
                boxes.append(cb);
                listLayout->addWidget(cb);
            }
            // 按值捕获复选框列表：列表内容此时已经填充完毕
            const QList<QCheckBox*> sectionBoxes = boxes;
            connect(selectAllBtn, &QPushButton::clicked, listWidget,
                    [sectionBoxes]() { for (QCheckBox* cb: sectionBoxes) cb->setChecked(true); });
            connect(deselectAllBtn, &QPushButton::clicked, listWidget,
                    [sectionBoxes]() { for (QCheckBox* cb: sectionBoxes) cb->setChecked(false); });
        };
        addSection(QStringLiteral("点数据 (Point Data)"), pointNames,
                   QStringLiteral("（无点属性）"), pointBoxes);
        addSection(QStringLiteral("单元数据 (Cell Data)"), cellNames,
                   QStringLiteral("（无单元属性）"), cellBoxes);
        listWidget->setLayout(listLayout);
        scrollArea->setWidget(listWidget);
        mainLayout->addWidget(scrollArea);

        QDialogButtonBox* buttonBox =
                new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, content);
        mainLayout->addWidget(buttonBox);

        dlg->setContentWidget(content);
        dlg->resize(420, 560);

        connect(buttonBox, &QDialogButtonBox::accepted, this,
                [this, dlg, input, pointBoxes, cellBoxes, pointNames, cellNames, title,
                 refreshFilterResult]() {
            std::vector<std::string> selectedPointNames;
            std::vector<std::string> selectedCellNames;
            for (int i = 0; i < pointBoxes.size(); ++i) {
                if (pointBoxes[i]->isChecked()) selectedPointNames.push_back(pointNames[i].toStdString());
            }
            for (int i = 0; i < cellBoxes.size(); ++i) {
                if (cellBoxes[i]->isChecked()) selectedCellNames.push_back(cellNames[i].toStdString());
            }
            if (selectedPointNames.empty() && selectedCellNames.empty()) {
                showDarkFramelessMessage(title, QStringLiteral("请至少勾选一个属性。"));
                return;
            }

            auto filter = PassArrays::New();
            filter->SetInput(input);
            // 点数据与单元数据的勾选结果分别记录，同名属性不会互相牵连
            filter->SetPointArrayNames(selectedPointNames);
            filter->SetCellArrayNames(selectedCellNames);
            if (!filter->Execute()) {
                showDarkFramelessMessage(title, QStringLiteral("数组过滤失败：输入数据类型不受支持。"));
                return;
            }
            auto output = filter->GetOutput();
            if (!output) {
                showDarkFramelessMessage(title, QStringLiteral("未得到输出数据。"));
                return;
            }
            output->SetName(input->GetName() + "_filtered");
            refreshFilterResult(input, output, title);
            dlg->accept();
        });
        connect(buttonBox, &QDialogButtonBox::rejected, dlg, &QDialog::reject);

        dlg->show();
    });
            return true;
        }
        if (filterId == QStringLiteral("perlin_noise")) {
            connect(action, &QAction::triggered, this, [this](bool) {
                auto scene = rendererWidget ? rendererWidget->GetScene() : nullptr;
                auto currentModel = scene ? scene->GetCurrentModel() : nullptr;
                if (!currentModel) {
                    showDarkFramelessMessage(QStringLiteral("Perlin 噪声"), QStringLiteral("请先选择一个模型。"));
                    return;
                }
                auto object = currentModel->GetDataObject();
                if (object == nullptr || iGame::DynamicCast<iGame::PointSet>(object).IsNull()) {
                    showDarkFramelessMessage(QStringLiteral("Perlin 噪声"),
                                             QStringLiteral("当前对象不支持 Perlin 噪声（需要网格 / 点集）"));
                    return;
                }

                auto* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(QStringLiteral("Perlin 噪声"));
                dialog->setFilterDescription(
                        QStringLiteral("在每个输入点上采样 3D Perlin 噪声，输出新模型 + 点标量数组"));
                const int ampId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                       QStringLiteral("振幅 (Amplitude)"), "1");
                const int fxId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("频率 X (Frequency X)"), "1");
                const int fyId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("频率 Y (Frequency Y)"), "1");
                const int fzId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("频率 Z (Frequency Z)"), "1");
                const int pxId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("相位 X (Phase X)"), "0");
                const int pyId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("相位 Y (Phase Y)"), "0");
                const int pzId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("相位 Z (Phase Z)"), "0");
                dialog->show();
                dialog->setApplyFunctor([=, this]() {
                    const int ids[7] = {ampId, fxId, fyId, fzId, pxId, pyId, pzId};
                    const char* names[7] = {"振幅 (Amplitude)", "频率 X (Frequency X)",
                                            "频率 Y (Frequency Y)", "频率 Z (Frequency Z)",
                                            "相位 X (Phase X)", "相位 Y (Phase Y)",
                                            "相位 Z (Phase Z)"};
                    double values[7] = {0.0};
                    bool ok = false;
                    for (int i = 0; i < 7; ++i) {
                        values[i] = dialog->getDouble(ids[i], ok);
                        if (!ok) {
                            showDarkFramelessMessage(
                                    QStringLiteral("错误"),
                                    QString::fromUtf8(names[i]) + QStringLiteral(" 不是有效数字。"));
                            return;
                        }
                    }

                    auto filter = PerlinNoiseFilter::New();
                    filter->SetAmplitude(values[0]);
                    filter->SetFrequency(values[1], values[2], values[3]);
                    filter->SetPhase(values[4], values[5], values[6]);
                    filter->SetInput(object);
                    if (filter->Execute()) {
                        modelTreeWidget->addDataObjectToModelTree(filter->GetOutput(), ItemSource::Algorithm);
                        modelTreeWidget->updateCloudPicture();
                        rendererWidget->update();
                        dialog->close();
                    } else {
                        showDarkFramelessMessage(QStringLiteral("错误"),
                                                 QStringLiteral("Perlin 噪声生成失败（需要网格 / 点集）"));
                    }
                });
            });
            return true;
        }
        if (filterId == QStringLiteral("texture_map_to_plane")) {
            connect(action, &QAction::triggered, this, [this]() {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (!model) {
            showDarkFramelessMessage(QStringLiteral("生成平面纹理坐标"),
                                     QStringLiteral("请先在模型树中选择一个包含点数据的网格模型。"));
            return;
        }
        auto* panel = new igQtTextureMapToPlaneWidget(rendererWidget, modelTreeWidget, model, this);
        if (!panel->isReady()) {
            showDarkFramelessMessage(QStringLiteral("生成平面纹理坐标"),
                                     QStringLiteral("当前模型不属于 PointSet 网格体系，或模型中没有点；操作已取消。"));
            panel->deleteLater();
            return;
        }
        panel->show();
        panel->raise();
        panel->activateWindow();
    });
            return true;
        }
        if (filterId == QStringLiteral("texture_map_to_sphere")) {
            connect(action, &QAction::triggered, this, [this](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        auto data = model->GetDataObject();
        if (data == nullptr || data->GetPoints() == nullptr) {
            showDarkFramelessMessage(QStringLiteral("Warning"),
                                     QStringLiteral("当前模型没有点数据，无法生成球面纹理坐标。"));
            return;
        }

        // 默认值与算法的自动球心一致：所有输入点的坐标平均值。
        double center[3] = {0.0, 0.0, 0.0};
        auto spherePoints = data->GetPoints();
        const IGsize spherePointCount = spherePoints->GetNumberOfPoints();
        for (IGsize i = 0; i < spherePointCount; ++i) {
            const auto& point = spherePoints->GetPoint(i);
            for (int axis = 0; axis < 3; ++axis) { center[axis] += static_cast<double>(point[axis]); }
        }
        if (spherePointCount > 0) {
            for (double& value : center) { value /= static_cast<double>(spherePointCount); }
        }
        const QString defaultX = QString::number(center[0], 'g', 6);
        const QString defaultY = QString::number(center[1], 'g', 6);
        const QString defaultZ = QString::number(center[2], 'g', 6);

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("球面纹理坐标 (Texture Map To Sphere)"));
        int autoCenterId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                                QStringLiteral("自动使用点坐标平均值"), "true");
        int centerXId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("球心 X"),
                                             defaultX);
        int centerYId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("球心 Y"),
                                             defaultY);
        int centerZId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("球心 Z"),
                                             defaultZ);
        int seamId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                          QStringLiteral("防止接缝 (PreventSeam)"), "true");
        dialog->setApplyFunctor([=, this]() {
            bool okAuto = false, okSeam = false;
            const bool autoCenter = dialog->getChecked(autoCenterId, okAuto);
            const bool preventSeam = dialog->getChecked(seamId, okSeam);

            auto filter = iGame::TextureMapToSphereFilter::New();
            filter->SetPreventSeam(preventSeam);
            if (!autoCenter) {
                bool okX = false, okY = false, okZ = false;
                const double cx = dialog->getDouble(centerXId, okX);
                const double cy = dialog->getDouble(centerYId, okY);
                const double cz = dialog->getDouble(centerZId, okZ);
                if (!okX || !okY || !okZ) {
                    showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("球心坐标必须是数字"));
                    return;
                }
                filter->SetCenter(cx, cy, cz);
            }
            filter->SetInput(0, data);
            if (!filter->Execute()) {
                showDarkFramelessMessage(QStringLiteral("Warning"),
                                         QStringLiteral("球面纹理坐标执行失败：不支持的网格类型"));
                return;
            }
            auto outObj = filter->GetOutput(); // 独立输出，原模型不变
            if (auto drawObject = iGame::DynamicCast<iGame::DrawObject>(outObj)) {
                drawObject->ForceReConvertToDrawableData();
            }
            modelTreeWidget->addDataObjectToModelTree(outObj, ItemSource::Algorithm);
            rendererWidget->update();
            showDarkFramelessMessage(QStringLiteral("球面纹理坐标"),
                                     QStringLiteral("已生成点属性 TextureCoordinates（2 分量：s、t）。<br>"
                                                    "可在“查找信息”面板里查看数值。"),
                                     true);
            dialog->close();
        });
        dialog->show();
    });
            return true;
        }
        if (filterId == QStringLiteral("smooth")) {
            connect(action, &QAction::triggered, this, showSmoothPanel);
            return true;
        }
        // Approved filters retain independent entries in the development menus.
        if (filterId == QStringLiteral("validate_cells")) {
            connect(action, &QAction::triggered, this, [this]() {
                auto scene = rendererWidget->GetScene();
                if (scene == nullptr || scene->GetCurrentModel() == nullptr) {
                    showDarkFramelessMessage(QStringLiteral("无可用模型"),
                                             QStringLiteral("请先加载并选择一个网格模型。"));
                    return;
                }
                const auto model = scene->GetCurrentModel();
                const auto sourceModelId = scene->GetCurrentModelID();
                const auto dataObject = model == nullptr ? nullptr : model->GetDataObject();
                if (model == nullptr || dataObject == nullptr ||
                    (DynamicCast<UnstructuredMesh>(dataObject).IsNull() &&
                     DynamicCast<SurfaceMesh>(dataObject).IsNull())) {
                    showDarkFramelessMessage(
                            QStringLiteral("单元几何校验"),
                            QStringLiteral("请先在模型树中选择非结构网格或表面网格。"));
                    return;
                }

                auto filter = ValidateCellsFilter::New();
                filter->SetInput(dataObject);
                if (!filter->Execute()) {
                    const auto& error = filter->GetLastError();
                    showDarkFramelessMessage(
                            QStringLiteral("单元几何校验"),
                            QStringLiteral("执行失败：%1")
                                    .arg(error.empty() ? QStringLiteral("未知错误。")
                                                       : QString::fromStdString(error)));
                    return;
                }

                // Filter 返回独立网格（含 ValidityState 单元标量）；原模型保持不变。
                auto data = filter->GetOutput();
                if (data == nullptr) {
                    showDarkFramelessMessage(QStringLiteral("单元几何校验"),
                                             QStringLiteral("执行失败：未生成有效输出。"));
                    return;
                }

                const auto outputModelId = modelTreeWidget->addDataObjectToModelTree(
                        data, ItemSource::Algorithm);
                auto outputModel = scene->GetModelById(outputModelId);
                const int attributeIndex = data->GetAttributeSet()->GetAttributeIndex(
                        ValidateCellsFilter::ValidityStateArrayName());
                auto item = modelTreeWidget->getItemFromObject(data);
                if (item != nullptr && attributeIndex >= 0 &&
                    attributeIndex < item->childCount()) {
                    item->setExpanded(true);
                    auto* child = item->child(attributeIndex);
                    item->setCurrentChild(child);
                    item->setSelected(false);
                    item->viewAttribute(attributeIndex, 0);
                    child->setSelected(true);
                    modelTreeWidget->setCurrentItem(child);
                }

                const auto& invalidIds = filter->GetInvalidCellIds();
                auto selection = outputModel == nullptr ? nullptr
                                                        : outputModel->GetSelection();
                if (selection != nullptr) {
                    selection->Reset();
                    if (!invalidIds.empty()) {
                        selection->SelectionCallBackEvent(
                                IG_CELL, invalidIds, Selection::Operate::Add);
                        selection->SetSelectItemVisable(true);
                    }
                }

                if (outputModel != nullptr) {
                    model->SetVisibility(false);
                    outputModel->SetVisibility(true);
                    scene->SetCurrentModel(outputModel);

                    // 结果与输入节点互斥显示：选择输入时恢复原始外观，
                    // 选择结果时显示 ValidityState 标量和无效单元高亮。
                    const bool hasInvalid = !invalidIds.empty();
                    connect(modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged, this,
                            [this, scene, sourceModelId, outputModelId, hasInvalid]() {
                                auto sourceModel = scene->GetModelById(sourceModelId);
                                auto resultModel = scene->GetModelById(outputModelId);
                                if (resultModel == nullptr) return;

                                const auto currentModelId = scene->GetCurrentModelID();
                                const bool resultSelected = currentModelId == outputModelId;
                                resultModel->SetVisibility(resultSelected);
                                if (auto resultSelection = resultModel->GetSelection())
                                    resultSelection->SetSelectItemVisable(
                                            resultSelected && hasInvalid);

                                if (sourceModel != nullptr &&
                                    (currentModelId == sourceModelId || resultSelected))
                                    sourceModel->SetVisibility(!resultSelected);
                                rendererWidget->update();
                            });
                }
                rendererWidget->update();

                // ValidityState 取值分布，例如 “0:7400, 16:50, 48:22”。
                std::map<short, int> stateDistribution;
                for (short state : filter->GetValidityStates()) {
                    stateDistribution[state]++;
                }
                QStringList distributionText;
                for (const auto& entry : stateDistribution) {
                    distributionText.push_back(
                            QStringLiteral("%1:%2").arg(entry.first).arg(entry.second));
                }

                showDarkFramelessMessage(
                        QStringLiteral("单元几何校验"),
                        QStringLiteral("校验完成：共 %1 个单元，有效 %2，无效 %3。<br>"
                                       "ValidityState 分布：%4")
                                .arg(filter->GetCheckedCellCount())
                                .arg(filter->GetValidCellCount())
                                .arg(filter->GetInvalidCellCount())
                                .arg(distributionText.join(QStringLiteral(", "))));
    });
            return true;
        }
        if (filterId == QStringLiteral("overlapping_cells_detector")) {
            connect(action, &QAction::triggered, this, [this]() {
        auto scene = rendererWidget->GetScene();
        if (scene == nullptr || scene->GetCurrentModel() == nullptr) return;

        const auto model = scene->GetCurrentModel();
        const auto sourceModelId = scene->GetCurrentModelID();
        const auto dataObject = model == nullptr ? nullptr : model->GetDataObject();
        if (model == nullptr || dataObject == nullptr ||
            (DynamicCast<UnstructuredMesh>(dataObject).IsNull() && DynamicCast<VolumeMesh>(dataObject).IsNull() && DynamicCast<SurfaceMesh>(dataObject).IsNull())) {
            showDarkFramelessMessage(
                    QStringLiteral("检测重叠单元"),
                    QStringLiteral("请选择三角形或平面凸四边形表面网格，或包含线性体单元的网格。"));
            return;
        }

        auto* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("检测重叠单元"));
        dialog->setFilterDescription(QStringLiteral(
                "检测三角形、平面凸四边形的内部相交，或四面体、六面体、三棱柱、金字塔的共同体积。<br>"
                "执行后生成单元标量 <b>NumberOfOverlapsPerCell</b>；值大于 0 的单元以选中边线高亮显示。<br>"
                "正常邻接不计为重叠；凹或扭曲四边形、高阶、多面体、复合数据及二维/三维混合网格暂不支持。"));
        const int toleranceId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                     QStringLiteral("公差 (Tolerance)"), "0.0");
        dialog->show();
        dialog->setApplyFunctor([this, dialog, model, scene, sourceModelId, toleranceId]() {
            bool toleranceOk = false;
            const double tolerance = dialog->getDouble(toleranceId, toleranceOk);
            if (!toleranceOk || tolerance < 0.0) {
                showDarkFramelessMessage(QStringLiteral("检测重叠单元"), QStringLiteral("公差必须是非负数。"));
                return;
            }
            if (model == nullptr) return;

            auto filter = OverlappingCellsDetectorFilter::New();
            filter->SetInput(model->GetDataObject());
            filter->SetTolerance(tolerance);
            if (!filter->Execute()) {
                const auto& error = filter->GetLastError();
                showDarkFramelessMessage(
                        QStringLiteral("检测重叠单元"),
                        QStringLiteral("执行失败：%1")
                                .arg(error.empty() ? QStringLiteral("未知错误。") : QString::fromStdString(error)));
                return;
            }

            // Filter 返回独立网格；原模型及其属性保持不变。
            auto data = filter->GetOutput();
            if (data == nullptr) {
                showDarkFramelessMessage(QStringLiteral("检测重叠单元"),
                                         QStringLiteral("执行失败：未生成有效输出。"));
                return;
            }
            const auto outputModelId = modelTreeWidget->addDataObjectToModelTree(
                    data, ItemSource::Algorithm);
            auto outputModel = scene->GetModelById(outputModelId);
            const int attributeIndex = data->GetAttributeSet()->GetAttributeIndex(
                    OverlappingCellsDetectorFilter::NumberOfOverlapsPerCellArrayName());
            auto item = modelTreeWidget->getItemFromObject(data);
            if (item != nullptr && attributeIndex >= 0 && attributeIndex < item->childCount()) {
                item->setExpanded(true);
                auto* child = item->child(attributeIndex);
                item->setCurrentChild(child);
                item->setSelected(false);
                item->viewAttribute(attributeIndex, 0);
                child->setSelected(true);
                modelTreeWidget->setCurrentItem(child);
            }

            std::vector<igIndex> overlappingCellIds;
            const auto& overlapCounts = filter->GetNumberOfOverlapsPerCell();
            for (igIndex cellId = 0; cellId < overlapCounts.size(); ++cellId) {
                if (overlapCounts[cellId] > 0) overlappingCellIds.push_back(cellId);
            }
            QStringList overlapCountPreview;
            constexpr int kPreviewCount = 20;
            for (int cellId = 0; cellId < static_cast<int>(overlapCounts.size()) && cellId < kPreviewCount; ++cellId) {
                overlapCountPreview.push_back(QString::number(overlapCounts[cellId]));
            }
            const QString countText =
                    overlapCounts.size() <= kPreviewCount
                            ? QStringLiteral("[%1]").arg(overlapCountPreview.join(QStringLiteral(", ")))
                            : QStringLiteral("[%1, ...]").arg(overlapCountPreview.join(QStringLiteral(", ")));
            auto selection = outputModel == nullptr ? nullptr : outputModel->GetSelection();
            if (selection != nullptr) {
                selection->Reset();
                if (!overlappingCellIds.empty()) {
                    selection->SelectionCallBackEvent(IG_CELL, overlappingCellIds, Selection::Operate::Add);
                    selection->SetSelectItemVisable(true);
                }
            }
            if (outputModel != nullptr) {
                model->SetVisibility(false);
                outputModel->SetVisibility(true);
                scene->SetCurrentModel(outputModel);

                // 该结果与输入节点采用互斥显示：选择输入时恢复原始外观，
                // 选择结果时显示标量和重叠高亮，避免结果轮廓覆盖原模型。
                const bool hasOverlaps = !overlappingCellIds.empty();
                connect(modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged, this,
                        [this, scene, sourceModelId, outputModelId, hasOverlaps]() {
                            auto sourceModel = scene->GetModelById(sourceModelId);
                            auto resultModel = scene->GetModelById(outputModelId);
                            if (resultModel == nullptr) return;

                            const auto currentModelId = scene->GetCurrentModelID();
                            const bool resultSelected = currentModelId == outputModelId;
                            resultModel->SetVisibility(resultSelected);
                            if (auto resultSelection = resultModel->GetSelection())
                                resultSelection->SetSelectItemVisable(resultSelected && hasOverlaps);

                            if (sourceModel != nullptr &&
                                (currentModelId == sourceModelId || resultSelected))
                                sourceModel->SetVisibility(!resultSelected);
                            rendererWidget->update();
                        });
            }
            rendererWidget->update();

            showDarkFramelessMessage(QStringLiteral("检测重叠单元"),
                                     QStringLiteral("检测完成：发现 %1 对重叠单元；高亮 %2 个单元。<br>"
                                                    "NumberOfOverlapsPerCell = %3")
                                             .arg(static_cast<qulonglong>(filter->GetOverlappingCellPairs().size()))
                                             .arg(static_cast<qulonglong>(overlappingCellIds.size()))
                                             .arg(countText));
            dialog->close();
        });
    });
            return true;
        }
        if (filterId == QStringLiteral("extract_cells_by_type")) {
            connect(action, &QAction::triggered, this, [=, this]() {
        const QString title = QStringLiteral("按单元类型提取");
        auto input = currentFilterInput(title);
        if (!input) return;
        auto filter = ExtractCellsByTypeFilter::New();
        filter->SetInput(input);
        const auto types = filter->GetAvailableCellTypes();
        if (types.empty()) { showDarkFramelessMessage(title, QStringLiteral("当前模型没有可提取的单元类型。")); return; }
        auto* dock = new QDockWidget(title, this);
        dock->setAttribute(Qt::WA_DeleteOnClose);
        dock->setObjectName(QStringLiteral("dockWidget_ExtractCellsByType"));
        auto* panel = new igQtExtractCellsByTypeWidget(dock);
        panel->SetDataObject(input);
        dock->setWidget(panel);
        addDockWidget(Qt::LeftDockWidgetArea, dock);
        panel->onApply = [=, this, resultModel = Model::Pointer{}]() mutable {
            auto selected = panel->GetSelectedCellTypes();
            if (selected.empty()) { showDarkFramelessMessage(title, QStringLiteral("请至少选择一种单元类型。")); return; }
            filter->SetExtractCellTypes(selected);
            if (!filter->Execute()) { showDarkFramelessMessage(title, QStringLiteral("按类型提取失败。")); return; }
            auto output = filter->GetOutput();
            if (!resultModel) {
                const int id = modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
                resultModel = rendererWidget->GetScene()->GetModelById(id);
            } else {
                resultModel->SetDataObject(output);
                modelTreeWidget->updateItemName(output);
                modelTreeWidget->updateAllAttriubute(output);
            }
            rendererWidget->update();
        };
        dock->show();
        panel->onApply();
    });
            return true;
        }
        if (filterId == QStringLiteral("extract_location")) {
            connect(action, &QAction::triggered, this, [this]() {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (!model) {
            showDarkFramelessMessage(QStringLiteral("提取指定位置数据"),
                                     QStringLiteral("请先在模型树中选择一个网格模型。"));
            return;
        }
        auto* panel = new igQtExtractLocationWidget(rendererWidget, modelTreeWidget, model, this);
        if (!panel->isReady()) {
            showDarkFramelessMessage(QStringLiteral("提取指定位置数据"),
                                     QStringLiteral("当前仅支持非结构网格（UnstructuredMesh）；操作已取消。"));
            panel->deleteLater();
            return;
        }
        panel->show();
        panel->raise();
        panel->activateWindow();
    });
            return true;
        }
        if (filterId == QStringLiteral("outline")) {
            connect(action, &QAction::triggered, this, [this](bool) {
        auto scene = rendererWidget->GetScene();
        auto model = scene ? scene->GetCurrentModel() : nullptr;
        if (!model) {
            showDarkFramelessMessage(QStringLiteral("无可用模型"),
                                     QStringLiteral("请先加载并在模型树中选择一个模型。"));
            return;
        }

        auto input = model->GetDataObject();
        if (!input) {
            showDarkFramelessMessage(QStringLiteral("无可用模型"),
                                     QStringLiteral("当前模型没有可用数据。"));
            return;
        }

        auto filter = OutlineFilter::New();
        filter->SetInput(input);
        if (!filter->Execute()) {
            showDarkFramelessMessage(QStringLiteral("生成包围盒失败"),
                                     QString::fromStdString(filter->GetMessage()));
            return;
        }

        auto output = DynamicCast<DrawObject>(filter->GetOutput());
        if (!output) {
            showDarkFramelessMessage(QStringLiteral("生成包围盒失败"),
                                     QStringLiteral("包围盒输出为空或无法显示。"));
            return;
        }

        // 将白色包围盒作为独立结果加入模型树
        output->SetName(QStringLiteral("Outline_%1").arg(++m_outlineCount).toStdString());
        output->SetViewStyle(IG_WIREFRAME);
        output->SetLineColor(igm::vec3(1.0f, 1.0f, 1.0f));
        output->SetLineWidth(2.0f);
        modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
        rendererWidget->update();
    });
            return true;
        }
        if (filterId == QStringLiteral("linear_extrusion")) {
            connect(action, &QAction::triggered, this, [this](bool checked) {
        auto scene = rendererWidget->GetScene();
        if (scene == nullptr || scene->GetCurrentModel() == nullptr) {
            showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("请先选择一个模型。"));
            return;
        }
        auto data = scene->GetCurrentModel()->GetDataObject();
        if (data == nullptr) {
            showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("当前模型没有数据。"));
            return;
        }
        if (m_linearExtrusionDialog == nullptr) {
            // 首次打开时创建独立弹窗：非模态，用户点 X 才关闭；
            // 面板内容仍是 igQtLinearExtrusionWidget，只是宿主从左侧工具面板换成独立窗口
            m_linearExtrusionDialog = new QDialog(this);
            m_linearExtrusionDialog->setWindowTitle(QStringLiteral("线性拉伸"));
            // 面板配色只作用于本弹窗：黑底白字、边框可见、字号比全局 12pt 更小。
            // 复选框必须连 ::indicator 一起显式定义：一旦给 QCheckBox 设了样式表，Qt 就不再画系统
            // 原生指示器；而全局样式里的对勾图片路径写错了（qrc 前缀是 /Ticon，样式里却写成 :/icons/…，
            // 大小写也不对），所以 :checked 只剩底色，看上去是个纯蓝色方块、分不出选中与否。
            // image 必须给「文件路径 / :/资源路径」，Qt 样式表不接受 data: URI（内联 SVG 试过，不生效）；
            // 也不能复用 Icons/check.png：它描边是 #1296DB，压在本面板蓝底 #0E639C 上对比度只有约 1.95，
            // 14px 下看不出对勾，因此新增白色版 Icons/check_white.svg（对比度约 6.4）。
            const QString panelStyle = QString::fromUtf8(
                    "QDialog { background-color: #1E1E1E; }"
                    "QLabel { color: #FFFFFF; font-size: 11px; }"
                    "QLabel:disabled { color: #6A6A6A; }"
                    "QGroupBox { color: #FFFFFF; border: 1px solid #3C3C3C; border-radius: 4px; padding: 4px; }"
                    "QGroupBox:disabled { color: #6A6A6A; }"
                    "QCheckBox { color: #FFFFFF; font-size: 11px; spacing: 6px; }"
                    "QCheckBox::indicator { width: 14px; height: 14px; border: 1px solid #6A6A6A;"
                    " border-radius: 2px; background-color: #2A2A2A; }"
                    "QCheckBox::indicator:unchecked { background-color: #2A2A2A; }"
                    "QCheckBox::indicator:checked { background-color: #0E639C; border: 1px solid #1F8AD2;"
                    " image: url(:/Ticon/Icons/check_white.svg); }"
                    "QCheckBox::indicator:disabled { border-color: #3C3C3C; background-color: #232323; }"
                    "QComboBox, QLineEdit { background-color: #2A2A2A; color: #FFFFFF; border: 1px solid #4A4A4A;"
                    " border-radius: 3px; padding: 2px 6px; min-height: 18px; font-size: 11px; }"
                    "QComboBox:disabled, QLineEdit:disabled { color: #6A6A6A; }"
                    "QComboBox QAbstractItemView { background-color: #2A2A2A; color: #FFFFFF;"
                    " selection-background-color: #007399; }"
                    "QPushButton { background-color: #2D2D30; color: #FFFFFF; border: 1px solid #4A4A4A;"
                    " border-radius: 3px; padding: 3px 12px; min-height: 18px; font-size: 11px; }"
                    "QPushButton:hover { background-color: #3A3A3D; }");
            m_linearExtrusionDialog->setStyleSheet(panelStyle);
            auto* layout = new QVBoxLayout(m_linearExtrusionDialog);
            layout->setContentsMargins(0, 0, 0, 0);
            // 不锁定弹窗尺寸：宽度与高度都交给用户自由拖动
            layout->setSizeConstraint(QLayout::SetDefaultConstraint);
            m_linearExtrusionWidget = new igQtLinearExtrusionWidget(m_linearExtrusionDialog);
            m_linearExtrusionWidget->setMinimumSize(0, 0);
            layout->addWidget(m_linearExtrusionWidget);
            m_linearExtrusionDialog->setMinimumSize(0, 0);
            m_linearExtrusionDialog->resize(360, 380);

            connect(m_linearExtrusionWidget, &igQtLinearExtrusionWidget::DrawLinearExtrusionModel, this,
                    [this](iGame::DataObject::Pointer res) {
                        modelTreeWidget->addDataObjectToModelTree(res, ItemSource::Algorithm);
                    });
            connect(m_linearExtrusionWidget, &igQtLinearExtrusionWidget::UpdateLinearExtrusionModel, this,
                    [this](iGame::DataObject::Pointer res) {
                        modelTreeWidget->updateCurrentModelInfo();
                        rendererWidget->update();
                    });
            connect(m_linearExtrusionWidget, &igQtLinearExtrusionWidget::ApplyFailed, this,
                    [this](const QString& message) { showDarkFramelessMessage(QStringLiteral("Warning"), message); });
        }
        // 每次打开都按当前模型刷新输入数据
        m_linearExtrusionWidget->SetOriginDataObject(data);
        m_linearExtrusionDialog->show();
        m_linearExtrusionDialog->raise();
        m_linearExtrusionDialog->activateWindow();
    });
            return true;
        }
        if (filterId == QStringLiteral("warp_by_scalar")) {
            connect(action, &QAction::triggered, this, [this](bool) {
        // 变形过滤器只改写点坐标，因此只接受点集派生类型
        auto resolveWarpInput = [this](DataObject::Pointer& obj, const QString& title) -> bool {
            obj = nullptr;
            auto scene = rendererWidget->GetScene();
            if (scene == nullptr || scene->GetCurrentModel() == nullptr) {
                showDarkFramelessMessage(title, QStringLiteral("请先在模型树中选择一个模型。"));
                return false;
            }
            obj = scene->GetCurrentModel()->GetDataObject();
            if (!obj) {
                showDarkFramelessMessage(title, QStringLiteral("当前模型没有可用的数据对象。"));
                return false;
            }
            const IGenum type = obj->GetDataObjectType();
            if (type != IG_POINT_SET && type != IG_SURFACE_MESH && type != IG_VOLUME_MESH &&
                type != IG_UNSTRUCTURED_MESH && type != IG_STRUCTURED_MESH) {
                showDarkFramelessMessage(
                        title, QStringLiteral("该算法只支持点集 / 表面网格 / 体网格 / 非结构网格 / 结构网格。"));
                return false;
            }
            return true;
        };

        struct WarpArrayInfo {
            QString name;
            int dimension{1};
            QString typeName;
        };
        auto collectPointArrays = [](DataObject::Pointer obj, std::vector<WarpArrayInfo>& out) {
            auto attrSet = obj->GetAttributeSet();
            if (!attrSet) { return; }
            auto attrs = attrSet->GetAllPointAttributes();
            if (!attrs) { return; }
            for (IGsize i = 0; i < attrs->GetNumberOfElements(); ++i) {
                auto& attr = attrs->GetElement(i);
                if (attr.IsNone() || !attr.pointer) { continue; }
                // Warp By Scalar accepts only single-component point arrays.
                if (attr.pointer->GetDimension() != 1) { continue; }
                WarpArrayInfo info;
                info.name = QString::fromStdString(attr.pointer->GetName());
                info.dimension = attr.pointer->GetDimension();
                switch (attr.type) {
                    case IG_SCALAR:
                        info.typeName = QStringLiteral("标量");
                        break;
                    case IG_VECTOR:
                        info.typeName = QStringLiteral("向量");
                        break;
                    case IG_NORMAL:
                        info.typeName = QStringLiteral("法向");
                        break;
                    case IG_TCOORD:
                        info.typeName = QStringLiteral("纹理坐标");
                        break;
                    case IG_TENSOR:
                        info.typeName = QStringLiteral("张量");
                        break;
                    default:
                        info.typeName = QStringLiteral("属性");
                        break;
                }
                out.push_back(info);
            }
        };
        DataObject::Pointer obj = nullptr;
        if (!resolveWarpInput(obj, QStringLiteral("按标量变形"))) { return; }

        std::vector<WarpArrayInfo> arrays;
        collectPointArrays(obj, arrays);
        if (arrays.empty()) {
            showDarkFramelessMessage(QStringLiteral("按标量变形"),
                                     QStringLiteral("当前模型没有单分量点属性数组，无法按标量变形。\n"
                                                    "可以先用「转换为点数据」把单元属性转到点上。"));
            return;
        }
        std::vector<QString> items;
        for (const auto& a: arrays) {
            items.push_back(QStringLiteral("%1 (%2, %3 分量)").arg(a.name, a.typeName).arg(a.dimension));
        }

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("按标量变形 (Warp By Scalar)"));
        dialog->setFilterDescription(QStringLiteral("沿法向按标量值移动点：x' = x + n * s * 缩放系数。\n"
                                                    "未勾选「使用指定法向」时优先使用输入的点法向数组，"
                                                    "缺失时退回下面的法向。"));
        int arrayId = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX,
                                           QStringLiteral("标量数组 (Scalars)"), items);
        int scaleId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                           QStringLiteral("缩放系数 (Scale Factor)"), "1.0");
        int useNormalId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                               QStringLiteral("使用指定法向 (Use Normal)"), "false");
        int nxId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                        QStringLiteral("法向 X (Normal X)"), "0.0");
        int nyId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                        QStringLiteral("法向 Y (Normal Y)"), "0.0");
        int nzId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                        QStringLiteral("法向 Z (Normal Z)"), "1.0");
        int xyPlaneId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                             QStringLiteral("XY 平面模式 (XY Plane)"), "false");

        dialog->setFixedWidth(360);
        if (auto* sa = dialog->findChild<QScrollArea*>(QStringLiteral("scrollArea"))) {
            sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
        dialog->show();

        dialog->setApplyFunctor([=, this]() {
            bool ok = false;
            const int choice = dialog->getComboIndex(arrayId, ok);
            if (!ok || choice < 0 || choice >= static_cast<int>(arrays.size())) {
                showDarkFramelessMessage(QStringLiteral("按标量变形"), QStringLiteral("请选择有效的标量数组。"));
                return;
            }
            const double scale = dialog->getDouble(scaleId, ok);
            if (!ok) {
                showDarkFramelessMessage(QStringLiteral("按标量变形"), QStringLiteral("缩放系数必须是数值。"));
                return;
            }
            const double nx = dialog->getDouble(nxId, ok);
            if (!ok) {
                showDarkFramelessMessage(QStringLiteral("按标量变形"), QStringLiteral("法向 X 必须是数值。"));
                return;
            }
            const double ny = dialog->getDouble(nyId, ok);
            if (!ok) {
                showDarkFramelessMessage(QStringLiteral("按标量变形"), QStringLiteral("法向 Y 必须是数值。"));
                return;
            }
            const double nz = dialog->getDouble(nzId, ok);
            if (!ok) {
                showDarkFramelessMessage(QStringLiteral("按标量变形"), QStringLiteral("法向 Z 必须是数值。"));
                return;
            }
            const bool useNormal = dialog->getChecked(useNormalId, ok);
            const bool xyPlane = dialog->getChecked(xyPlaneId, ok);

            auto filter = WarpByScalar::New();
            filter->SetScalarsArrayName(arrays[static_cast<size_t>(choice)].name.toStdString());
            filter->SetScaleFactor(scale);
            filter->SetUseNormal(useNormal);
            filter->SetNormal(nx, ny, nz);
            filter->SetXYPlane(xyPlane);
            filter->SetInput(obj);
            if (!filter->Execute()) {
                showDarkFramelessMessage(QStringLiteral("按标量变形"),
                                         QStringLiteral("变形失败：请检查标量数组与法向设置。"));
                return;
            }
            auto output = filter->GetOutput();
            if (!output) {
                showDarkFramelessMessage(QStringLiteral("按标量变形"), QStringLiteral("算法未产生有效结果。"));
                return;
            }
            output->SetName(obj->GetName() + "_warp_scalar");
            modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
            rendererWidget->GetScene()->Modified();
            rendererWidget->GetScene()->Update();
            rendererWidget->update();
            dialog->close();
        });
    });
            return true;
        }
        if (filterId == QStringLiteral("append_attributes")) {
            connect(action, &QAction::triggered, this, [this](bool) {
        auto scene = rendererWidget->GetScene();
        if (scene == nullptr) { return; }
        auto modelList = scene->GetModelList();
        if (!modelList) { return; }

        struct AppendInputInfo {
            QString name;
            DataObject::Pointer object;
            IGsize points{0};
            IGsize cells{0};
        };
        // 计算单元数，用于在界面上提示各输入是否逐项对应
        auto cellCountOf = [](DataObject::Pointer obj) -> IGsize {
            switch (obj->GetDataObjectType()) {
                case IG_POINT_SET:
                    return 0;
                case IG_SURFACE_MESH: {
                    auto mesh = DynamicCast<SurfaceMesh>(obj);
                    return mesh ? mesh->GetNumberOfFaces() : IGsize(0);
                }
                case IG_VOLUME_MESH: {
                    auto mesh = DynamicCast<VolumeMesh>(obj);
                    return mesh ? mesh->GetNumberOfVolumes() : IGsize(0);
                }
                case IG_UNSTRUCTURED_MESH: {
                    auto mesh = DynamicCast<UnstructuredMesh>(obj);
                    return mesh ? mesh->GetNumberOfCells() : IGsize(0);
                }
                case IG_STRUCTURED_MESH: {
                    auto mesh = DynamicCast<StructuredMesh>(obj);
                    return mesh ? mesh->GetNumberOfCells() : IGsize(0);
                }
                default: {
                    auto cells = obj->GetCellArray();
                    return cells ? cells->GetNumberOfCells() : IGsize(0);
                }
            }
        };

        std::vector<AppendInputInfo> infos;
        for (auto it = modelList->Begin(); it != modelList->End(); ++it) {
            auto model = it->second;
            if (!model) { continue; }
            auto dataObj = model->GetDataObject();
            if (!dataObj) { continue; }
            const IGenum type = dataObj->GetDataObjectType();
            if (type != IG_POINT_SET && type != IG_SURFACE_MESH && type != IG_VOLUME_MESH &&
                type != IG_UNSTRUCTURED_MESH && type != IG_STRUCTURED_MESH) {
                continue;
            }
            AppendInputInfo info;
            info.object = dataObj;
            info.name = QString::fromStdString(dataObj->GetName());
            auto pts = dataObj->GetPoints();
            info.points = pts ? pts->GetNumberOfPoints() : IGsize(0);
            info.cells = cellCountOf(dataObj);
            infos.push_back(info);
        }
        if (infos.empty()) {
            showDarkFramelessMessage(QStringLiteral("追加属性"),
                                        QStringLiteral("场景中没有可用于追加属性的数据对象。"));
            return;
        }

        igQtChromeFramelessDialog* dlg = new igQtChromeFramelessDialog(this);
        dlg->setDialogTitle(QStringLiteral("追加属性 (Append Attributes)"));
        dlg->setMaximizeEnabled(false);
        dlg->setAttribute(Qt::WA_DeleteOnClose);

        QWidget* content = new QWidget(dlg->contentHost());
        content->setStyleSheet(
                "QWidget { background-color: #252526; color: #D4D4D4; }"
                "QCheckBox { color: #D4D4D4; spacing: 6px; }"
                "QCheckBox::indicator { width: 16px; height: 16px; }"
                "QCheckBox::indicator:unchecked { border: 1px solid #6A6A6A; background-color: #3A3A3A; }"
                "QCheckBox::indicator:checked { border: 1px solid #0E639C; background-color: #0E639C; }"
                "QCheckBox::indicator:unchecked:hover { border: 1px solid #9A9A9A; }"
                "QCheckBox::indicator:checked:hover { background-color: #1177BB; }"
                "QPushButton { background-color: #3A3A3A; color: #D4D4D4; border: 1px solid #4A4A4A; "
                "              padding: 6px 12px; border-radius: 4px; }"
                "QPushButton:hover { background-color: #4A4A4A; border-color: #5A5A5A; }"
                "QPushButton:pressed { background-color: #2A2A2A; }"
                "QScrollArea { background-color: #1E1E1E; border: none; }"
                "QScrollBar:vertical { background: #1E1E1E; width: 10px; margin: 0; }"
                "QScrollBar::handle:vertical { background: #4A4A4A; border-radius: 5px; min-height: 20px; }"
                "QScrollBar::handle:vertical:hover { background: #5A5A5A; }"
                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }");
        QVBoxLayout* mainLayout = new QVBoxLayout(content);
        mainLayout->setContentsMargins(12, 12, 12, 12);
        mainLayout->setSpacing(8);

        QLabel* tip = new QLabel(
                QStringLiteral("选择要合并属性的数据对象（列表顺序即输入顺序，第一个提供输出几何与数据类型）。\n"
                                "要求所选对象的点数和单元数逐项对应；同名的属性会全部保留，"),
                content);
        tip->setWordWrap(true);
        mainLayout->addWidget(tip);

        QHBoxLayout* btnLayout = new QHBoxLayout();
        QPushButton* selectAllBtn = new QPushButton(QStringLiteral("全选"), content);
        QPushButton* deselectAllBtn = new QPushButton(QStringLiteral("全不选"), content);
        btnLayout->addWidget(selectAllBtn);
        btnLayout->addWidget(deselectAllBtn);
        btnLayout->addStretch();
        mainLayout->addLayout(btnLayout);

        QScrollArea* scrollArea = new QScrollArea(content);
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        QWidget* listWidget = new QWidget(scrollArea);
        QVBoxLayout* listLayout = new QVBoxLayout(listWidget);
        listLayout->setSpacing(4);
        listLayout->setContentsMargins(0, 0, 0, 0);
        QList<QCheckBox*> checkBoxes;
        for (const AppendInputInfo& info: infos) {
            QCheckBox* cb = new QCheckBox(QStringLiteral("%1  (点: %2, 单元: %3)")
                                                    .arg(info.name)
                                                    .arg(info.points)
                                                    .arg(info.cells),
                                            listWidget);
            cb->setChecked(true);
            checkBoxes.append(cb);
            listLayout->addWidget(cb);
        }
        listWidget->setLayout(listLayout);
        scrollArea->setWidget(listWidget);
        mainLayout->addWidget(scrollArea);

        QCheckBox* pointCb = new QCheckBox(QStringLiteral("追加点属性 (Point Data)"), content);
        pointCb->setChecked(true);
        QCheckBox* cellCb = new QCheckBox(QStringLiteral("追加单元属性 (Cell Data)"), content);
        cellCb->setChecked(true);
        mainLayout->addWidget(pointCb);
        mainLayout->addWidget(cellCb);

        QDialogButtonBox* buttonBox =
                new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, content);
        mainLayout->addWidget(buttonBox);

        dlg->setContentWidget(content);
        dlg->resize(440, 540);

        connect(selectAllBtn, &QPushButton::clicked, [checkBoxes]() {
            for (QCheckBox* cb: checkBoxes) { cb->setChecked(true); }
        });
        connect(deselectAllBtn, &QPushButton::clicked, [checkBoxes]() {
            for (QCheckBox* cb: checkBoxes) { cb->setChecked(false); }
        });
        connect(buttonBox, &QDialogButtonBox::accepted, this, [this, dlg, checkBoxes, infos, pointCb, cellCb]() {
            std::vector<DataObject::Pointer> selected;
            for (int i = 0; i < checkBoxes.size(); ++i) {
                if (checkBoxes[i]->isChecked()) { selected.push_back(infos[i].object); }
            }
            if (selected.empty()) {
                showDarkFramelessMessage(QStringLiteral("追加属性"), QStringLiteral("请至少选择一个数据对象。"));
                return;
            }
            if (!pointCb->isChecked() && !cellCb->isChecked()) {
                showDarkFramelessMessage(QStringLiteral("追加属性"),
                                            QStringLiteral("请至少勾选「点属性」或「单元属性」之一。"));
                return;
            }

            auto filter = AppendAttributes::New();
            for (const auto& obj: selected) { filter->AddInput(obj); }
            filter->SetAppendPointData(pointCb->isChecked());
            filter->SetAppendCellData(cellCb->isChecked());

            if (!filter->Execute()) {
                showDarkFramelessMessage(
                        QStringLiteral("追加属性"),
                        QStringLiteral("执行失败：请确认所选数据对象的点数与单元数逐项对应。"));
                return;
            }
            auto output = filter->GetOutput();
            if (!output) {
                showDarkFramelessMessage(QStringLiteral("追加属性"), QStringLiteral("算法未产生有效结果。"));
                return;
            }
            output->SetName(selected[0]->GetName() + "_appended");
            modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
            rendererWidget->GetScene()->Modified();
            rendererWidget->GetScene()->Update();
            rendererWidget->update();
            dlg->accept();
        });
        connect(buttonBox, &QDialogButtonBox::rejected, dlg, &QDialog::reject);

        dlg->show();
    });
            return true;
        }
        if (filterId == QStringLiteral("tube")) {
            connect(action, &QAction::triggered, this, [this](bool checked) {
                auto* scene = rendererWidget->GetScene();
                if (scene == nullptr || scene->GetCurrentModel() == nullptr) {
                    showDarkFramelessMessage(QStringLiteral("无可用模型"),
                                             QStringLiteral("请先加载并选择一个网格模型。"));
                    return;
                }
                auto model = scene->GetCurrentModel();
                auto obj = model->GetDataObject();
                if (obj == nullptr) {
                    showDarkFramelessMessage(QStringLiteral("无可用数据"),
                                             QStringLiteral("当前模型没有可用的网格数据。"));
                    return;
                }

                igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(QStringLiteral("管道生成 (Tube)"));
                dialog->setFilterDescription(QStringLiteral(
                    "将线段（如流线、折线）沿路径扫掠成圆管。<br>"
                    "在每个路径点上确定垂直于前进方向的平面，画一个由半径和边数决定的正多边形截面，"
                    "再沿路径对齐、缝合。<br>"
                    "截面朝向通过平行传递延续，不会翻转；首尾可用端面封闭。"));

                const int radiusId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT,
                        QStringLiteral("半径 (Radius)"), "0.1");
                const int sidesId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT,
                        QStringLiteral("边数 (Number Of Sides)"), "6");
                const int capId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_CHECK_BOX,
                        QStringLiteral("端面封端 (Capping)"), "true");
                const int useDefId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_CHECK_BOX,
                        QStringLiteral("手动指定初始法向 (Use Default Normal)"), "false");
                const int nxId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT,
                        QStringLiteral("初始法向 X (Normal X)"), "0");
                const int nyId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT,
                        QStringLiteral("初始法向 Y (Normal Y)"), "0");
                const int nzId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT,
                        QStringLiteral("初始法向 Z (Normal Z)"), "1");

                // 加宽面板，避免说明文字与标签过度换行
                dialog->setFixedWidth(660);

                // “手动指定初始法向”勾选后，下方三个初始法向输入框才可用（默认置灰）
                auto* useDefCheck = qobject_cast<QCheckBox*>(dialog->getWidget(useDefId));
                QWidget* nxEdit = dialog->getWidget(nxId);
                QWidget* nyEdit = dialog->getWidget(nyId);
                QWidget* nzEdit = dialog->getWidget(nzId);
                auto syncNormalEditors = [nxEdit, nyEdit, nzEdit](bool on) {
                    nxEdit->setEnabled(on);
                    nyEdit->setEnabled(on);
                    nzEdit->setEnabled(on);
                };
                connect(useDefCheck, &QCheckBox::toggled, dialog, syncNormalEditors);
                syncNormalEditors(false);
                useDefCheck->setToolTip(QStringLiteral(
                    "勾选后，使用下方“初始法向 X/Y/Z”指定的方向确定第一个截面的朝向；"
                    "不勾选（默认）则由程序自动选择（管子起始竖直时也会自动换轴）。"));
                const QString normalEditTip = QStringLiteral(
                    "仅在勾选上方“手动指定初始法向”后生效，用于确定第一个截面的初始朝向。");
                nxEdit->setToolTip(normalEditTip);
                nyEdit->setToolTip(normalEditTip);
                nzEdit->setToolTip(normalEditTip);

                dialog->setApplyFunctor([this, dialog, obj, radiusId, sidesId,
                                        capId, useDefId, nxId, nyId, nzId]() {
                    bool rok = false, sok = false,
                         cok = false, uok = false,
                         xok = false, yok = false, zok = false;

                    double radius = dialog->getDouble(radiusId, rok);
                    int sides = dialog->getInt(sidesId, sok);
                    bool capping = dialog->getChecked(capId, cok);
                    bool useDef = dialog->getChecked(useDefId, uok);
                    double nx = dialog->getDouble(nxId, xok);
                    double ny = dialog->getDouble(nyId, yok);
                    double nz = dialog->getDouble(nzId, zok);

                    if (!rok || radius <= 0.0) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("半径必须是大于 0 的数字。"));
                        return;
                    }
                    if (!sok || sides < 3) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("边数必须是不小于 3 的整数。"));
                        return;
                    }

                    TubeFilter::Pointer filter = TubeFilter::New();
                    filter->SetInput(obj);
                    filter->SetRadius(radius);
                    filter->SetNumberOfSides(sides);
                    filter->SetCapping(cok ? capping : true);
                    filter->SetUseDefaultNormal(uok ? useDef : false);
                    if (xok && yok && zok)
                        filter->SetDefaultNormal(Vector3d(nx, ny, nz));

                    if (!filter->Execute()) {
                        showDarkFramelessMessage(
                                QStringLiteral("数据类型不匹配"),
                                QStringLiteral("管道生成仅支持含线段（IG_LINE / IG_POLY_LINE）的"
                                               "非结构网格或表面网格，请检查输入数据类型。"));
                        return;
                    }

                    auto outMesh = DynamicCast<SurfaceMesh>(filter->GetOutput());
                    modelTreeWidget->addDataObjectToModelTree(outMesh, Algorithm);
                    rendererWidget->update();
                    // 先弹模态提示（此时参数面板仍在、主窗口布局稳定），
                    // 用户点“确定”返回后再关闭面板，避免先 close dock 触发布局
                    // 重建、紧接着开模态对话框而导致 Qt 访问违例。
                    showDarkFramelessMessage(QStringLiteral("管道生成完成"),
                                             QStringLiteral("已将线段转换为圆管，可在模型树中查看输出结果。"),
                                             true);
                    dialog->close();
                });
                dialog->show();
            });
            return true;
        }
        if (filterId == QStringLiteral("subdivide")) {
            connect(action, &QAction::triggered, this, [this](bool checked) {
                auto* scene = rendererWidget->GetScene();
                if (scene == nullptr || scene->GetCurrentModel() == nullptr) {
                    showDarkFramelessMessage(QStringLiteral("无可用模型"),
                                             QStringLiteral("请先加载并选择一个网格模型。"));
                    return;
                }
                auto model = scene->GetCurrentModel();
                auto obj = model->GetDataObject();
                if (obj == nullptr) {
                    showDarkFramelessMessage(QStringLiteral("无可用数据"),
                                             QStringLiteral("当前模型没有可用的网格数据。"));
                    return;
                }

                igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(QStringLiteral("三角面细分 (Subdivide)"));
                int subId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                 QStringLiteral("细分次数 (1~8，每次三角形数量 ×4)"), "1");
                dialog->setApplyFunctor([=, this]() {
                    bool ok = false;
                    int tms = dialog->getInt(subId, ok);
                    if (!ok || tms < 1 || tms > 8) {
                        showDarkFramelessMessage(QStringLiteral("Warning"),
                                                 QStringLiteral("请输入 1 ~ 8 之间的整数细分次数。"));
                        return;
                    }

                    auto filter = SubdivideFilter::New();
                    filter->SetNumberOfSubdivisions(tms);
                    filter->SetInput(obj);
                    if (!filter->Execute()) {
                        showDarkFramelessMessage(
                                QStringLiteral("数据类型不匹配"),
                                QStringLiteral("三角面细分仅支持多边形表面网格（Poly Data），请检查输入数据类型。"));
                        return;
                    }

                    auto outMesh = DynamicCast<SurfaceMesh>(filter->GetOutput());
                    modelTreeWidget->addDataObjectToModelTree(outMesh, Algorithm);
                    rendererWidget->update();
                    // 先弹模态提示（面板仍在、布局稳定），用户点“确定”后再关面板
                    showDarkFramelessMessage(QStringLiteral("三角面细分完成"),
                                             QStringLiteral("已对三角面做线性细分，旧顶点保持不变、边中点为线性插值；"
                                                            "可将显示模式改为 Surface With Edges 查看加密后的网格。"),
                                             true);
                    dialog->close();
                });
                dialog->show();
            });
            return true;
        }
        if (filterId == QStringLiteral("reverse_sense")) {
            connect(action, &QAction::triggered, this, [this](bool) {
                auto scene = rendererWidget ? rendererWidget->GetScene() : nullptr;
                auto currentModel = scene ? scene->GetCurrentModel() : nullptr;
                if (!currentModel) {
                    showDarkFramelessMessage(QStringLiteral("反转面朝向"), QStringLiteral("请先选择一个模型。"));
                    return;
                }
                auto object = currentModel->GetDataObject();
                if (iGame::DynamicCast<iGame::SurfaceMesh>(object).IsNull()) {
                    showDarkFramelessMessage(QStringLiteral("反转面朝向"),
                                             QStringLiteral("当前模型不是曲面网格（SurfaceMesh）"));
                    return;
                }

                auto* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(QStringLiteral("反转面朝向"));
                dialog->setFilterDescription(QStringLiteral("反转面的顶点环序，并可取反法向"));
                const int cellsId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_CHECK_BOX, QStringLiteral("反转面顶点顺序"), "true");
                const int normalsId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_CHECK_BOX, QStringLiteral("反转法向"), "true");
                dialog->setParameterLayoutVertical();
                auto* help = new QLabel(QStringLiteral(
                        "反转面顶点顺序：改变面的朝向。\n"
                        "反转法向：取反已有的点法向和面法向，不生成新法向。"), dialog);
                help->setWordWrap(true);
                help->setContentsMargins(0, 8, 0, 0);
                dialog->addRowWidget(help);
                dialog->setMinimumWidth(420);
                dialog->resize(qMax(460, help->fontMetrics().horizontalAdvance(
                        QStringLiteral("反转法向：取反已有的点法向和面法向")) + 80), 330);
                if (auto* scrollArea = dialog->findChild<QScrollArea*>(QStringLiteral("scrollArea"))) {
                    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                }
                dialog->show();

                dialog->setApplyFunctor([=, this]() {
                    bool ok = false;
                    auto filter = iGame::ReverseSenseFilter::New();
                    filter->SetReverseCells(dialog->getChecked(cellsId, ok));
                    filter->SetReverseNormals(dialog->getChecked(normalsId, ok));
                    filter->SetInput(object);
                    if (!filter->Execute()) {
                        showDarkFramelessMessage(
                                QStringLiteral("反转面朝向"),
                                QStringLiteral("执行失败：%1").arg(QString::fromStdString(filter->GetMessage())));
                        return;
                    }
                    auto output = filter->GetOutput();
                    if (!output) {
                        showDarkFramelessMessage(QStringLiteral("反转面朝向"), QStringLiteral("输出对象为空"));
                        return;
                    }
                    output->SetName(object->GetName() + "_Reversed");

                    auto inputDraw = iGame::DynamicCast<iGame::DrawObject>(object);
                    auto outDraw = iGame::DynamicCast<iGame::DrawObject>(output);
                    if (inputDraw && outDraw) {
                        outDraw->SetViewStyle(static_cast<IGenum>(inputDraw->GetViewStyle()));
                    }

                    modelTreeWidget->addDataObjectToModelTree(output, ItemSource::Algorithm);
                    modelTreeWidget->updateCloudPicture();
                    rendererWidget->update();
                    dialog->close();
                });
            });
            return true;
        }
        if (connectThirdBatchFilterAction(action, filterId)) return true;
        if (connectImportedFilterAction(action, filterId)) return true;
        if (filterId == QStringLiteral("coordinates")) {
            connect(action, &QAction::triggered, this, [=, this](bool) {
                const QString title = QStringLiteral("坐标 (coordinates)");
                auto obj = currentFilterInput(title);
                if (!obj) return;
                auto filter = PointCoordinatesFilter::New();
                filter->SetInput(obj);
                filter->SetArrayName("Coordinates");
                if (!filter->Execute()) {
                    showDarkFramelessMessage(title, QStringLiteral("提取点坐标失败。"));
                    return;
                }
                // PointCoordinatesFilter returns an independent deep-copy output.
                // Add it to the model tree instead of treating it as an in-place update.
                refreshFilterResult(obj, filter->GetOutput(), title);
            });
            return true;
        }

        if (filterId == QStringLiteral("cell_centers")) {
            connect(action, &QAction::triggered, this, [=, this](bool) {
                const QString title = QStringLiteral("单元中心 (cell_centers)");
                auto obj = currentFilterInput(title);
                if (!obj) return;
                auto filter = CellCenterFilter::New();
                filter->SetInput(obj);
                if (!filter->Execute()) {
                    showDarkFramelessMessage(title, QStringLiteral("计算单元中心失败。"));
                    return;
                }
                refreshFilterResult(obj, filter->GetOutput(), title);
            });
            return true;
        }

        if (filterId == QStringLiteral("random_vectors")) {
            connect(action, &QAction::triggered, this, [=, this](bool) {
                const QString title = QStringLiteral("随机向量 (random_vectors)");
                auto obj = currentFilterInput(title);
                if (!obj) return;
                igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(title);
                dialog->setFilterDescription(QStringLiteral("为每个点生成 BrownianVectors 三分量随机向量。"));
                int minId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                 QStringLiteral("最小速度"), "0");
                int maxId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                 QStringLiteral("最大速度"), "1");
                dialog->show();
                dialog->setApplyFunctor([=, this]() {
                    bool okMin = false, okMax = false;
                    const double minSpeed = dialog->getDouble(minId, okMin);
                    const double maxSpeed = dialog->getDouble(maxId, okMax);
                    if (!okMin || !okMax || maxSpeed < minSpeed) {
                        showDarkFramelessMessage(title, QStringLiteral("请输入有效速度范围。"));
                        return;
                    }
                    auto filter = RandomVectorsFilter::New();
                    filter->SetInput(obj);
                    filter->SetMinimumSpeed(minSpeed);
                    filter->SetMaximumSpeed(maxSpeed);
                    if (!filter->Execute()) {
                        showDarkFramelessMessage(title, QStringLiteral("生成随机向量失败。"));
                        return;
                    }
                    refreshFilterResult(obj, filter->GetOutput(), title);
                    dialog->close();
                });
            });
            return true;
        }

        if (filterId == QStringLiteral("remove_ghost_information")) {
            connect(action, &QAction::triggered, this, [=, this](bool) {
                const QString title = QStringLiteral("移除 Ghost 信息 (remove_ghost_information)");
                auto obj = currentFilterInput(title);
                if (!obj) return;
                auto filter = RemoveGhostInformationFilter::New();
                filter->SetInput(obj);
                if (!filter->Execute()) {
                    showDarkFramelessMessage(title, QStringLiteral("当前数据不支持移除 Ghost 信息，或 Ghost 数组不合法。"));
                    return;
                }
                refreshFilterResult(obj, filter->GetOutput(), title, !filter->WasModified());
            });
            return true;
        }

        if (filterId == QStringLiteral("elevation")) {
            connect(action, &QAction::triggered, this, [=, this](bool) {
                const QString title = QStringLiteral("高程 (elevation)");
                auto obj = currentFilterInput(title);
                if (!obj) return;
                // 防御：包围盒无效时无法按轴预填默认低/高点
                const auto& bb = obj->GetBoundingBox();
                if (bb.isNull()) {
                    showDarkFramelessMessage(title, QStringLiteral("输入模型包围盒无效，无法计算高程。"));
                    return;
                }

                igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(title);
                dialog->setFilterDescription(QStringLiteral("沿低点到高点的标尺线段计算投影高程标量（t 饱和于 [0,1] 后映射到标量范围），语义与 ParaView Elevation 一致。"));

                // X/Y/Z 轴按钮行（占参数区第一行）：互斥选中，默认选中 X；
                // 点击按包围盒铺满低/高点（被选轴取 min/max，其余轴取中心）
                auto* axisRow = new QWidget(dialog);
                auto* axisLayout = new QHBoxLayout(axisRow);
                axisLayout->setContentsMargins(0, 0, 0, 0);
                axisLayout->setSpacing(6);
                axisLayout->addWidget(new QLabel(QStringLiteral("投影轴"), axisRow));
                auto* axisGroup = new QButtonGroup(axisRow);
                axisGroup->setExclusive(true);
                QPushButton* axisBtns[3] = {};
                for (int i = 0; i < 3; ++i) {
                    auto* btn = new QPushButton(QString(QChar('X' + i)), axisRow);
                    btn->setCheckable(true);
                    axisGroup->addButton(btn);
                    axisLayout->addWidget(btn);
                    axisBtns[i] = btn;
                }
                axisBtns[0]->setChecked(true);
                dialog->addRowWidget(axisRow);

                // 8 个参数：低点 xyz、高点 xyz、标量范围下限/上限（默认按包围盒 X 轴铺满，范围 [0,1]）
                const auto center = bb.center();
                auto num = [](double v) { return QString::number(v); };
                std::array<int, 3> lowIds{}, highIds{};
                for (int i = 0; i < 3; ++i) {
                    lowIds[i] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                     QStringLiteral("低点 ") + QString(QChar('X' + i)),
                                                     num(i == 0 ? bb.min[0] : center[i]));
                }
                for (int i = 0; i < 3; ++i) {
                    highIds[i] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("高点 ") + QString(QChar('X' + i)),
                                                      num(i == 0 ? bb.max[0] : center[i]));
                }
                int rangeLowId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("标量范围下限"), "0");
                int rangeHighId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("标量范围上限"), "1");

                // 按轴回填输入框（回填规则与参数面板 fillRangeByAxis 一致）
                auto fillByAxis = [dialog, bb, center, lowIds, highIds](int axis) {
                    for (int i = 0; i < 3; ++i) {
                        const double lowV = (i == axis) ? bb.min[i] : center[i];
                        const double highV = (i == axis) ? bb.max[i] : center[i];
                        if (auto* line = qobject_cast<QLineEdit*>(dialog->getWidget(lowIds[i]))) {
                            line->setText(QString::number(lowV));
                        }
                        if (auto* line = qobject_cast<QLineEdit*>(dialog->getWidget(highIds[i]))) {
                            line->setText(QString::number(highV));
                        }
                    }
                };
                for (int i = 0; i < 3; ++i) {
                    connect(axisBtns[i], &QPushButton::clicked, dialog, [fillByAxis, i]() { fillByAxis(i); });
                }
                dialog->show();
                dialog->setApplyFunctor([=, this]() {
                    // 读取低点/高点 xyz（任一解析失败即中止）
                    bool okAll = true;
                    double lowPt[3] = {}, highPt[3] = {};
                    for (int i = 0; i < 3; ++i) {
                        bool okL = false, okH = false;
                        lowPt[i] = dialog->getDouble(lowIds[i], okL);
                        highPt[i] = dialog->getDouble(highIds[i], okH);
                        okAll = okAll && okL && okH;
                    }
                    bool okRL = false, okRH = false;
                    const double rLow = dialog->getDouble(rangeLowId, okRL);
                    const double rHigh = dialog->getDouble(rangeHighId, okRH);
                    if (!okAll || !okRL || !okRH) {
                        showDarkFramelessMessage(title, QStringLiteral("请输入有效的低点、高点和标量范围。"));
                        return;
                    }
                    // 前置校验：低点与高点重合 -> 投影方向为零向量；标量范围必须下限 < 上限
                    const double vx = highPt[0] - lowPt[0], vy = highPt[1] - lowPt[1], vz = highPt[2] - lowPt[2];
                    if (vx * vx + vy * vy + vz * vz == 0.0) {
                        showDarkFramelessMessage(title, QStringLiteral("低点不能与高点重合。"));
                        return;
                    }
                    if (rLow >= rHigh) {
                        showDarkFramelessMessage(title, QStringLiteral("标量范围下限必须小于上限。"));
                        return;
                    }

                    auto filter = ElevationFilter::New();
                    filter->SetInput(obj);
                    filter->SetLowPoint(lowPt[0], lowPt[1], lowPt[2]);
                    filter->SetHighPoint(highPt[0], highPt[1], highPt[2]);
                    filter->SetScalarRange(rLow, rHigh);
                    if (!filter->Execute()) {
                        showDarkFramelessMessage(title, QStringLiteral("生成高程标量失败。"));
                        return;
                    }
                    // 首次执行：独立输出挂模型树；随后绑定参数面板会话，供持续实时调整（对齐 ParaView Properties）
                    refreshFilterResult(obj, filter->GetOutput(), title);
                    ElevationFilterPanel->BindSession(obj, filter);
                    dialog->close();
                });
            });
            return true;
        }

        if (filterId == QStringLiteral("mask") || filterId == QStringLiteral("mask_points")) {
            connect(action, &QAction::triggered, this, [=, this](bool) {
                const QString title = filterId == QStringLiteral("mask")
                                              ? QStringLiteral("掩码 (mask)")
                                              : QStringLiteral("点掩码 (mask_points)");
                auto obj = currentFilterInput(title);
                if (!obj) return;
                igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(title);
                dialog->setFilterDescription(QStringLiteral("按步长或随机方式采样点，输出点/顶点集合。"));
                int ratioId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                   QStringLiteral("采样步长 OnRatio"), "2");
                int maxId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                 QStringLiteral("最大点数（0 为不限）"), "0");
                int randomId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                                    QStringLiteral("随机采样"), "false");
                int seedId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                  QStringLiteral("随机种子"), "1");
                dialog->show();
                dialog->setApplyFunctor([=, this]() {
                    bool okRatio = false, okMax = false, okRandom = false, okSeed = false;
                    const int ratio = dialog->getInt(ratioId, okRatio);
                    const int maxPoints = dialog->getInt(maxId, okMax);
                    const bool randomMode = dialog->getChecked(randomId, okRandom);
                    const int seed = dialog->getInt(seedId, okSeed);
                    if (!okRatio || !okMax || !okRandom || !okSeed || ratio <= 0 || maxPoints < 0) {
                        showDarkFramelessMessage(title, QStringLiteral("请输入有效采样参数。"));
                        return;
                    }
                    auto filter = MaskPointsFilter::New();
                    filter->SetInput(obj);
                    filter->SetOnRatio(ratio);
                    filter->SetMaximumNumberOfPoints(maxPoints);
                    filter->SetRandomMode(randomMode);
                    filter->SetRandomModeType(MaskPointsFilter::RANDOM_SAMPLING);
                    filter->SetRandomSeed(static_cast<unsigned int>(std::max(seed, 0)));
                    filter->SetGenerateVertices(true);
                    filter->SetSingleVertexPerCell(true);
                    if (!filter->Execute()) {
                        showDarkFramelessMessage(title, QStringLiteral("点掩码执行失败。该实现目前要求输入为非结构网格。"));
                        return;
                    }
                    refreshFilterResult(obj, filter->GetOutput(), title);
                    dialog->close();
                });
            });
            return true;
        }

        if (filterId == QStringLiteral("feature_edges")) {
            connect(action, &QAction::triggered, this, [=, this](bool) {
                const QString title = QStringLiteral("特征边 (feature_edges)");
                auto obj = currentFilterInput(title);
                if (!obj) return;
                igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(title);
                int angleId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                   QStringLiteral("特征角度"), "30");
                int boundaryId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                                      QStringLiteral("边界边"), "true");
                int featureId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                                     QStringLiteral("折痕边"), "true");
                int nonManifoldId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                                         QStringLiteral("非流形边"), "true");
                int manifoldId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                                      QStringLiteral("流形边"), "false");
                dialog->show();
                dialog->setApplyFunctor([=, this]() {
                    bool okAngle = false, okBoundary = false, okFeature = false, okNonManifold = false, okManifold = false;
                    auto filter = FeatureEdgesFilter::New();
                    filter->SetInput(obj);
                    filter->SetFeatureAngle(dialog->getDouble(angleId, okAngle));
                    filter->SetBoundaryEdges(dialog->getChecked(boundaryId, okBoundary));
                    filter->SetFeatureEdges(dialog->getChecked(featureId, okFeature));
                    filter->SetNonManifoldEdges(dialog->getChecked(nonManifoldId, okNonManifold));
                    filter->SetManifoldEdges(dialog->getChecked(manifoldId, okManifold));
                    if (!okAngle || !okBoundary || !okFeature || !okNonManifold || !okManifold) {
                        showDarkFramelessMessage(title, QStringLiteral("请输入有效特征边参数。"));
                        return;
                    }
                    if (!filter->Execute()) {
                        showDarkFramelessMessage(title, QStringLiteral("提取特征边失败。"));
                        return;
                    }
                    refreshFilterResult(obj, filter->GetOutput(), title);
                    dialog->close();
                });
            });
            return true;
        }

        if (filterId == QStringLiteral("extract_subset")) {
            connect(action, &QAction::triggered, this, [=, this](bool) {
                const QString title = QStringLiteral("提取子集 (extract_subset)");
                auto obj = currentFilterInput(title);
                if (!obj) return;
                auto structured = DynamicCast<StructuredMesh>(obj);
                if (!structured) {
                    showDarkFramelessMessage(title, QStringLiteral("Extract Subset 需要结构化网格输入。"));
                    return;
                }
                igIndex* size = structured->GetDimensionSize();
                igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(title);
                int minI = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("min I"), "0");
                int maxI = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("max I"),
                                                QString::number(size[0] - 1));
                int minJ = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("min J"), "0");
                int maxJ = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("max J"),
                                                QString::number(size[1] - 1));
                int minK = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("min K"), "0");
                int maxK = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("max K"),
                                                QString::number(size[2] - 1));
                dialog->show();
                dialog->setApplyFunctor([=, this]() {
                    bool ok[6] = {};
                    const int voi[6] = {
                            dialog->getInt(minI, ok[0]), dialog->getInt(maxI, ok[1]),
                            dialog->getInt(minJ, ok[2]), dialog->getInt(maxJ, ok[3]),
                            dialog->getInt(minK, ok[4]), dialog->getInt(maxK, ok[5])};
                    if (!ok[0] || !ok[1] || !ok[2] || !ok[3] || !ok[4] || !ok[5]) {
                        showDarkFramelessMessage(title, QStringLiteral("请输入有效 VOI 范围。"));
                        return;
                    }
                    auto filter = ExtractSubsetFilter::New();
                    filter->SetInput(obj);
                    filter->SetVOI(voi[0], voi[1], voi[2], voi[3], voi[4], voi[5]);
                    if (!filter->Execute()) {
                        showDarkFramelessMessage(title, QStringLiteral("提取结构化网格子集失败。"));
                        return;
                    }
                    refreshFilterResult(obj, filter->GetOutput(), title);
                    dialog->close();
                });
            });
            return true;
        }

        if (filterId == QStringLiteral("outline_corners")) {
            connect(action, &QAction::triggered, this, [=, this](bool) {
                const QString title = QStringLiteral("轮廓角点 (outline_corners)");
                auto obj = currentFilterInput(title);
                if (!obj) return;
                igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(title);
                int factorId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                    QStringLiteral("角长度比例"), "0.2");
                dialog->show();
                dialog->setApplyFunctor([=, this]() {
                    bool ok = false;
                    const double factor = dialog->getDouble(factorId, ok);
                    if (!ok || factor <= 0.0) {
                        showDarkFramelessMessage(title, QStringLiteral("请输入有效角长度比例。"));
                        return;
                    }
                    auto filter = OutlineCornerFilter::New();
                    filter->SetInput(obj);
                    filter->SetCornerFactor(static_cast<float>(factor));
                    if (!filter->Execute()) {
                        showDarkFramelessMessage(title, QString::fromStdString(filter->GetMessage()));
                        return;
                    }
                    refreshFilterResult(obj, filter->GetOutput(), title);
                    dialog->close();
                });
            });
            return true;
        }

        if (filterId == QStringLiteral("probe") || filterId == QStringLiteral("probe_location")) {
            connect(action, &QAction::triggered, this, [this](bool) {
                if (!rendererWidget->GetScene() || !rendererWidget->GetScene()->GetCurrentModel()) return;
                auto* dock = findChild<QDockWidget*>(QStringLiteral("probeFilterPanel"));
                if (!dock) {
                    dock = new QDockWidget(QStringLiteral("探测 / 位置探测"), this);
                    dock->setObjectName(QStringLiteral("probeFilterPanel"));
                    auto* widget = new igQtProbeWidget(dock);
                    dock->setWidget(widget);
                    widget->setRenderWidget(rendererWidget);
                    widget->setContext([this]() { return rendererWidget->GetScene(); }, modelTreeWidget,
                                       [this]() { rendererWidget->update(); });
                    addDockWidget(Qt::RightDockWidgetArea, dock);
                    connect(modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged, widget,
                            [dock, widget]() {
                                if (dock->isVisible()) widget->refreshFromCurrentModel();
                            });
                }
                auto* widget = qobject_cast<igQtProbeWidget*>(dock->widget());
                widget->ensureQueryPointSet();
                widget->refreshFromCurrentModel();
                dock->show();
                dock->raise();
                resizeDocks({dock}, {440}, Qt::Horizontal);
            });
            return true;
        }

        if (filterId == QStringLiteral("convert_to_vertex")) {
            connect(action, &QAction::triggered, this, [=, this](bool) {
                const QString title = QStringLiteral("转换为顶点 (convert_to_vertex)");
                auto obj = currentFilterInput(title);
                if (!obj) return;
                auto filter = ConvertToVertexFilter::New();
                filter->SetInput(obj);
                if (!filter->Execute()) {
                    showDarkFramelessMessage(title, QStringLiteral("转换为顶点单元失败。"));
                    return;
                }
                refreshFilterResult(obj, filter->GetOutput(), title);
            });
            return true;
        }

        return false;
    };

    for (const StandardFilterEntry& entry: standardFilterEntries) {
        const QString filterId = QString::fromLatin1(entry.id);
        const QString actionText = QStringLiteral("%1（%2）")
                                           .arg(filterId, QString::fromUtf8(entry.chineseName));
        QAction* action = new QAction(actionText, standardFilters);
        action->setObjectName(QStringLiteral("action_filter_%1").arg(filterId));
        action->setData(filterId);
        action->setStatusTip(QStringLiteral("Filter 标识：%1").arg(filterId));

        alphabeticalFilters->addAction(action);
        categoryMenu(entry.category)->addAction(action);
        if (entry.common) commonFilters->addAction(action);

        if (connectStandardFilterAction(action, filterId)) continue;

        connect(action, &QAction::triggered, this, [this, action, filterId]() {
            showDarkFramelessMessage(
                    QStringLiteral("Filter 尚未接入"),
                    QStringLiteral("%1\n\n菜单入口已经创建，对应算法尚未接入项目。\nFilter 标识：%2")
                            .arg(action->text(), filterId),
                    true);
        });
    }
    ui->menu_filters->addSeparator();

    connect(mesh_processing->addAction(QStringLiteral("表面网格简化 (Surface Simplification)")), &QAction::triggered, this, [&](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("表面网格简化"));
        int reductionId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("简化比例 (0..1)"), "0.5");
        int preserveId =
                dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, QStringLiteral("保留网格边界"), "true");
        int scalarId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, QStringLiteral("检查网格全部标量"),
                                            "true");
        int checkId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, QStringLiteral("几何相似性度量"),
                                           "false");
        tuneMeshSimplifyFilterDialog(dialog);
        dialog->show();
        dialog->setApplyFunctor([=, this]() {
            bool ok;
            QString result = "";

            MeshTriangulationFilter::Pointer triangulation = MeshTriangulationFilter::New();
            auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
            triangulation->SetInput(obj);
            ok = triangulation->Execute();

            if (!ok) {
                result = QString("网格简化算法只支持表面网格");
                showDarkFramelessMessage(QStringLiteral("非表面网格"), result);
                dialog->close();
                return;
            }

            obj = triangulation->GetOutput();

            MeshSimplificationFilter::Pointer filter = MeshSimplificationFilter::New();
            filter->SetTargetReduction(1 - dialog->getDouble(reductionId, ok));
            filter->SetPreserveBoundary(dialog->getChecked(preserveId, ok));
            filter->SetAllScalarCheck(dialog->getChecked(scalarId, ok));
            filter->SetInput(obj);

            ok = filter->Execute();

            if (!ok) {
                const QString detail = QString::fromStdString(filter->GetErrorMessage());
                result = detail.isEmpty()
                                 ? QStringLiteral("表面网格简化未能完成。")
                                 : detail;
                result += QStringLiteral(
                        "\n\n处理建议：\n"
                        "1. 检查并清理重复面、退化三角形和非流形边；\n"
                        "2. 降低简化比例，或取消“检查网格全部标量”后重试；\n"
                        "3. 若仍无法处理，可改用“快速表面简化”。");
                showDarkFramelessMessage(QStringLiteral("表面网格简化失败"), result);
                dialog->close();
                return;
            }

            auto oldMesh = DynamicCast<SurfaceMesh>(obj);
            auto outObj = filter->GetOutput();
            auto newMesh = DynamicCast<SurfaceMesh>(outObj);
            auto oldPoints = oldMesh->GetPoints();
            auto newPoints = newMesh->GetPoints();


            if (dialog->getChecked(checkId, ok)) {
                PointFinder::Pointer newPicker = PointFinder::New();
                newPicker->SetPoints(newPoints);
                newPicker->Initialize();

                double w1 = 0.0, w2 = 0.0;
                // 计算原始网格的表面积
                for (int i = 0; i < oldMesh->GetNumberOfFaces(); i++) {
                    igIndex f[3]{};
                    oldMesh->GetFacePointIds(i, f);
                    Point v0 = oldMesh->GetPoint(f[0]);
                    Point v1 = oldMesh->GetPoint(f[1]);
                    Point v2 = oldMesh->GetPoint(f[2]);

                    Vector3f d10 = v1 - v0;
                    Vector3f d20 = v2 - v0;

                    w1 += CrossProduct(d10, d20).norm() / 2.0;
                }

                double d1 = 0.0, d2 = 0.0;
                double d3 = 0.0, d4 = 0.0;

                iGame::ProgressObserver* ProgressBar = iGame::ProgressObserver::Instance();
                ProgressBar->UpdateProgress(0);
                int blockNum = oldPoints->GetNumberOfPoints() / 100, progress = 0;
                // 计算平均平方距离
                for (int i = 0; i < oldPoints->GetNumberOfPoints(); i++) {
                    if (i > progress * blockNum) {
                        ProgressBar->UpdateProgress(progress * 0.01);
                        progress++;
                    }
                    auto p = oldPoints->GetPoint(i);

                    igIndex id = newPicker->FindClosestPoint(p);
                    if (id != -1) {
                        Point cp = newPoints->GetPoint(id);
                        d1 += (p - cp).squaredNorm();
                        d3 += (p - cp).norm();
                    }
                }

                double d = 1.0 / w1 * d1 /*+ 1.0 / w2 * d2*/;
                double dd = 1.0 / oldPoints->GetNumberOfPoints() * d3 /*+ 1.0 / newPoints->GetNumberOfPoints() * d4*/;

                result += "\n几何相似性度量";
                result += "\n Squared Mean Distance: " + QString::number(d);
                result += "\n Mean Distance: " + QString::number(dd);
                result += "\nSquared Mean Distance: " + QString::number(d * 100) + "%";
                result += "\nMean Distance: " + QString::number(dd / oldMesh->GetBoundingBox().diag() * 100) + "%";
                result += "\n\n累计几何误差: " + QString::number(filter->GetError());
            } else {
                result += "\n累计几何误差: " + QString::number(filter->GetError());
            }

            modelTreeWidget->addDataObjectToModelTree(outObj, Algorithm);
            rendererWidget->update();

            // QMessageBox::information(this, "简化成功", result);
            dialog->close();
        });
    });

    connect(mesh_processing->addAction(QStringLiteral("Fast Surface Simplification（快速表面简化）")), &QAction::triggered, this, [&](bool checked) {
        if (rendererWidget->GetScene() == nullptr
            || rendererWidget->GetScene()->GetCurrentModel() == nullptr) {
            return;
        }

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("快速表面简化"));
        int reductionId =
                dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("目标简化比例 (0..1)"), "0.5");
        int faceCountId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("目标面数"), "0");

        int preserveId =
                dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, QStringLiteral("保留网格边界"), "true");
        //int scalarId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, QStringLiteral("检查网格全部标量"),
        //                                    "true");

        tuneMeshSimplifyFilterDialog(dialog);
        dialog->show();
        dialog->setApplyFunctor([=, this]() {
            bool ok;
            QString result = "";

            if (rendererWidget->GetScene() == nullptr
                || rendererWidget->GetScene()->GetCurrentModel() == nullptr) {
                showDarkFramelessMessage(QStringLiteral("无可用模型"), QStringLiteral("请先加载并选择模型。"));
                dialog->close();
                return;
            }
            auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
            if (obj == nullptr) {
                showDarkFramelessMessage(QStringLiteral("无可用模型"), QStringLiteral("当前模型没有可用数据。"));
                dialog->close();
                return;
            }

            MeshSimplificationFilterPro::Pointer filter = MeshSimplificationFilterPro::New();
            filter->SetInput(obj);
            filter->SetTargetReduction(dialog->getDouble(reductionId, ok));
            filter->SetTargetFaceCount(dialog->getInt(faceCountId, ok));
            filter->SetPreserveBoundary(dialog->getChecked(preserveId, ok));
            filter->SetFreeze(true);
            filter->SetTransformToCellData(true);
            ok = filter->Execute();

            if (!ok) {
                result = QStringLiteral("算法执行错误");
                showDarkFramelessMessage(QStringLiteral("执行出错"), result);
                dialog->close();
                return;
            }

            auto new_mesh = filter->GetOutput(0);
            if (new_mesh == nullptr) {
                showDarkFramelessMessage(QStringLiteral("执行出错"), QStringLiteral("算法未产生有效结果。"));
                dialog->close();
                return;
            }
            modelTreeWidget->addDataObjectToModelTree(new_mesh, Algorithm);
            rendererWidget->update();
            // QMessageBox::information(this, "执行成功", result);
            dialog->close();
        });
    });

    // connect(mesh_processing->addAction(QStringLiteral("表面简化 (Surface Simplification)")),
    //     &QAction::triggered, this, [&](bool checked) {
    //     auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();

    //     SurfaceMesh::Pointer mesh;
    //     if (obj->GetDataObjectType() == IG_SURFACE_MESH) {
    //         mesh = DynamicCast<SurfaceMesh>(obj);
    //     } else if (obj->GetDataObjectType() == IG_UNSTRUCTURED_MESH) {
    //         mesh = DynamicCast<UnstructuredMesh>(obj)->TransferToSurfaceMesh();
    //     }

    //     MeshTriangulationFilter::Pointer triangulation = MeshTriangulationFilter::New();
    //     triangulation->SetInput(mesh);
    //     if (!triangulation->Execute()) return false;
    //     mesh = DynamicCast<SurfaceMesh>(triangulation->GetOutput());

    //     igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this);
    //     int reductionId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "Reduction (0..1)", "0.05");
    //     int faceCountId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "Target Face Count", "0");

    //     dialog->show();
    //     dialog->setApplyFunctor([=, this]() {
    //         bool ok;
    //         QString result = "";

    //         std::vector<FVector> V;
    //         std::vector<int> F;
    //         std::vector<std::vector<float>> A;
    //         std::vector<float> AW;

    //         float minv[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
    //         float maxv[3] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    //         for (size_t i = 0; i < mesh->GetNumberOfPoints(); ++i) {
    //             auto& v = mesh->GetPoint(i);

    //             V.push_back({v[0], v[1], v[2]});

    //             for (int j = 0; j < 3; ++j) {
    //                 float vj = v[j];

    //                 minv[j] = minv[j] > vj ? vj : minv[j];
    //                 maxv[j] = maxv[j] < vj ? vj : maxv[j];
    //             }
    //         }

    //         float extent = 0.f;

    //         extent = (maxv[0] - minv[0]) < extent ? extent : (maxv[0] - minv[0]);
    //         extent = (maxv[1] - minv[1]) < extent ? extent : (maxv[1] - minv[1]);
    //         extent = (maxv[2] - minv[2]) < extent ? extent : (maxv[2] - minv[2]);

    //         float scale = extent == 0 ? 0.f : 1.f / extent;

    //         for (size_t i = 0; i < mesh->GetNumberOfPoints(); ++i) {
    //             V[i].x = (V[i].x - minv[0]) * scale;
    //             V[i].y = (V[i].y - minv[1]) * scale;
    //             V[i].z = (V[i].z - minv[2]) * scale;
    //         }

    //         igIndex ids[3];
    //         for (int i = 0; i < mesh->GetNumberOfFaces(); i++) {
    //             mesh->GetFacePointIds(i, ids);
    //             F.push_back(ids[0]);
    //             F.push_back(ids[1]);
    //             F.push_back(ids[2]);
    //         }

    //         for (int i = 0; i < mesh->GetAttributeSet()->GetNumberOfAttributes(); i++) {
    //             auto& attr = mesh->GetAttributeSet()->GetAttribute(i);
    //             int dim = attr.pointer->GetDimension();
    //             for (int d = 0; d < dim; d++) {
    //                 double val_max = -FLT_MAX;
    //                 double val_min = FLT_MAX;
    //                 for (size_t j = 0; j < mesh->GetNumberOfPoints(); j++) {
    //                     double val = attr.pointer->GetValue(j * dim + d);
    //                     val_max = val_max < val ? val : val_max;
    //                     val_min = val_min > val ? val : val_min;
    //                 }
    //                 if (val_min == val_max) {
    //                     std::vector<float> data(mesh->GetNumberOfPoints(), 0.f);
    //                     A.push_back(std::move(data));
    //                     AW.push_back(0.f);
    //                 } else {
    //                     std::vector<float> data;
    //                     for (size_t j = 0; j < mesh->GetNumberOfPoints(); j++) {
    //                         data.push_back(attr.pointer->GetValue(j * dim + d));
    //                     }
    //                     //for (size_t j = 0; j < mesh->GetNumberOfPoints(); j++) {
    //                     //    data.push_back((attr.pointer->GetValue(j * dim + d)));
    //                     //}
    //                     A.push_back(std::move(data));
    //                     AW.push_back(1.0f / (val_max - val_min));
    //                 }
    //             }
    //         }

    //         clock_t start = clock();
    //         MeshSaliencyCalculator saliencyCalculator(V, F);
    //         saliencyCalculator.Execute();

    //         MeshSimplifierWithAttributes simplifier(V, F, A, AW, saliencyCalculator.NormalCurvature,
    //                                                 dialog->getDouble(reductionId, ok));

    //         simplifier.SetUseVertexImportance(false);
    //         simplifier.SetUseDynamicAttributePenalty(true);

    //         simplifier.IsOptimizedPosition = true;
    //         simplifier.Execute();

    //         clock_t end = clock();
    //         std::cout << "Time taken: " << double(end - start) / CLOCKS_PER_SEC << " seconds." << std::endl;


    //         auto newMesh = SurfaceMesh::New();
    //         for (const auto& v: V) {
    //             newMesh->AddPoint(Point(v.x * extent + minv[0], v.y * extent + minv[1], v.z * extent + minv[2]));
    //         }
    //         //for (const auto& v: V) {
    //         //    newMesh->AddPoint(Point(v.x, v.y, v.z));
    //         //}
    //         auto CellArray = CellArray::New();
    //         for (int i = 0; i < F.size() / 3; i++) { CellArray->AddCellId3(F[i * 3], F[i * 3 + 1], F[i * 3 + 2]); }
    //         newMesh->SetFaces(CellArray);

    //         int count = 0;
    //         auto Attributes = AttributeSet::New();
    //         for (int i = 0; i < mesh->GetAttributeSet()->GetNumberOfAttributes(); i++) {
    //             auto& attr = mesh->GetAttributeSet()->GetAttribute(i);
    //             int dim = attr.pointer->GetDimension();
    //             auto arr = FloatArray::New();
    //             arr->SetDimension(dim);
    //             arr->Resize(mesh->GetNumberOfPoints());
    //             arr->SetName(attr.pointer->GetName());

    //             for (int d = 0; d < dim; d++) {
    //                 for (size_t j = 0; j < mesh->GetNumberOfPoints(); j++) {
    //                     arr->SetValue(j * dim + d, A[count + d][j]);
    //                 }
    //             }
    //             count += dim;

    //             Attributes->AddAttribute(attr.type, attr.attachmentType, arr);
    //         }
    //         newMesh->SetAttributeSet(Attributes);

    //         modelTreeWidget->addDataObjectToModelTree(newMesh, Algorithm);
    //         rendererWidget->update();
    //         dialog->close();
    //     });
    // });

    connect(mesh_processing->addAction(QStringLiteral("Surface Triangulation（表面三角化）")), &QAction::triggered, this, [this, showSurfaceTopologyReport](bool checked) {
        auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();

        MeshTriangulationFilter::Pointer triangulation = MeshTriangulationFilter::New();
        triangulation->SetInput(obj);
        if (triangulation->Execute()) {
            auto mesh = DynamicCast<SurfaceMesh>(triangulation->GetOutput());

            modelTreeWidget->addDataObjectToModelTree(mesh, Algorithm);
            rendererWidget->update();
            showSurfaceTopologyReport(mesh, QStringLiteral("表面三角化后"));
        }
    });

    connect(mesh_processing->addAction(QStringLiteral("表面提取 (Extract Surface)")), &QAction::triggered, this, [&](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        if (!obj) return;

        auto filter = ConvertToSurfaceMeshFilter::New();
        filter->SetInput(obj);
        filter->SetConvertMethod(ConvertToSurfaceMeshFilter::IG_EXTRACT_SURFACE_MESH);
        if (!filter->Execute()) {
            showDarkFramelessMessage(QStringLiteral("Warning"),
                                     QStringLiteral("当前数据类型不支持表面提取。"));
            return;
        }

        auto surface = filter->GetSurfaceMesh();
        if (!surface) {
            showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("表面提取失败。"));
            return;
        }
        if (surface.GetPointer() == obj.GetPointer()) {
            showDarkFramelessMessage(QStringLiteral("Warning"),
                                     QStringLiteral("当前模型已经是表面网格，无需提取。"));
            return;
        }
        if (surface->GetNumberOfFaces() == 0) {
            showDarkFramelessMessage(QStringLiteral("Warning"),
                                     QStringLiteral("提取结果为空，当前模型没有可提取的表面单元。"));
            return;
        }

        surface->SetName(obj->GetName() + "_surface");
        modelTreeWidget->addDataObjectToModelTree(surface, Algorithm);
        rendererWidget->update();
    });

    auto* tetraAction = mesh_processing->addAction(QStringLiteral("Mesh Tetrahedralize（四面体化）"));
    tetraAction->setObjectName("action_filter_mesh_tetrahedralize");
    connectThirdBatchFilterAction(tetraAction, "mesh_tetrahedralize");
    auto* simplifyAction = mesh_processing->addAction(QStringLiteral("Volume Mesh Simplification（体网格简化）"));
    simplifyAction->setObjectName("action_filter_volume_mesh_simplification");
    connectThirdBatchFilterAction(simplifyAction, "volume_mesh_simplification");

    //connect(mesh_processing->addAction("Test"), &QAction::triggered, this, [&](bool checked) {
    //    auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();

    //    auto m_StreamBase = iGame::StreamBase::New();
    //    auto streamtracer = m_StreamBase->streamFilter;
    //    streamtracer->initStreamTracer(obj);
    //    //auto seeds=streamtracer->getModelSelect();//当实际已经选中了重点区域时直接调用该函数
    //    Vector3f boundMax = streamtracer->GetMesh()->GetBoundingBox().max; //包围盒区域
    //    Vector3f boundMin = streamtracer->GetMesh()->GetBoundingBox().min;
    //    Vector3f centerMax = (boundMax - boundMin) / 5 + boundMin; //模拟被选中重点区域
    //    auto seeds = streamtracer->getAllSubBlockCenters(boundMax, boundMin, centerMax, boundMin, 2,
    //                                                     4); //4，6为划分子块的数量
    //    float lengthOfStreamLine = 5;
    //    float lengthOfStep = 0.3;
    //    float maxSteps = 1000;
    //    float terminalSpeed = 0.005;
    //    streamtracer->SetInput(seeds, "V", lengthOfStreamLine, lengthOfStep, terminalSpeed, maxSteps);
    //    streamtracer->Execute();
    //    std::cout << seeds.size() << std::endl;
    //    auto output = streamtracer->GetOutput();

    //    modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
    //    rendererWidget->update();
    //});

    //connect(mesh_processing->addAction("Test2"), &QAction::triggered, this, [&](bool checked) { 
    //    auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();

    //    auto filter = iGame::VolumeMeshMetricsFilter::New();
    //    filter->SetVolumeMetric(VolumeMeshMetricsFilter::HEX_VOLUME);
    //    filter->SetInput(obj);
    //    filter->Execute();

    //    modelTreeWidget->addDataObjectToModelTree(filter->GetOutput(), Algorithm);
    //    rendererWidget->update();
    //    });
    //connect(mesh_processing->addAction("Test3"), &QAction::triggered, this, [&](bool checked) 
    //    { 
    //        CellArray::Pointer cellArray = CellArray::New();
    //        clock_t start = clock();
    //        igIndex cell[3]{};
    //        cellArray->AddCellIds(cell, 2);
    //        for (int i = 0; i < 10000000; i++) { 
    //            cellArray->AddCellIds(cell, 3);
    //        }
    //        clock_t end = clock();
    //        std::cout << end - start << std::endl;

    //    });
    // 转换就地作用于「当前帧」的数据（普通模型=自身；PVD 等复合模型=当前挂载的所有子块），
    // 因为不再 addDataObjectToModelTree()，所以不会再出现“转换后多出一个同名模型”的问题。
    connect(convert->addAction(QStringLiteral("Convert To Point Data（转换为点数据）")), &QAction::triggered, this, [&](bool checked) {
        QString reason;
        QStringList names;
        const int created = createConvertedFrameModel(true, reason, names);
        if (created <= 0) {
            showDarkFramelessMessage(QStringLiteral("转换未完成"),
                                     reason.isEmpty() ? QStringLiteral("未能完成转换。") : reason);
            return;
        }
        showDarkFramelessMessage(
                QStringLiteral("转换完成"),
                QStringLiteral("已就地转换当前帧，模型改名为「%1」")
                        .arg(names.join(QStringLiteral("、"))),
                true);
    });
    connect(convert->addAction(QStringLiteral("Convert To Cell Data（转换为单元数据）")), &QAction::triggered, this, [&](bool checked) {
        QString reason;
        QStringList names;
        const int created = createConvertedFrameModel(false, reason, names);
        if (created <= 0) {
            showDarkFramelessMessage(QStringLiteral("转换未完成"),
                                     reason.isEmpty() ? QStringLiteral("未能完成转换。") : reason);
            return;
        }
        showDarkFramelessMessage(
                QStringLiteral("转换完成"),
                QStringLiteral("已就地转换当前帧，模型改名为「%1」")
                        .arg(names.join(QStringLiteral("、"))),
                true);
    });


    // ===== IsoVolume 等值面体提取 =====
    connect(extractionFilters->addAction(QStringLiteral("等值面体提取 (IsoVolume)")), &QAction::triggered, this,
            [&](bool checked) {
                if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) {
                    showDarkFramelessMessage(QStringLiteral("提示"), QStringLiteral("请先加载一个模型"));
                    return;
                }
                auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
                if (!obj || !obj->GetAttributeSet()) return;
                auto attrs = obj->GetAttributeSet()->GetAllPointAttributes();
                if (!attrs || attrs->GetNumberOfElements() == 0) {
                    showDarkFramelessMessage(QStringLiteral("提示"),
                                             QStringLiteral("当前模型没有点标量数据，无法进行等值面体提取"));
                    return;
                }

                igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(QStringLiteral("等值面体提取 (IsoVolume)"));
                dialog->setFilterDescription(QStringLiteral("提取标量值落在 [lower, upper] 区间内的体数据"));

                std::vector<QString> arrNames;
                for (igIndex a = 0; a < attrs->GetNumberOfElements(); ++a) {
                    arrNames.push_back(QString::fromStdString(attrs->GetElement(a).pointer->GetName()));
                }
                int arrayId = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX,
                                                   QStringLiteral("点属性数组"), arrNames);

                std::vector<QString> comps0{QStringLiteral("分量 0")};
                int compId = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX,
                                                  QStringLiteral("标量分量"), comps0);
                int lowerId =
                        dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("lower"), "0");
                int upperId =
                        dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("upper"), "0");

                // 按 (数组, 分量) 刷新分量下拉框与 lower/upper 默认值
                auto updateRange = [=](int arrayIdx, int compIdx) {
                    if (arrayIdx < 0 || arrayIdx >= (int) attrs->GetNumberOfElements()) return;
                    auto& at = attrs->GetElement(arrayIdx);
                    auto arr = at.pointer;
                    if (!arr) return;
                    const int d = arr->GetDimension();
                    QComboBox* compCombo = dynamic_cast<QComboBox*>(dialog->getWidget(compId));
                    if (compCombo) {
                        compCombo->blockSignals(true);
                        compCombo->clear();
                        const int n = (d > 0 ? d : 1);
                        for (int i = 0; i < n; ++i) { compCombo->addItem(QStringLiteral("分量 %1").arg(i)); }
                        compCombo->setCurrentIndex(compIdx >= 0 && compIdx < n ? compIdx : 0);
                        compCombo->blockSignals(false);
                    }
                    const int c = compCombo ? compCombo->currentIndex() : 0;
                    auto range = at.GetDataRange();
                    double smin = 0.0, smax = 1.0;
                    if (range) {
                        const int base = 2 + 2 * c;
                        if (range->GetNumberOfElements() >= base + 2) {
                            smin = range->GetValue(base);
                            smax = range->GetValue(base + 1);
                        } else if (range->GetNumberOfElements() >= 2) {
                            smin = range->GetValue(0);
                            smax = range->GetValue(1);
                        }
                    }
                    if (smax <= smin) smax = smin + 1.0;
                    if (auto lo = dynamic_cast<QLineEdit*>(dialog->getWidget(lowerId))) {
                        lo->setText(QString::number(smin + (smax - smin) / 3.0));
                    }
                    if (auto up = dynamic_cast<QLineEdit*>(dialog->getWidget(upperId))) {
                        up->setText(QString::number(smin + (smax - smin) * 2.0 / 3.0));
                    }
                };
                updateRange(0, 0);

                QComboBox* arrCombo = dynamic_cast<QComboBox*>(dialog->getWidget(arrayId));
                QComboBox* compComboW = dynamic_cast<QComboBox*>(dialog->getWidget(compId));
                if (arrCombo) {
                    connect(arrCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                            [updateRange](int idx) { updateRange(idx, 0); });
                }
                if (compComboW) {
                    connect(compComboW, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                            [updateRange, arrCombo](int ci) {
                                const int ai = arrCombo ? arrCombo->currentIndex() : 0;
                                updateRange(ai, ci);
                            });
                }
                dialog->show();

                dialog->setApplyFunctor([=, this]() {
                    bool okArr = false, okComp = false, okLower = false, okUpper = false;
                    int arrIdx = dialog->getComboIndex(arrayId, okArr);
                    int comp = dialog->getComboIndex(compId, okComp);
                    double lower = dialog->getDouble(lowerId, okLower);
                    double upper = dialog->getDouble(upperId, okUpper);
                    if (!okLower || !okUpper) {
                        showDarkFramelessMessage(QStringLiteral("提示"),
                                                 QStringLiteral("lower / upper 请输入有效数字"));
                        return;
                    }
                    if (lower > upper) { std::swap(lower, upper); }
                    if (arrIdx < 0 || arrIdx >= (int) attrs->GetNumberOfElements()) {
                        showDarkFramelessMessage(QStringLiteral("提示"), QStringLiteral("请选择有效的点属性数组"));
                        return;
                    }
                    auto array = attrs->GetElement(arrIdx).pointer;

                    auto filter = IsoVolumeFilter::New();
                    filter->SetInput(obj);
                    filter->SetIsoScalarData(array, lower, upper, comp);
                    if (!filter->Execute()) {
                        showDarkFramelessMessage(QStringLiteral("警告"),
                                                 QStringLiteral("等值面体提取执行失败，请检查数据与区间"));
                        return;
                    }
                    auto out = filter->GetOutput();
                    if (!out) return;
                    out->SetName(obj->GetName() + "_isovolume");
                    if (auto outMesh = DynamicCast<UnstructuredMesh>(out)) {
                        showDarkFramelessMessage(QStringLiteral("等值面体提取结果"),
                                                 QStringLiteral("输出 %1 点 / %2 单元（区间 [%3, %4]，含点合并）")
                                                         .arg(outMesh->GetNumberOfPoints())
                                                         .arg(outMesh->GetNumberOfCells())
                                                         .arg(lower)
                                                         .arg(upper));
                    }
                    modelTreeWidget->addDataObjectToModelTree(out, Algorithm);
                    rendererWidget->update();
                    dialog->close();
                });
            });

    auto runAdvancedGradient = [this]() {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        auto data = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        auto attrSet = data->GetAttributeSet();
        if (!attrSet || attrSet->GetNumberOfAttributes() <= 0) {
            showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("当前模型没有可用属性"));
            return;
        }

        // 自动选择一个属性：优先使用当前选中属性；
        // 如果没有选中属性，则优先选择向量属性（dim==3），否则选第一个属性
        int index = data->GetAttributeIndex();
        if (index < 0 || index >= static_cast<int>(attrSet->GetNumberOfAttributes())) {
            index = -1;
            for (int i = 0; i < static_cast<int>(attrSet->GetNumberOfAttributes()); ++i) {
                auto& attr = attrSet->GetAttribute(i);
                if (attr.pointer && attr.pointer->GetDimension() == 3) {
                    index = i;
                    break;
                }
            }
            if (index < 0) index = 0;
        }

        // 自动确定分量：标量用 0；向量如果没有选中分量，默认用第 0 个分量
        int component = data->GetCurrentAttributeDimension();
        int attrDim = attrSet->GetAttribute(index).pointer->GetDimension();
        if (attrDim == 1) {
            component = 0;
        } else if (component < 0) {
            component = 0;
        }

        AdvancedGradientFilter::Pointer filter = AdvancedGradientFilter::New();
        filter->SetInput(data);
        filter->SetAttributeByIndex(index);
        filter->SetCurrentAttributeDimension(component);
        // - 标量输入 → 3 分量向量
        // - 向量输入 → 9 分量张量
        // 输出点属性
        filter->SetComputeGradientTensor(true);
        filter->SetOutputToPointData(true);
        if (filter->Execute()) {
            auto drawObject = DynamicCast<DrawObject>(data);
            if (drawObject) {
                drawObject->GetColorMapper()->SetRangeStable(false);
            }
            modelTreeWidget->updateAllAttriubute(data);
            if (drawObject) {
                auto item = modelTreeWidget->getItemFromObject(data);
                if (item && item->childCount() > 0) {
                    // 选中新生成的 gradient 属性
                    int newIndex = static_cast<int>(data->GetAttributeSet()->GetNumberOfAttributes() - 1);
                    // 默认显示 magnitude，不显示 x/g0
                    int viewDim = -1;
                    item->setExpanded(true);
                    auto child = item->child(newIndex);
                    if (child) {
                        item->setCurrentChild(child);
                        item->setSelected(false);
                        item->viewAttribute(newIndex, viewDim);
                        // 强制重新生成可绘制颜色数据，避免只切换属性索引但颜色未刷新
                        drawObject->ForceReConvertToDrawableData();
                        child->setSelected(true);
                        modelTreeWidget->setCurrentItem(child);
                        rendererWidget->update();


                    }
                }
            }
        } else {
            std::string message = filter->GetMessage();
            showDarkFramelessMessage(QStringLiteral("Warning"), QString::fromStdString(message));
        }
    };

    QAction* gradient = view->addAction(QStringLiteral("Compute Gradient（计算梯度）"));
    connect(gradient, &QAction::triggered, this, [this, runAdvancedGradient](bool) {
        runAdvancedGradient();
    });

    QAction* laplacian = view->addAction(QStringLiteral("Compute Laplacian（计算拉普拉斯）"));
    connect(laplacian, &QAction::triggered, this, [this](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        LaplacianFilter::Pointer filter = LaplacianFilter::New();
        auto data = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        filter->SetInput(data);
        filter->SetAttributeByIndex(data->GetAttributeIndex());
        int index = data->GetAttributeIndex();
        if (filter->Execute()) {
            modelTreeWidget->updateAllAttriubute(data);
            auto drawObject = DynamicCast<DrawObject>(data);
            if (drawObject) {
                auto item = modelTreeWidget->getItemFromObject(data);
                if (item && item->childCount() > 0) {
                    item->setExpanded(true);
                    auto child = item->child(index);
                    if (child) {
                        item->setCurrentChild(child);
                        item->setSelected(false);
                        item->viewAttribute(index, -1);
                        child->setSelected(true);
                        modelTreeWidget->setCurrentItem(child);
                    }
                }
            }
        }
        else {
            std::string message = filter->GetMessage();
            showDarkFramelessMessage(QStringLiteral("Warning"), QString::fromStdString(message));
        }
    });

    QAction* curvature = view->addAction(QStringLiteral("Compute Curvature（计算曲率）"));
    connect(curvature, &QAction::triggered, this, [this](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        CurvatureFilter::Pointer filter = CurvatureFilter::New();
        auto data = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        filter->SetInput(data);
        filter->SetAttributeByIndex(data->GetAttributeIndex());
        int index = data->GetAttributeIndex();
        if (filter->Execute()) {
            modelTreeWidget->updateAllAttriubute(data);
            auto drawObject = DynamicCast<DrawObject>(data);
            if (drawObject) {
                auto item = modelTreeWidget->getItemFromObject(data);
                if (item && item->childCount() > 0) {
                    item->setExpanded(true);
                    auto child = item->child(index);
                    if (child) {
                        item->setCurrentChild(child);
                        item->setSelected(false);
                        item->viewAttribute(index, -1);
                        child->setSelected(true);
                        modelTreeWidget->setCurrentItem(child);
                    }
                }
            }
        }
        else {
            std::string message = filter->GetMessage();
            showDarkFramelessMessage(QStringLiteral("Warning"), QString::fromStdString(message));
        }
    });

    QAction* vortex = view->addAction(QStringLiteral("Compute Vorticity（计算涡量）"));
    connect(vortex, &QAction::triggered, this, [this](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        auto data = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        int index = data->GetAttributeIndex();

        // 结果刷新到模型树 + 云图，单帧 / 时序共用
        auto refreshTree = [this, data, index]() {
            modelTreeWidget->updateAllAttriubute(data);
            auto drawObject = DynamicCast<DrawObject>(data);
            if (!drawObject) return;
            drawObject->ConvertToDrawableData();
            auto item = modelTreeWidget->getItemFromObject(data);
            if (item && item->childCount() > 0) {
                item->setExpanded(true);
                auto child = item->child(index);
                if (child) {
                    item->setCurrentChild(child);
                    item->setSelected(false);
                    item->viewAttribute(index, -1);
                    child->setSelected(true);
                    modelTreeWidget->setCurrentItem(child);
                }
            }
        };

        // 时序数据（PVD / 多选 VTU 等 MultiSubFiles）：
        // 这里只算「当前帧」，随后开启播放期按需计算——切帧时若该帧已带 vorticities
        // （缓存命中）就直接复用，否则同步算完再渲染该帧。
        // 这样既不用一次性等全部帧算完，也不会因缓存被清而出现空白帧。
        auto frames = data->PeekTimeFrames();
        const int frameNum = frames ? static_cast<int>(frames->GetTimeNum()) : 0;
        if (frameNum > 1) {
            // 属性按名字下传：各帧 AttributeSet 独立，索引不保证一致
            std::string attrName;
            if (auto attrSet = data->GetAttributeSet()) {
                if (index >= 0 && index < attrSet->GetNumberOfAttributes()) {
                    if (auto ptr = attrSet->GetAttribute(index).pointer) { attrName = ptr->GetName(); }
                }
            }
            if (attrName.empty()) {
                showDarkFramelessMessage(QStringLiteral("Warning"),
                                         QStringLiteral("请先选择一个矢量属性"));
                return;
            }

            // 尽量把算过的帧留在缓存里，避免播放时反复重算。
            // 在这里声明诉求，可使后续任何 initAnimationComponents
            //（如选中属性触发 CurrendModelChanged）都不会把缓存关掉。
            ui->widget_Animation->setPreferredCacheNum(frameNum);
            if (frames->GetMaxCacheSize() < static_cast<unsigned int>(frameNum)) {
                frames->EnableCache(frameNum);
            }

            // 只算当前帧；父容器属性登记、值域刷新与进度条都在该函数内部完成
            const int vortIndex = ui->widget_Animation->ensureVortexForCurrentFrame(data, attrName, 0);

            if (vortIndex < 0) {
                showDarkFramelessMessage(QStringLiteral("Warning"),
                                         QStringLiteral("当前帧的涡量计算失败"));
                return;
            }

            modelTreeWidget->updateAllAttriubute(data);
            if (auto drawObj = DynamicCast<DrawObject>(data)) { drawObj->ConvertToDrawableData(); }
            auto item = modelTreeWidget->getItemFromObject(data);
            if (item && item->childCount() > vortIndex) {
                item->setExpanded(true);
                auto child = item->child(vortIndex);
                if (child) {
                    item->setCurrentChild(child);
                    item->setSelected(false);
                    item->viewAttribute(vortIndex, -1);
                    child->setSelected(true);
                    modelTreeWidget->setCurrentItem(child);
                }
            }

            // 必须放在模型树操作之后：setCurrentItem 会触发 CurrendModelChanged →
            // initAnimationComponents，那里会按模型是否变化决定关闭按需计算。
            // 若提前开启，会被这条链路当成「尚未绑定模型」而立即关掉。
            ui->widget_Animation->setVortexAutoCompute(true, attrName);
            return;
        }

        // 非时序：保持原有单次计算行为
        VortexFilter::Pointer filter = VortexFilter::New();
        filter->SetAttributeByIndex(index);
        filter->SetInput(data);
        if (filter->Execute()) {
            refreshTree();
        } else {
            std::string message = filter->GetMessage();
            showDarkFramelessMessage(QStringLiteral("Warning"), QString::fromStdString(message));
        }
    });

    QAction* vortexPrection = view->addAction(QStringLiteral("Predict Vortex（涡旋预测）"));
    connect(vortexPrection, &QAction::triggered, this, [this](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        VortexDetection::Pointer filter = VortexDetection::New();
        auto data = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        filter->SetInput(data);
        filter->SetAttributeByIndex(data->GetAttributeIndex());
        int index = data->GetAttributeIndex();
        if (filter->Execute()) {
            modelTreeWidget->updateAllAttriubute(data);
            rendererWidget->update();
            auto drawObject = DynamicCast<DrawObject>(data);
            if (drawObject) {
                auto item = modelTreeWidget->getItemFromObject(data);
                if (item && item->childCount() > 0) {
                    item->setExpanded(true);
                    auto child = item->child(index);
                    if (child) {
                        item->setCurrentChild(child);
                        item->setSelected(false);
                        item->viewAttribute(index, -1);
                        child->setSelected(true);
                        modelTreeWidget->setCurrentItem(child);
                    }

                }
            }
            // 显示指标
            // double acc  = filter->GetAccuracy();
            // double prec = filter->GetPrecision();
            // double rec  = filter->GetRecall();
            //
            // if (acc > 0.0 && prec > 0.0 && rec > 0.0 &&
            //     !std::isnan(acc) && !std::isnan(prec) && !std::isnan(rec)) {
            //     QDialog* dialog = this->property("vortexMetricsDialog").value<QDialog*>();
            //
            //     if (!dialog) {
            //         dialog = new QDialog(this);
            //         dialog->setWindowTitle("Vortex Prediction Metrics");
            //         dialog->setAttribute(Qt::WA_DeleteOnClose);
            //         dialog->setModal(false);
            //
            //         this->setProperty("vortexMetricsDialog", QVariant::fromValue(dialog));
            //
            //         QLabel* label = new QLabel(dialog);
            //         label->setObjectName("vortexMetricsLabel");
            //         label->setTextFormat(Qt::RichText);
            //         label->setAlignment(Qt::AlignCenter);
            //
            //         QVBoxLayout* layout = new QVBoxLayout(dialog);
            //         layout->addWidget(label);
            //         dialog->setLayout(layout);
            //         dialog->resize(270, 100);
            //         connect(dialog, &QDialog::destroyed, this, [this]() {
            //             this->setProperty("vortexMetricsDialog", QVariant());
            //         });
            //     }
            //     QLabel* label = dialog->findChild<QLabel*>("vortexMetricsLabel");
            //     if (label) {
            //         QString msg = QString(
            //             "<table align='center' cellspacing='6'>"
            //             // "<tr><td>Accuracy</td><td>:</td><td>%1</td></tr>"
            //             "<tr><td>Precision</td><td>:</td><td>%1%<</td></tr>"
            //             "<tr><td>Recall</td><td>:</td><td>%2%<</td></tr>"
            //             "</table>"
            //         )
            //         // .arg(acc,  0, 'f', 3)
            //         .arg(prec * 100.0, 0, 'f', 2)
            //         .arg(rec * 100.0,  0, 'f', 2);
            //
            //         label->setText(msg);
            //     }
            //     QPointer<QDialog> safeDialog(dialog);
            //     QTimer::singleShot(48, this, [safeDialog]() {
            //         if (!safeDialog) return;
            //         safeDialog->show();
            //         safeDialog->raise();
            //     });
            // }

            // old version
            // vortexMetricsLabel
            // double acc  = filter->GetAccuracy();
            // double prec = filter->GetPrecision();
            // double rec  = filter->GetRecall();
            // if (acc > 0.0 && prec > 0.0 && rec > 0.0) {
            //     QString txt = QString("Acc: %1  Prec: %2  Rec: %3")
            //                       .arg(acc,  0, 'f', 3)
            //                       .arg(prec, 0, 'f', 3)
            //                       .arg(rec,  0, 'f', 3);
            //     vortexMetricsLabel->setText(txt);
            //     vortexMetricsLabel->show();
            //     updateVortexMetricsLabelPos();
            // } else {
            //     vortexMetricsLabel->clear();
            //     vortexMetricsLabel->hide();
            // }
        }else {
            std::string message = filter->GetMessage();
            showDarkFramelessMessage(QStringLiteral("Warning"), QString::fromStdString(message));
        }
    });

    QMenu* attrDiffMenu = view->addMenu(QStringLiteral("属性差值(AttrDiffPerFrame)"));
    auto funcAttrDiff = [this](int mode) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        auto data = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        if (!data) return;
        auto frames = data->PeekTimeFrames();
        const int frameNum = frames ? static_cast<int>(frames->GetTimeNum()) : 0;
        if (frameNum <= 1) {
            showDarkFramelessMessage(QStringLiteral("Warning"),
                                     QStringLiteral("时间帧序列数不足"));
            return;
        }
        int index = data->GetAttributeIndex();
        if (index < 0) {
            showDarkFramelessMessage(QStringLiteral("Warning"),
                                     QStringLiteral("模型树未选择属性"));
            return;
        }
        std::string attrName;
        if (auto attrSet = data->GetAttributeSet()) {
            if (index < static_cast<int>(attrSet->GetNumberOfAttributes())) {
                if (auto ptr = attrSet->GetAttribute(index).pointer) {
                    attrName = ptr->GetName();
                }
            }
        }
        if (attrName.empty()) {
            showDarkFramelessMessage(QStringLiteral("Warning"),
                                     QStringLiteral("属性无效"));
            return;
        }
        // 算过的帧尽量保存在缓存中
        ui->widget_Animation->setPreferredCacheNum(frameNum);
        if (frames->GetMaxCacheSize() < static_cast<unsigned int>(frameNum)) {
            frames->EnableCache(frameNum);
        }

        ui->widget_Animation->SetDiffMode(mode);
        int diffIndex = -1;
        for (int j = 0; j < frameNum; ++j) {
            data->UpdateAnimation(j);
            diffIndex = ui->widget_Animation->EnsureTimeDifferenceForCurrentFrame(data, attrName, j);
            if (diffIndex < 0) {
                showDarkFramelessMessage(QStringLiteral("Warning"),
                                         QStringLiteral("第 %1 帧的属性差值计算失败").arg(j + 1));
                return;
            }
        }
        // 重建属性子节点
        modelTreeWidget->updateAllAttriubute(data);
        if(auto drawObj = DynamicCast<DrawObject>(data)) {
            drawObj->ConvertToDrawableData();
        }
        // 自动选中diff
        auto item = modelTreeWidget->getItemFromObject(data);
        if (item && item->childCount() > diffIndex) {
            item->setExpanded(true);
            auto child = item->child(diffIndex);
            if (child) {
                item->setCurrentChild(child);
                item->setSelected(false);
                item->viewAttribute(diffIndex, -1);
                child->setSelected(true);
                modelTreeWidget->setCurrentItem(child);
            }
         }
        // 在模型树操作之后,setCurrentItem触发CurrendModelChanged→initAnimationComponents，
        // 若提前开启会被当成尚未绑定模型而立即关掉。
        ui->widget_Animation->SetDiffAutoCompute(true, attrName);
    };
    QAction* attrDiffSigned = attrDiffMenu->addAction(QStringLiteral("带符号差"));
    connect(attrDiffSigned, &QAction::triggered, this, [funcAttrDiff](bool) { funcAttrDiff(0); });
    QAction* attrDiffAbs = attrDiffMenu->addAction(QStringLiteral("绝对差"));
    connect(attrDiffAbs, &QAction::triggered, this, [funcAttrDiff](bool) { funcAttrDiff(1); });
    QAction* attrDiffRel = attrDiffMenu->addAction(QStringLiteral("相对变化率"));
    connect(attrDiffRel, &QAction::triggered, this, [funcAttrDiff](bool) { funcAttrDiff(2); });

    QAction* lagrangeUnstructedMesh_visualization = convert->addAction(
            QStringLiteral("拉格朗日非结构网格可视化 (LagrangeUnstructedMesh Visualization)"));
    connect(lagrangeUnstructedMesh_visualization, &QAction::triggered, this, [&](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        ConvertToLagrangeUnstructuredMeshFilter::Pointer filter = ConvertToLagrangeUnstructuredMeshFilter::New();
        auto data = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        filter->SetInput(data);
        if (filter->Execute()) {
            DataObject::Pointer res = filter->GetOutput(0);
            res->SetName(data->GetName());
            modelTreeWidget->addDataObjectToModelTree(res, Algorithm);
        }
    });
    // dayuwan77 的开发菜单只提供独立入口；已通过的实现仍由标准过滤器持有。
    // 使用代理 QAction 避免改动标准菜单中 QAction 的文字、位置或连接。
    QMenu* developingFilters = ui->menu_filters->addMenu(QStringLiteral("开发中filter"));
    developingFilters->setObjectName(QStringLiteral("menu_developing_filters"));
    QAction* cellMeshMetrics = developingFilters->addAction(QStringLiteral("单元质量评估 (Cell Quality)"));
    connect(cellMeshMetrics, &QAction::triggered, this, [&](bool checked) {
        // 1. 创建对话框
        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("单元质量评估 (Cell Quality)"));
        dialog->setFilterDescription(
                QStringLiteral("对齐 ParaView Cell Quality：选择一个统一指标作用于模型内所有单元；\n"
                               "不适用的单元填入「不支持值」，执行后汇总报告跳过数量。\n"
                               "适用范围：四面体、六面体、三角形、四边形；\n"
                               "各指标对单元类型的适用性按 ParaView Cell Quality 对齐。"));

        // 2. 统一指标下拉框（与 ParaView 一致的交互，不再区分四面体/六面体专用算法）
        //    20 项按 ParaView Cell Quality 的 Quality Measure 列表原样排列
        std::vector<QString> metricNames = {
                QStringLiteral("面积 (Area)"),
                QStringLiteral("纵横比 (Aspect Ratio)"),
                QStringLiteral("条件数 (Condition)"),
                QStringLiteral("畸变度 (Distortion)"),
                QStringLiteral("边长比 (Edge Ratio)"),
                QStringLiteral("等角歪斜 (Equiangle Skew)"),
                QStringLiteral("雅可比行列式 (Jacobian)"),
                QStringLiteral("最小内角 (Minimum Angle)"),
                QStringLiteral("Oddy"),
                QStringLiteral("半径比 (Radius Ratio)"),
                QStringLiteral("相对尺寸平方 (Relative Size Squared)"),
                QStringLiteral("归一化雅可比 (Scaled Jacobian)"),
                QStringLiteral("形状 (Shape)"),
                QStringLiteral("形状与尺寸 (Shape and Size)"),
                QStringLiteral("剪切 (Shear)"),
                QStringLiteral("剪切与尺寸 (Shear and Size)"),
                QStringLiteral("歪斜度 (Skew)"),
                QStringLiteral("伸展度 (Stretch)"),
                QStringLiteral("锥度 (Taper)"),
                QStringLiteral("翘曲度 (Warpage)")};

        std::vector<iGame::CellMeshMetricsFilter::CellQualityMetric> metricEnums = {
                iGame::CellMeshMetricsFilter::QUALITY_AREA,
                iGame::CellMeshMetricsFilter::QUALITY_ASPECT_RATIO,
                iGame::CellMeshMetricsFilter::QUALITY_CONDITION,
                iGame::CellMeshMetricsFilter::QUALITY_DISTORTION,
                iGame::CellMeshMetricsFilter::QUALITY_EDGE_RATIO,
                iGame::CellMeshMetricsFilter::QUALITY_EQUIANGLE_SKEW,
                iGame::CellMeshMetricsFilter::QUALITY_JACOBIAN,
                iGame::CellMeshMetricsFilter::QUALITY_MIN_ANGLE,
                iGame::CellMeshMetricsFilter::QUALITY_ODDY,
                iGame::CellMeshMetricsFilter::QUALITY_RADIUS_RATIO,
                iGame::CellMeshMetricsFilter::QUALITY_RELATIVE_SIZE_SQUARED,
                iGame::CellMeshMetricsFilter::QUALITY_SCALED_JACOBIAN,
                iGame::CellMeshMetricsFilter::QUALITY_SHAPE,
                iGame::CellMeshMetricsFilter::QUALITY_SHAPE_AND_SIZE,
                iGame::CellMeshMetricsFilter::QUALITY_SHEAR,
                iGame::CellMeshMetricsFilter::QUALITY_SHEAR_AND_SIZE,
                iGame::CellMeshMetricsFilter::QUALITY_SKEW,
                iGame::CellMeshMetricsFilter::QUALITY_STRETCH,
                iGame::CellMeshMetricsFilter::QUALITY_TAPER,
                iGame::CellMeshMetricsFilter::QUALITY_WARPAGE};

        int comboID =
                dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX, QStringLiteral("评估指标"), metricNames);

        // 3. 不支持/无效值（对齐 ParaView 的 UnsupportedGeometry，默认 -1.0）
        int unsupportedId =
                dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("不支持/无效值"), "-1.0");

        dialog->show();

        // 4. 确认回调逻辑
        dialog->setApplyFunctor([=, this]() {
            // 4.1 前置校验：必须有选中模型
            auto scene = rendererWidget ? rendererWidget->GetScene() : nullptr;
            if (scene == nullptr || scene->GetCurrentModel() == nullptr) {
                showDarkFramelessMessage(QStringLiteral("无可用模型"),
                                         QStringLiteral("请先在模型树中选中需要评估的体网格模型。"));
                dialog->close();
                return;
            }
            auto obj = scene->GetCurrentModel()->GetDataObject();
            if (obj == nullptr) {
                showDarkFramelessMessage(QStringLiteral("无可用模型"), QStringLiteral("当前模型数据对象为空。"));
                dialog->close();
                return;
            }

            // 4.2 读取指标
            bool ok = false;
            int selectedIndex = dialog->getComboIndex(comboID, ok);
            if (!ok || selectedIndex < 0 || selectedIndex >= (int) metricEnums.size()) {
                showDarkFramelessMessage(QStringLiteral("无效选择"), QStringLiteral("请选择一个有效的评估指标。"));
                dialog->close();
                return;
            }
            auto metric = metricEnums[selectedIndex];

            // 4.3 读取"不支持值"，非法输入则回退到 -1.0
            double unsupportedValue = -1.0;
            bool okValue = false;
            double parsed = dialog->getDouble(unsupportedId, okValue);
            if (okValue) {
                unsupportedValue = parsed;
            } else {
                showDarkFramelessMessage(QStringLiteral("参数提示"),
                                         QStringLiteral("「不支持/无效值」不是合法数字，已回退为默认值 -1.0。"));
            }

            // 4.4 执行 filter
            auto filter = iGame::CellMeshMetricsFilter::New();
            filter->setMetric(metric);
            filter->setUnsupportedValue(unsupportedValue);
            filter->SetInput(obj);
            if (!filter->Execute()) {
                showDarkFramelessMessage(
                        QStringLiteral("计算失败"),
                        QStringLiteral("单元质量评估执行失败，请检查输入是否为可识别的体网格。"));
                dialog->close();
                return;
            }

            // 4.5 输出独立结果节点（不修改原模型）
            auto qualityObj = filter->GetOutput();
            if (qualityObj == nullptr) {
                showDarkFramelessMessage(QStringLiteral("计算失败"), QStringLiteral("未产生有效的输出结果。"));
                dialog->close();
                return;
            }
            qualityObj->SetName(obj->GetName() + "_CellQuality");
            modelTreeWidget->addDataObjectToModelTree(qualityObj, Algorithm);
            rendererWidget->update();

            // 4.6 汇总报告：支持数量 / 跳过数量 / 使用的指标与不支持值
            const int supported = filter->GetSupportedCount();
            const int unsupported = filter->GetUnsupportedCount();
            const int skipped = filter->GetSkippedBlockCount();
            QString report = QStringLiteral("单元质量评估完成。\n\n"
                                            "• 评估指标：%1\n"
                                            "• 成功计算单元：%2 个\n"
                                            "• 跳过不适用单元：%3 个（已标记为 %4）\n"
                                            "• 无法处理的网格块：%5 个\n"
                                            "• 结果已生成为独立节点：%6")
                                     .arg(metricNames[selectedIndex])
                                     .arg(supported)
                                     .arg(unsupported)
                                     .arg(unsupportedValue)
                                     .arg(skipped)
                                     .arg(QString::fromStdString(qualityObj->GetName()));
            if (supported == 0 && unsupported > 0) {
                report += QStringLiteral(
                        "\n\n提示：当前指标对该模型中的所有单元均不适用。\n"
                        "请确认单元类型（四面体/六面体/三角形/四边形），或更换评估指标 ——\n"
                        "不同指标对单元类型的适用范围与 ParaView Cell Quality 一致。");
            }
            showDarkFramelessMessage(QStringLiteral("评估完成"), report, true);
        });
    });
    connect(developingFilters->addAction(QStringLiteral("平面插值采样 (Plane Sampling)")), &QAction::triggered, this,
            [this](bool) {
                auto currentModel = rendererWidget->GetScene()->GetCurrentModel();
                if (!currentModel) {
                    showDarkFramelessMessage(QStringLiteral("提示"), QStringLiteral("请先加载一个模型"));
                    return;
                }

                auto obj = currentModel->GetDataObject();
                if (!obj) {
                    showDarkFramelessMessage(QStringLiteral("提示"), QStringLiteral("当前模型没有有效数据"));
                    return;
                }

                igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(QStringLiteral("平面插值采样"));
                dialog->setFilterDescription(QStringLiteral("在指定平面上生成规则采样点，读取模型在该位置的数值。"));

                // ---- 添加参数控件 ----
                int originXId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                     QStringLiteral("平面原点 X"), "0.0");
                int originYId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                     QStringLiteral("平面原点 Y"), "0.0");
                int originZId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                     QStringLiteral("平面原点 Z"), "0.0");

                int normalXId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                     QStringLiteral("平面法向 X"), "0.0");
                int normalYId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                     QStringLiteral("平面法向 Y"), "0.0");
                int normalZId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                     QStringLiteral("平面法向 Z"), "1.0");

                int resolutionId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                        QStringLiteral("采样分辨率 (NxN)"), "20");

                // ---- 获取模型属性列表 ----
                std::vector<QString> attrNames;
                auto attrSet = obj->GetAttributeSet();
                if (attrSet) {
                    auto allAttrs = attrSet->GetAllAttributes();
                    if (allAttrs && allAttrs->GetNumberOfElements() > 0) {
                        for (igIndex i = 0; i < allAttrs->GetNumberOfElements(); i++) {
                            auto& attr = allAttrs->GetElement(i);
                            if (attr.pointer && !attr.isDeleted) {
                                QString name = QString::fromStdString(attr.pointer->GetName());
                                if (std::find(attrNames.begin(), attrNames.end(), name) == attrNames.end()) {
                                    attrNames.push_back(name);
                                }
                            }
                        }
                    }
                }
                if (attrNames.empty()) { attrNames.push_back(QStringLiteral("无可用属性")); }

                int attrId = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX, QStringLiteral("采样属性"),
                                                  attrNames);

                // ---- 显示对话框 ----
                dialog->show();

                // ---- Apply 回调 ----
                dialog->setApplyFunctor([=, this]() {
                    bool ok;

                    double originX = dialog->getDouble(originXId, ok);
                    if (!ok) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"), QStringLiteral("请输入有效的平面原点 X"));
                        return;
                    }
                    double originY = dialog->getDouble(originYId, ok);
                    if (!ok) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"), QStringLiteral("请输入有效的平面原点 Y"));
                        return;
                    }
                    double originZ = dialog->getDouble(originZId, ok);
                    if (!ok) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"), QStringLiteral("请输入有效的平面原点 Z"));
                        return;
                    }

                    double normalX = dialog->getDouble(normalXId, ok);
                    if (!ok) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"), QStringLiteral("请输入有效的平面法向 X"));
                        return;
                    }
                    double normalY = dialog->getDouble(normalYId, ok);
                    if (!ok) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"), QStringLiteral("请输入有效的平面法向 Y"));
                        return;
                    }
                    double normalZ = dialog->getDouble(normalZId, ok);
                    if (!ok) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"), QStringLiteral("请输入有效的平面法向 Z"));
                        return;
                    }

                    if (std::abs(normalX) < 1e-10 && std::abs(normalY) < 1e-10 && std::abs(normalZ) < 1e-10) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"), QStringLiteral("平面法向不能为零向量"));
                        return;
                    }

                    int resolution = dialog->getInt(resolutionId, ok);
                    if (!ok || resolution < 2) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("采样分辨率必须为大于 1 的整数"));
                        return;
                    }

                    int attrIdx = dialog->getComboIndex(attrId, ok);
                    std::string attrName = "";
                    if (ok && attrIdx >= 0 && attrIdx < (int) attrNames.size()) {
                        QString selectedName = attrNames[attrIdx];
                        if (selectedName != QStringLiteral("无可用属性")) { attrName = selectedName.toStdString(); }
                    }

                    // ---- 执行滤镜 ----
                    auto filter = PlaneSamplingFilter::New();
                    filter->SetInput(obj);

                    double origin[3] = {originX, originY, originZ};
                    double normal[3] = {normalX, normalY, normalZ};
                    filter->SetPlaneOrigin(origin);
                    filter->SetPlaneNormal(normal);
                    filter->SetResolution(resolution);

                    if (!attrName.empty()) { filter->SetAttributeName(attrName); }

                    if (!filter->Execute()) {
                        showDarkFramelessMessage(QStringLiteral("执行失败"),
                                                 QStringLiteral("平面采样执行失败，请检查输入数据"));
                        return;
                    }

                    auto output = filter->GetOutput();
                    if (!output) {
                        showDarkFramelessMessage(QStringLiteral("执行失败"), QStringLiteral("滤镜没有产生输出"));
                        return;
                    }

                    QString outputName = QString::fromStdString(obj->GetName()) + "_plane_sampling";
                    output->SetName(outputName.toStdString());

                    modelTreeWidget->addDataObjectToModelTree(output, ItemSource::Algorithm);

                    // ---- 自动切换到新模型 ----
                    auto scene = rendererWidget->GetScene();
                    auto modelList = scene->GetModelList();
                    for (auto it = modelList->Begin(); it != modelList->End(); ++it) {
                        auto model = it->second;
                        if (model->GetDataObject() == output) {
                            scene->SetCurrentModel(model);
                            break;
                        }
                    }

                    // ---- 自动显示第一个属性 ----
                    auto outputAttrSet = output->GetAttributeSet();
                    if (outputAttrSet && outputAttrSet->GetNumberOfAttributes() > 0) {
                        for (igIndex i = 0; i < outputAttrSet->GetNumberOfAttributes(); i++) {
                            auto& attr = outputAttrSet->GetAttribute(i);
                            if (attr.pointer) {
                                QString name = QString::fromStdString(attr.pointer->GetName());
                                if (name != "vtkValidPointMask") {
                                    auto drawObj = DynamicCast<DrawObject>(output);
                                    if (drawObj) {
                                        drawObj->ViewCloudPicture(scene, i, -1);
                                        modelTreeWidget->updateAllAttriubute(output);
                                        auto item = modelTreeWidget->getItemFromObject(output);
                                        if (item && item->childCount() > i) {
                                            item->setExpanded(true);
                                            auto child = item->child(i);
                                            if (child) {
                                                item->setCurrentChild(child);
                                                item->setSelected(false);
                                                child->setSelected(true);
                                                modelTreeWidget->setCurrentItem(child);
                                            }
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                    }

                    rendererWidget->update();

                    showDarkFramelessMessage(QStringLiteral("采样完成"),
                                             QStringLiteral("生成了 %1 x %2 = %3 个采样点")
                                                     .arg(resolution)
                                                     .arg(resolution)
                                                     .arg(resolution * resolution),
                                             true);

                    dialog->close();
                });
            });

   

    // 第二批开发入口与第一批并列，后续新增 filter 可添加到此菜单。
    QMenu* developingFiltersBatch2 = ui->menu_filters->addMenu(QStringLiteral("新增filter/第二批"));
    developingFiltersBatch2->setObjectName(QStringLiteral("menu_developing_filters_batch2"));

    QAction* outlineAction = developingFiltersBatch2->addAction(QStringLiteral("生成包围盒 (Outline)"));
    outlineAction->setObjectName(QStringLiteral("action_Outline"));
    connect(outlineAction, &QAction::triggered, this, [this](bool) {
        auto scene = rendererWidget->GetScene();
        auto model = scene ? scene->GetCurrentModel() : nullptr;
        if (!model) {
            showDarkFramelessMessage(QStringLiteral("无可用模型"),
                                     QStringLiteral("请先加载并在模型树中选择一个模型。"));
            return;
        }

        auto input = model->GetDataObject();
        if (!input) {
            showDarkFramelessMessage(QStringLiteral("无可用模型"),
                                     QStringLiteral("当前模型没有可用数据。"));
            return;
        }

        auto filter = OutlineFilter::New();
        filter->SetInput(input);
        if (!filter->Execute()) {
            showDarkFramelessMessage(QStringLiteral("生成包围盒失败"),
                                     QString::fromStdString(filter->GetMessage()));
            return;
        }

        auto output = DynamicCast<DrawObject>(filter->GetOutput());
        if (!output) {
            showDarkFramelessMessage(QStringLiteral("生成包围盒失败"),
                                     QStringLiteral("包围盒输出为空或无法显示。"));
            return;
        }

        // 将白色包围盒作为独立结果加入模型树
        output->SetName(QStringLiteral("Outline_%1").arg(++m_outlineCount).toStdString());
        output->SetViewStyle(IG_WIREFRAME);
        output->SetLineColor(igm::vec3(1.0f, 1.0f, 1.0f));
        output->SetLineWidth(2.0f);
        modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
        rendererWidget->update();
    });

     // 继承语义：首次执行新增模型树节点，再次执行更新结果节点
    QAction* linearExtrusion = developingFiltersBatch2->addAction(QStringLiteral("线性拉伸 (Linear Extrusion)"));
    connect(linearExtrusion, &QAction::triggered, this, [this](bool checked) {
        auto scene = rendererWidget->GetScene();
        if (scene == nullptr || scene->GetCurrentModel() == nullptr) {
            showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("请先选择一个模型。"));
            return;
        }
        auto data = scene->GetCurrentModel()->GetDataObject();
        if (data == nullptr) {
            showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("当前模型没有数据。"));
            return;
        }
        if (m_linearExtrusionDialog == nullptr) {
            // 首次打开时创建独立弹窗：非模态，用户点 X 才关闭；
            // 面板内容仍是 igQtLinearExtrusionWidget，只是宿主从左侧工具面板换成独立窗口
            m_linearExtrusionDialog = new QDialog(this);
            m_linearExtrusionDialog->setWindowTitle(QStringLiteral("线性拉伸"));
            // 面板配色只作用于本弹窗：黑底白字、边框可见、字号比全局 12pt 更小。
            // 复选框必须连 ::indicator 一起显式定义：一旦给 QCheckBox 设了样式表，Qt 就不再画系统
            // 原生指示器；而全局样式里的对勾图片路径写错了（qrc 前缀是 /Ticon，样式里却写成 :/icons/…，
            // 大小写也不对），所以 :checked 只剩底色，看上去是个纯蓝色方块、分不出选中与否。
            // image 必须给「文件路径 / :/资源路径」，Qt 样式表不接受 data: URI（内联 SVG 试过，不生效）；
            // 也不能复用 Icons/check.png：它描边是 #1296DB，压在本面板蓝底 #0E639C 上对比度只有约 1.95，
            // 14px 下看不出对勾，因此新增白色版 Icons/check_white.svg（对比度约 6.4）。
            const QString panelStyle = QString::fromUtf8(
                    "QDialog { background-color: #1E1E1E; }"
                    "QLabel { color: #FFFFFF; font-size: 11px; }"
                    "QLabel:disabled { color: #6A6A6A; }"
                    "QGroupBox { color: #FFFFFF; border: 1px solid #3C3C3C; border-radius: 4px; padding: 4px; }"
                    "QGroupBox:disabled { color: #6A6A6A; }"
                    "QCheckBox { color: #FFFFFF; font-size: 11px; spacing: 6px; }"
                    "QCheckBox::indicator { width: 14px; height: 14px; border: 1px solid #6A6A6A;"
                    " border-radius: 2px; background-color: #2A2A2A; }"
                    "QCheckBox::indicator:unchecked { background-color: #2A2A2A; }"
                    "QCheckBox::indicator:checked { background-color: #0E639C; border: 1px solid #1F8AD2;"
                    " image: url(:/Ticon/Icons/check_white.svg); }"
                    "QCheckBox::indicator:disabled { border-color: #3C3C3C; background-color: #232323; }"
                    "QComboBox, QLineEdit { background-color: #2A2A2A; color: #FFFFFF; border: 1px solid #4A4A4A;"
                    " border-radius: 3px; padding: 2px 6px; min-height: 18px; font-size: 11px; }"
                    "QComboBox:disabled, QLineEdit:disabled { color: #6A6A6A; }"
                    "QComboBox QAbstractItemView { background-color: #2A2A2A; color: #FFFFFF;"
                    " selection-background-color: #007399; }"
                    "QPushButton { background-color: #2D2D30; color: #FFFFFF; border: 1px solid #4A4A4A;"
                    " border-radius: 3px; padding: 3px 12px; min-height: 18px; font-size: 11px; }"
                    "QPushButton:hover { background-color: #3A3A3D; }");
            m_linearExtrusionDialog->setStyleSheet(panelStyle);
            auto* layout = new QVBoxLayout(m_linearExtrusionDialog);
            layout->setContentsMargins(0, 0, 0, 0);
            // 不锁定弹窗尺寸：宽度与高度都交给用户自由拖动
            layout->setSizeConstraint(QLayout::SetDefaultConstraint);
            m_linearExtrusionWidget = new igQtLinearExtrusionWidget(m_linearExtrusionDialog);
            m_linearExtrusionWidget->setMinimumSize(0, 0);
            layout->addWidget(m_linearExtrusionWidget);
            m_linearExtrusionDialog->setMinimumSize(0, 0);
            m_linearExtrusionDialog->resize(360, 380);

            connect(m_linearExtrusionWidget, &igQtLinearExtrusionWidget::DrawLinearExtrusionModel, this,
                    [this](iGame::DataObject::Pointer res) {
                        modelTreeWidget->addDataObjectToModelTree(res, ItemSource::Algorithm);
                    });
            connect(m_linearExtrusionWidget, &igQtLinearExtrusionWidget::UpdateLinearExtrusionModel, this,
                    [this](iGame::DataObject::Pointer res) {
                        modelTreeWidget->updateCurrentModelInfo();
                        rendererWidget->update();
                    });
            connect(m_linearExtrusionWidget, &igQtLinearExtrusionWidget::ApplyFailed, this,
                    [this](const QString& message) { showDarkFramelessMessage(QStringLiteral("Warning"), message); });
        }
        // 每次打开都按当前模型刷新输入数据
        m_linearExtrusionWidget->SetOriginDataObject(data);
        m_linearExtrusionDialog->show();
        m_linearExtrusionDialog->raise();
        m_linearExtrusionDialog->activateWindow();
    });

    // ===== 封闭点提取 (Extract Enclosed Points) =====
    QAction* extractEnclosedPoints =
            developingFiltersBatch2->addAction(QStringLiteral("封闭点提取 (Extract Enclosed Points)"));
    connect(extractEnclosedPoints, &QAction::triggered, this, [this](bool) {
        auto scene = rendererWidget->GetScene();
        if (!scene) return;

        // ---- 收集 Surface 候选（SurfaceMesh 派生）----
        std::vector<QString> surfaceNames;
        std::vector<DataObject::Pointer> surfaceObjs;
        // ---- 收集 Input 候选（所有 PointSet 派生）----
        std::vector<QString> inputNames;
        std::vector<DataObject::Pointer> inputObjs;

        // Input 列表第一项固定是“手工构造”
        inputNames.push_back(QStringLiteral("【手工构造随机点云】"));
        inputObjs.push_back(nullptr);

        for (auto it = scene->GetModelList()->Begin(); it != scene->GetModelList()->End(); ++it) {
            auto m = it->second;
            if (!m) continue;
            auto obj = m->GetDataObject();
            if (!obj) continue;

            if (!DynamicCast<PointSet>(obj).IsNull()) {
                inputNames.push_back(QString::fromStdString(obj->GetName()));
                inputObjs.push_back(obj);
            }
            if (!DynamicCast<SurfaceMesh>(obj).IsNull() && DynamicCast<SurfaceMesh>(obj)->GetNumberOfFaces() > 0) {
                surfaceNames.push_back(QString::fromStdString(obj->GetName()));
                surfaceObjs.push_back(obj);
            }
        }

        if (surfaceNames.empty()) {
            showDarkFramelessMessage(QStringLiteral("封闭点提取"),
                                     QStringLiteral("场景中没有可用的表面网格 (SurfaceMesh)。\n"
                                                    "请先加载一个封闭表面模型，"
                                                    "或先用“算法处理 → 数据处理 → 表面提取”生成表面网格。"));
            return;
        }

        int defaultSurfaceIdx = 0;
        auto currentModel = scene->GetCurrentModel();
        auto currentObj = currentModel ? currentModel->GetDataObject() : nullptr;
        if (currentObj) {
            for (size_t i = 0; i < surfaceObjs.size(); ++i) {
                if (surfaceObjs[i] == currentObj) {
                    defaultSurfaceIdx = (int) i;
                    break;
                }
            }
        }

        auto* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("封闭点提取 (Extract Enclosed Points)"));
        dialog->setFilterDescription(QStringLiteral("判定一组点是否位于封闭表面内部，输出内部的点。\n"
                                                    "•Input：待判定的点集（可选“手工构造随机点云”）\n"
                                                    "•Surface：封闭流形表面（必须是SurfaceMesh）\n"
                                                    "两个输入可以相同，也可以不同。"));

        const int inputModelId = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX,
                                                      QStringLiteral("Input（待判定点集）"), inputNames);
        const int surfaceModelId = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX,
                                                        QStringLiteral("Surface（封闭表面）"), surfaceNames);
        const int checkClosedId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                                       QStringLiteral("检查表面封闭性"), "true");
        const int insideOutId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                                     QStringLiteral("输出外部点 (InsideOut)"), "false");
        const int toleranceId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                     QStringLiteral("容差 Tolerance"), "0.001");

        if (auto* sc = qobject_cast<QComboBox*>(dialog->getWidget(surfaceModelId))) {
            sc->setCurrentIndex(defaultSurfaceIdx);
        }

        dialog->show();

        dialog->setApplyFunctor([=, this]() {
            bool ok = false;
            const int inIdx = dialog->getComboIndex(inputModelId, ok);
            const int surIdx = dialog->getComboIndex(surfaceModelId, ok);
            if (!ok || inIdx < 0 || inIdx >= (int) inputObjs.size() || surIdx < 0 ||
                surIdx >= (int) surfaceObjs.size()) {
                showDarkFramelessMessage(QStringLiteral("参数错误"), QStringLiteral("请选择有效的 Input 和 Surface。"));
                return;
            }
            const bool checkClosed = dialog->getChecked(checkClosedId, ok);
            const bool insideOut = dialog->getChecked(insideOutId, ok);
            const double tol = dialog->getDouble(toleranceId, ok);
            if (!ok || tol < 0.0) {
                showDarkFramelessMessage(QStringLiteral("参数错误"), QStringLiteral("容差必须是非负数。"));
                return;
            }

            auto surface = DynamicCast<SurfaceMesh>(surfaceObjs[surIdx]);
            if (surface.IsNull() || surface->GetNumberOfFaces() == 0) {
                showDarkFramelessMessage(QStringLiteral("参数错误"),
                                         QStringLiteral("Surface 必须是带面片的表面网格。"));
                return;
            }

            auto runFilter = [this, surface, checkClosed, insideOut, tol](DataObject::Pointer finalInput) {
                if (!finalInput) return;

                auto filter = ExtractEnclosedPointsFilter::New();
                filter->SetInput(0, finalInput);
                filter->SetInput(1, surface);
                filter->SetCheckSurface(checkClosed);
                filter->SetInsideOut(insideOut);
                filter->SetTolerance(tol);

                if (!filter->Execute()) {
                    showDarkFramelessMessage(QStringLiteral("执行失败"),
                                             QStringLiteral("封闭点提取执行失败，请检查输入。"));
                    return;
                }
                auto output = filter->GetOutput();
                if (!output) {
                    showDarkFramelessMessage(QStringLiteral("执行失败"), QStringLiteral("未产生有效输出。"));
                    return;
                }

                auto inPS = DynamicCast<PointSet>(finalInput);
                auto outPS = DynamicCast<PointSet>(output);
                const IGsize inCount = inPS ? inPS->GetNumberOfPoints() : 0;
                const IGsize outCount = outPS ? outPS->GetNumberOfPoints() : 0;

                qDebug() << "[ExtractEnclosedPoints] 封闭点提取完成\n"
                         << "  Input        :" << QString::fromStdString(finalInput->GetName()) << "\n"
                         << "  Surface      :" << QString::fromStdString(surface->GetName()) << "\n"
                         << "  InputPoints  :" << inCount << "\n"
                         << "  OutputPoints :" << outCount << "\n"
                         << "  InsideOut    :" << (insideOut ? "true" : "false") << "\n"
                         << "  Tolerance    :" << tol << "\n"
                         << "  CheckSurface :" << (checkClosed ? "true" : "false");

                output->SetName(finalInput->GetName() + "_enclosed");
                modelTreeWidget->addDataObjectToModelTree(output, Algorithm);

                if (auto drawObj = DynamicCast<DrawObject>(output)) {
                    drawObj->ConvertToDrawableData();
                    drawObj->SetViewStyle(IG_POINTS);
                    drawObj->SetPointSize(4.0f);
                    drawObj->SetVisibility(true);
                }

                rendererWidget->update();
            };

            const bool isManual = (inputObjs[inIdx] == nullptr);
            if (!isManual) {
                runFilter(inputObjs[inIdx]);
                dialog->deleteLater();
                return;
            }

            auto* manualDialog = new igQtFilterDialogDockWidget(this, true);
            manualDialog->setFilterTitle(QStringLiteral("手工构造随机点云"));
            manualDialog->setFilterDescription(
                    QStringLiteral("在 Surface 包围盒的基础上，扩展一定倍数后均匀随机采样。\n"
                                   "生成的随机点云会加入场景，便于与结果对比。"));

            const int sampleCountId = manualDialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                                 QStringLiteral("采样点数"), "2000");
            const int expandFactorId = manualDialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                                  QStringLiteral("包围盒扩展倍数"), "1.5");
            const int addToSceneId = manualDialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                                                QStringLiteral("加入场景以对比"), "true");
            const int seedId = manualDialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                          QStringLiteral("随机种子"), "42");

            manualDialog->show();

            manualDialog->setApplyFunctor([=, this]() {
                bool ok2 = false;
                const int sampleCount = manualDialog->getInt(sampleCountId, ok2);
                const double expand = manualDialog->getDouble(expandFactorId, ok2);
                const bool addToScene = manualDialog->getChecked(addToSceneId, ok2);
                const int seed = manualDialog->getInt(seedId, ok2);
                if (!ok2 || sampleCount <= 0 || expand <= 0.0) {
                    showDarkFramelessMessage(QStringLiteral("参数错误"),
                                             QStringLiteral("采样点数必须是正整数，扩展倍数必须是正数。"));
                    return;
                }

                auto cloud = PointSet::New();
                auto pts = cloud->GetPoints();
                std::mt19937 rng(static_cast<unsigned int>(seed));
                const auto& bbox = surface->GetBoundingBox();
                const auto center = (bbox.min + bbox.max) * 0.5;
                const auto half = (bbox.max - bbox.min) * (expand * 0.5);
                std::uniform_real_distribution<float> rx(static_cast<float>(center[0] - half[0]),
                                                         static_cast<float>(center[0] + half[0]));
                std::uniform_real_distribution<float> ry(static_cast<float>(center[1] - half[1]),
                                                         static_cast<float>(center[1] + half[1]));
                std::uniform_real_distribution<float> rz(static_cast<float>(center[2] - half[2]),
                                                         static_cast<float>(center[2] + half[2]));
                pts->Reserve(sampleCount);
                for (int i = 0; i < sampleCount; ++i) { pts->AddPoint(Point(rx(rng), ry(rng), rz(rng))); }
                cloud->SetName("RandomCloud");

                if (addToScene) {
                    if (auto drawCloud = DynamicCast<DrawObject>(cloud)) {
                        drawCloud->ConvertToDrawableData();
                        drawCloud->SetViewStyle(IG_POINTS);
                        drawCloud->SetPointSize(3.0f);
                        drawCloud->SetVisibility(true);
                    }
                    modelTreeWidget->addDataObjectToModelTree(cloud, Algorithm);
                }

                runFilter(cloud);

                manualDialog->deleteLater();
                dialog->deleteLater();
            });
    });
    });

    auto addDevelopmentAction = [this](QMenu* menu, QAction* approved, const QString& label) {
        if (!approved) return;
        QAction* action = menu->addAction(label);
        action->setObjectName(QStringLiteral("action_developing_%1").arg(menu->actions().size()));
        action->setData(approved->data());
        connect(action, &QAction::triggered, this, [approved]() { approved->trigger(); });
    };
    auto approvedFilter = [standardFilters](const char* id) -> QAction* {
        return standardFilters->findChild<QAction*>(QStringLiteral("action_filter_%1")
                .arg(QString::fromLatin1(id)));
    };
    auto approvedMenuAction = [](QMenu* menu, const QString& label) -> QAction* {
        for (QAction* action : menu->actions()) {
            if (action->text() == label) return action;
        }
        return nullptr;
    };
    struct DevelopmentEntry { const char* id; const char* label; };
    static const DevelopmentEntry directEntries[] = {
        {"iso_volume", "等值面体提取 (IsoVolume)"},
        {"extract_cells_by_region", "按区域提取单元 (Extract Cells By Region)"},
        {"threshold", "阈值 (Threshold)"},
        {"generate_ids", "生成ID (GenerateIds)"},
        {"clean_to_grid", "网格清理 (Clean to Grid)"},
        {"feature_edges", "特征边提取 (Feature Edges)"},
        {"extract_subset", "提取子集 (Extract Subset)"},
        {"point_and_cell_ids", "生成点与单元ID (Point And Cell IDs)"},
        {"validate_cells", "单元几何校验 (Validate Cells)"},
        {"surface_normals", "面/点法向量计算 (Surface Normals)"},
        {"extract_location", "提取指定位置数据 (Extract Location)"},
        {"remove_ghost_information", "移除Ghost信息 (Remove Ghost Information)"},
        {"shrink", "单元收缩 (Shrink)"},
        {"overlapping_cells_detector", "检测重叠单元 (Overlapping Cells Detector)"},
        {"ghost_cells", "Ghost 单元标记 (Ghost Cells)"},
        {"mask_points", "点抽样（Mask Points）"},
        {"cell_centers", "单元几何中心 (Cell Center)"},
        {"point_line_interpolator", "点线插值 (Point Line Interpolator)"},
        {"process_ids", "生成进程ID (GenerateProcessIds)"},
        {"coordinates", "提取点坐标 (Extract Point Coordinates)"},
        {"extract_cells_by_type", "按单元类型提取 (Extract Cells By Type)"},
        {"axis_aligned_reflection", "反射 (Axis Aligned Reflection)"},
        {"convert_to_vertex", "转换为顶点单元 (Convert To Vertex)"},
        {"mesh_quality", "网格质量评估 (MeshQuality)"},
        {"extract_component", "提取分量 (Extract Component)"},
        {"transform", "通用几何变换 (Transform)"},
        {"cell_size", "计算单元尺寸 (ComputeCellSize)"},
        {"boundary_mesh_quality", "边界网格质量 (Boundary Mesh Quality)"},
        {"multiblock_surface_as_multiblock", "多块模型表面提取 (Extract Surface)"},
        {"feature_edges_region_ids", "特征区域Id (FeatureEdgeRegion id)"},
        {"append_location_attributes", "附加点坐标到属性(AppendLocaitonAttribute)"},
        {"pass_arrays", "传递过滤数据数组 (Pass Arrays)"},
        {"volume_of_revolution", "旋转体生成 (Volume of Revolution)"},
        {"point_volume_interpolator", "点体积插值 (Point Volume Interpolator)"},
        {"angular_periodic", "角度周期复制 (Angular Periodic)"},
        {"resample_to_line", "重采样至直线(ResampleToLine)"},
        {"resample_with_dataset", "重采样至数据集 (Resample With DataSet)"},
        {"rename_arrays", "重命名数组 (Rename Arrays)"},
        {"point_set_to_octree_image", "点集转八叉树图像 (PointSetToOctree)"},
        {"resample_to_image", "重采样到图像 (ResampleToImage)"},
        {"random_attributes", "随机属性生成 (Random Attributes)"},
        {"deflect_normals", "法向偏转 (Deflect Normals)"},
        {"probe", "探测 (probe)"},
        {"extract_edges", "边提取 (ExtractEdges)"},
        {"count_cell_vertices", "统计单元顶点数 (CountCellVertices)"},
    };
    for (const DevelopmentEntry& entry : directEntries) {
        addDevelopmentAction(developingFilters, approvedFilter(entry.id),
                             QString::fromUtf8(entry.label));
        if (!developingFilters->actions().isEmpty()) {
            developingFilters->actions().last()->setData(QString::fromLatin1(entry.id));
        }
    }

    static const DevelopmentEntry directEntries_2[] = {
        {"resample_with_dataset", "重采样至数据集 (Resample With DataSet)"},
        {"rename_arrays", "重命名数组 (Rename Arrays)"},
    };
    for (const DevelopmentEntry& entry : directEntries_2) {
        addDevelopmentAction(developingFiltersBatch2, approvedFilter(entry.id),
            QString::fromUtf8(entry.label));
        if (!developingFiltersBatch2->actions().isEmpty()) {
            developingFiltersBatch2->actions().last()->setData(QString::fromLatin1(entry.id));
        }
    }

    auto bindDevelopmentAction = [this, developingFilters](const char* id, std::function<void()> run) {
        const QString filterId = QString::fromLatin1(id);
        for (QAction* action : developingFilters->findChildren<QAction*>()) {
            if (action->data().toString() != filterId) continue;
            QObject::disconnect(action, nullptr, this, nullptr);
            connect(action, &QAction::triggered, this, [run]() { run(); });
            return;
        }
    };
    auto executeDevelopmentFilter = [this, currentFilterInput, refreshFilterResult](
            const QString& title, auto filter) {
        auto input = currentFilterInput(title);
        if (!input) return;
        filter->SetInput(input);
        if (!filter->Execute()) {
            showDarkFramelessMessage(title, QStringLiteral("Filter 执行失败，请检查输入模型与参数。"));
            return;
        }
        refreshFilterResult(input, filter->GetOutput(), title);
    };
    bindDevelopmentAction("validate_cells", [this]() {
                auto scene = rendererWidget->GetScene();
                if (scene == nullptr || scene->GetCurrentModel() == nullptr) {
                    showDarkFramelessMessage(QStringLiteral("无可用模型"),
                                             QStringLiteral("请先加载并选择一个网格模型。"));
                    return;
                }
                const auto model = scene->GetCurrentModel();
                const auto sourceModelId = scene->GetCurrentModelID();
                const auto dataObject = model == nullptr ? nullptr : model->GetDataObject();
                if (model == nullptr || dataObject == nullptr ||
                    (DynamicCast<UnstructuredMesh>(dataObject).IsNull() &&
                     DynamicCast<SurfaceMesh>(dataObject).IsNull())) {
                    showDarkFramelessMessage(
                            QStringLiteral("单元几何校验"),
                            QStringLiteral("请先在模型树中选择非结构网格或表面网格。"));
                    return;
                }

                auto filter = ValidateCellsFilter::New();
                filter->SetInput(dataObject);
                if (!filter->Execute()) {
                    const auto& error = filter->GetLastError();
                    showDarkFramelessMessage(
                            QStringLiteral("单元几何校验"),
                            QStringLiteral("执行失败：%1")
                                    .arg(error.empty() ? QStringLiteral("未知错误。")
                                                       : QString::fromStdString(error)));
                    return;
                }

                // Filter 返回独立网格（含 ValidityState 单元标量）；原模型保持不变。
                auto data = filter->GetOutput();
                if (data == nullptr) {
                    showDarkFramelessMessage(QStringLiteral("单元几何校验"),
                                             QStringLiteral("执行失败：未生成有效输出。"));
                    return;
                }

                const auto outputModelId = modelTreeWidget->addDataObjectToModelTree(
                        data, ItemSource::Algorithm);
                auto outputModel = scene->GetModelById(outputModelId);
                const int attributeIndex = data->GetAttributeSet()->GetAttributeIndex(
                        ValidateCellsFilter::ValidityStateArrayName());
                auto item = modelTreeWidget->getItemFromObject(data);
                if (item != nullptr && attributeIndex >= 0 &&
                    attributeIndex < item->childCount()) {
                    item->setExpanded(true);
                    auto* child = item->child(attributeIndex);
                    item->setCurrentChild(child);
                    item->setSelected(false);
                    item->viewAttribute(attributeIndex, 0);
                    child->setSelected(true);
                    modelTreeWidget->setCurrentItem(child);
                }

                const auto& invalidIds = filter->GetInvalidCellIds();
                auto selection = outputModel == nullptr ? nullptr
                                                        : outputModel->GetSelection();
                if (selection != nullptr) {
                    selection->Reset();
                    if (!invalidIds.empty()) {
                        selection->SelectionCallBackEvent(
                                IG_CELL, invalidIds, Selection::Operate::Add);
                        selection->SetSelectItemVisable(true);
                    }
                }

                if (outputModel != nullptr) {
                    model->SetVisibility(false);
                    outputModel->SetVisibility(true);
                    scene->SetCurrentModel(outputModel);

                    // 结果与输入节点互斥显示：选择输入时恢复原始外观，
                    // 选择结果时显示 ValidityState 标量和无效单元高亮。
                    const bool hasInvalid = !invalidIds.empty();
                    connect(modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged, this,
                            [this, scene, sourceModelId, outputModelId, hasInvalid]() {
                                auto sourceModel = scene->GetModelById(sourceModelId);
                                auto resultModel = scene->GetModelById(outputModelId);
                                if (resultModel == nullptr) return;

                                const auto currentModelId = scene->GetCurrentModelID();
                                const bool resultSelected = currentModelId == outputModelId;
                                resultModel->SetVisibility(resultSelected);
                                if (auto resultSelection = resultModel->GetSelection())
                                    resultSelection->SetSelectItemVisable(
                                            resultSelected && hasInvalid);

                                if (sourceModel != nullptr &&
                                    (currentModelId == sourceModelId || resultSelected))
                                    sourceModel->SetVisibility(!resultSelected);
                                rendererWidget->update();
                            });
                }
                rendererWidget->update();

                // ValidityState 取值分布，例如 “0:7400, 16:50, 48:22”。
                std::map<short, int> stateDistribution;
                for (short state : filter->GetValidityStates()) {
                    stateDistribution[state]++;
                }
                QStringList distributionText;
                for (const auto& entry : stateDistribution) {
                    distributionText.push_back(
                            QStringLiteral("%1:%2").arg(entry.first).arg(entry.second));
                }

                showDarkFramelessMessage(
                        QStringLiteral("单元几何校验"),
                        QStringLiteral("校验完成：共 %1 个单元，有效 %2，无效 %3。<br>"
                                       "ValidityState 分布：%4")
                                .arg(filter->GetCheckedCellCount())
                                .arg(filter->GetValidCellCount())
                                .arg(filter->GetInvalidCellCount())
                                .arg(distributionText.join(QStringLiteral(", "))));
    });
    bindDevelopmentAction("overlapping_cells_detector", [this]() {
        auto scene = rendererWidget->GetScene();
        if (scene == nullptr || scene->GetCurrentModel() == nullptr) return;

        const auto model = scene->GetCurrentModel();
        const auto sourceModelId = scene->GetCurrentModelID();
        const auto dataObject = model == nullptr ? nullptr : model->GetDataObject();
        if (model == nullptr || dataObject == nullptr ||
            (DynamicCast<UnstructuredMesh>(dataObject).IsNull() && DynamicCast<VolumeMesh>(dataObject).IsNull() && DynamicCast<SurfaceMesh>(dataObject).IsNull())) {
            showDarkFramelessMessage(
                    QStringLiteral("检测重叠单元"),
                    QStringLiteral("请选择三角形或平面凸四边形表面网格，或包含线性体单元的网格。"));
            return;
        }

        auto* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("检测重叠单元"));
        dialog->setFilterDescription(QStringLiteral(
                "检测三角形、平面凸四边形的内部相交，或四面体、六面体、三棱柱、金字塔的共同体积。<br>"
                "执行后生成单元标量 <b>NumberOfOverlapsPerCell</b>；值大于 0 的单元以选中边线高亮显示。<br>"
                "正常邻接不计为重叠；凹或扭曲四边形、高阶、多面体、复合数据及二维/三维混合网格暂不支持。"));
        const int toleranceId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                     QStringLiteral("公差 (Tolerance)"), "0.0");
        dialog->show();
        dialog->setApplyFunctor([this, dialog, model, scene, sourceModelId, toleranceId]() {
            bool toleranceOk = false;
            const double tolerance = dialog->getDouble(toleranceId, toleranceOk);
            if (!toleranceOk || tolerance < 0.0) {
                showDarkFramelessMessage(QStringLiteral("检测重叠单元"), QStringLiteral("公差必须是非负数。"));
                return;
            }
            if (model == nullptr) return;

            auto filter = OverlappingCellsDetectorFilter::New();
            filter->SetInput(model->GetDataObject());
            filter->SetTolerance(tolerance);
            if (!filter->Execute()) {
                const auto& error = filter->GetLastError();
                showDarkFramelessMessage(
                        QStringLiteral("检测重叠单元"),
                        QStringLiteral("执行失败：%1")
                                .arg(error.empty() ? QStringLiteral("未知错误。") : QString::fromStdString(error)));
                return;
            }

            // Filter 返回独立网格；原模型及其属性保持不变。
            auto data = filter->GetOutput();
            if (data == nullptr) {
                showDarkFramelessMessage(QStringLiteral("检测重叠单元"),
                                         QStringLiteral("执行失败：未生成有效输出。"));
                return;
            }
            const auto outputModelId = modelTreeWidget->addDataObjectToModelTree(
                    data, ItemSource::Algorithm);
            auto outputModel = scene->GetModelById(outputModelId);
            const int attributeIndex = data->GetAttributeSet()->GetAttributeIndex(
                    OverlappingCellsDetectorFilter::NumberOfOverlapsPerCellArrayName());
            auto item = modelTreeWidget->getItemFromObject(data);
            if (item != nullptr && attributeIndex >= 0 && attributeIndex < item->childCount()) {
                item->setExpanded(true);
                auto* child = item->child(attributeIndex);
                item->setCurrentChild(child);
                item->setSelected(false);
                item->viewAttribute(attributeIndex, 0);
                child->setSelected(true);
                modelTreeWidget->setCurrentItem(child);
            }

            std::vector<igIndex> overlappingCellIds;
            const auto& overlapCounts = filter->GetNumberOfOverlapsPerCell();
            for (igIndex cellId = 0; cellId < overlapCounts.size(); ++cellId) {
                if (overlapCounts[cellId] > 0) overlappingCellIds.push_back(cellId);
            }
            QStringList overlapCountPreview;
            constexpr int kPreviewCount = 20;
            for (int cellId = 0; cellId < static_cast<int>(overlapCounts.size()) && cellId < kPreviewCount; ++cellId) {
                overlapCountPreview.push_back(QString::number(overlapCounts[cellId]));
            }
            const QString countText =
                    overlapCounts.size() <= kPreviewCount
                            ? QStringLiteral("[%1]").arg(overlapCountPreview.join(QStringLiteral(", ")))
                            : QStringLiteral("[%1, ...]").arg(overlapCountPreview.join(QStringLiteral(", ")));
            auto selection = outputModel == nullptr ? nullptr : outputModel->GetSelection();
            if (selection != nullptr) {
                selection->Reset();
                if (!overlappingCellIds.empty()) {
                    selection->SelectionCallBackEvent(IG_CELL, overlappingCellIds, Selection::Operate::Add);
                    selection->SetSelectItemVisable(true);
                }
            }
            if (outputModel != nullptr) {
                model->SetVisibility(false);
                outputModel->SetVisibility(true);
                scene->SetCurrentModel(outputModel);

                // 该结果与输入节点采用互斥显示：选择输入时恢复原始外观，
                // 选择结果时显示标量和重叠高亮，避免结果轮廓覆盖原模型。
                const bool hasOverlaps = !overlappingCellIds.empty();
                connect(modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged, this,
                        [this, scene, sourceModelId, outputModelId, hasOverlaps]() {
                            auto sourceModel = scene->GetModelById(sourceModelId);
                            auto resultModel = scene->GetModelById(outputModelId);
                            if (resultModel == nullptr) return;

                            const auto currentModelId = scene->GetCurrentModelID();
                            const bool resultSelected = currentModelId == outputModelId;
                            resultModel->SetVisibility(resultSelected);
                            if (auto resultSelection = resultModel->GetSelection())
                                resultSelection->SetSelectItemVisable(resultSelected && hasOverlaps);

                            if (sourceModel != nullptr &&
                                (currentModelId == sourceModelId || resultSelected))
                                sourceModel->SetVisibility(!resultSelected);
                            rendererWidget->update();
                        });
            }
            rendererWidget->update();

            showDarkFramelessMessage(QStringLiteral("检测重叠单元"),
                                     QStringLiteral("检测完成：发现 %1 对重叠单元；高亮 %2 个单元。<br>"
                                                    "NumberOfOverlapsPerCell = %3")
                                             .arg(static_cast<qulonglong>(filter->GetOverlappingCellPairs().size()))
                                             .arg(static_cast<qulonglong>(overlappingCellIds.size()))
                                             .arg(countText));
            dialog->close();
        });
    });


    bindDevelopmentAction("ghost_cells", [executeDevelopmentFilter]() {
        executeDevelopmentFilter(QStringLiteral("Ghost 单元标记"), GhostCellFilter::New());
    });
    bindDevelopmentAction("cell_mesh_metrics", [executeDevelopmentFilter]() {
        executeDevelopmentFilter(QStringLiteral("单元网格指标"), CellMeshMetricsFilter::New());
    });
    // 多块表面提取：升级自分支增强逻辑（输入校验 + 失败块汇总），入口复用开发菜单中的既有多块项。
    bindDevelopmentAction("multiblock_surface_as_multiblock",
                          [this, currentFilterInput, refreshFilterResult]() {
        const QString title = QStringLiteral("多块模型表面提取");
        auto input = currentFilterInput(title);
        if (!input) return;

        auto multiBlockFilter = MultiBlockGeometryFilter::New();
        multiBlockFilter->SetInput(input);
        if (!multiBlockFilter->Execute()) {
            QString errorMsg = QString::fromStdString(multiBlockFilter->GetMessage());
            if (errorMsg.isEmpty()) { errorMsg = QStringLiteral("多块模型表面提取失败。"); }
            showDarkFramelessMessage(title, errorMsg);
            return;
        }

        auto multiBlockObj = multiBlockFilter->GetOutput();
        if (multiBlockObj) {
            multiBlockObj->SetName(input->GetName() + "_MultiBlockSurface");
        }
        refreshFilterResult(input, multiBlockObj, title);

        // 子构件层面的异常信息汇总展示（避免静默丢块）
        const auto& failedBlocks = multiBlockFilter->GetFailedBlocks();
        if (!failedBlocks.empty()) {
            QString warningMsg = QStringLiteral("多块表面提取已完成，但以下 %1 个子构件处理异常（已为您原样保留）：\n")
                                     .arg(failedBlocks.size());
            for (const auto& block : failedBlocks) {
                warningMsg += QStringLiteral("• [%1]: %2\n")
                                  .arg(QString::fromStdString(block.path), QString::fromStdString(block.reason));
            }
            showDarkFramelessMessage(QStringLiteral("部分子构件提示"), warningMsg);
        }
    });
    bindDevelopmentAction("axis_aligned_reflection", [this]() {
        auto scene = rendererWidget->GetScene();
        if (!scene || !scene->GetCurrentModel()) {
            showDarkFramelessMessage(QStringLiteral("反射"), QStringLiteral("请先选择一个模型。"));
            return;
        }
        auto input = scene->GetCurrentModel()->GetDataObject();
        if (!DynamicCast<UnstructuredMesh>(input)) {
            showDarkFramelessMessage(QStringLiteral("反射"),
                                     QStringLiteral("当前版本仅支持非结构网格 (UnstructuredMesh)。"));
            return;
        }

        // 首次打开：懒创建右侧 Dock，连接面板的 applyRequested 执行反射
        if (m_axisAlignedReflectionDock == nullptr) {
            m_axisAlignedReflectionDock = igQtAxisAlignedReflectionWidget::createDockWidget(this);
            addDockWidget(Qt::RightDockWidgetArea, m_axisAlignedReflectionDock);
            auto* reflectionWidget = qobject_cast<igQtAxisAlignedReflectionWidget*>(
                    m_axisAlignedReflectionDock->widget());
            connect(reflectionWidget, &igQtAxisAlignedReflectionWidget::applyRequested, this,
                    [this, reflectionWidget]() {
                        if (!m_axisAlignedReflectionFilter) return;
                        m_axisAlignedReflectionFilter->SetPlane(reflectionWidget->plane());
                        m_axisAlignedReflectionFilter->SetCenter(reflectionWidget->center());
                        m_axisAlignedReflectionFilter->SetCopyInput(reflectionWidget->copyInput());
                        m_axisAlignedReflectionFilter->SetFlipAllInputArrays(
                                reflectionWidget->flipAllInputArrays());
                        if (!m_axisAlignedReflectionFilter->Execute()) {
                            showDarkFramelessMessage(QStringLiteral("反射"),
                                                     QStringLiteral("反射执行失败，请检查输入网格和参数。"));
                            return;
                        }
                        auto output = DynamicCast<UnstructuredMesh>(
                                m_axisAlignedReflectionFilter->GetOutput());
                        if (!output) {
                            showDarkFramelessMessage(QStringLiteral("反射"),
                                                     QStringLiteral("反射未生成有效的非结构网格。"));
                            return;
                        }
                        const QString outputName =
                                QStringLiteral("Reflect_%1").arg(m_axisAlignedReflectionCount);
                        output->SetName(outputName.toStdString());
                        if (!m_axisAlignedReflectionModel) {
                            const int id = modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
                            m_axisAlignedReflectionModel =
                                    rendererWidget->GetScene()->GetModelById(id);
                        } else {
                            m_axisAlignedReflectionModel->SetDataObject(output);
                            modelTreeWidget->updateItemName(output);
                            modelTreeWidget->updateAllAttriubute(output);
                            modelTreeWidget->updateCurrentModelInfo();
                        }
                        rendererWidget->update();
                    });
        }

        // 每次打开：为当前模型创建新 filter 并重置面板
        m_axisAlignedReflectionFilter = AxisAlignedReflectionFilter::New();
        m_axisAlignedReflectionFilter->SetInput(input);
        m_axisAlignedReflectionModel = nullptr;
        ++m_axisAlignedReflectionCount;

        auto* reflectionWidget = qobject_cast<igQtAxisAlignedReflectionWidget*>(
                m_axisAlignedReflectionDock->widget());
        reflectionWidget->resetParameters();
        m_axisAlignedReflectionDock->show();
        m_axisAlignedReflectionDock->raise();
        reflectionWidget->setFocus(Qt::OtherFocusReason);
    });
    bindDevelopmentAction("extract_location", [this]() {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (!model) {
            showDarkFramelessMessage(QStringLiteral("提取指定位置数据"),
                                     QStringLiteral("请先在模型树中选择一个网格模型。"));
            return;
        }
        auto* panel = new igQtExtractLocationWidget(rendererWidget, modelTreeWidget, model, this);
        if (!panel->isReady()) {
            showDarkFramelessMessage(QStringLiteral("提取指定位置数据"),
                                     QStringLiteral("当前仅支持非结构网格（UnstructuredMesh）；操作已取消。"));
            panel->deleteLater();
            return;
        }
        panel->show();
        panel->raise();
        panel->activateWindow();
    });

    bindDevelopmentAction("extract_cells_by_region", [=, this]() {
        const QString title = QStringLiteral("按区域提取单元");
        auto input = currentFilterInput(title);
        if (!input) return;
        auto mesh = DynamicCast<UnstructuredMesh>(input);
        if (!mesh) {
            showDarkFramelessMessage(title, QStringLiteral("请选择非结构化网格。"));
            return;
        }
        const auto bounds = mesh->GetBoundingBox();
        const Vector3d center = (bounds.min + bounds.max) * 0.5;
        const Vector3d size = bounds.max - bounds.min;
        const double scale = std::max({size[0], size[1], size[2], 1.e-6});
        auto scene = rendererWidget->GetScene();
        auto interactor = scene ? scene->GetInteractor() : nullptr;
        if (!interactor) return;
        auto previousBox = interactor->GetSpecialInteractor("SelectBox");
        const bool previousHaveBox = iGame::SelectionParameter::Instance().GetHaveBox();
        const bool previousInSelection = iGame::SelectionParameter::Instance().GetInSelection();
        if (auto oldBox = DynamicCast<iGame::BoxStyle>(previousBox)) oldBox->ClearDraw();
        auto number = [](double value) { return QString::number(value, 'g', 12); };

        auto* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(title);
        dialog->setFilterDescription(QStringLiteral(
            "悬停高亮后按住<b>左键</b>拖动。<br>点击执行后提取单元。"));
        dialog->resize(340, 520);
        dialog->enablePreview(false);
        const int shape = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX,
            QStringLiteral("区域"), std::vector<QString>{QStringLiteral("Box"), QStringLiteral("Sphere"), QStringLiteral("Plane")});
        std::array<int, 3> position{}, lengths{}, normal{};
        const QString axes[] = {QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("Z")};
        for (int i = 0; i < 3; ++i)
            position[i] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("中心/原点 ") + axes[i], number(center[i]));
        for (int i = 0; i < 3; ++i)
            lengths[i] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("尺寸 ") + axes[i], number(std::max(size[i] * .5, scale * .16)));
        const int radius = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
            QStringLiteral("球半径"), number(scale * .25));
        for (int i = 0; i < 3; ++i)
            normal[i] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("平面法向 ") + axes[i], i == 0 ? "1" : "0");
        const int strict = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
            QStringLiteral("全部顶点在内"), "true");
        auto* countLabel = new QLabel(QStringLiteral("调整区域后点击执行"), dialog);
        dialog->addRowWidget(countLabel);
        auto* planeButtons = new QWidget(dialog);
        auto* planeLayout = new QHBoxLayout(planeButtons);
        planeLayout->setContentsMargins(0, 0, 0, 0);
        std::array<QPushButton*, 3> axisButtons{};
        for (int i = 0; i < 3; ++i) {
            axisButtons[i] = new QPushButton(axes[i], planeButtons);
            axisButtons[i]->setToolTip(QStringLiteral("法向 ") + axes[i]);
            planeLayout->addWidget(axisButtons[i]);
        }
        auto* flipButton = new QPushButton(QStringLiteral("翻转法向"), planeButtons);
        planeLayout->addWidget(flipButton);
        dialog->addRowWidget(planeButtons);

        struct RegionSession {
            iGame::BoxStyle::Pointer box;
            iGame::RadialStyle::Pointer sphere;
            iGame::RegionPlaneStyle::Pointer plane;
            DataObject::Pointer output;
            bool closed = false;
            int activeMode = -1;
        };
        auto session = std::make_shared<RegionSession>();
        session->box = iGame::BoxStyle::New();
        session->box->Initialize(interactor);
        session->box->SetLeftButtonOnly(true);
        session->sphere = iGame::RadialStyle::New();
        session->sphere->Initialize(interactor);
        session->sphere->SetMoveEndWithStart(true);
        session->sphere->SetShowSphere(true);
        session->sphere->SetCenterTranslationMode(true);
        session->sphere->SetAllowBoundaryDrag(true);
        session->sphere->SetParaViewInteraction(true);
        session->sphere->SetLeftButtonOnly(true);
        session->sphere->SetAllowRegionResize(true, std::max(scale * 1.e-6, 1.e-7));
        session->plane = iGame::RegionPlaneStyle::New();
        session->plane->Initialize(interactor);
        session->plane->SetBounds(bounds);
        QPointer<igQtFilterDialogDockWidget> safeDialog(dialog);
        auto line = [dialog](int id) { return qobject_cast<QLineEdit*>(dialog->getWidget(id)); };
        auto dirty = [=]() {
            if (!session->closed && safeDialog) countLabel->setText(QStringLiteral("参数已修改，点击执行更新结果"));
        };
        auto readVector = [=](const std::array<int, 3>& ids, Vector3d& value) {
            for (int i = 0; i < 3; ++i) {
                bool ok = false;
                value[i] = dialog->getDouble(ids[i], ok);
                if (!ok || !std::isfinite(value[i])) return false;
            }
            return true;
        };
        auto writeVector = [=](const std::array<int, 3>& ids, const iGame::Point& value) {
            for (int i = 0; i < 3; ++i) line(ids[i])->setText(number(value[i]));
        };
        auto updateWidget = [=]() -> bool {
            if (session->closed || !safeDialog) return false;
            bool ok = false;
            const int mode = dialog->getComboIndex(shape, ok);
            Vector3d p, v;
            if (!ok || !readVector(position, p)) return false;
            if (mode == 0) {
                if (!readVector(lengths, v) || v[0] <= 0. || v[1] <= 0. || v[2] <= 0.) return false;
                const auto rotation = session->box->GetBox() ? session->box->GetBox()->GetRotationMatrix() : igm::mat4(1.f);
                session->box->InitBox(iGame::Point(p[0] - v[0] * .5, p[1] - v[1] * .5, p[2] - v[2] * .5),
                                      iGame::Point(p[0] + v[0] * .5, p[1] + v[1] * .5, p[2] + v[2] * .5));
                session->box->GetBox()->SetRotation(rotation);
                session->box->ClearDraw();
                session->box->ToDraw();
            } else if (mode == 1) {
                const double r = dialog->getDouble(radius, ok);
                if (!ok || !std::isfinite(r) || r <= 0.) return false;
                session->sphere->SetPoint(iGame::Point(p[0], p[1], p[2]), iGame::Point(p[0] + r, p[1], p[2]));
            } else if (mode == 2) {
                if (!readVector(normal, v) || !std::isfinite(v.squaredNorm()) || v.squaredNorm() < 1.e-24) return false;
                session->plane->SetPlane(iGame::Point(p[0], p[1], p[2]), iGame::Point(v[0], v[1], v[2]));
            } else return false;
            rendererWidget->update();
            return true;
        };
        auto switchShape = [=]() {
            if (session->closed || !safeDialog) return;
            bool ok = false;
            const int mode = dialog->getComboIndex(shape, ok);
            if (!ok) return;
            const QString gestures[] = {
                QStringLiteral("拖边平移；拖红点缩放；拖面旋转。"),
                QStringLiteral("拖中心或线框平移；拖红点改半径。"),
                QStringLiteral("拖中心平移；拖端点或圆环旋转。<br>拖平面沿法向移动，保留法向负侧。")};
            dialog->setFilterDescription(QStringLiteral("悬停高亮后按住<b>左键</b>拖动。<br>") + gestures[mode] +
                QStringLiteral("<br>空白处旋转模型；点击执行后提取。"));
            session->box->ClearDraw();
            session->sphere->SetShow(false);
            session->plane->SetShow(false);
            interactor->RemoveSepcialInteractor("SelectBox");
            interactor->RemoveSepcialInteractor("ExtractCellsByRegionSphere");
            interactor->RemoveSepcialInteractor("ExtractCellsByRegionPlane");
            iGame::SelectionParameter::Instance().SetHaveBox(mode == 0);
            for (int id : lengths) dialog->setParameterVisible(id, mode == 0);
            dialog->setParameterVisible(radius, mode == 1);
            for (int id : normal) dialog->setParameterVisible(id, mode == 2);
            planeButtons->setVisible(mode == 2);
            if (mode == 0) interactor->_SetSpecialInteractor("SelectBox", session->box);
            else if (mode == 1) interactor->_SetSpecialInteractor("ExtractCellsByRegionSphere", session->sphere);
            else interactor->_SetSpecialInteractor("ExtractCellsByRegionPlane", session->plane);
            session->activeMode = mode;
            updateWidget();
            if (mode == 1) session->sphere->SetShow(true);
            if (mode == 2) session->plane->SetShow(true);
            dirty();
            rendererWidget->update();
        };
        session->box->_SetPointMoveCallBack("ExtractCellsByRegion", [=]() {
            if (session->closed || !safeDialog || !session->box->GetBox()) return;
            writeVector(position, session->box->GetBox()->GetMidPoint());
            writeVector(lengths, session->box->GetBox()->GetLength());
            dirty();
            rendererWidget->update();
        });
        session->sphere->_SetPointMoveCallBack("ExtractCellsByRegion", [=]() {
            if (session->closed || !safeDialog) return;
            writeVector(position, session->sphere->GetStartPoint());
            line(radius)->setText(number((session->sphere->GetEndPoint() - session->sphere->GetStartPoint()).length()));
            dirty();
            rendererWidget->update();
        });
        session->plane->SetChangedCallback([=]() {
            if (session->closed || !safeDialog) return;
            writeVector(position, session->plane->GetOrigin());
            writeVector(normal, session->plane->GetNormal());
            dirty();
            rendererWidget->update();
        });
        dialog->setApplyFunctor([=, this]() {
            if (!updateWidget()) {
                showDarkFramelessMessage(title, QStringLiteral("请填写有效的区域参数：尺寸和半径须大于零，法向须非零。"));
                return;
            }
            auto filter = ExtractCellsByRegionFilter::New();
            filter->SetInput(input);
            if (session->activeMode == 0) {
                const auto box = session->box->GetBox();
                const auto& p = box->GetMidPoint();
                const auto& length = box->GetLength();
                const auto& rotation = box->GetRotationMatrix();
                std::array<Vector3d, 3> boxAxes;
                for (int i = 0; i < 3; ++i) boxAxes[i] = Vector3d(rotation[i][0], rotation[i][1], rotation[i][2]);
                filter->SetOrientedBox(Vector3d(p[0], p[1], p[2]),
                    Vector3d(length[0] * .5, length[1] * .5, length[2] * .5), boxAxes);
            } else if (session->activeMode == 1) {
                const auto& p = session->sphere->GetStartPoint();
                filter->SetSphere(Vector3d(p[0], p[1], p[2]), (session->sphere->GetEndPoint() - p).length());
            } else {
                const auto& p = session->plane->GetOrigin();
                const auto& n = session->plane->GetNormal();
                filter->SetPlane(Vector3d(p[0], p[1], p[2]), Vector3d(n[0], n[1], n[2]));
            }
            bool ok = false;
            filter->SetRequireAllPoints(dialog->getChecked(strict, ok));
            if (!filter->Execute()) { showDarkFramelessMessage(title, QStringLiteral("区域提取失败。")); return; }
            if (filter->GetSelectedCellIds().empty()) {
                countLabel->setText(QStringLiteral("当前区域没有单元；未更新结果。"));
                return;
            }
            auto output = filter->GetOutput();
            output->SetName(input->GetName() + "_ExtractCellsByRegion");
            auto* resultItem = session->output ? modelTreeWidget->getItemFromObject(session->output) : nullptr;
            if (resultItem) {
                auto* resultModel = resultItem->getModel();
                resultModel->SetDataObject(output);
                scene->SetCurrentModel(resultModel);
                modelTreeWidget->updateAllAttriubute(output);
                modelTreeWidget->setCurrentItem(resultItem);
                modelTreeWidget->updateCurrentModelInfo();
                scene->Update();
                rendererWidget->update();
            } else modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
            session->output = output;
            countLabel->setText(QStringLiteral("已提取：%1 个单元").arg(filter->GetSelectedCellIds().size()));
        });
        dialog->setCloseFunctor([=]() {
            session->closed = true;
            session->box->RemovePointMoveCallBack("ExtractCellsByRegion");
            session->sphere->RemovePointMoveCallBack("ExtractCellsByRegion");
            session->plane->SetChangedCallback({});
            session->box->ClearDraw();
            session->sphere->SetShow(false);
            session->plane->SetShow(false);
            interactor->RemoveSepcialInteractor("ExtractCellsByRegionSphere");
            interactor->RemoveSepcialInteractor("ExtractCellsByRegionPlane");
            interactor->RemoveSepcialInteractor("SelectBox");
            if (previousBox) interactor->_SetSpecialInteractor("SelectBox", previousBox);
            iGame::SelectionParameter::Instance().SetHaveBox(previousHaveBox);
            iGame::SelectionParameter::Instance().SetInSelection(previousInSelection);
            if (auto oldBox = DynamicCast<iGame::BoxStyle>(previousBox)) if (oldBox->GetBox()) oldBox->ToDraw();
            rendererWidget->update();
        });
        iGame::SelectionParameter::Instance().SetInSelection(false);
        for (const auto& ids : {position, lengths, normal}) for (int id : ids)
            connect(line(id), &QLineEdit::editingFinished, dialog, [=]() { updateWidget(); dirty(); });
        connect(line(radius), &QLineEdit::editingFinished, dialog, [=]() { updateWidget(); dirty(); });
        connect(qobject_cast<QComboBox*>(dialog->getWidget(shape)), QOverload<int>::of(&QComboBox::currentIndexChanged),
            dialog, [=](int) { switchShape(); });
        connect(qobject_cast<QCheckBox*>(dialog->getWidget(strict)), &QCheckBox::toggled, dialog, [=](bool) { dirty(); });
        for (int axis = 0; axis < 3; ++axis) connect(axisButtons[axis], &QPushButton::clicked, dialog, [=]() {
            for (int i = 0; i < 3; ++i) line(normal[i])->setText(i == axis ? "1" : "0");
            updateWidget(); dirty();
        });
        connect(flipButton, &QPushButton::clicked, dialog, [=]() {
            Vector3d n;
            if (!readVector(normal, n)) return;
            for (int i = 0; i < 3; ++i) line(normal[i])->setText(number(-n[i]));
            updateWidget(); dirty();
        });
        switchShape();
        dialog->show();
        QTimer::singleShot(0, dialog, [this, dialog]() {
            const int x = std::max(8, rendererWidget->width() - dialog->width() - 16);
            dialog->move(rendererWidget->mapToGlobal(QPoint(x, 16)));
        });
        dialog->raise();
        dialog->activateWindow();
    });
    bindDevelopmentAction("extract_cells_by_type", [=, this]() {
        const QString title = QStringLiteral("按单元类型提取");
        auto input = currentFilterInput(title);
        if (!input) return;
        auto filter = ExtractCellsByTypeFilter::New();
        filter->SetInput(input);
        const auto types = filter->GetAvailableCellTypes();
        if (types.empty()) { showDarkFramelessMessage(title, QStringLiteral("当前模型没有可提取的单元类型。")); return; }
        auto* dock = new QDockWidget(title, this);
        dock->setAttribute(Qt::WA_DeleteOnClose);
        dock->setObjectName(QStringLiteral("dockWidget_ExtractCellsByType"));
        auto* panel = new igQtExtractCellsByTypeWidget(dock);
        panel->SetDataObject(input);
        dock->setWidget(panel);
        addDockWidget(Qt::LeftDockWidgetArea, dock);
        panel->onApply = [=, this, resultModel = Model::Pointer{}]() mutable {
            auto selected = panel->GetSelectedCellTypes();
            if (selected.empty()) { showDarkFramelessMessage(title, QStringLiteral("请至少选择一种单元类型。")); return; }
            filter->SetExtractCellTypes(selected);
            if (!filter->Execute()) { showDarkFramelessMessage(title, QStringLiteral("按类型提取失败。")); return; }
            auto output = filter->GetOutput();
            if (!resultModel) {
                const int id = modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
                resultModel = rendererWidget->GetScene()->GetModelById(id);
            } else {
                resultModel->SetDataObject(output);
                modelTreeWidget->updateItemName(output);
                modelTreeWidget->updateAllAttriubute(output);
            }
            rendererWidget->update();
        };
        dock->show();
        panel->onApply();
    });

    bindDevelopmentAction("point_volume_interpolator", [=, this]() {
        const QString title = QStringLiteral("点体积插值");
        auto input = currentFilterInput(title);
        if (!input) return;
        auto* dock = new QDockWidget(title, this);
        dock->setAttribute(Qt::WA_DeleteOnClose);
        dock->setObjectName(QStringLiteral("dockWidget_PointVolumeInterpolator"));
        auto* panel = new igQtPointVolumeInterpolatorWidget(dock);
        panel->SetOutputCallback([=](DataObject::Pointer output) {
            refreshFilterResult(input, output, title);
        });
        dock->setWidget(panel);
        addDockWidget(Qt::LeftDockWidgetArea, dock);
        panel->RefreshModel();
        dock->show();
    });

    bindDevelopmentAction("point_set_to_octree_image", [=, this]() {
        const QString title = QStringLiteral("点集转八叉树图像");
        auto input = currentFilterInput(title);
        if (!input) return;
        auto* dock = igQtPointSetToOctreeWidget::createDockWidget(this);
        dock->setAttribute(Qt::WA_DeleteOnClose);
        auto* panel = qobject_cast<igQtPointSetToOctreeWidget*>(dock->widget());
        panel->setCurrentModel(rendererWidget->GetScene()->GetCurrentModel());
        connect(panel, &igQtPointSetToOctreeWidget::resultReady, this,
                [=](DataObject::Pointer output) { refreshFilterResult(input, output, title); });
        connect(panel, &igQtPointSetToOctreeWidget::closeRequested, dock, &QDockWidget::close);
        addDockWidget(Qt::LeftDockWidgetArea, dock);
        dock->show();
    });

    bindDevelopmentAction("random_attributes", [=, this]() {
        const QString title = QStringLiteral("随机属性生成");
        auto input = currentFilterInput(title);
        if (!input) return;
        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(title);
        dialog->setFixedWidth(580);

        // 数据类型下拉框（10 种，与核心 Filter 支持一致）
        const int dataTypeId = dialog->addParameter(
                igQtFilterDialogDockWidget::QT_COMBO_BOX, QStringLiteral("数据类型 (Data Type)"),
                std::vector<QString>{QStringLiteral("Float"), QStringLiteral("Double"), QStringLiteral("Int"),
                        QStringLiteral("Unsigned Int"), QStringLiteral("Char"), QStringLiteral("Unsigned Char"),
                        QStringLiteral("Short"), QStringLiteral("Unsigned Short"), QStringLiteral("Long Long"),
                        QStringLiteral("Unsigned Long Long")});
        const int minId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("最小值 (Minimum)"), "0");
        const int maxId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("最大值 (Maximum)"), "255");

        // 挂载位置下拉框（Point / Cell）
        const int attachId = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX,
                QStringLiteral("挂载位置 (Attach To)"),
                std::vector<QString>{QStringLiteral("点 (Point)"), QStringLiteral("面/单元 (Cell)")});
        const int seedId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("随机种子 (Seed)"), "0");

        // 数组名称 + 同名冲突处理
        const int attrNameId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("数组名称 (Attribute Name)"),
                QStringLiteral("RandomPointScalars"));
        const int conflictId = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX,
                QStringLiteral("同名处理 (Name Conflict)"),
                std::vector<QString>{QStringLiteral("追加 (Append)"),
                        QStringLiteral("替换 (Replace)"),
                        QStringLiteral("自动改名 (Auto Rename)")});
        dialog->show();
        dialog->setApplyFunctor([=, this]() {
            bool ok{};
            static const int kTypes[] = {
                    IG_FLOAT, IG_DOUBLE, IG_INT, IG_UNSIGNED_INT, IG_CHAR, IG_UNSIGNED_CHAR,
                    IG_SHORT, IG_UNSIGNED_SHORT, IG_LONG_LONG, IG_UNSIGNED_LONG_LONG};
            const int typeIdx = dialog->getComboIndex(dataTypeId, ok);
            const double lo = dialog->getDouble(minId, ok);
            const double hi = dialog->getDouble(maxId, ok);
            const int attachIdx = dialog->getComboIndex(attachId, ok);
            const unsigned int seed = static_cast<unsigned int>(dialog->getInt(seedId, ok));
            const int conflictIdx = dialog->getComboIndex(conflictId, ok);
            if (!ok || typeIdx < 0 || typeIdx >= 10 || lo > hi || seed < 0) {
                showDarkFramelessMessage(title, QStringLiteral("随机属性参数无效。"));
                dialog->close();
                return;
            }
            std::string attrName;
            if (auto nameEdit = dynamic_cast<QLineEdit*>(dialog->getWidget(attrNameId))) {
                attrName = nameEdit->text().trimmed().toStdString();
            }
            RandomAttributesFilter::Pointer filter = RandomAttributesFilter::New();
            filter->SetInput(input);
            filter->SetRange(lo, hi);
            filter->SetDataType(kTypes[typeIdx]);
            filter->SetAttachmentType(attachIdx == 0 ? IG_POINT : IG_CELL);
            filter->SetSeed(seed);
            if (!attrName.empty()) filter->SetAttributeName(attrName);
            switch (conflictIdx) {
            case 1: filter->SetNameConflictMode(RandomAttributesFilter::NameConflictMode::Replace); break;
            case 2: filter->SetNameConflictMode(RandomAttributesFilter::NameConflictMode::AutoRename); break;
            default: filter->SetNameConflictMode(RandomAttributesFilter::NameConflictMode::Append); break;
            }
            if (!filter->Execute()) {
                showDarkFramelessMessage(title, QString::fromStdString(filter->GetMessage()));
                dialog->close();
                return;
            }
            // 核心已生成独立输出，直接使用
            auto rawOut = filter->GetOutput();
            if (!rawOut) { dialog->close(); return; }
            // 模型节点名兜底：无名 → RandomAttrResult；有名 → 原名 + 后缀
            rawOut->SetName(rawOut->GetName().empty() ? std::string("RandomAttrResult")
                                                      : rawOut->GetName() + " (RandomAttr)");
            modelTreeWidget->addDataObjectToModelTree(rawOut, Algorithm);
            // 执行后选中结果，默认按新随机数组着色
            if (auto drawObj = DynamicCast<DrawObject>(rawOut)) {
                auto outAttrSet = rawOut->GetAttributeSet();
                int idx = outAttrSet ? outAttrSet->GetAttributeIndex(
                        attrName.empty() ? "RandomPointScalars" : attrName) : -1;
                if (idx < 0 && outAttrSet) idx = (int)outAttrSet->GetNumberOfAttributes() - 1;
                if (idx >= 0) drawObj->ViewCloudPicture(rendererWidget->GetScene(), idx, 0);
            }
            rendererWidget->update();
            dialog->close();
        });
    });
    bindDevelopmentAction("pass_arrays", [=, this]() {
        const QString title = QStringLiteral("传递过滤数据数组");
        auto input = currentFilterInput(title);
        if (!input) return;

        auto attrSet = input->GetAttributeSet();
        if (attrSet == nullptr) {
            showDarkFramelessMessage(title, QStringLiteral("当前模型没有属性集。"));
            return;
        }

        // 收集属性名称：点数据与单元数据分开收集，后续也分开记录勾选结果
        QStringList pointNames;
        QStringList cellNames;
        auto collectNames = [](auto attrs, QStringList& out) {
            if (!attrs) return;
            for (IGsize i = 0; i < attrs->GetNumberOfElements(); ++i) {
                auto& attr = attrs->GetElement(i);
                if (attr.IsNone() || !attr.pointer) continue;
                out.append(QString::fromStdString(attr.pointer->GetName()));
            }
        };
        collectNames(attrSet->GetAllPointAttributes(), pointNames);
        collectNames(attrSet->GetAllCellAttributes(), cellNames);
        if (pointNames.isEmpty() && cellNames.isEmpty()) {
            showDarkFramelessMessage(title, QStringLiteral("当前模型没有任何属性可传递。"));
            return;
        }

        // ---------- 属性多选框对话框 ----------
        igQtChromeFramelessDialog* dlg = new igQtChromeFramelessDialog(this);
        dlg->setDialogTitle(QStringLiteral("选择要传递的属性"));
        dlg->setMaximizeEnabled(false);
        dlg->setAttribute(Qt::WA_DeleteOnClose);

        QWidget* content = new QWidget(dlg->contentHost());
        content->setStyleSheet(
                "QWidget { background-color: #252526; color: #D4D4D4; }"
                "QCheckBox { color: #D4D4D4; spacing: 6px; }"
                "QCheckBox::indicator { width: 16px; height: 16px; }"
                "QCheckBox::indicator:unchecked { border: 1px solid #6A6A6A; background-color: #3A3A3A; }"
                "QCheckBox::indicator:checked { border: 1px solid #0E639C; background-color: #0E639C; }"
                "QCheckBox::indicator:unchecked:hover { border: 1px solid #9A9A9A; }"
                "QCheckBox::indicator:checked:hover { background-color: #1177BB; }"
                "QPushButton { background-color: #3A3A3A; color: #D4D4D4; border: 1px solid #4A4A4A; "
                "              padding: 6px 12px; border-radius: 4px; }"
                "QPushButton:hover { background-color: #4A4A4A; border-color: #5A5A5A; }"
                "QPushButton:pressed { background-color: #2A2A2A; }"
                "QScrollArea { background-color: #1E1E1E; border: none; }"
                "QScrollBar:vertical { background: #1E1E1E; width: 10px; margin: 0; }"
                "QScrollBar::handle:vertical { background: #4A4A4A; border-radius: 5px; min-height: 20px; }"
                "QScrollBar::handle:vertical:hover { background: #5A5A5A; }"
                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }");
        QVBoxLayout* mainLayout = new QVBoxLayout(content);
        mainLayout->setContentsMargins(12, 12, 12, 12);
        mainLayout->setSpacing(8);

        QLabel* tip = new QLabel(
                QStringLiteral("勾选需要传递到输出的属性，未勾选的属性会被丢弃。"),
                content);
        tip->setWordWrap(true);
        mainLayout->addWidget(tip);

        QScrollArea* scrollArea = new QScrollArea(content);
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        QWidget* listWidget = new QWidget(scrollArea);
        QVBoxLayout* listLayout = new QVBoxLayout(listWidget);
        listLayout->setSpacing(4);
        listLayout->setContentsMargins(0, 0, 0, 0);

        QList<QCheckBox*> pointBoxes;
        QList<QCheckBox*> cellBoxes;
        auto addSection = [&](const QString& header, const QStringList& names, const QString& emptyTip,
                              QList<QCheckBox*>& boxes) {
            QLabel* headerLabel = new QLabel(header, listWidget);
            QFont headerFont = headerLabel->font();
            headerFont.setBold(true);
            headerLabel->setFont(headerFont);
            listLayout->addWidget(headerLabel);

            if (names.isEmpty()) {
                listLayout->addWidget(new QLabel(emptyTip, listWidget));
                return;
            }

            QHBoxLayout* btnRow = new QHBoxLayout();
            btnRow->setContentsMargins(0, 0, 0, 0);
            QPushButton* selectAllBtn = new QPushButton(QStringLiteral("全选"), listWidget);
            QPushButton* deselectAllBtn = new QPushButton(QStringLiteral("全不选"), listWidget);
            btnRow->addWidget(selectAllBtn);
            btnRow->addWidget(deselectAllBtn);
            btnRow->addStretch();
            listLayout->addLayout(btnRow);

            for (const QString& name: names) {
                QCheckBox* cb = new QCheckBox(name, listWidget);
                cb->setChecked(true); // 默认全部选中
                boxes.append(cb);
                listLayout->addWidget(cb);
            }
            // 按值捕获复选框列表：列表内容此时已经填充完毕
            const QList<QCheckBox*> sectionBoxes = boxes;
            connect(selectAllBtn, &QPushButton::clicked, listWidget,
                    [sectionBoxes]() { for (QCheckBox* cb: sectionBoxes) cb->setChecked(true); });
            connect(deselectAllBtn, &QPushButton::clicked, listWidget,
                    [sectionBoxes]() { for (QCheckBox* cb: sectionBoxes) cb->setChecked(false); });
        };
        addSection(QStringLiteral("点数据 (Point Data)"), pointNames,
                   QStringLiteral("（无点属性）"), pointBoxes);
        addSection(QStringLiteral("单元数据 (Cell Data)"), cellNames,
                   QStringLiteral("（无单元属性）"), cellBoxes);
        listWidget->setLayout(listLayout);
        scrollArea->setWidget(listWidget);
        mainLayout->addWidget(scrollArea);

        QDialogButtonBox* buttonBox =
                new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, content);
        mainLayout->addWidget(buttonBox);

        dlg->setContentWidget(content);
        dlg->resize(420, 560);

        connect(buttonBox, &QDialogButtonBox::accepted, this,
                [this, dlg, input, pointBoxes, cellBoxes, pointNames, cellNames, title,
                 refreshFilterResult]() {
            std::vector<std::string> selectedPointNames;
            std::vector<std::string> selectedCellNames;
            for (int i = 0; i < pointBoxes.size(); ++i) {
                if (pointBoxes[i]->isChecked()) selectedPointNames.push_back(pointNames[i].toStdString());
            }
            for (int i = 0; i < cellBoxes.size(); ++i) {
                if (cellBoxes[i]->isChecked()) selectedCellNames.push_back(cellNames[i].toStdString());
            }
            if (selectedPointNames.empty() && selectedCellNames.empty()) {
                showDarkFramelessMessage(title, QStringLiteral("请至少勾选一个属性。"));
                return;
            }

            auto filter = PassArrays::New();
            filter->SetInput(input);
            // 点数据与单元数据的勾选结果分别记录，同名属性不会互相牵连
            filter->SetPointArrayNames(selectedPointNames);
            filter->SetCellArrayNames(selectedCellNames);
            if (!filter->Execute()) {
                showDarkFramelessMessage(title, QStringLiteral("数组过滤失败：输入数据类型不受支持。"));
                return;
            }
            auto output = filter->GetOutput();
            if (!output) {
                showDarkFramelessMessage(title, QStringLiteral("未得到输出数据。"));
                return;
            }
            output->SetName(input->GetName() + "_filtered");
            refreshFilterResult(input, output, title);
            dlg->accept();
        });
        connect(buttonBox, &QDialogButtonBox::rejected, dlg, &QDialog::reject);

        dlg->show();
    });


    bindDevelopmentAction("deflect_normals", [=, this]() {
        const QString title = QStringLiteral("法向偏转");
        auto input = currentFilterInput(title);
        if (!input) return;
        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(title);
        dialog->setFixedWidth(640);

        // 向量场所在位置（自动 / 点 / 单元）：显式指定，避免同名属性取错
        const int vfAttachId = dialog->addParameter(
                igQtFilterDialogDockWidget::QT_COMBO_BOX,
                QStringLiteral("向量场所在位置 (Vector Field Location)"),
                std::vector<QString>{QStringLiteral("自动 Auto (Point -> Cell 兜底)"),
                        QStringLiteral("点数据 Point Data"),
                        QStringLiteral("单元/面数据 Cell Data")});
        // 向量场下拉框（按位置联动筛选，显示带位置标记，userData 存纯名称）
        const int vecFieldId = dialog->addParameter(
                igQtFilterDialogDockWidget::QT_COMBO_BOX,
                QStringLiteral("向量场 (Vector Field)"), std::vector<QString>{QStringLiteral("(请选择)")});

        auto attrSet = input->GetAttributeSet();
        auto* vecCombo = dynamic_cast<QComboBox*>(dialog->getWidget(vecFieldId));
        auto refreshVectorFields = [vecCombo, attrSet](int locationFilter) {
            if (!vecCombo) return;
            vecCombo->clear();
            if (!attrSet) { vecCombo->addItem(QStringLiteral("(无可用向量属性)")); return; }
            bool found = false;
            for (size_t i = 0; i < attrSet->GetNumberOfAttributes(); ++i) {
                auto& attr = attrSet->GetAttribute(i);
                if (!attr.pointer || attr.isDeleted || attr.pointer->GetDimension() != 3) continue;
                if (locationFilter == 1 && attr.attachmentType != IG_POINT) continue;
                if (locationFilter == 2 && attr.attachmentType != IG_CELL) continue;
                QString name = QString::fromStdString(attr.pointer->GetName());
                QString tag = (attr.attachmentType == IG_POINT) ? QStringLiteral(" (Point)")
                                                               : QStringLiteral(" (Cell)");
                vecCombo->addItem(name + tag, name);
                found = true;
            }
            if (!found) vecCombo->addItem(QStringLiteral("(无可用向量属性)"));
        };
        refreshVectorFields(0);
        if (auto* vfCombo = dynamic_cast<QComboBox*>(dialog->getWidget(vfAttachId))) {
            connect(vfCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                    refreshVectorFields);
        }

        const int strengthId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("偏转强度 (Scale Factor)"), "1.0");
        const int useUserNormalId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                QStringLiteral("使用自定义法向 (Use User Normal)"), "false");
        const int normalXId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("法向 X (Normal X)"), "0");
        const int normalYId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("法向 Y (Normal Y)"), "0");
        const int normalZId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("法向 Z (Normal Z)"), "1");

        dialog->show();
        dialog->setApplyFunctor([=, this]() {
            bool ok{};
            auto combo = dynamic_cast<QComboBox*>(dialog->getWidget(vecFieldId));
            if (!combo || combo->currentText() == QStringLiteral("(无可用向量属性)") ||
                combo->currentText() == QStringLiteral("(请选择)")) {
                showDarkFramelessMessage(QStringLiteral("缺少向量场"),
                                         QStringLiteral("请先选择一个 3 分量的向量属性。"));
                return;
            }
            QString fieldName = combo->currentData().toString();
            if (fieldName.isEmpty()) fieldName = combo->currentText();
            const int attachIdx = dialog->getComboIndex(vfAttachId, ok);
            IGenum vfAttach = IG_NONE;
            if (attachIdx == 1) vfAttach = IG_POINT;
            else if (attachIdx == 2) vfAttach = IG_CELL;
            const double strength = dialog->getDouble(strengthId, ok);
            const bool useUserNormal = dialog->getChecked(useUserNormalId, ok);
            const double nx = dialog->getDouble(normalXId, ok);
            const double ny = dialog->getDouble(normalYId, ok);
            const double nz = dialog->getDouble(normalZId, ok);

            DeflectNormalsFilter::Pointer filter = DeflectNormalsFilter::New();
            filter->SetInput(input);
            filter->SetAttributeByName(fieldName.toStdString());
            filter->SetVectorFieldAttachment(vfAttach);
            filter->SetDeflectStrength(static_cast<float>(strength));
            filter->SetUseUserNormal(useUserNormal);
            filter->SetUserNormal(nx, ny, nz);
            if (!filter->Execute()) {
                showDarkFramelessMessage(QStringLiteral("执行出错"),
                                         QString::fromStdString(filter->GetMessage()));
                dialog->close();
                return;
            }
            // 界面明确显示基准法向来源
            QString baseSrc = QString::fromStdString(filter->GetBaseNormalSource());
            showDarkFramelessMessage(QStringLiteral("基准法向来源"),
                                     baseSrc.isEmpty() ? QStringLiteral("未知") : baseSrc);
            // 核心已生成独立输出，直接使用
            auto rawOut = filter->GetOutput();
            if (!rawOut) { dialog->close(); return; }
            // 模型节点名兜底：无名 → DeflectResult；有名 → 原名 + 后缀
            rawOut->SetName(rawOut->GetName().empty() ? std::string("DeflectResult")
                                                      : rawOut->GetName() + " (Deflect)");
            modelTreeWidget->addDataObjectToModelTree(rawOut, Algorithm);

            // 执行后提供“法向显示模式”切换框（偏转法向 / 几何法向）
            QDialog* displayDlg = new QDialog(this);
            displayDlg->setWindowTitle(QStringLiteral("法向显示模式 (Display Normal)"));
            displayDlg->setFixedWidth(440);
            auto* dlay = new QVBoxLayout(displayDlg);
            dlay->addWidget(new QLabel(QStringLiteral("显示法向 (Display Normal):"), displayDlg));
            auto* modeCombo = new QComboBox(displayDlg);
            modeCombo->addItems({QStringLiteral("使用偏转法向 (Deflected)"),
                                 QStringLiteral("使用几何法向 (Geometric)")});
            dlay->addWidget(modeCombo);

            auto applyDisplayNormal = [this, rawOut](int mode) {
                auto drawObj = DynamicCast<DrawObject>(rawOut);
                if (!drawObj) return;
                auto outAttrSet = rawOut->GetAttributeSet();
                int idx = -1;
                if (mode == 0) {
                    // 偏转法向：把 DeflectedNormals 作为渲染器实际使用的光照法向，并按它着色
                    if (outAttrSet) idx = outAttrSet->GetAttributeIndex("DeflectedNormals");
                    if (idx >= 0) {
                        auto nArr = DynamicCast<FloatArray>(outAttrSet->GetAttribute(idx).pointer);
                        if (nArr) drawObj->SetLightingNormal(nArr);
                    }
                } else {
                    // 几何法向：回退到几何法向，并按其着色
                    drawObj->ClearLightingNormal();
                    if (outAttrSet) idx = outAttrSet->GetAttributeIndex("Normals");
                    if (idx < 0) idx = -1;
                }
                drawObj->ViewCloudPicture(rendererWidget->GetScene(), idx, 0);
                drawObj->ForceReConvertToDrawableData();
                rendererWidget->update();
            };
            connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                    applyDisplayNormal);
            applyDisplayNormal(modeCombo->currentIndex());
            displayDlg->show();
            rendererWidget->update();
            dialog->close();
        });
    });

    bindDevelopmentAction("transform", [=, this]() {
        const QString title = QStringLiteral("通用几何变换");
        // 面板已打开：只把它提到最前
        if (m_transformController != nullptr) {
            m_transformController->FocusPanel();
            return;
        }

        auto input = currentFilterInput(title);
        if (!input) return;

        const IGenum inputType = input->GetDataObjectType();
        if (inputType != IG_SURFACE_MESH && inputType != IG_VOLUME_MESH &&
            inputType != IG_STRUCTURED_MESH && inputType != IG_UNSTRUCTURED_MESH &&
            inputType != IG_POINT_SET) {
            showDarkFramelessMessage(title,
                                     QStringLiteral("请在模型树中选择网格或点云（多帧容器暂不支持交互变换）。"));
            return;
        }

        auto* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(title);
        dialog->setFilterDescription(QStringLiteral(
                "三维盒子交互：按住鼠标<b>中键</b>拖动。<br>"
                "拖棱或中心：平移<br>"
                "拖面手柄：缩放<br>"
                "拖面：旋转"));

        std::array<int, igQtTransformBoxController::kParamCount> parameterIds{};
        parameterIds[0] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("平移 X"), "0");
        parameterIds[1] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("平移 Y"), "0");
        parameterIds[2] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("平移 Z"), "0");
        parameterIds[3] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("旋转 X (度)"), "0");
        parameterIds[4] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("旋转 Y (度)"), "0");
        parameterIds[5] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("旋转 Z (度)"), "0");
        parameterIds[6] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("缩放 X"), "1");
        parameterIds[7] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("缩放 Y"), "1");
        parameterIds[8] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("缩放 Z"), "1");

        // 「重置」按钮：参数回到单位变换、盒子回到初始包围盒（模型不变，需要应用才生效）
        auto* resetRow = new QWidget(dialog);
        auto* resetLayout = new QHBoxLayout(resetRow);
        resetLayout->setContentsMargins(0, 0, 0, 0);
        auto* resetButton = new QPushButton(QStringLiteral("重置"), resetRow);
        resetLayout->addStretch();
        resetLayout->addWidget(resetButton);
        dialog->addRowWidget(resetRow);
        dialog->show();

        m_transformController =
                new igQtTransformBoxController(input, rendererWidget, dialog, parameterIds, this);
        connect(resetButton, &QPushButton::clicked, this, [this]() {
            if (m_transformController != nullptr) { m_transformController->Reset(); }
        });
        connect(m_transformController, &igQtTransformBoxController::Applied, this,
                [=, this](DataObject::Pointer output) {
                    refreshFilterResult(input, output, title);
                    if (auto* item = modelTreeWidget->getItemFromObject(input)) {
                        item->changeVisibility(false);
                        rendererWidget->update();
                    }
                });
        connect(m_transformController, &igQtTransformBoxController::Failed, this,
                [this, title]() { showDarkFramelessMessage(title, QStringLiteral("几何变换失败。")); });
        connect(m_transformController, &igQtTransformBoxController::Closed, this, [this]() {
            if (m_transformController == nullptr) { return; }
            m_transformController->deleteLater();
            m_transformController = nullptr;
        });
        if (!m_transformController->HasBox()) {
            showDarkFramelessMessage(title, QStringLiteral("交互盒子创建失败，仍可手动输入参数后应用。"));
        }

        dialog->setApplyFunctor([this]() {
            if (m_transformController != nullptr) { m_transformController->Apply(); }
        });
    });

    bindDevelopmentAction("volume_of_revolution", [=, this]() {
        const QString title = QStringLiteral("旋转体生成");
        auto input = currentFilterInput(title);
        if (!input) return;
        auto* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(title);
        const int ax = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("轴方向 X"), "0");
        const int ay = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("轴方向 Y"), "0");
        const int az = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("轴方向 Z"), "1");
        const int px = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("轴上一点 X"), "0");
        const int py = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("轴上一点 Y"), "0");
        const int pz = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("轴上一点 Z"), "0");
        const int resolution = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("分段数"), "12");
        const int angle = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("角度 (度)"), "360");
        dialog->show();
        dialog->setApplyFunctor([=, this]() {
            const int ids[] = {ax, ay, az, px, py, pz, resolution, angle};
            double values[8]{};
            for (int i = 0; i < 8; ++i) {
                bool ok = false;
                values[i] = dialog->getDouble(ids[i], ok);
                if (!ok) { showDarkFramelessMessage(title, QStringLiteral("旋转参数必须是数字。")); return; }
            }
            if (values[0] * values[0] + values[1] * values[1] + values[2] * values[2] == 0 ||
                values[6] < 3 || values[7] <= 0 || values[7] > 360) {
                showDarkFramelessMessage(title, QStringLiteral("轴、分段数或角度无效。")); return;
            }
            auto filter = VolumeOfRevolutionFilter::New();
            filter->SetInput(input);
            filter->SetAxis(Vector3d(values[0], values[1], values[2]),
                            Vector3d(values[3], values[4], values[5]));
            filter->SetResolution(static_cast<int>(values[6]));
            filter->SetAngle(values[7] * 3.141592653589793 / 180.0);
            if (!filter->Execute()) {
                const std::string message = filter->GetMessage();
                showDarkFramelessMessage(title, message.empty()
                                                    ? QStringLiteral("旋转体生成失败。")
                                                    : QString::fromStdString(message));
                return;
            }
            auto output=filter->GetOutput();
            output->SetName(input->GetName() + "_revolution");
            refreshFilterResult(input, output, title);
            dialog->close();
        });
    });
    bindDevelopmentAction("iso_volume", [=, this]() {
        const QString title = QStringLiteral("等值面体提取");
        auto input = currentFilterInput(title);
        if (!input || !input->GetAttributeSet()) return;
        auto attrs = input->GetAttributeSet()->GetAllPointAttributes();
        if (!attrs || attrs->GetNumberOfElements() == 0) {
            showDarkFramelessMessage(title, QStringLiteral("当前模型没有点属性。")); return;
        }
        std::vector<QString> names;
        for (igIndex i = 0; i < attrs->GetNumberOfElements(); ++i)
            names.push_back(QString::fromStdString(attrs->GetElement(i).pointer->GetName()));
        auto* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(title);
        const int array = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX,
                QStringLiteral("点属性"), names);
        const int component = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("分量 (从0开始)"), "0");
        const int lower = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("下限"), "0");
        const int upper = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("上限"), "1");
        dialog->show();
        dialog->setApplyFunctor([=, this]() {
            bool ok[4]{};
            const int index = dialog->getComboIndex(array, ok[0]);
            const int comp = dialog->getInt(component, ok[1]);
            const double lo = dialog->getDouble(lower, ok[2]), hi = dialog->getDouble(upper, ok[3]);
            if (!ok[0] || !ok[1] || !ok[2] || !ok[3] || index < 0 ||
                index >= attrs->GetNumberOfElements() || comp < 0 || lo > hi) {
                showDarkFramelessMessage(title, QStringLiteral("请选择有效的属性、分量和区间。")); return;
            }
            auto selected = attrs->GetElement(index).pointer;
            if (!selected || comp >= selected->GetDimension()) {
                showDarkFramelessMessage(title, QStringLiteral("分量超出所选属性范围。")); return;
            }
            auto filter = IsoVolumeFilter::New();
            filter->SetInput(input);
            filter->SetIsoScalarData(selected, lo, hi, comp);
            if (!filter->Execute()) { showDarkFramelessMessage(title, QStringLiteral("等值面体提取失败。")); return; }
            refreshFilterResult(input, filter->GetOutput(), title);
            dialog->close();
        });
    });
    addDevelopmentAction(developingFilters, tetraAction,
                         QStringLiteral("四面体化(Mesh Tetrahedralize)"));

    QMenu* developingData = developingFilters->addMenu(QStringLiteral("数据处理 (Data Processing)"));
    addDevelopmentAction(developingData, approvedMenuAction(mesh_processing,
            QStringLiteral("表面网格简化 (Surface Simplification)")),
            QStringLiteral("表面网格简化 (Surface Simplification)"));
    addDevelopmentAction(developingData, approvedMenuAction(mesh_processing,
            QStringLiteral("Fast Surface Simplification（快速表面简化）")),
            QStringLiteral("快速表面简化 (Fast Surface Simplification)"));
    addDevelopmentAction(developingData, approvedMenuAction(mesh_processing,
            QStringLiteral("Surface Triangulation（表面三角化）")),
            QStringLiteral("表面三角化 (Surface Triangulation)"));
    addDevelopmentAction(developingData, approvedMenuAction(mesh_processing,
            QStringLiteral("表面提取 (Extract Surface)")),
            QStringLiteral("表面提取 (Extract Surface)"));
    addDevelopmentAction(developingData, approvedFilter("force_static_mesh"),
            QStringLiteral("强制静态网格 (Force Static Mesh)"));
    auto forceStaticDevelopmentFilter = ForceStaticMeshFilter::New();
    bindDevelopmentAction("force_static_mesh", [executeDevelopmentFilter, forceStaticDevelopmentFilter]() {
        executeDevelopmentFilter(QStringLiteral("强制静态网格"), forceStaticDevelopmentFilter);
    });

    // 开发中filter/第二批入口。
    // ---------- 按标量变形 (Warp By Scalar) ----------
    QAction* warpScalarAction =
            developingFiltersBatch2->addAction(QStringLiteral("按标量变形 (Warp By Scalar)"));
    warpScalarAction->setData(QStringLiteral("warp_by_scalar"));
    connect(warpScalarAction, &QAction::triggered, this, [this](bool) {
        // 变形过滤器只改写点坐标，因此只接受点集派生类型
        auto resolveWarpInput = [this](DataObject::Pointer& obj, const QString& title) -> bool {
            obj = nullptr;
            auto scene = rendererWidget->GetScene();
            if (scene == nullptr || scene->GetCurrentModel() == nullptr) {
                showDarkFramelessMessage(title, QStringLiteral("请先在模型树中选择一个模型。"));
                return false;
            }
            obj = scene->GetCurrentModel()->GetDataObject();
            if (!obj) {
                showDarkFramelessMessage(title, QStringLiteral("当前模型没有可用的数据对象。"));
                return false;
            }
            const IGenum type = obj->GetDataObjectType();
            if (type != IG_POINT_SET && type != IG_SURFACE_MESH && type != IG_VOLUME_MESH &&
                type != IG_UNSTRUCTURED_MESH && type != IG_STRUCTURED_MESH) {
                showDarkFramelessMessage(
                        title, QStringLiteral("该算法只支持点集 / 表面网格 / 体网格 / 非结构网格 / 结构网格。"));
                return false;
            }
            return true;
        };

        struct WarpArrayInfo {
            QString name;
            int dimension{1};
            QString typeName;
        };
        auto collectPointArrays = [](DataObject::Pointer obj, std::vector<WarpArrayInfo>& out) {
            auto attrSet = obj->GetAttributeSet();
            if (!attrSet) { return; }
            auto attrs = attrSet->GetAllPointAttributes();
            if (!attrs) { return; }
            for (IGsize i = 0; i < attrs->GetNumberOfElements(); ++i) {
                auto& attr = attrs->GetElement(i);
                if (attr.IsNone() || !attr.pointer) { continue; }
                // Warp By Scalar accepts only single-component point arrays.
                if (attr.pointer->GetDimension() != 1) { continue; }
                WarpArrayInfo info;
                info.name = QString::fromStdString(attr.pointer->GetName());
                info.dimension = attr.pointer->GetDimension();
                switch (attr.type) {
                    case IG_SCALAR:
                        info.typeName = QStringLiteral("标量");
                        break;
                    case IG_VECTOR:
                        info.typeName = QStringLiteral("向量");
                        break;
                    case IG_NORMAL:
                        info.typeName = QStringLiteral("法向");
                        break;
                    case IG_TCOORD:
                        info.typeName = QStringLiteral("纹理坐标");
                        break;
                    case IG_TENSOR:
                        info.typeName = QStringLiteral("张量");
                        break;
                    default:
                        info.typeName = QStringLiteral("属性");
                        break;
                }
                out.push_back(info);
            }
        };
        DataObject::Pointer obj = nullptr;
        if (!resolveWarpInput(obj, QStringLiteral("按标量变形"))) { return; }

        std::vector<WarpArrayInfo> arrays;
        collectPointArrays(obj, arrays);
        if (arrays.empty()) {
            showDarkFramelessMessage(QStringLiteral("按标量变形"),
                                     QStringLiteral("当前模型没有单分量点属性数组，无法按标量变形。\n"
                                                    "可以先用「转换为点数据」把单元属性转到点上。"));
            return;
        }
        std::vector<QString> items;
        for (const auto& a: arrays) {
            items.push_back(QStringLiteral("%1 (%2, %3 分量)").arg(a.name, a.typeName).arg(a.dimension));
        }

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("按标量变形 (Warp By Scalar)"));
        dialog->setFilterDescription(QStringLiteral("沿法向按标量值移动点：x' = x + n * s * 缩放系数。\n"
                                                    "未勾选「使用指定法向」时优先使用输入的点法向数组，"
                                                    "缺失时退回下面的法向。"));
        int arrayId = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX,
                                           QStringLiteral("标量数组 (Scalars)"), items);
        int scaleId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                           QStringLiteral("缩放系数 (Scale Factor)"), "1.0");
        int useNormalId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                               QStringLiteral("使用指定法向 (Use Normal)"), "false");
        int nxId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                        QStringLiteral("法向 X (Normal X)"), "0.0");
        int nyId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                        QStringLiteral("法向 Y (Normal Y)"), "0.0");
        int nzId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                        QStringLiteral("法向 Z (Normal Z)"), "1.0");
        int xyPlaneId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                             QStringLiteral("XY 平面模式 (XY Plane)"), "false");

        dialog->setFixedWidth(360);
        if (auto* sa = dialog->findChild<QScrollArea*>(QStringLiteral("scrollArea"))) {
            sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
        dialog->show();

        dialog->setApplyFunctor([=, this]() {
            bool ok = false;
            const int choice = dialog->getComboIndex(arrayId, ok);
            if (!ok || choice < 0 || choice >= static_cast<int>(arrays.size())) {
                showDarkFramelessMessage(QStringLiteral("按标量变形"), QStringLiteral("请选择有效的标量数组。"));
                return;
            }
            const double scale = dialog->getDouble(scaleId, ok);
            if (!ok) {
                showDarkFramelessMessage(QStringLiteral("按标量变形"), QStringLiteral("缩放系数必须是数值。"));
                return;
            }
            const double nx = dialog->getDouble(nxId, ok);
            if (!ok) {
                showDarkFramelessMessage(QStringLiteral("按标量变形"), QStringLiteral("法向 X 必须是数值。"));
                return;
            }
            const double ny = dialog->getDouble(nyId, ok);
            if (!ok) {
                showDarkFramelessMessage(QStringLiteral("按标量变形"), QStringLiteral("法向 Y 必须是数值。"));
                return;
            }
            const double nz = dialog->getDouble(nzId, ok);
            if (!ok) {
                showDarkFramelessMessage(QStringLiteral("按标量变形"), QStringLiteral("法向 Z 必须是数值。"));
                return;
            }
            const bool useNormal = dialog->getChecked(useNormalId, ok);
            const bool xyPlane = dialog->getChecked(xyPlaneId, ok);

            auto filter = WarpByScalar::New();
            filter->SetScalarsArrayName(arrays[static_cast<size_t>(choice)].name.toStdString());
            filter->SetScaleFactor(scale);
            filter->SetUseNormal(useNormal);
            filter->SetNormal(nx, ny, nz);
            filter->SetXYPlane(xyPlane);
            filter->SetInput(obj);
            if (!filter->Execute()) {
                showDarkFramelessMessage(QStringLiteral("按标量变形"),
                                         QStringLiteral("变形失败：请检查标量数组与法向设置。"));
                return;
            }
            auto output = filter->GetOutput();
            if (!output) {
                showDarkFramelessMessage(QStringLiteral("按标量变形"), QStringLiteral("算法未产生有效结果。"));
                return;
            }
            output->SetName(obj->GetName() + "_warp_scalar");
            modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
            rendererWidget->GetScene()->Modified();
            rendererWidget->GetScene()->Update();
            rendererWidget->update();
            dialog->close();
        });
    });

    QMenu* developingConvert = developingFilters->addMenu(QStringLiteral("数据转换 (Convert)"));
    addDevelopmentAction(developingConvert, approvedMenuAction(convert,
            QStringLiteral("Convert To Point Data（转换为点数据）")),
            QStringLiteral("转换为点数据 (Convert To PointData)"));
    addDevelopmentAction(developingConvert, approvedMenuAction(convert,
            QStringLiteral("Convert To Cell Data（转换为单元数据）")),
            QStringLiteral("转换为单元数据 (Convert To CellData)"));
    addDevelopmentAction(developingConvert, lagrangeUnstructedMesh_visualization,
            QStringLiteral("拉格朗日非结构网格可视化 (LagrangeUnstructedMesh Visualization)"));

    QMenu* developingView = developingFilters->addMenu(QStringLiteral("特征提取"));
    addDevelopmentAction(developingView, approvedFilter("outline_corners"),
            QStringLiteral("提取包围盒角点 (Outline Corners)"));
    addDevelopmentAction(developingView, approvedFilter("count_cell_faces"),
            QStringLiteral("统计单元面数 (Count Cell Faces)"));
    addDevelopmentAction(developingView, gradient, QStringLiteral("计算梯度 (ComputeGradient)"));
    addDevelopmentAction(developingView, laplacian, QStringLiteral("计算拉普拉斯 (ComputeLaplacian)"));
    addDevelopmentAction(developingView, curvature, QStringLiteral("计算曲率 (ComputeCurvature)"));
    addDevelopmentAction(developingView, vortex, QStringLiteral("计算涡量 (ComputeVorticity)"));
    addDevelopmentAction(developingView, vortexPrection, QStringLiteral("涡旋预测 (PredictVortex)"));

    QMenu* developingAttributes = developingFilters->addMenu(QStringLiteral("数据属性操作 (Attribute Manipulation)"));
    addDevelopmentAction(developingAttributes, approvedFilter("random_vectors"),
            QStringLiteral("随机向量 (Random Vectors)"));

    // 开发中filter/第二批：后续新增 filter 追加在该菜单末尾。
    // ---------- 屈服准则 (Yield Criteria)：追加在“新增filter/第二批”菜单末尾 ----------
    connect(developingFiltersBatch2->addAction(QStringLiteral("屈服准则 (Yield Criteria)")),
            &QAction::triggered, this, [this](bool) {
                auto scene = rendererWidget ? rendererWidget->GetScene() : nullptr;
                auto currentModel = scene ? scene->GetCurrentModel() : nullptr;
                if (!currentModel) {
                    showDarkFramelessMessage(QStringLiteral("屈服准则"), QStringLiteral("请先选择一个模型。"));
                    return;
                }
                auto object = currentModel->GetDataObject();
                if (object == nullptr || iGame::DynamicCast<iGame::PointSet>(object).IsNull()) {
                    showDarkFramelessMessage(QStringLiteral("屈服准则"),
                                             QStringLiteral("当前对象不支持屈服准则（需要网格 / 点集）"));
                    return;
                }

                // 候选张量数组：6 / 9 分量的数组（不强制要求被标记为张量类型），按附着位置分为点 / 单元两组
                std::vector<QString> pointTensors;
                std::vector<QString> cellTensors;
                if (auto attrSet = object->GetAttributeSet()) {
                    const size_t numAttrs = attrSet->GetNumberOfAttributes();
                    for (size_t i = 0; i < numAttrs; ++i) {
                        auto& attr = attrSet->GetAttribute(static_cast<IGsize>(i));
                        if (attr.isDeleted || attr.pointer == nullptr) continue;
                        const int dim = attr.pointer->GetDimension();
                        if (dim != 6 && dim != 9) continue;
                        const QString name = QString::fromStdString(attr.pointer->GetName());
                        if (attr.attachmentType == IG_POINT) {
                            pointTensors.push_back(name);
                        } else if (attr.attachmentType == IG_CELL) {
                            cellTensors.push_back(name);
                        }
                    }
                }

                // ParaView-style checkable groups with select-all/partial state.
                auto makeTensorList = [](const QString& title,
                                         const std::vector<QString>& names) -> QListWidget* {
                    auto* row = new QWidget;
                    auto* layout = new QVBoxLayout(row);
                    layout->setContentsMargins(0, 0, 0, 0);
                    layout->setSpacing(2);
                    auto* label = new QCheckBox(title, row);
                    label->setTristate(true);
                    layout->addWidget(label);
                    auto* list = new QListWidget(row);
                    list->setSelectionMode(QAbstractItemView::NoSelection);
                    list->setAlternatingRowColors(true);
                    for (const auto& name : names) {
                        auto* item = new QListWidgetItem(name, list);
                        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                        item->setCheckState(Qt::Unchecked);
                    }
                    if (list->count() > 0) { list->item(0)->setCheckState(Qt::Checked); }
                    auto syncGroup = [list, label]() {
                        int checked = 0;
                        for (int i = 0; i < list->count(); ++i)
                            checked += list->item(i)->checkState() == Qt::Checked;
                        QSignalBlocker blocker(label);
                        label->setCheckState(checked == 0 ? Qt::Unchecked :
                            checked == list->count() ? Qt::Checked : Qt::PartiallyChecked);
                    };
                    QObject::connect(list, &QListWidget::itemChanged, row,
                                     [syncGroup](QListWidgetItem*) { syncGroup(); });
                    QObject::connect(label, &QCheckBox::clicked, row, [list, syncGroup](bool checked) {
                        {
                            QSignalBlocker blocker(list);
                            for (int i = 0; i < list->count(); ++i)
                                list->item(i)->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
                        }
                        syncGroup();
                    });
                    syncGroup();
                    // 高度自适应：AdjustToContents 会让 sizeHint 反映全部条目的真实高度
                    // （注意 sizeHintForRow 往往略小于实际行高，按它估算会差几像素而出现滚动条），
                    // 这里直接把 sizeHint 作为最小高度，条目很多时再用 maximumHeight 限制。
                    list->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
                    list->setMinimumHeight(list->sizeHint().height());
                    const int maxRows = 8;
                    const int rowHeight = std::max(20, list->sizeHintForRow(0));
                    list->setMaximumHeight(maxRows * rowHeight + 2 * list->frameWidth() + 8);
                    layout->addWidget(list);
                    return list;
                };

                auto* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(QStringLiteral("屈服准则"));
                dialog->setFilterDescription(
                        QStringLiteral("勾选需要计算的结果，可同时选择多个；全部不勾选时输出原模型的数据副本。"
                                       "各分组标题可全选或取消全选。"));
                QListWidget* pointList = nullptr;
                QListWidget* cellList = nullptr;
                if (!pointTensors.empty()) {
                    pointList = makeTensorList(QStringLiteral("点张量 (Point Tensors)"), pointTensors);
                    dialog->addRowWidget(pointList->parentWidget());
                }
                if (!cellTensors.empty()) {
                    cellList = makeTensorList(QStringLiteral("单元张量 (Cell Tensors)"), cellTensors);
                    dialog->addRowWidget(cellList->parentWidget());
                }
                auto* criterionList = makeTensorList(QStringLiteral("屈服准则 (Yield Criteria)"),
                        {QStringLiteral("Principal Stress"), QStringLiteral("Tresca Criterion"),
                         QStringLiteral("Von Mises Criterion")});
                dialog->addRowWidget(criterionList->parentWidget());
                dialog->show();
                dialog->setApplyFunctor([=, this]() {
                    // 收集两个多选列表里选中的张量数组
                    std::vector<std::string> selectedTensors;
                    auto collect = [&selectedTensors](QListWidget* list) {
                        if (list == nullptr) { return; }
                        for (int i = 0; i < list->count(); ++i) {
                            auto* item = list->item(i);
                            if (item->checkState() == Qt::Checked) selectedTensors.push_back(item->text().toStdString());
                        }
                    };
                    collect(pointList);
                    collect(cellList);
                    std::vector<YieldCriteriaFilter::Criterion> selectedCriteria;
                    for (int i = 0; i < criterionList->count(); ++i) {
                        if (criterionList->item(i)->checkState() == Qt::Checked)
                            selectedCriteria.push_back(static_cast<YieldCriteriaFilter::Criterion>(i));
                    }
                    if (!selectedCriteria.empty() && selectedTensors.empty()) {
                        showDarkFramelessMessage(QStringLiteral("错误"),
                                                 QStringLiteral("请至少选择一个 6 或 9 分量的张量数组。"));
                        return;
                    }

                    auto filter = YieldCriteriaFilter::New();
                    filter->SetTensorArrayNames(selectedTensors);
                    filter->SetCriteria(selectedCriteria);
                    filter->SetInput(object);
                    if (filter->Execute()) {
                        modelTreeWidget->addDataObjectToModelTree(filter->GetOutput(), ItemSource::Algorithm);
                        modelTreeWidget->updateCloudPicture();
                        rendererWidget->update();
                        dialog->close();
                    } else {
                        showDarkFramelessMessage(QStringLiteral("错误"),
                                                 QString::fromStdString(filter->GetStatusMessage()));
                    }
                });
            });

    // ---------- 追加属性 (Append Attributes) ----------
    QAction* appendAttributesAction =
            developingFiltersBatch2->addAction(QStringLiteral("追加属性 (Append Attributes)"));
    appendAttributesAction->setData(QStringLiteral("append_attributes"));
    connect(appendAttributesAction, &QAction::triggered, this, [this](bool) {
        auto scene = rendererWidget->GetScene();
        if (scene == nullptr) { return; }
        auto modelList = scene->GetModelList();
        if (!modelList) { return; }

        struct AppendInputInfo {
            QString name;
            DataObject::Pointer object;
            IGsize points{0};
            IGsize cells{0};
        };
        // 计算单元数，用于在界面上提示各输入是否逐项对应
        auto cellCountOf = [](DataObject::Pointer obj) -> IGsize {
            switch (obj->GetDataObjectType()) {
                case IG_POINT_SET:
                    return 0;
                case IG_SURFACE_MESH: {
                    auto mesh = DynamicCast<SurfaceMesh>(obj);
                    return mesh ? mesh->GetNumberOfFaces() : IGsize(0);
                }
                case IG_VOLUME_MESH: {
                    auto mesh = DynamicCast<VolumeMesh>(obj);
                    return mesh ? mesh->GetNumberOfVolumes() : IGsize(0);
                }
                case IG_UNSTRUCTURED_MESH: {
                    auto mesh = DynamicCast<UnstructuredMesh>(obj);
                    return mesh ? mesh->GetNumberOfCells() : IGsize(0);
                }
                case IG_STRUCTURED_MESH: {
                    auto mesh = DynamicCast<StructuredMesh>(obj);
                    return mesh ? mesh->GetNumberOfCells() : IGsize(0);
                }
                default: {
                    auto cells = obj->GetCellArray();
                    return cells ? cells->GetNumberOfCells() : IGsize(0);
                }
            }
        };

        std::vector<AppendInputInfo> infos;
        for (auto it = modelList->Begin(); it != modelList->End(); ++it) {
            auto model = it->second;
            if (!model) { continue; }
            auto dataObj = model->GetDataObject();
            if (!dataObj) { continue; }
            const IGenum type = dataObj->GetDataObjectType();
            if (type != IG_POINT_SET && type != IG_SURFACE_MESH && type != IG_VOLUME_MESH &&
                type != IG_UNSTRUCTURED_MESH && type != IG_STRUCTURED_MESH) {
                continue;
            }
            AppendInputInfo info;
            info.object = dataObj;
            info.name = QString::fromStdString(dataObj->GetName());
            auto pts = dataObj->GetPoints();
            info.points = pts ? pts->GetNumberOfPoints() : IGsize(0);
            info.cells = cellCountOf(dataObj);
            infos.push_back(info);
        }
        if (infos.empty()) {
            showDarkFramelessMessage(QStringLiteral("追加属性"),
                                        QStringLiteral("场景中没有可用于追加属性的数据对象。"));
            return;
        }

        igQtChromeFramelessDialog* dlg = new igQtChromeFramelessDialog(this);
        dlg->setDialogTitle(QStringLiteral("追加属性 (Append Attributes)"));
        dlg->setMaximizeEnabled(false);
        dlg->setAttribute(Qt::WA_DeleteOnClose);

        QWidget* content = new QWidget(dlg->contentHost());
        content->setStyleSheet(
                "QWidget { background-color: #252526; color: #D4D4D4; }"
                "QCheckBox { color: #D4D4D4; spacing: 6px; }"
                "QCheckBox::indicator { width: 16px; height: 16px; }"
                "QCheckBox::indicator:unchecked { border: 1px solid #6A6A6A; background-color: #3A3A3A; }"
                "QCheckBox::indicator:checked { border: 1px solid #0E639C; background-color: #0E639C; }"
                "QCheckBox::indicator:unchecked:hover { border: 1px solid #9A9A9A; }"
                "QCheckBox::indicator:checked:hover { background-color: #1177BB; }"
                "QPushButton { background-color: #3A3A3A; color: #D4D4D4; border: 1px solid #4A4A4A; "
                "              padding: 6px 12px; border-radius: 4px; }"
                "QPushButton:hover { background-color: #4A4A4A; border-color: #5A5A5A; }"
                "QPushButton:pressed { background-color: #2A2A2A; }"
                "QScrollArea { background-color: #1E1E1E; border: none; }"
                "QScrollBar:vertical { background: #1E1E1E; width: 10px; margin: 0; }"
                "QScrollBar::handle:vertical { background: #4A4A4A; border-radius: 5px; min-height: 20px; }"
                "QScrollBar::handle:vertical:hover { background: #5A5A5A; }"
                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }");
        QVBoxLayout* mainLayout = new QVBoxLayout(content);
        mainLayout->setContentsMargins(12, 12, 12, 12);
        mainLayout->setSpacing(8);

        QLabel* tip = new QLabel(
                QStringLiteral("选择要合并属性的数据对象（列表顺序即输入顺序，第一个提供输出几何与数据类型）。\n"
                                "要求所选对象的点数和单元数逐项对应；同名的属性会全部保留，"),
                content);
        tip->setWordWrap(true);
        mainLayout->addWidget(tip);

        QHBoxLayout* btnLayout = new QHBoxLayout();
        QPushButton* selectAllBtn = new QPushButton(QStringLiteral("全选"), content);
        QPushButton* deselectAllBtn = new QPushButton(QStringLiteral("全不选"), content);
        btnLayout->addWidget(selectAllBtn);
        btnLayout->addWidget(deselectAllBtn);
        btnLayout->addStretch();
        mainLayout->addLayout(btnLayout);

        QScrollArea* scrollArea = new QScrollArea(content);
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        QWidget* listWidget = new QWidget(scrollArea);
        QVBoxLayout* listLayout = new QVBoxLayout(listWidget);
        listLayout->setSpacing(4);
        listLayout->setContentsMargins(0, 0, 0, 0);
        QList<QCheckBox*> checkBoxes;
        for (const AppendInputInfo& info: infos) {
            QCheckBox* cb = new QCheckBox(QStringLiteral("%1  (点: %2, 单元: %3)")
                                                    .arg(info.name)
                                                    .arg(info.points)
                                                    .arg(info.cells),
                                            listWidget);
            cb->setChecked(true);
            checkBoxes.append(cb);
            listLayout->addWidget(cb);
        }
        listWidget->setLayout(listLayout);
        scrollArea->setWidget(listWidget);
        mainLayout->addWidget(scrollArea);

        QCheckBox* pointCb = new QCheckBox(QStringLiteral("追加点属性 (Point Data)"), content);
        pointCb->setChecked(true);
        QCheckBox* cellCb = new QCheckBox(QStringLiteral("追加单元属性 (Cell Data)"), content);
        cellCb->setChecked(true);
        mainLayout->addWidget(pointCb);
        mainLayout->addWidget(cellCb);

        QDialogButtonBox* buttonBox =
                new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, content);
        mainLayout->addWidget(buttonBox);

        dlg->setContentWidget(content);
        dlg->resize(440, 540);

        connect(selectAllBtn, &QPushButton::clicked, [checkBoxes]() {
            for (QCheckBox* cb: checkBoxes) { cb->setChecked(true); }
        });
        connect(deselectAllBtn, &QPushButton::clicked, [checkBoxes]() {
            for (QCheckBox* cb: checkBoxes) { cb->setChecked(false); }
        });
        connect(buttonBox, &QDialogButtonBox::accepted, this, [this, dlg, checkBoxes, infos, pointCb, cellCb]() {
            std::vector<DataObject::Pointer> selected;
            for (int i = 0; i < checkBoxes.size(); ++i) {
                if (checkBoxes[i]->isChecked()) { selected.push_back(infos[i].object); }
            }
            if (selected.empty()) {
                showDarkFramelessMessage(QStringLiteral("追加属性"), QStringLiteral("请至少选择一个数据对象。"));
                return;
            }
            if (!pointCb->isChecked() && !cellCb->isChecked()) {
                showDarkFramelessMessage(QStringLiteral("追加属性"),
                                            QStringLiteral("请至少勾选「点属性」或「单元属性」之一。"));
                return;
            }

            auto filter = AppendAttributes::New();
            for (const auto& obj: selected) { filter->AddInput(obj); }
            filter->SetAppendPointData(pointCb->isChecked());
            filter->SetAppendCellData(cellCb->isChecked());

            if (!filter->Execute()) {
                showDarkFramelessMessage(
                        QStringLiteral("追加属性"),
                        QStringLiteral("执行失败：请确认所选数据对象的点数与单元数逐项对应。"));
                return;
            }
            auto output = filter->GetOutput();
            if (!output) {
                showDarkFramelessMessage(QStringLiteral("追加属性"), QStringLiteral("算法未产生有效结果。"));
                return;
            }
            output->SetName(selected[0]->GetName() + "_appended");
            modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
            rendererWidget->GetScene()->Modified();
            rendererWidget->GetScene()->Update();
            rendererWidget->update();
            dlg->accept();
        });
        connect(buttonBox, &QDialogButtonBox::rejected, dlg, &QDialog::reject);

        dlg->show();
    });

    // 9.28 新增 filter 统一追加到“开发中filter/第二批”菜单末尾。
    QAction* integrateVariables =
            developingFiltersBatch2->addAction(QStringLiteral("变量积分 (Integrate Variables)"));
    integrateVariables->setObjectName(QStringLiteral("action_developing_integrate_variables"));
    integrateVariables->setData(QStringLiteral("integrate_variables"));
    connect(integrateVariables, &QAction::triggered, this, [this](bool) {
        auto* scene = rendererWidget->GetScene();
        auto model = scene ? scene->GetCurrentModel() : nullptr;
        auto data = model ? model->GetDataObject() : nullptr;
        if (data == nullptr) {
            showDarkFramelessMessage(QStringLiteral("No Model Available"),
                                     QStringLiteral("Please load and select a model first."));
            return;
        }
        const auto dtype = data->GetDataObjectType();
        if (dtype != IG_SURFACE_MESH && dtype != IG_VOLUME_MESH &&
            dtype != IG_UNSTRUCTURED_MESH && dtype != IG_STRUCTURED_MESH) {
            showDarkFramelessMessage(
                    QStringLiteral("Unsupported Model"),
                    QStringLiteral("Integrate Variables supports surface, volume, unstructured, and structured meshes."));
            return;
        }

        auto* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("积分变量 (Integrate Variables)"));
        dialog->setFilterDescription(QStringLiteral(
                "对最高维度单元的点属性和单元属性进行积分。输出为一个点，并保存积分结果及总长度、总面积或总体积。"));

        dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX,
                             QStringLiteral("积分策略"),
                             std::vector<QString>{QStringLiteral("Linear Strategy")});
        const int divideCellDataId = dialog->addParameter(
                igQtFilterDialogDockWidget::QT_CHECK_BOX,
                QStringLiteral("单元数据除以总测度"),
                QStringLiteral("false"));

        dialog->setApplyFunctor([this, dialog, data, divideCellDataId]() {
            bool optionOk = false;
            const bool divideCellData = dialog->getChecked(divideCellDataId, optionOk);
            if (!optionOk) {
                showDarkFramelessMessage(QStringLiteral("Warning"),
                                         QStringLiteral("无法读取积分选项。"));
                return;
            }

            IntegrateVariablesFilter::Pointer filter = IntegrateVariablesFilter::New();
            filter->SetInput(data);
            filter->SetDivideAllCellDataByMeasure(divideCellData);
            if (!filter->Execute()) {
                std::string message = filter->GetMessage();
                if (message.empty()) message = "IntegrateVariablesFilter execute failed";
                showDarkFramelessMessage(QStringLiteral("Warning"),
                                         QString::fromStdString(message));
                return;
            }

            auto output = filter->GetOutput();
            if (!output) {
                showDarkFramelessMessage(QStringLiteral("Warning"),
                                         QStringLiteral("积分算法没有生成有效输出。"));
                return;
            }

            modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
            if (auto item = modelTreeWidget->getItemFromObject(output)) item->setExpanded(true);
            rendererWidget->update();
            dialog->hide();
            dialog->deleteLater();

            ui->dockWidget_SearchInfo->show();
            ui->dockWidget_SearchInfo->raise();
            ui->widget_SearchInfo->setCurrentModel(rendererWidget->GetScene()->GetCurrentModel());
        });
        dialog->resize(480, 320);
        dialog->show();
    });
    QAction* decimatePolylineAction =
            developingFiltersBatch2->addAction(QStringLiteral("折线简化 (Decimate Polyline)"));

    decimatePolylineAction->setObjectName(QStringLiteral("action_developing_decimate_polyline"));
    decimatePolylineAction->setData(QStringLiteral("decimate_polyline"));
    connect(decimatePolylineAction, &QAction::triggered, this, [this](bool) {
                auto scene = rendererWidget ? rendererWidget->GetScene() : nullptr;
                auto currentModel = scene ? scene->GetCurrentModel() : nullptr;
                auto input = currentModel ? currentModel->GetDataObject() : nullptr;
                if (!DecimatePolylineFilter::CanProcessInput(input)) {
                    showDarkFramelessMessage(
                            QStringLiteral("折线简化失败"),
                            QStringLiteral("当前数据不支持该算法。"));
                    return;
                }

                auto* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(QStringLiteral("折线简化 (Decimate Polyline)"));
                dialog->setMinimumWidth(520);
                dialog->setFilterDescription(
                        QStringLiteral("与 ParaView 一致，支持角度、自定义点字段和距离三种策略；"
                                       "每条折线独立简化，端点始终保留。"));
                const int reductionId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT,
                        QStringLiteral("目标缩减率 (0..1)"), QStringLiteral("0.9"));
                const int maximumErrorId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT,
                        QStringLiteral("最大误差 (Maximum Error)"),
                        QStringLiteral("1.7976931348623157e+308"));
                const int strategyId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_COMBO_BOX,
                        QStringLiteral("简化策略 (Decimation Strategy)"),
                        std::vector<QString>{QStringLiteral("角度策略 (Angle)"),
                                             QStringLiteral("自定义字段策略 (Custom Field)"),
                                             QStringLiteral("距离策略 (Distance)")});
                std::vector<QString> customFieldNames;
                auto inputPointSet = DynamicCast<PointSet>(input);
                auto attributes = input->GetAttributeSet();
                auto allAttributes = attributes ? attributes->GetAllAttributes() : nullptr;
                if (allAttributes) {
                    for (IGsize i = 0; i < allAttributes->GetNumberOfElements(); ++i) {
                        const auto& attribute = allAttributes->GetElement(i);
                        if (attribute.isDeleted || attribute.attachmentType != IG_POINT ||
                            !attribute.pointer || attribute.pointer->GetDimension() < 1 ||
                            attribute.pointer->GetName().empty() ||
                            !inputPointSet || attribute.pointer->GetNumberOfElements() <
                                                      inputPointSet->GetNumberOfPoints()) {
                            continue;
                        }
                        switch (attribute.pointer->GetArrayType()) {
                        case IG_FloatArray:
                        case IG_DoubleArray:
                        case IG_IntArray:
                        case IG_UnsignedIntArray:
                        case IG_CharArray:
                        case IG_UnsignedCharArray:
                        case IG_ShortArray:
                        case IG_UnsignedShortArray:
                        case IG_LongLongArray:
                        case IG_UnsignedLongLongArray:
                            customFieldNames.push_back(
                                    QString::fromStdString(attribute.pointer->GetName()));
                            break;
                        default: break;
                        }
                    }
                }
                const int customFieldId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_COMBO_BOX,
                        QStringLiteral("点字段 (Field Name)"),
                        customFieldNames.empty()
                                ? std::vector<QString>{QStringLiteral("<无可用点字段>")}
                                : customFieldNames);
                auto* strategyCombo = qobject_cast<QComboBox*>(dialog->getWidget(strategyId));
                auto* customFieldCombo = qobject_cast<QComboBox*>(dialog->getWidget(customFieldId));
                if (strategyCombo) strategyCombo->setCurrentIndex(2);

                // ParaView exposes FieldName as a child property of the custom-field
                // strategy. Keep the whole row hidden for Angle/Distance and reveal it
                // only when Custom Field is selected.
                auto findRowLabel = [](QWidget* value) -> QLabel* {
                    if (!value || !value->parentWidget()) return nullptr;
                    const auto grids = value->parentWidget()->findChildren<QGridLayout*>();
                    for (auto* grid : grids) {
                        const int itemIndex = grid->indexOf(value);
                        if (itemIndex < 0) continue;
                        int row = 0, column = 0, rowSpan = 0, columnSpan = 0;
                        grid->getItemPosition(itemIndex, &row, &column, &rowSpan, &columnSpan);
                        for (int c = 0; c < grid->columnCount(); ++c) {
                            auto* item = grid->itemAtPosition(row, c);
                            if (!item || item->widget() == value) continue;
                            if (auto* label = qobject_cast<QLabel*>(item->widget())) return label;
                        }
                    }
                    return nullptr;
                };
                auto* customFieldLabel = findRowLabel(customFieldCombo);
                auto setCustomFieldVisible = [customFieldCombo, customFieldLabel](bool visible) {
                    if (customFieldLabel) customFieldLabel->setVisible(visible);
                    if (customFieldCombo) customFieldCombo->setVisible(visible);
                };
                setCustomFieldVisible(false);
                if (strategyCombo && customFieldCombo) {
                    connect(strategyCombo, qOverload<int>(&QComboBox::currentIndexChanged), dialog,
                            [setCustomFieldVisible](int index) {
                                setCustomFieldVisible(index == 1);
                            });
                }
                dialog->show();

                dialog->setApplyFunctor([=, this]() {
                    bool reductionOk = false;
                    bool maximumErrorOk = false;
                    bool strategyOk = false;
                    const double targetReduction = dialog->getDouble(reductionId, reductionOk);
                    const double maximumError = dialog->getDouble(maximumErrorId, maximumErrorOk);
                    const int strategy = dialog->getComboIndex(strategyId, strategyOk);
                    if (!reductionOk || !std::isfinite(targetReduction) ||
                        targetReduction < 0.0 || targetReduction > 1.0) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("目标缩减率必须是 0 到 1 之间的数值。"));
                        return;
                    }
                    if (!maximumErrorOk || !std::isfinite(maximumError) || maximumError < 0.0) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("最大误差必须是非负数值。"));
                        return;
                    }
                    if (!strategyOk || strategy < 0 || strategy > 2) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("请选择有效的简化策略。"));
                        return;
                    }
                    if (strategy == 1 && customFieldNames.empty()) {
                        showDarkFramelessMessage(
                                QStringLiteral("参数错误"),
                                QStringLiteral("自定义字段策略需要至少一个数值型点数据数组。"));
                        return;
                    }

                    auto filter = DecimatePolylineFilter::New();
                    filter->SetInput(input);
                    filter->SetTargetReduction(targetReduction);
                    filter->SetMaximumError(maximumError);
                    if (strategy == 0) {
                        filter->SetDecimationStrategy(
                                DecimatePolylineFilter::DecimationStrategy::Angle);
                    } else if (strategy == 1) {
                        filter->SetDecimationStrategy(
                                DecimatePolylineFilter::DecimationStrategy::CustomField);
                        if (customFieldCombo) {
                            filter->SetCustomFieldName(
                                    customFieldCombo->currentText().toStdString());
                        }
                    } else {
                        filter->SetDecimationStrategy(
                                DecimatePolylineFilter::DecimationStrategy::Distance);
                    }
                    if (!filter->Execute()) {
                        showDarkFramelessMessage(
                                QStringLiteral("执行出错"),
                                QStringLiteral("Poly Data 必须包含显式 LINES；自定义字段策略还要求"
                                               "选择有效的数值型点数据数组。"));
                        return;
                    }

                    auto output = filter->GetOutput();
                    output->SetName(input->GetName() + "_DecimatePolyline");
                    if (auto outputDrawObject = DynamicCast<DrawObject>(output)) {
                        // A line-only result is clearest when both retained vertices and segments are shown.
                        outputDrawObject->SetViewStyle(IG_POINTS | IG_WIREFRAME);
                        outputDrawObject->SetPointSize(7.0f);
                        outputDrawObject->SetLineWidth(3.0f);
                    }
                    // ParaView hides the source when a filter result is applied. The eye icon can
                    // still be used to show it again for an overlay comparison.
                    if (auto* inputItem = modelTreeWidget->getItemFromObject(input)) {
                        inputItem->changeVisibility(false);
                    }
                    modelTreeWidget->addDataObjectToModelTree(output, ItemSource::Algorithm);
                    rendererWidget->update();
                    dialog->close();
                });
            });
    // ---------- 管道生成 (Tube)：追加在“开发中filter/第二批”菜单末尾 ----------
    QAction* tubeAction = developingFiltersBatch2->addAction(QStringLiteral("管道生成 (Tube)"));
    connect(tubeAction, &QAction::triggered, this,
            [this](bool checked) {
                auto* scene = rendererWidget->GetScene();
                if (scene == nullptr || scene->GetCurrentModel() == nullptr) {
                    showDarkFramelessMessage(QStringLiteral("无可用模型"),
                                             QStringLiteral("请先加载并选择一个网格模型。"));
                    return;
                }
                auto model = scene->GetCurrentModel();
                auto obj = model->GetDataObject();
                if (obj == nullptr) {
                    showDarkFramelessMessage(QStringLiteral("无可用数据"),
                                             QStringLiteral("当前模型没有可用的网格数据。"));
                    return;
                }

                igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(QStringLiteral("管道生成 (Tube)"));
                dialog->setFilterDescription(QStringLiteral(
                    "将线段（如流线、折线）沿路径扫掠成圆管。<br>"
                    "在每个路径点上确定垂直于前进方向的平面，画一个由半径和边数决定的正多边形截面，"
                    "再沿路径对齐、缝合。<br>"
                    "截面朝向通过平行传递延续，不会翻转；首尾可用端面封闭。"));

                const int radiusId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT,
                        QStringLiteral("半径 (Radius)"), "0.1");
                const int sidesId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT,
                        QStringLiteral("边数 (Number Of Sides)"), "6");
                const int capId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_CHECK_BOX,
                        QStringLiteral("端面封端 (Capping)"), "true");
                const int useDefId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_CHECK_BOX,
                        QStringLiteral("手动指定初始法向 (Use Default Normal)"), "false");
                const int nxId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT,
                        QStringLiteral("初始法向 X (Normal X)"), "0");
                const int nyId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT,
                        QStringLiteral("初始法向 Y (Normal Y)"), "0");
                const int nzId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT,
                        QStringLiteral("初始法向 Z (Normal Z)"), "1");

                // 加宽面板，避免说明文字与标签过度换行
                dialog->setFixedWidth(660);

                // “手动指定初始法向”勾选后，下方三个初始法向输入框才可用（默认置灰）
                auto* useDefCheck = qobject_cast<QCheckBox*>(dialog->getWidget(useDefId));
                QWidget* nxEdit = dialog->getWidget(nxId);
                QWidget* nyEdit = dialog->getWidget(nyId);
                QWidget* nzEdit = dialog->getWidget(nzId);
                auto syncNormalEditors = [nxEdit, nyEdit, nzEdit](bool on) {
                    nxEdit->setEnabled(on);
                    nyEdit->setEnabled(on);
                    nzEdit->setEnabled(on);
                };
                connect(useDefCheck, &QCheckBox::toggled, dialog, syncNormalEditors);
                syncNormalEditors(false);
                useDefCheck->setToolTip(QStringLiteral(
                    "勾选后，使用下方“初始法向 X/Y/Z”指定的方向确定第一个截面的朝向；"
                    "不勾选（默认）则由程序自动选择（管子起始竖直时也会自动换轴）。"));
                const QString normalEditTip = QStringLiteral(
                    "仅在勾选上方“手动指定初始法向”后生效，用于确定第一个截面的初始朝向。");
                nxEdit->setToolTip(normalEditTip);
                nyEdit->setToolTip(normalEditTip);
                nzEdit->setToolTip(normalEditTip);

                dialog->setApplyFunctor([this, dialog, obj, radiusId, sidesId,
                                        capId, useDefId, nxId, nyId, nzId]() {
                    bool rok = false, sok = false,
                         cok = false, uok = false,
                         xok = false, yok = false, zok = false;

                    double radius = dialog->getDouble(radiusId, rok);
                    int sides = dialog->getInt(sidesId, sok);
                    bool capping = dialog->getChecked(capId, cok);
                    bool useDef = dialog->getChecked(useDefId, uok);
                    double nx = dialog->getDouble(nxId, xok);
                    double ny = dialog->getDouble(nyId, yok);
                    double nz = dialog->getDouble(nzId, zok);

                    if (!rok || radius <= 0.0) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("半径必须是大于 0 的数字。"));
                        return;
                    }
                    if (!sok || sides < 3) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("边数必须是不小于 3 的整数。"));
                        return;
                    }

                    TubeFilter::Pointer filter = TubeFilter::New();
                    filter->SetInput(obj);
                    filter->SetRadius(radius);
                    filter->SetNumberOfSides(sides);
                    filter->SetCapping(cok ? capping : true);
                    filter->SetUseDefaultNormal(uok ? useDef : false);
                    if (xok && yok && zok)
                        filter->SetDefaultNormal(Vector3d(nx, ny, nz));

                    if (!filter->Execute()) {
                        showDarkFramelessMessage(
                                QStringLiteral("数据类型不匹配"),
                                QStringLiteral("管道生成仅支持含线段（IG_LINE / IG_POLY_LINE）的"
                                               "非结构网格或表面网格，请检查输入数据类型。"));
                        return;
                    }

                    auto outMesh = DynamicCast<SurfaceMesh>(filter->GetOutput());
                    modelTreeWidget->addDataObjectToModelTree(outMesh, Algorithm);
                    rendererWidget->update();
                    // 先弹模态提示（此时参数面板仍在、主窗口布局稳定），
                    // 用户点“确定”返回后再关闭面板，避免先 close dock 触发布局
                    // 重建、紧接着开模态对话框而导致 Qt 访问违例。
                    showDarkFramelessMessage(QStringLiteral("管道生成完成"),
                                             QStringLiteral("已将线段转换为圆管，可在模型树中查看输出结果。"),
                                             true);
                    dialog->close();
                });
                dialog->show();
            });

    // ---------- 中值滤波 (Median)：追加在“开发中filter/第二批”菜单末尾 ----------
    QAction* medianFilterAction = developingFiltersBatch2->addAction(QStringLiteral("中值滤波 (Median)"));
    connect(medianFilterAction, &QAction::triggered, this, [this](bool) {
        openLeftToolPanel(LeftToolPanelId::MedianFilter);
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        if (!scene) return;
        auto CurrentModel = scene->GetCurrentModel();
        if (!CurrentModel) return;
        auto dataObject = CurrentModel->GetDataObject();
        if (!dataObject) return;
        ui->widget_MedianFilter->SetOriginDataObject(dataObject);
    });
    // ---------- 三角面线性细分 (Subdivide)：追加在“开发中filter/第二批”菜单末尾 ----------
    QAction* subdivideAction = developingFiltersBatch2->addAction(QStringLiteral("三角面细分 (Subdivide)"));
    connect(subdivideAction, &QAction::triggered, this,
            [this](bool checked) {
                auto* scene = rendererWidget->GetScene();
                if (scene == nullptr || scene->GetCurrentModel() == nullptr) {
                    showDarkFramelessMessage(QStringLiteral("无可用模型"),
                                             QStringLiteral("请先加载并选择一个网格模型。"));
                    return;
                }
                auto model = scene->GetCurrentModel();
                auto obj = model->GetDataObject();
                if (obj == nullptr) {
                    showDarkFramelessMessage(QStringLiteral("无可用数据"),
                                             QStringLiteral("当前模型没有可用的网格数据。"));
                    return;
                }

                igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(QStringLiteral("三角面细分 (Subdivide)"));
                int subId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                 QStringLiteral("细分次数 (1~8，每次三角形数量 ×4)"), "1");
                dialog->setApplyFunctor([=, this]() {
                    bool ok = false;
                    int tms = dialog->getInt(subId, ok);
                    if (!ok || tms < 1 || tms > 8) {
                        showDarkFramelessMessage(QStringLiteral("Warning"),
                                                 QStringLiteral("请输入 1 ~ 8 之间的整数细分次数。"));
                        return;
                    }

                    auto filter = SubdivideFilter::New();
                    filter->SetNumberOfSubdivisions(tms);
                    filter->SetInput(obj);
                    if (!filter->Execute()) {
                        showDarkFramelessMessage(
                                QStringLiteral("数据类型不匹配"),
                                QStringLiteral("三角面细分仅支持多边形表面网格（Poly Data），请检查输入数据类型。"));
                        return;
                    }

                    auto outMesh = DynamicCast<SurfaceMesh>(filter->GetOutput());
                    modelTreeWidget->addDataObjectToModelTree(outMesh, Algorithm);
                    rendererWidget->update();
                    // 先弹模态提示（面板仍在、布局稳定），用户点“确定”后再关面板
                    showDarkFramelessMessage(QStringLiteral("三角面细分完成"),
                                             QStringLiteral("已对三角面做线性细分，旧顶点保持不变、边中点为线性插值；"
                                                            "可将显示模式改为 Surface With Edges 查看加密后的网格。"),
                                             true);
                    dialog->close();
                });
                dialog->show();
            });

        // ---------- 按向量变形 (Warp By Vector)：追加在“开发中filter/第二批”菜单末尾 ----------
    QAction* warpByVectorAction = developingFiltersBatch2->addAction(QStringLiteral("按向量变形 (Warp By Vector)"));
    connect(warpByVectorAction, &QAction::triggered, this, [this](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        if (!obj) return;

        // 收集可作为形变向量的数组:优先点关联的三维向量
        std::vector<QString> vectorItems;
        std::vector<std::string> vectorNames;
        if (auto attrs = obj->GetAttributeSet()) {
            for (IGsize i = 0; i < static_cast<IGsize>(attrs->GetNumberOfAttributes()); ++i) {
                auto& attr = attrs->GetAttribute(i);
                if (attr.isDeleted || !attr.pointer) continue;
                if (attr.attachmentType != IG_POINT) continue;
                if (attr.type != IG_VECTOR && attr.pointer->GetDimension() < 3) continue;
                const std::string name = attr.pointer->GetName();
                vectorNames.push_back(name);
                vectorItems.push_back(QStringLiteral("%1 (dim=%2)")
                                              .arg(QString::fromStdString(name))
                                              .arg(attr.pointer->GetDimension()));
            }
        }
        if (vectorItems.empty()) {
            showDarkFramelessMessage(QStringLiteral("Warning"),
                                     QStringLiteral("当前模型没有点关联的三维向量数组，无法进行向量形变。"));
            return;
        }

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("按向量变形 (Warp By Vector)"));
        int vectorId = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX, QStringLiteral("向量 (Vectors)"),
                                            vectorItems);
        int scaleId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                           QStringLiteral("缩放系数 (Scale Factor)"), "1");
        int autoScaleId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                               QStringLiteral("自动缩放(按模型尺度)"), "false");
        int limitId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                           QStringLiteral("最大位移比例 (0 = 不限)"), "0");
        dialog->show();
        dialog->setApplyFunctor([=, this]() {
            bool ok = false;
            const int choice = dialog->getComboIndex(vectorId, ok);
            if (!ok || choice < 0 || choice >= static_cast<int>(vectorNames.size())) {
                showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("请选择有效的向量数组。"));
                return;
            }
            const double scale = dialog->getDouble(scaleId, ok);
            const bool autoScale = dialog->getChecked(autoScaleId, ok);
            const double limit = dialog->getDouble(limitId, ok);

            auto filter = WarpByVectorFilter::New();
            filter->SetInput(obj);
            filter->SetVectorArrayName(vectorNames[static_cast<size_t>(choice)]);
            filter->SetScaleFactor(scale);
            filter->SetAutoScale(autoScale);
            filter->SetMaxDisplacementRatio(limit > 0.0 ? limit : 0.0);
            if (!filter->Execute()) {
                showDarkFramelessMessage(QStringLiteral("Warning"),
                                         QStringLiteral("向量形变失败，请检查向量数组与缩放系数。"));
                return;
            }

            // 输出为独立的新数据对象,原模型保持不变
            auto warpOutput = filter->GetOutput();
            if (!warpOutput) {
                showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("向量形变未产生有效结果。"));
                return;
            }
            warpOutput->SetName(obj->GetName() + "_warp");
            modelTreeWidget->addDataObjectToModelTree(warpOutput, Algorithm);
            rendererWidget->update();
            dialog->close();
        });
    });

    // 反转面朝向 (Reverse Sense)。
    connect(developingFiltersBatch2->addAction(QStringLiteral("反转面朝向 (Reverse Sense)")),
            &QAction::triggered, this, [this](bool) {
                auto scene = rendererWidget ? rendererWidget->GetScene() : nullptr;
                auto currentModel = scene ? scene->GetCurrentModel() : nullptr;
                if (!currentModel) {
                    showDarkFramelessMessage(QStringLiteral("反转面朝向"), QStringLiteral("请先选择一个模型。"));
                    return;
                }
                auto object = currentModel->GetDataObject();
                if (iGame::DynamicCast<iGame::SurfaceMesh>(object).IsNull()) {
                    showDarkFramelessMessage(QStringLiteral("反转面朝向"),
                                             QStringLiteral("当前模型不是曲面网格（SurfaceMesh）"));
                    return;
                }

                auto* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(QStringLiteral("反转面朝向"));
                dialog->setFilterDescription(QStringLiteral("反转面的顶点环序，并可取反法向"));
                const int cellsId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_CHECK_BOX, QStringLiteral("反转面顶点顺序"), "true");
                const int normalsId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_CHECK_BOX, QStringLiteral("反转法向"), "true");
                dialog->setParameterLayoutVertical();
                auto* help = new QLabel(QStringLiteral(
                        "反转面顶点顺序：改变面的朝向。\n"
                        "反转法向：取反已有的点法向和面法向，不生成新法向。"), dialog);
                help->setWordWrap(true);
                help->setContentsMargins(0, 8, 0, 0);
                dialog->addRowWidget(help);
                dialog->setMinimumWidth(420);
                dialog->resize(qMax(460, help->fontMetrics().horizontalAdvance(
                        QStringLiteral("反转法向：取反已有的点法向和面法向")) + 80), 330);
                if (auto* scrollArea = dialog->findChild<QScrollArea*>(QStringLiteral("scrollArea"))) {
                    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                }
                dialog->show();

                dialog->setApplyFunctor([=, this]() {
                    bool ok = false;
                    auto filter = iGame::ReverseSenseFilter::New();
                    filter->SetReverseCells(dialog->getChecked(cellsId, ok));
                    filter->SetReverseNormals(dialog->getChecked(normalsId, ok));
                    filter->SetInput(object);
                    if (!filter->Execute()) {
                        showDarkFramelessMessage(
                                QStringLiteral("反转面朝向"),
                                QStringLiteral("执行失败：%1").arg(QString::fromStdString(filter->GetMessage())));
                        return;
                    }
                    auto output = filter->GetOutput();
                    if (!output) {
                        showDarkFramelessMessage(QStringLiteral("反转面朝向"), QStringLiteral("输出对象为空"));
                        return;
                    }
                    output->SetName(object->GetName() + "_Reversed");

                    auto inputDraw = iGame::DynamicCast<iGame::DrawObject>(object);
                    auto outDraw = iGame::DynamicCast<iGame::DrawObject>(output);
                    if (inputDraw && outDraw) {
                        outDraw->SetViewStyle(static_cast<IGenum>(inputDraw->GetViewStyle()));
                    }

                    modelTreeWidget->addDataObjectToModelTree(output, ItemSource::Algorithm);
                    modelTreeWidget->updateCloudPicture();
                    rendererWidget->update();
                    dialog->close();
                });
            });
    addDevelopmentAction(developingFiltersBatch2, simplifyAction,
                         QStringLiteral("体网格简化 (Volume Mesh Simplification)"));
    // 9.28 布置的第二批 filter：按上游约定追加在「开发中filter/第二批」末尾。
    // 这里只放代理入口，实际行为由「标准过滤器」里同 id 的 QAction 承担
    // （见 igQtMainWindowStandardFilters.cpp 的 filterPanel 懒加载面板）。
    static const DevelopmentEntry batch2Entries[] = {
        {"texture_map_to_cylinder", "圆柱纹理坐标 (Texture Map To Cylinder)"},
    };
    for (const DevelopmentEntry& entry : batch2Entries) {
        addDevelopmentAction(developingFiltersBatch2, approvedFilter(entry.id),
                             QString::fromUtf8(entry.label));
    }
    // ---------- 表面切向量 (Generate Surface Tangents)：追加在“开发中filter/第二批”菜单末尾 ----------
    QAction* generateSurfaceTangentsAction =
            developingFiltersBatch2->addAction(QStringLiteral("表面切向量 (Generate Surface Tangents)"));
    connect(generateSurfaceTangentsAction, &QAction::triggered, this, [this](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        auto data = model->GetDataObject();
        if (data == nullptr || data->GetPoints() == nullptr) {
            showDarkFramelessMessage(QStringLiteral("Warning"),
                                     QStringLiteral("当前模型没有点数据，无法计算表面切向量。"));
            return;
        }

        // 先把模型里的纹理坐标点属性（IG_TCOORD，至少 2 个分量）列出来给用户选
        auto attrs = data->GetAttributeSet();
        std::vector<QString> texLabels;
        std::vector<std::string> texNames;
        if (attrs) {
            for (IGsize i = 0; i < static_cast<IGsize>(attrs->GetNumberOfAttributes()); ++i) {
                auto& attr = attrs->GetAttribute(i);
                if (attr.isDeleted || !attr.pointer) continue;
                if (attr.attachmentType != IG_POINT || attr.type != IG_TCOORD) continue;
                if (attr.pointer->GetDimension() < 2) continue;
                texNames.push_back(attr.pointer->GetName());
                texLabels.push_back(QStringLiteral("%1（%2 个分量）")
                                            .arg(QString::fromStdString(attr.pointer->GetName()))
                                            .arg(attr.pointer->GetDimension()));
            }
        }
        if (texLabels.empty()) {
            showDarkFramelessMessage(
                    QStringLiteral("Warning"),
                    QStringLiteral("当前模型没有纹理坐标（IG_TCOORD 点属性），<br>"
                                   "切向量必须由纹理坐标算出来。<br>"
                                   "请先做一次「球面纹理坐标 (Texture Map To Sphere)」之类的 filter。"));
            return;
        }

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("表面切向量 (Generate Surface Tangents)"));
        int texId =
                dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX, QStringLiteral("纹理坐标"), texLabels);
        int pointId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                           QStringLiteral("输出点切向量 Tangents"), "true");
        int cellId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                          QStringLiteral("输出单元切向量（不归一化）"), "false");
        int bitangentId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                               QStringLiteral("输出副切向量 Bitangents"), "false");
        dialog->setApplyFunctor([=, this]() {
            bool okTex = false, okPoint = false, okCell = false, okBitangent = false;
            const int texIndex = dialog->getComboIndex(texId, okTex);
            const bool computePoint = dialog->getChecked(pointId, okPoint);
            const bool computeCell = dialog->getChecked(cellId, okCell);
            const bool computeBitangent = dialog->getChecked(bitangentId, okBitangent);
            if (!okPoint || !okCell || !okBitangent) {
                showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("参数读取失败"));
                return;
            }
            if (!computePoint && !computeCell && !computeBitangent) {
                showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("至少勾选一种切向量输出"));
                return;
            }

            auto filter = iGame::GenerateSurfaceTangentsFilter::New();
            if (okTex && texIndex >= 0 && texIndex < static_cast<int>(texNames.size())) {
                filter->SetTextureCoordinatesArrayName(texNames[texIndex]);
            }
            filter->SetComputePointTangents(computePoint);
            filter->SetComputeCellTangents(computeCell);
            filter->SetComputeBitangents(computeBitangent);
            filter->SetInput(0, data);
            if (!filter->Execute()) {
                showDarkFramelessMessage(QStringLiteral("Warning"),
                                         QStringLiteral("表面切向量执行失败：<br>"
                                                        "模型需要有纹理坐标，以及三角形 / 四边形 / 多边形单元"
                                                        "（体网格请先「转换为表面网格」）。<br>"
                                                        "细节见日志。"));
                return;
            }
            auto outObj = filter->GetOutput(); // 独立输出，原模型不变
            if (auto drawObject = iGame::DynamicCast<iGame::DrawObject>(outObj)) {
                drawObject->ForceReConvertToDrawableData();
            }
            modelTreeWidget->addDataObjectToModelTree(outObj, Algorithm);
            // 把新节点选成当前模型
            if (auto item = modelTreeWidget->getItemFromObject(outObj)) { modelTreeWidget->setCurrentItem(item); }
            rendererWidget->update();
            showDarkFramelessMessage(
                    QStringLiteral("表面切向量"),
                    QStringLiteral("已生成点属性 Tangents（单位向量，方向沿纹理坐标 u 增大的方向）。<br>"), true);
            dialog->close();
        });
        dialog->show();
    });

    // ===== GroupDatasets: 合并数据集（多选合组，对齐 ParaView Group Datasets）=====
    // 注册到「新增filter/第二批」菜单末尾（与其它新增 filter 一致）。
    QAction* groupDatasetsAction = developingFiltersBatch2->addAction(QStringLiteral("合并数据集 (Group Datasets)"));
    connect(groupDatasetsAction, &QAction::triggered, this,
            [this](bool) {
                // 1) 读取模型树中当前选中的数据对象（支持多选，也支持多块装配体的子块）
                auto inputs = modelTreeWidget->GetSelectedDataObjects();
                if (inputs.empty()) {
                    showDarkFramelessMessage(
                            QStringLiteral("合并数据集"),
                            QStringLiteral("请先在左侧模型树中选中对象。\n\n"
                                           "• 多选：按住 Ctrl 或 Shift 点击\n"
                                           "• 支持选中多块装配体的子块"));
                    return;
                }

                // 2) 唯一命名。必须在注册进场景【之前】算，因为它扫描的是场景的模型列表。
                const std::string groupName = UniqueGroupDatasetsName();

                // 3) 执行组合（只做容器组合，不合并几何）
                auto filter = iGame::GroupDatasetsFilter::New();
                filter->SetInputs(inputs);
                filter->SetOutputName(groupName);
                if (!filter->Execute()) {
                    showDarkFramelessMessage(QStringLiteral("合并数据集失败"),
                                             QString::fromStdString(filter->GetMessage()));
                    return;
                }

                auto group = filter->GetOutput(0);
                if (!group) {
                    showDarkFramelessMessage(QStringLiteral("合并数据集失败"),
                                             QStringLiteral("未生成结果节点。"));
                    return;
                }

                // 4) 挂到场景 + 模型树。
                //    注意：addDataObjectToModelTree 内部已经调用 scene->AddModel()，
                //    这里不要再单独 AddModel，否则会重复注册出冗余节点。
                modelTreeWidget->addDataObjectToModelTree(group, ItemSource::Algorithm);

                // 5) 【不隐藏原始输入】。
                //
                //    组的每个子块都是原模型的【深拷贝克隆】（几何 / 属性 / 显隐样式各持一份），
                //    隐藏原模型不会影响组的内容，隐藏组内的块也不会影响原模型 —— 新旧完全独立。
                //    因此原模型保持原状即可；这里仅刷新全树眼睛图标，让每处图标都反映
                //    各自对象的真实显隐状态。
                modelTreeWidget->RefreshModelTreeVisibilityIcons();

                rendererWidget->update();

                // 6) 汇报结果（子块按序号列出，重名也能区分）
                QString blockList;
                const auto& blockNames = filter->GetBlockNames();
                for (size_t i = 0; i < blockNames.size(); ++i) {
                    blockList += QStringLiteral("%1. %2\n")
                                         .arg(static_cast<int>(i) + 1)
                                         .arg(QString::fromStdString(blockNames[i]));
                }
                const QString report = QStringLiteral("已合并 %1 个对象 → %2\n%3")
                                               .arg(filter->GetGroupedCount())
                                               .arg(QString::fromStdString(groupName))
                                               .arg(blockList);
                showDarkFramelessMessage(QStringLiteral("合并数据集"), report, true);
            });

    // ---------- 时域统计 (Temporal Statistics) ----------
    QAction* temporalStatisticsAction =
            developingFiltersBatch2->addAction(QStringLiteral("时域统计 (Temporal Statistics)"));
    temporalStatisticsAction->setData(QStringLiteral("temporal_statistics"));
    connect(temporalStatisticsAction, &QAction::triggered, this, [this](bool) {
        const QString title = QStringLiteral("时域统计");
        auto scene = rendererWidget->GetScene();
        auto currentModel = scene ? scene->GetCurrentModel() : nullptr;
        if (!currentModel) {
            showDarkFramelessMessage(title, QStringLiteral("请先在模型树中选择一个模型。"));
            return;
        }
        auto obj = currentModel->GetDataObject();
        if (!obj) {
            showDarkFramelessMessage(title, QStringLiteral("当前模型没有可用的数据对象。"));
            return;
        }

        // 首次打开：懒创建**左侧**参数面板（勾选要输出的统计量，形态同「重采样到图像」）
        if (m_temporalStatisticsDock == nullptr) {
            m_temporalStatisticsDock = igQtTemporalStatisticsWidget::createDockWidget(this);
            addDockWidget(Qt::LeftDockWidgetArea, m_temporalStatisticsDock);
            auto* panel = qobject_cast<igQtTemporalStatisticsWidget*>(m_temporalStatisticsDock->widget());
            connect(panel, &igQtTemporalStatisticsWidget::resultReady, this,
                    [this, obj](DataObject::Pointer output) {
                        if (!output) { return; }
                        output->SetName(obj->GetName() + "_temporal_statistics");
                        modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
                        // 选中第一个统计数组，便于直接着色查看
                        if (auto attributeSet = output->GetAttributeSet()) {
                            if (attributeSet->GetNumberOfAttributes() > 0) {
                                if (auto* item = modelTreeWidget->getItemFromObject(output)) {
                                    item->setExpanded(true);
                                    if (item->childCount() > 0) {
                                        if (auto* child = item->child(0)) {
                                            item->setCurrentChild(child);
                                            item->setSelected(false);
                                            item->viewAttribute(0, -1);
                                            child->setSelected(true);
                                            modelTreeWidget->setCurrentItem(child);
                                        }
                                    }
                                }
                            }
                        }
                        modelTreeWidget->updateCloudPicture();
                        rendererWidget->update();
                    }
            );
            connect(panel, &igQtTemporalStatisticsWidget::closeRequested, m_temporalStatisticsDock,
                    &QDockWidget::hide);
        }
        auto* panel = qobject_cast<igQtTemporalStatisticsWidget*>(m_temporalStatisticsDock->widget());
        if (panel != nullptr) { panel->setCurrentModel(currentModel); }
        m_temporalStatisticsDock->show();
        m_temporalStatisticsDock->raise();
    });
    // ---------- Perlin 噪声 (Perlin Noise)：追加在“新增filter/第二批”菜单末尾 ----------
    connect(developingFiltersBatch2->addAction(QStringLiteral("Perlin 噪声 (Perlin Noise)")),
            &QAction::triggered, this, [this](bool) {
                auto scene = rendererWidget ? rendererWidget->GetScene() : nullptr;
                auto currentModel = scene ? scene->GetCurrentModel() : nullptr;
                if (!currentModel) {
                    showDarkFramelessMessage(QStringLiteral("Perlin 噪声"), QStringLiteral("请先选择一个模型。"));
                    return;
                }
                auto object = currentModel->GetDataObject();
                if (object == nullptr || iGame::DynamicCast<iGame::PointSet>(object).IsNull()) {
                    showDarkFramelessMessage(QStringLiteral("Perlin 噪声"),
                                             QStringLiteral("当前对象不支持 Perlin 噪声（需要网格 / 点集）"));
                    return;
                }

                auto* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(QStringLiteral("Perlin 噪声"));
                dialog->setFilterDescription(
                        QStringLiteral("在每个输入点上采样 3D Perlin 噪声，输出新模型 + 点标量数组"));
                const int ampId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                       QStringLiteral("振幅 (Amplitude)"), "1");
                const int fxId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("频率 X (Frequency X)"), "1");
                const int fyId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("频率 Y (Frequency Y)"), "1");
                const int fzId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("频率 Z (Frequency Z)"), "1");
                const int pxId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("相位 X (Phase X)"), "0");
                const int pyId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("相位 Y (Phase Y)"), "0");
                const int pzId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("相位 Z (Phase Z)"), "0");
                dialog->show();
                dialog->setApplyFunctor([=, this]() {
                    const int ids[7] = {ampId, fxId, fyId, fzId, pxId, pyId, pzId};
                    const char* names[7] = {"振幅 (Amplitude)", "频率 X (Frequency X)",
                                            "频率 Y (Frequency Y)", "频率 Z (Frequency Z)",
                                            "相位 X (Phase X)", "相位 Y (Phase Y)",
                                            "相位 Z (Phase Z)"};
                    double values[7] = {0.0};
                    bool ok = false;
                    for (int i = 0; i < 7; ++i) {
                        values[i] = dialog->getDouble(ids[i], ok);
                        if (!ok) {
                            showDarkFramelessMessage(
                                    QStringLiteral("错误"),
                                    QString::fromUtf8(names[i]) + QStringLiteral(" 不是有效数字。"));
                            return;
                        }
                    }

                    auto filter = PerlinNoiseFilter::New();
                    filter->SetAmplitude(values[0]);
                    filter->SetFrequency(values[1], values[2], values[3]);
                    filter->SetPhase(values[4], values[5], values[6]);
                    filter->SetInput(object);
                    if (filter->Execute()) {
                        modelTreeWidget->addDataObjectToModelTree(filter->GetOutput(), ItemSource::Algorithm);
                        modelTreeWidget->updateCloudPicture();
                        rendererWidget->update();
                        dialog->close();
                    } else {
                        showDarkFramelessMessage(QStringLiteral("错误"),
                                                 QStringLiteral("Perlin 噪声生成失败（需要网格 / 点集）"));
                    }
                });
            });

    QAction* textureMapToPlaneAction = developingFiltersBatch2->addAction(
            QStringLiteral("生成平面纹理坐标 (Texture Map to Plane)"));
    textureMapToPlaneAction->setObjectName(QStringLiteral("action_developing_texture_map_to_plane"));
    connect(textureMapToPlaneAction, &QAction::triggered, this, [this]() {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (!model) {
            showDarkFramelessMessage(QStringLiteral("生成平面纹理坐标"),
                                     QStringLiteral("请先在模型树中选择一个包含点数据的网格模型。"));
            return;
        }
        auto* panel = new igQtTextureMapToPlaneWidget(rendererWidget, modelTreeWidget, model, this);
        if (!panel->isReady()) {
            showDarkFramelessMessage(QStringLiteral("生成平面纹理坐标"),
                                     QStringLiteral("当前模型不属于 PointSet 网格体系，或模型中没有点；操作已取消。"));
            panel->deleteLater();
            return;
        }
        panel->show();
        panel->raise();
        panel->activateWindow();
    });
    QAction* textureMapToSphereAction = developingFiltersBatch2->addAction(
            QStringLiteral("球面纹理坐标 (Texture Map To Sphere)"));
    connect(textureMapToSphereAction, &QAction::triggered, this, [this](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        auto data = model->GetDataObject();
        if (data == nullptr || data->GetPoints() == nullptr) {
            showDarkFramelessMessage(QStringLiteral("Warning"),
                                     QStringLiteral("当前模型没有点数据，无法生成球面纹理坐标。"));
            return;
        }

        // 默认值与算法的自动球心一致：所有输入点的坐标平均值。
        double center[3] = {0.0, 0.0, 0.0};
        auto spherePoints = data->GetPoints();
        const IGsize spherePointCount = spherePoints->GetNumberOfPoints();
        for (IGsize i = 0; i < spherePointCount; ++i) {
            const auto& point = spherePoints->GetPoint(i);
            for (int axis = 0; axis < 3; ++axis) { center[axis] += static_cast<double>(point[axis]); }
        }
        if (spherePointCount > 0) {
            for (double& value : center) { value /= static_cast<double>(spherePointCount); }
        }
        const QString defaultX = QString::number(center[0], 'g', 6);
        const QString defaultY = QString::number(center[1], 'g', 6);
        const QString defaultZ = QString::number(center[2], 'g', 6);

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("球面纹理坐标 (Texture Map To Sphere)"));
        int autoCenterId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                                QStringLiteral("自动使用点坐标平均值"), "true");
        int centerXId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("球心 X"),
                                             defaultX);
        int centerYId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("球心 Y"),
                                             defaultY);
        int centerZId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("球心 Z"),
                                             defaultZ);
        int seamId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                          QStringLiteral("防止接缝 (PreventSeam)"), "true");
        dialog->setApplyFunctor([=, this]() {
            bool okAuto = false, okSeam = false;
            const bool autoCenter = dialog->getChecked(autoCenterId, okAuto);
            const bool preventSeam = dialog->getChecked(seamId, okSeam);

            auto filter = iGame::TextureMapToSphereFilter::New();
            filter->SetPreventSeam(preventSeam);
            if (!autoCenter) {
                bool okX = false, okY = false, okZ = false;
                const double cx = dialog->getDouble(centerXId, okX);
                const double cy = dialog->getDouble(centerYId, okY);
                const double cz = dialog->getDouble(centerZId, okZ);
                if (!okX || !okY || !okZ) {
                    showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("球心坐标必须是数字"));
                    return;
                }
                filter->SetCenter(cx, cy, cz);
            }
            filter->SetInput(0, data);
            if (!filter->Execute()) {
                showDarkFramelessMessage(QStringLiteral("Warning"),
                                         QStringLiteral("球面纹理坐标执行失败：不支持的网格类型"));
                return;
            }
            auto outObj = filter->GetOutput(); // 独立输出，原模型不变
            if (auto drawObject = iGame::DynamicCast<iGame::DrawObject>(outObj)) {
                drawObject->ForceReConvertToDrawableData();
            }
            modelTreeWidget->addDataObjectToModelTree(outObj, ItemSource::Algorithm);
            rendererWidget->update();
            showDarkFramelessMessage(QStringLiteral("球面纹理坐标"),
                                     QStringLiteral("已生成点属性 TextureCoordinates（2 分量：s、t）。<br>"
                                                    "可在“查找信息”面板里查看数值。"),
                                     true);
            dialog->close();
        });
        dialog->show();
    });

    QAction* smoothAction = developingFiltersBatch2->addAction(QStringLiteral("表面平滑 (Smooth)"));
    smoothAction->setObjectName(QStringLiteral("action_Smooth"));
    connect(smoothAction, &QAction::triggered, this, showSmoothPanel);

    // 同步当前模型，避免面板继续处理已删除的模型
    connect(modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged, this, [this]() {
        SmoothWidget->setCurrentModel(nullptr);
        QTimer::singleShot(0, this, [this]() {
            auto scene = rendererWidget->GetScene();
            auto model = scene ? scene->GetCurrentModel() : nullptr;
            if (model && modelTreeWidget->getItemFromObject(model->GetDataObject())) {
                SmoothWidget->setCurrentModel(model.get());
            }
        });
    });

    // ===== 属性直方图 Histogram：按需求追加在“开发中filter/第二批”菜单末尾 =====
    // ===== 属性直方图 Histogram（数组选择 + 分箱数；核心算法见 iGameCore/Filters/Histogram）=====
    connect(developingFiltersBatch2->addAction(QStringLiteral("属性直方图 (Histogram)")), &QAction::triggered, this,
            [&](bool checked) {
                if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) {
                    showDarkFramelessMessage(QStringLiteral("提示"), QStringLiteral("请先加载一个模型"));
                    return;
                }
                auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
                if (!obj) return;
                auto attributeSet = obj->GetAttributeSet();
                auto attributes = attributeSet ? attributeSet->GetAllAttributes() : nullptr;
                if (!attributes || attributes->GetNumberOfElements() == 0) {
                    showDarkFramelessMessage(QStringLiteral("提示"),
                                             QStringLiteral("当前模型没有属性数据，无法统计取值分布"));
                    return;
                }

                closeHistogramParameters();
                auto* dialog = new igQtFilterDialogDockWidget(this);
                m_histogramParametersDock = dialog;
                m_histogramParameterContents = dialog->widget();
                dialog->setObjectName(QStringLiteral("HistogramParametersDock"));
                dialog->setAllowedAreas(Qt::LeftDockWidgetArea);
                dialog->setFloating(false);
                dialog->setFeatures(QDockWidget::DockWidgetClosable);
                // Restore the content to its shell before deleting the parameter dialog.
                auto* closeButton = dialog->findChild<QPushButton*>(QStringLiteral("closeButton"));
                disconnect(closeButton, &QPushButton::clicked, dialog, &igQtFilterDialogDockWidget::close);
                connect(closeButton, &QPushButton::clicked, this, &igQtMainWindow::closeHistogramParameters);
                dialog->setFilterTitle(QStringLiteral("属性直方图 (Histogram)"));
                dialog->setFilterDescription(
                        QStringLiteral("统计所选属性数组的取值分布；参数与 ParaView 的 Histogram 过滤器一一对应"));

                // 下拉框条目顺序与属性下标可能因空槽而错位，这里显式保存"条目序号 -> 属性下标"的映射
                std::vector<QString> arrNames;
                std::vector<int> arrIndexes;
                for (igIndex a = 0; a < attributes->GetNumberOfElements(); ++a) {
                    auto& at = attributes->GetElement(a);
                    if (at.isDeleted || at.pointer == nullptr) continue;
                    arrNames.push_back(QStringLiteral("%1: %2")
                                               .arg(at.attachmentType == IG_POINT ? QStringLiteral("点")
                                                                                  : QStringLiteral("单元"))
                                               .arg(QString::fromStdString(at.pointer->GetName())));
                    arrIndexes.push_back(static_cast<int>(a));
                }
                int arrayId = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX,
                                                   QStringLiteral("数组选择"), arrNames);
                // 与 ParaView 一致：面板默认选中「当前正在显示的数组」（DataObject::GetAttributeIndex()），
                // 而不是属性集里的第一个数组。否则属性集首项是单元数组时（例如 c 在 v 前面），
                // 默认统计/绘图的就是那个数组。
                if (auto* arrayCombo = dynamic_cast<QComboBox*>(dialog->getWidget(arrayId)); arrayCombo != nullptr) {
                    const int currentAttribute = obj->GetAttributeIndex();
                    for (size_t row = 0; row < arrIndexes.size(); ++row) {
                        if (arrIndexes[row] == currentAttribute) {
                            arrayCombo->setCurrentIndex(static_cast<int>(row));
                            break;
                        }
                    }
                }
                int binsId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                  QStringLiteral("分箱数"), "10");
                int componentId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                       QStringLiteral("分量"), "0");
                int centerId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                                    QStringLiteral("居中分箱"), "false");
                int useRangeId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                                      QStringLiteral("自定义范围"), "false");
                int rangeMinId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("范围下限"), "0");
                int rangeMaxId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("范围上限"), "1");
                int normalizeId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                                       QStringLiteral("归一化"), "false");
                int averagesId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                                      QStringLiteral("计算均值"), "false");
                openLeftToolPanel(LeftToolPanelId::HistogramParameters);
                resizeDocks({m_leftFieldDock}, {420}, Qt::Horizontal);

                dialog->setApplyFunctor([=, this, previousHistogram = HistogramData::Pointer{}]() mutable {
                    bool okArray = false;
                    int arrayIdx = dialog->getComboIndex(arrayId, okArray);
                    bool okBins = false;
                    int bins = dialog->getInt(binsId, okBins);
                    bool okComponent = false;
                    int component = dialog->getInt(componentId, okComponent);
                    bool okCenter = false, okUseRange = false, okRangeMin = false, okRangeMax = false;
                    bool okNormalize = false, okAverages = false;
                    const bool center = dialog->getChecked(centerId, okCenter);
                    const bool useRange = dialog->getChecked(useRangeId, okUseRange);
                    const double rangeMin = dialog->getDouble(rangeMinId, okRangeMin);
                    const double rangeMax = dialog->getDouble(rangeMaxId, okRangeMax);
                    const bool normalize = dialog->getChecked(normalizeId, okNormalize);
                    const bool calculateAverages = dialog->getChecked(averagesId, okAverages);
                    if (!okArray || !okBins || arrayIdx < 0 || arrayIdx >= static_cast<int>(arrIndexes.size())) {
                        showDarkFramelessMessage(QStringLiteral("提示"), QStringLiteral("请选择数组并填写分箱数"));
                        return;
                    }
                    if (bins < 1 || bins > 10000) {
                        showDarkFramelessMessage(QStringLiteral("提示"),
                                                 QStringLiteral("分箱数请填 1 ~ 10000 之间的整数"));
                        return;
                    }
                    if (!okComponent || component < 0) {
                        showDarkFramelessMessage(QStringLiteral("提示"),
                                                 QStringLiteral("分量请填不小于 0 的整数（等于数组维数时按模长统计）"));
                        return;
                    }
                    if (useRange && (!okRangeMin || !okRangeMax)) {
                        showDarkFramelessMessage(QStringLiteral("提示"),
                                                 QStringLiteral("自定义范围请填写范围下限与范围上限"));
                        return;
                    }
                    auto filter = HistogramFilter::New();
                    filter->SetInput(obj);
                    filter->SetAttributeIndex(arrIndexes[arrayIdx]);
                    filter->SetNumberOfBins(bins);
                    filter->SetComponent(component);
                    filter->SetCenterBinsAroundMinAndMax(center);
                    if (useRange) {
                        filter->SetUseCustomBinRanges(true);
                        filter->SetCustomBinRanges(rangeMin, rangeMax);
                    }
                    filter->SetNormalize(normalize);
                    filter->SetCalculateAverages(calculateAverages);
                    if (!filter->Execute()) {
                        showDarkFramelessMessage(QStringLiteral("警告"), QString::fromStdString(filter->GetMessage()));
                        return;
                    }
                    auto histogram = filter->GetHistogramData();
                    // Input and Histogram outputs share one chart with independent representations.
                    if (m_histogramDock == nullptr) {
                        m_histogramChartWidget = new igQtHistogramChartWidget(this);
                        m_histogramDock = new QDockWidget(this);
                        m_histogramDock->setObjectName(QStringLiteral("dockWidget_Histogram"));
                        m_histogramDock->setWidget(m_histogramChartWidget);
                        m_histogramDock->setAllowedAreas(Qt::RightDockWidgetArea);
                        m_histogramDock->setFeatures(QDockWidget::DockWidgetClosable |
                                                     QDockWidget::DockWidgetMovable |
                                                     QDockWidget::DockWidgetFloatable);
                        this->addDockWidget(Qt::RightDockWidgetArea, m_histogramDock);
                        resizeDocks({m_histogramDock}, {640}, Qt::Horizontal);
                        m_histogramSeriesDock = new QDockWidget(QStringLiteral("直方图系列"), this);
                        m_histogramSeriesDock->setObjectName(QStringLiteral("dockWidget_HistogramSeries"));
                        m_histogramSeriesDock->setAllowedAreas(Qt::LeftDockWidgetArea);
                        m_histogramSeriesDock->setFeatures(QDockWidget::DockWidgetClosable);
                        m_histogramSeriesDock->setWidget(m_histogramChartWidget->settingsWidget());
                        connect(modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged,
                                m_histogramChartWidget, [this]() {
                                    auto model = rendererWidget->GetScene()->GetCurrentModel();
                                    if (model) m_histogramChartWidget->SelectSource(model->GetDataObject().get());
                                });
                    }
                    m_histogramDock->setWindowTitle(
                            QStringLiteral("属性直方图 — %1").arg(QString::fromStdString(obj->GetName())));
                    m_histogramChartWidget->SetHistogram(obj, histogram, arrIndexes[arrayIdx], component,
                                                         previousHistogram.get());
                    previousHistogram = histogram;
                    if (m_histogramDock->isHidden()) { m_histogramDock->show(); }
                    m_histogramDock->raise();
                    openLeftToolPanel(LeftToolPanelId::HistogramSeries);
                    closeHistogramParameters();
                });
            });

    // ===== 追加弧长 AppendArcLength：按需求追加在“开发中filter/第二批”菜单末尾 =====
    QAction* appendArcLength = developingFiltersBatch2->addAction(QStringLiteral("追加弧长 (AppendArcLength)"));
    connect(appendArcLength, &QAction::triggered, this, [this](bool checked) {
        auto* scene = rendererWidget->GetScene();
        auto model = scene ? scene->GetCurrentModel() : nullptr;
        auto data = model ? model->GetDataObject() : nullptr;
        if (data == nullptr) {
            showDarkFramelessMessage(QStringLiteral("No Model Available"),
                QStringLiteral("Please load and select a model first."));
            return;
        }
        AppendArcLengthFilter::Pointer filter = AppendArcLengthFilter::New();
        filter->SetInput(data);
        if (!filter->Execute()) {
            std::string message = filter->GetMessage();
            if (message.empty()) message = "AppendArcLengthFilter execute failed";
            showDarkFramelessMessage(QStringLiteral("Warning"), QString::fromStdString(message));
            return;
        }
        // Register a distinct pipeline result, retaining the original source model.
        auto sourceItem = modelTreeWidget->getItemFromObject(data);
        data = filter->GetOutput();
        static int polylineLengthNumber = 0;
        const QString outputName = QStringLiteral("PolylineLength%1").arg(++polylineLengthNumber);
        data->SetName(outputName.toStdString());
        const int outputModelId = modelTreeWidget->addDataObjectToModelTree(data, ItemSource::Algorithm);
        modelTreeWidget->renameModelRow(data, outputName);
        model = scene->GetModelById(outputModelId);
        if (sourceItem) { sourceItem->changeVisibility(false); }
        auto attributeSet = data->GetAttributeSet();
        int attrIndex = attributeSet ? attributeSet->GetAttributeIndex(AppendArcLengthFilter::GetArrayName()) : -1;
        IGsize valueCount = 0;
        if (attributeSet && attrIndex >= 0) {
            auto& attribute = attributeSet->GetAttribute(attrIndex);
            if (attribute.pointer) { valueCount = attribute.pointer->GetNumberOfValues(); }
        }
        auto item = modelTreeWidget->getItemFromObject(data);
        if (item && attrIndex >= 0) {
            item->setExpanded(true);
            for (int i = 0; i < item->childCount(); ++i) {
                auto child = dynamic_cast<AttribTreeWidgetItem*>(item->child(i));
                if (!child || child->attributeIndex() != attrIndex) { continue; }
                item->setCurrentChild(child);
                item->setSelected(false);
                item->viewAttribute(attrIndex, -1);
                child->setSelected(true);
                modelTreeWidget->setCurrentItem(child);
                break;
            }
        }
        if (ui->dockWidget_SearchInfo && ui->dockWidget_SearchInfo->isVisible()) {
            ui->widget_SearchInfo->setCurrentModel(model);
        }
        showDarkFramelessMessage(QStringLiteral("Success"),
            QStringLiteral("Arc length appended: %1 point(s), %2 polyline(s).")
            .arg(static_cast<qulonglong>(valueCount))
            .arg(static_cast<qulonglong>(filter->GetNumberOfProcessedLines())),
            true);
        });


    // 点平面插值 (Point Plane Interpolator) 
    QAction* pointPlaneInterpolatorAction =
            developingFiltersBatch2->addAction(QStringLiteral("点平面插值 (Point Plane Interpolator)"));
    connect(pointPlaneInterpolatorAction, &QAction::triggered, this, [this](bool) {
        auto* scene = rendererWidget->GetScene();
        auto model = scene ? scene->GetCurrentModel() : nullptr;
        auto input = model ? model->GetDataObject() : nullptr;
        if (!input || !DynamicCast<PointSet>(input)) {
            showDarkFramelessMessage(QStringLiteral("点平面插值"), QStringLiteral("请先加载并选择一个点集或网格。"));
            return;
        }

        // 如果已经存在，彻底销毁旧的，防止底层手柄重叠和状态污染
        for (int i = 0; i < m_leftFieldTabs->count(); ++i) {
            if (m_leftFieldTabs->tabText(i) == QStringLiteral("点平面插值")) {
                QWidget* oldPage = m_leftFieldTabs->widget(i);
                m_leftFieldTabs->removeTab(i);
                delete oldPage;
            }
        }

        // 1. 创建 Tab 容器及滚动区域
        QWidget* page = new QWidget();
        QVBoxLayout* pageLayout = new QVBoxLayout(page);
        pageLayout->setContentsMargins(0, 0, 0, 0);
        pageLayout->setSpacing(0);

        QScrollArea* scroll = new QScrollArea(page);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setStyleSheet("QScrollArea { background: transparent; } QScrollArea > QWidget > QWidget { background: "
                              "transparent; }");

        QWidget* body = new QWidget(scroll);

        // 统一使用基础 QSS 并挂载主题自适应
        QString baseQss =
                "QWidget { background-color: transparent; color: #EAEAEA; font-size: 13px; }"
                "QLabel { color: #D8D8D8; background: transparent; font-size: 13px; padding-top: 4px; }"
                "QLineEdit, QComboBox, QDoubleSpinBox, QSpinBox { background-color: #2A2A2A; color: #EAEAEA; "
                "border: 1px solid #3A3A3A; padding: 3px 6px; border-radius: 3px; font-size: 13px; min-height: 20px; }"
                "QComboBox::drop-down { border: none; background: transparent; width: 20px; }"
                "QComboBox QAbstractItemView { background-color: #2A2A2A; color: #EAEAEA; selection-background-color: "
                "#3A3A3A; }"
                "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button { width: 16px; border: none; background: "
                "transparent; }"
                "QPushButton { background-color: #2A2A2A; color: #EAEAEA; border: 1px solid #3A3A3A; padding: 6px "
                "12px; border-radius: 4px; font-size: 13px; }"
                "QPushButton:hover { background-color: #3A3A3A; }"
                "QGroupBox { border: 1px solid #3A3A3A; border-radius: 4px; margin-top: 8px; padding-top: 12px; }"
                "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 0 5px; color: "
                "#D8D8D8; }";

        body->setStyleSheet(baseQss);
        igQtPanelTheme::attach(body);

        QVBoxLayout* mainLayout = new QVBoxLayout(body);
        mainLayout->setContentsMargins(10, 10, 10, 10);
        mainLayout->setSpacing(10);

        QLabel* inputLabel = new QLabel(QStringLiteral("输入：") + QString::fromStdString(input->GetName()));
        inputLabel->setStyleSheet("color: #FFFFFF; font-weight: bold;");
        inputLabel->setProperty("igPanelBaseQss", inputLabel->styleSheet());
        igQtPanelTheme::refresh(inputLabel);
        mainLayout->addWidget(inputLabel);

        auto bbox = input->GetBoundingBox();
        double cx = (bbox.min[0] + bbox.max[0]) * 0.5;
        double cy = (bbox.min[1] + bbox.max[1]) * 0.5;
        double cz = (bbox.min[2] + bbox.max[2]) * 0.5;

        // ================= 2. 平面参数分组 =================
        QGroupBox* groupPlane = new QGroupBox(QStringLiteral("平面参数"), body);
        QFormLayout* formPlane = new QFormLayout(groupPlane);
        formPlane->setSpacing(6);

        QDoubleSpinBox* originX = new QDoubleSpinBox(groupPlane);
        originX->setRange(-1e9, 1e9);
        originX->setValue(cx);
        originX->setDecimals(4);
        QDoubleSpinBox* originY = new QDoubleSpinBox(groupPlane);
        originY->setRange(-1e9, 1e9);
        originY->setValue(cy);
        originY->setDecimals(4);
        QDoubleSpinBox* originZ = new QDoubleSpinBox(groupPlane);
        originZ->setRange(-1e9, 1e9);
        originZ->setValue(cz);
        originZ->setDecimals(4);

        QDoubleSpinBox* normalX = new QDoubleSpinBox(groupPlane);
        normalX->setRange(-1, 1);
        normalX->setValue(1.0);
        normalX->setDecimals(4);
        QDoubleSpinBox* normalY = new QDoubleSpinBox(groupPlane);
        normalY->setRange(-1, 1);
        normalY->setValue(0.0);
        normalY->setDecimals(4);
        QDoubleSpinBox* normalZ = new QDoubleSpinBox(groupPlane);
        normalZ->setRange(-1, 1);
        normalZ->setValue(0.0);
        normalZ->setDecimals(4);

        QSignalBlocker b1(originX), b2(originY), b3(originZ);
        QSignalBlocker b4(normalX), b5(normalY), b6(normalZ);

        formPlane->addRow(QStringLiteral("原点 X："), originX);
        formPlane->addRow(QStringLiteral("原点 Y："), originY);
        formPlane->addRow(QStringLiteral("原点 Z："), originZ);
        formPlane->addRow(QStringLiteral("法向 X："), normalX);
        formPlane->addRow(QStringLiteral("法向 Y："), normalY);
        formPlane->addRow(QStringLiteral("法向 Z："), normalZ);
        mainLayout->addWidget(groupPlane);

        // ================= 3. 插值设置分组 =================
        QGroupBox* groupInterp = new QGroupBox(QStringLiteral("插值设置"), body);
        QFormLayout* formInterp = new QFormLayout(groupInterp);
        formInterp->setSpacing(6);

        QSpinBox* res = new QSpinBox(groupInterp);
        res->setRange(2, 1024);
        res->setValue(100);
        formInterp->addRow(QStringLiteral("分辨率："), res);

        QComboBox* kernel = new QComboBox(groupInterp);
        kernel->addItems({QStringLiteral("Voronoi"), QStringLiteral("Gaussian"), QStringLiteral("Shepard")});
        formInterp->addRow(QStringLiteral("插值核："), kernel);

        QComboBox* footprint = new QComboBox(groupInterp);
        footprint->addItems({QStringLiteral("Radius"), QStringLiteral("N Closest")});
        formInterp->addRow(QStringLiteral("邻域搜索："), footprint);

        QDoubleSpinBox* radius = new QDoubleSpinBox(groupInterp);
        radius->setRange(0.0001, 1e6);
        radius->setValue(std::max(0.001, static_cast<double>(bbox.diag()) / 10));
        formInterp->addRow(QStringLiteral("半径："), radius);

        QSpinBox* nClosest = new QSpinBox(groupInterp);
        nClosest->setRange(1, 1000);
        nClosest->setValue(8);
        formInterp->addRow(QStringLiteral("最近邻点数："), nClosest);

        QDoubleSpinBox* sharpness = new QDoubleSpinBox(groupInterp);
        sharpness->setRange(0.0001, 100);
        sharpness->setValue(2.0);
        formInterp->addRow(QStringLiteral("高斯锐度："), sharpness);

        QDoubleSpinBox* power = new QDoubleSpinBox(groupInterp);
        power->setRange(0.0001, 100);
        power->setValue(2.0);
        formInterp->addRow(QStringLiteral("Shepard幂次："), power);

        QComboBox* nullStrategy = new QComboBox(groupInterp);
        nullStrategy->addItems({QStringLiteral("掩码"), QStringLiteral("空值"), QStringLiteral("最近点")});
        nullStrategy->setCurrentIndex(2);
        formInterp->addRow(QStringLiteral("空邻域："), nullStrategy);
        mainLayout->addWidget(groupInterp);

        // ================= 4. 动态显隐逻辑 =================
        auto updateUIState = [=]() {
            int k = kernel->currentIndex();
            int f = footprint->currentIndex();
            bool isVoronoi = (k == 0), isGaussian = (k == 1), isShepard = (k == 2);

            if (auto* l = formInterp->labelForField(footprint)) l->setVisible(!isVoronoi);
            footprint->setVisible(!isVoronoi);

            bool showRadius = !isVoronoi && (f == 0), showNClosest = !isVoronoi && (f == 1);
            if (auto* l = formInterp->labelForField(radius)) l->setVisible(showRadius);
            radius->setVisible(showRadius);
            if (auto* l = formInterp->labelForField(nClosest)) l->setVisible(showNClosest);
            nClosest->setVisible(showNClosest);

            if (auto* l = formInterp->labelForField(sharpness)) l->setVisible(isGaussian);
            sharpness->setVisible(isGaussian);
            if (auto* l = formInterp->labelForField(power)) l->setVisible(isShepard);
            power->setVisible(isShepard);
        };
        connect(kernel, qOverload<int>(&QComboBox::currentIndexChanged), page,
                [updateUIState](int) { updateUIState(); });
        connect(footprint, qOverload<int>(&QComboBox::currentIndexChanged), page,
                [updateUIState](int) { updateUIState(); });
        kernel->setCurrentIndex(0);
        updateUIState();

        QLabel* tip = new QLabel(
                QStringLiteral("悬停高亮后左键拖动：中心点平移、平面沿法向移动、法向端点或圆环旋转。\n未命中组件时左键旋转模型；点击应用后执行插值。"), body);
        tip->setStyleSheet("color: #888888; font-size: 12px; margin-top: 10px;");
        mainLayout->addWidget(tip);

        // ================= 5. 状态与按钮 =================
        QLabel* status = new QLabel(body);
        status->setStyleSheet("color: #A0A0A0; font-weight: bold; margin-top: 5px;");
        mainLayout->addWidget(status);

        QPushButton* applyBtn = new QPushButton(QStringLiteral("应用"), body);
        applyBtn->setStyleSheet("QPushButton { background-color: #2A2A2A; color: #EAEAEA; border: 1px solid #3A3A3A; "
                                "padding: 8px; border-radius: 4px; font-weight: bold; margin-top: 5px; } "
                                "QPushButton:hover { background-color: #3A3A3A; border-color: #4A4A4A; }");
        applyBtn->setProperty("igPanelBaseQss", applyBtn->styleSheet());
        igQtPanelTheme::refresh(applyBtn);

        mainLayout->addWidget(applyBtn);
        mainLayout->addStretch();

        scroll->setWidget(body);
        pageLayout->addWidget(scroll);

        // ================= 6. 核心逻辑初始化 =================
        auto selection = ClipSelection::New();
        selection->PlanePoint = Vector3d(cx, cy, cz);
        selection->PlaneNormal = Vector3d(1.0, 0.0, 0.0);

        auto interactor = scene->GetInteractor();
        auto previousBox = interactor->GetSpecialInteractor("SelectBox");
        auto previousTransform = interactor->GetSpecialInteractor("TransformBox");
        const bool previousHaveBox = SelectionParameter::Instance().GetHaveBox();
        const bool previousInSelection = SelectionParameter::Instance().GetInSelection();
        for (auto previous : {previousBox, previousTransform})
            if (auto box = DynamicCast<BoxStyle>(previous)) box->ClearDraw();
        interactor->RemoveSepcialInteractor("SelectBox");
        interactor->RemoveSepcialInteractor("TransformBox");
        auto planeStyle = RegionPlaneStyle::New();
        planeStyle->Initialize(interactor);
        planeStyle->SetBounds(bbox);
        planeStyle->SetPlane(Point(cx, cy, cz), Point(1., 0., 0.));
        // 7. 直接应用 按钮绑定
        connect(applyBtn, &QPushButton::clicked, page, [=]() mutable {
            status->setText(QStringLiteral("计算中..."));
            auto filter = PointPlaneInterpolatorFilter::New();
            filter->SetInput(input);
            filter->SetOrigin(Point(selection->PlanePoint[0], selection->PlanePoint[1], selection->PlanePoint[2]));

            Vector3d n(selection->PlaneNormal[0], selection->PlaneNormal[1], selection->PlaneNormal[2]);
            if (n.norm() > 1e-6) n = n.normalized();
            filter->SetNormal(Point(n[0], n[1], n[2]));
            filter->SetResolution(res->value());
            filter->SetKernelType(static_cast<PointPlaneInterpolatorFilter::KernelType>(kernel->currentIndex()));
            filter->SetKernelFootprint(
                    static_cast<PointPlaneInterpolatorFilter::KernelFootprint>(footprint->currentIndex()));
            filter->SetRadius(radius->value());
            filter->SetNumberOfPoints(nClosest->value());
            filter->SetSharpness(sharpness->value());
            filter->SetPowerParameter(power->value());
            filter->SetNullPointsStrategy(
                    static_cast<PointPlaneInterpolatorFilter::NullPointsStrategy>(nullStrategy->currentIndex()));

            if (filter->Execute()) {
                auto output = filter->GetOutput();
                if (!output) return;

                std::string targetName = input->GetName() + "_PlaneInterpolated";
                output->SetName(targetName);

                Model::Pointer existingModel = nullptr;
                auto modelList = rendererWidget->GetScene()->GetModelList();
                for (auto it = modelList->Begin(); it != modelList->End(); ++it) {
                    if (it->second->GetDataObject()->GetName() == targetName) {
                        existingModel = it->second;
                        break;
                    }
                }

                if (!existingModel) {
                    int id = modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
                    existingModel = rendererWidget->GetScene()->GetModelById(id);
                } else {
                    existingModel->SetDataObject(output);
                    modelTreeWidget->updateItemName(output);
                    modelTreeWidget->updateAllAttriubute(output);
                }

                if (existingModel) rendererWidget->GetScene()->SetCurrentModel(existingModel);

                if (auto draw = DynamicCast<DrawObject>(output)) {
                    draw->ForceReConvertToDrawableData();
                    auto attrSet = output->GetAttributeSet();
                    if (attrSet && attrSet->GetNumberOfAttributes() > 0) {
                        for (int i = 0; i < attrSet->GetNumberOfAttributes(); ++i) {
                            auto& attr = attrSet->GetAttribute(i);
                            if (attr.pointer && attr.attachmentType == IG_POINT &&
                                attr.pointer->GetName() != "vtkValidPointMask") {
                                draw->ViewCloudPicture(rendererWidget->GetScene(), i, -1);
                                break;
                            }
                        }
                    }
                }
                rendererWidget->update();
                status->setText(QStringLiteral("更新成功！"));
            } else {
                status->setText(QStringLiteral("执行失败，请检查参数。"));
            }
        });

        // Reuse the region extractor's Plane widget without changing interpolation.
        QPointer<QWidget> pageGuard(page);
        auto syncUIto3D = [=]() {
            Vector3d n(normalX->value(), normalY->value(), normalZ->value());
            if (n.norm() < 1.e-6) return;
            n = n.normalized();
            selection->PlanePoint = Vector3d(originX->value(), originY->value(), originZ->value());
            selection->PlaneNormal = n;
            planeStyle->SetPlane(Point(originX->value(), originY->value(), originZ->value()),
                                 Point(n[0], n[1], n[2]));
            rendererWidget->update();
        };
        planeStyle->SetChangedCallback([=]() {
            if (!pageGuard) return;
            const auto& p = planeStyle->GetOrigin();
            const auto& n = planeStyle->GetNormal();
            selection->PlanePoint = Vector3d(p[0], p[1], p[2]);
            selection->PlaneNormal = Vector3d(n[0], n[1], n[2]);
            QSignalBlocker b1(originX), b2(originY), b3(originZ);
            QSignalBlocker b4(normalX), b5(normalY), b6(normalZ);
            originX->setValue(p[0]); originY->setValue(p[1]); originZ->setValue(p[2]);
            normalX->setValue(n[0]); normalY->setValue(n[1]); normalZ->setValue(n[2]);
            rendererWidget->update();
        });
        for (auto* p : {originX, originY, originZ, normalX, normalY, normalZ})
            connect(p, qOverload<double>(&QDoubleSpinBox::valueChanged), page, [=](double) { syncUIto3D(); });

        QPointer<igQtRenderWidget> renderGuard(rendererWidget);
        auto activate = [=]() {
            if (!renderGuard) return;
            interactor->RequestBasicStyle();
            interactor->_SetSpecialInteractor("PointPlaneInterpolatorPlane", planeStyle);
            SelectionParameter::Instance().SetHaveBox(false);
            SelectionParameter::Instance().SetInSelection(false);
            planeStyle->SetShow(true);
            renderGuard->update();
        };
        auto deactivate = [=]() {
            planeStyle->SetChangedCallback({});
            if (!renderGuard) return;
            planeStyle->SetShow(false);
            interactor->RemoveSepcialInteractor("PointPlaneInterpolatorPlane");
            if (previousBox) interactor->_SetSpecialInteractor("SelectBox", previousBox);
            if (previousTransform) interactor->_SetSpecialInteractor("TransformBox", previousTransform);
            SelectionParameter::Instance().SetHaveBox(previousHaveBox);
            SelectionParameter::Instance().SetInSelection(previousInSelection);
            for (auto previous : {previousBox, previousTransform})
                if (auto box = DynamicCast<BoxStyle>(previous)) if (box->GetBox()) box->ToDraw();
            renderGuard->update();
        };
        // 10. 嵌入左侧工具面板
        const int idx = m_leftFieldTabs->addTab(page, QStringLiteral("点平面插值"));
        m_leftFieldDock->show();
        m_leftFieldDock->raise();
        m_leftFieldTabs->setCurrentIndex(idx);

        // 11. 页面销毁时移除平面组件，并恢复之前的交互盒子。
        connect(page, &QObject::destroyed, this, [=]() {
            deactivate();
        });

        // 12. 安全处理 Tab 关闭事件
        QMetaObject::Connection* connPtr = new QMetaObject::Connection();
        *connPtr = connect(m_leftFieldTabs, &QTabWidget::tabCloseRequested, this, [=](int index) {
            if (index == m_leftFieldTabs->indexOf(pageGuard)) {
                m_leftFieldTabs->removeTab(index);
                pageGuard->deleteLater();
            }
        });
        connect(page, &QObject::destroyed, this, [this, connPtr]() {
            disconnect(*connPtr);
            delete connPtr;
        });

        // 13. 激活操作杆
        activate();
    });

    // 连通区域 (Connectivity)。
    connect(developingFiltersBatch2->addAction(QStringLiteral("连通区域 (Connectivity)")),
            &QAction::triggered, this, [this](bool) {
                auto scene = rendererWidget ? rendererWidget->GetScene() : nullptr;
                auto currentModel = scene ? scene->GetCurrentModel() : nullptr;
                if (!currentModel) {
                    showDarkFramelessMessage(QStringLiteral("连通区域"), QStringLiteral("请先选择一个模型。"));
                    return;
                }
                auto object = currentModel->GetDataObject();
                if (iGame::DynamicCast<iGame::SurfaceMesh>(object).IsNull()) {
                    showDarkFramelessMessage(QStringLiteral("连通区域"),
                                             QStringLiteral("当前模型不是曲面网格（SurfaceMesh）"));
                    return;
                }

                auto* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(QStringLiteral("连通区域"));
                dialog->setFilterDescription(
                        QStringLiteral("按共享点连通性给区域编号或抽取区域"));
                const int modeId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_COMBO_BOX, QStringLiteral("抽取模式"),
                        std::vector<QString>{QStringLiteral("点种子"),
                                             QStringLiteral("单元种子"),
                                             QStringLiteral("指定区域"),
                                             QStringLiteral("最大区域"),
                                             QStringLiteral("全部区域"),
                                             QStringLiteral("最近点")});
                const int colorId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_CHECK_BOX, QStringLiteral("按区域编号着色"), "true");
                const int assignId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_COMBO_BOX, QStringLiteral("编号方式"),
                        std::vector<QString>{QStringLiteral("遍历顺序"), QStringLiteral("单元数升序"),
                                             QStringLiteral("单元数降序")});
                const int seedsId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("种子编号"), "0");
                const int specId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("区域编号"), "0");
                const int closestId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("最近点坐标 (x, y, z)"), "0,0,0");
                dialog->setParameterLayoutVertical();
                dialog->setMinimumWidth(440);
                dialog->resize(qMax(480, dialog->fontMetrics().horizontalAdvance(
                        QStringLiteral("最近点坐标 (x, y, z)")) + 120), 520);
                for (int id : {seedsId, specId}) {
                    auto* edit = qobject_cast<QLineEdit*>(dialog->getWidget(id));
                    edit->setPlaceholderText(QStringLiteral("例如：0, 2, 5（编号从 0 开始）"));
                    edit->setToolTip(QStringLiteral("多个编号用英文逗号或空格分隔，编号从 0 开始。"));
                }
                auto* closestEdit = qobject_cast<QLineEdit*>(dialog->getWidget(closestId));
                closestEdit->setPlaceholderText(QStringLiteral("例如：1.5, 0, -2.0"));
                closestEdit->setToolTip(QStringLiteral("输入模型坐标中的 x、y、z，用英文逗号或空格分隔。"));
                if (auto* scrollArea = dialog->findChild<QScrollArea*>(QStringLiteral("scrollArea"))) {
                    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                }
                // 默认「全部区域」：一次即可看到所有连通区域的编号着色。
                if (auto* modeCombo = qobject_cast<QComboBox*>(dialog->getWidget(modeId))) {
                    modeCombo->setCurrentIndex(4);
                }
                auto* inputError = new QLabel(dialog);
                inputError->setObjectName(QStringLiteral("ConnectivityInputError"));
                inputError->setWordWrap(true);
                inputError->setStyleSheet(QStringLiteral("color: #ffb4ab;"));
                dialog->addRowWidget(inputError);
                inputError->hide();
                for (int id : {seedsId, specId, closestId}) {
                    connect(qobject_cast<QLineEdit*>(dialog->getWidget(id)), &QLineEdit::textChanged,
                            inputError, &QWidget::hide);
                }
                auto* modeCombo = qobject_cast<QComboBox*>(dialog->getWidget(modeId));
                auto updateModeParameters = [=](int mode) {
                    inputError->hide();
                    dialog->setParameterVisible(seedsId, mode == 0 || mode == 1);
                    dialog->setParameterVisible(specId, mode == 2);
                    dialog->setParameterVisible(closestId, mode == 5);
                    const QString descriptions[] = {
                        QStringLiteral("抽取包含指定点的连通区域。填写点编号，多个编号用逗号分隔。"),
                        QStringLiteral("抽取包含指定单元的连通区域。填写单元编号，多个编号用逗号分隔。"),
                        QStringLiteral("抽取指定编号的区域。区域编号从 0 开始，由编号方式决定。"),
                        QStringLiteral("仅保留单元数最多的连通区域，无需额外参数。"),
                        QStringLiteral("保留全部连通区域，可按区域编号着色，无需额外参数。"),
                        QStringLiteral("抽取距离指定坐标最近的连通区域。填写模型坐标 x、y、z。")};
                    dialog->setFilterDescription(descriptions[mode]);
                    if (mode == 0 || mode == 1) {
                        auto* seeds = qobject_cast<QLineEdit*>(dialog->getWidget(seedsId));
                        seeds->setToolTip(mode == 0 ? QStringLiteral("填写点编号，多个编号用逗号或空格分隔。")
                                                   : QStringLiteral("填写单元编号，多个编号用逗号或空格分隔。"));
                    }
                };
                connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                        dialog, updateModeParameters);
                updateModeParameters(modeCombo->currentIndex());
                dialog->show();

                dialog->setApplyFunctor([=, this]() {
                    bool ok = false;
                    const int mode = dialog->getComboIndex(modeId, ok);
                    auto filter = iGame::ConnectivityFilter::New();
                    switch (mode) {
                        case 0: filter->SetExtractionMode(iGame::ConnectivityFilter::POINT_SEEDED_REGIONS); break;
                        case 1: filter->SetExtractionMode(iGame::ConnectivityFilter::CELL_SEEDED_REGIONS); break;
                        case 2: filter->SetExtractionMode(iGame::ConnectivityFilter::SPECIFIED_REGIONS); break;
                        case 3: filter->SetExtractionMode(iGame::ConnectivityFilter::LARGEST_REGION); break;
                        case 4: filter->SetExtractionMode(iGame::ConnectivityFilter::ALL_REGIONS); break;
                        default: filter->SetExtractionMode(iGame::ConnectivityFilter::CLOSEST_POINT_REGION); break;
                    }
                    filter->SetColorRegions(dialog->getChecked(colorId, ok));
                    const int assign = dialog->getComboIndex(assignId, ok);
                    if (assign == 1) {
                        filter->SetRegionIdAssignmentMode(iGame::ConnectivityFilter::CELL_COUNT_ASCENDING);
                    } else if (assign == 2) {
                        filter->SetRegionIdAssignmentMode(iGame::ConnectivityFilter::CELL_COUNT_DESCENDING);
                    } else {
                        filter->SetRegionIdAssignmentMode(iGame::ConnectivityFilter::UNSPECIFIED);
                    }

                    auto parseDoubles = [](const QString& text, std::vector<double>& out) -> bool {
                        out.clear();
                        QString t = text;
                        t.replace(',', ' ');
                        const auto parts = t.simplified().split(' ', Qt::SkipEmptyParts);
                        for (const auto& p : parts) {
                            bool c = false;
                            const double v = p.toDouble(&c);
                            if (!c || !std::isfinite(v)) { return false; }
                            out.push_back(v);
                        }
                        return !out.empty();
                    };

                    auto rejectInput = [=](int id, const QString& message) {
                        inputError->setText(message);
                        inputError->show();
                        if (auto* edit = qobject_cast<QLineEdit*>(dialog->getWidget(id))) {
                            edit->setFocus();
                            edit->selectAll();
                        }
                    };
                    auto parseIds = [&](int id, qlonglong maxId, std::vector<igIndex>& values) {
                        auto* edit = qobject_cast<QLineEdit*>(dialog->getWidget(id));
                        QString text = edit->text();
                        text.replace(',', ' ');
                        const auto parts = text.simplified().split(' ', Qt::SkipEmptyParts);
                        values.clear();
                        for (const auto& part : parts) {
                            bool valid = false;
                            const qlonglong value = part.toLongLong(&valid);
                            if (!valid || value < 0 || value > maxId ||
                                static_cast<unsigned long long>(value) > static_cast<unsigned long long>(std::numeric_limits<igIndex>::max())) {
                                rejectInput(id, QStringLiteral("请输入 0 到 %1 范围内的整数编号，多个编号用逗号或空格分隔。").arg(maxId));
                                return false;
                            }
                            values.push_back(static_cast<igIndex>(value));
                        }
                        if (values.empty()) {
                            rejectInput(id, QStringLiteral("请至少输入一个整数编号。"));
                            return false;
                        }
                        return true;
                    };
                    inputError->hide();
                    if (mode == 0 || mode == 1) {
                        auto mesh = iGame::DynamicCast<iGame::SurfaceMesh>(object);
                        const qlonglong count = static_cast<qlonglong>(mode == 0 ? mesh->GetNumberOfPoints() : mesh->GetNumberOfFaces());
                        std::vector<igIndex> values;
                        if (!parseIds(seedsId, count - 1, values)) return;
                        filter->InitializeSeedList();
                        for (igIndex value : values) filter->AddSeed(value);
                    } else if (mode == 2) {
                        std::vector<igIndex> values;
                        if (!parseIds(specId, std::numeric_limits<int>::max(), values)) return;
                        filter->InitializeSpecifiedRegionList();
                        for (igIndex value : values) filter->AddSpecifiedRegion(static_cast<int>(value));
                    } else if (mode == 5) {
                        auto* edit = qobject_cast<QLineEdit*>(dialog->getWidget(closestId));
                        std::vector<double> values;
                        if (!parseDoubles(edit->text(), values) || values.size() != 3) {
                            rejectInput(closestId, QStringLiteral("请输入三个有限坐标值 x, y, z，例如：1.5, 0, -2.0。"));
                            return;
                        }
                        filter->SetClosestPoint(values[0], values[1], values[2]);
                    }

                    filter->SetInput(object);
                    if (!filter->Execute()) {
                        if (mode == 2 && (filter->GetMessage() == "Specified region ID is outside the valid range" ||
                                          filter->GetMessage() == "Extraction produced an empty mesh")) {
                            rejectInput(specId, QStringLiteral("请填写 0 到 %1 范围内的区域编号（按当前编号方式排序）。")
                                    .arg(filter->GetNumberOfExtractedRegions() - 1));
                            return;
                        }
                        showDarkFramelessMessage(
                                QStringLiteral("连通区域"),
                                QStringLiteral("执行失败：%1").arg(QString::fromStdString(filter->GetMessage())));
                        return;
                    }
                    auto output = filter->GetOutput();
                    if (!output) {
                        showDarkFramelessMessage(QStringLiteral("连通区域"), QStringLiteral("输出对象为空"));
                        return;
                    }
                    output->SetName(object->GetName() + "_Connectivity");

                    auto inputDraw = iGame::DynamicCast<iGame::DrawObject>(object);
                    auto outDraw = iGame::DynamicCast<iGame::DrawObject>(output);
                    if (inputDraw && outDraw) {
                        // 只继承视图样式；不继承颜色映射表——RegionId 是新增属性，需用输出自身的
                        // 颜色映射（range 由 RegionId 数据范围确定），否则会出现只有一种颜色。
                        outDraw->SetViewStyle(static_cast<IGenum>(inputDraw->GetViewStyle()));
                    }

                    dialog->setFilterDescription(
                            QStringLiteral("共 %1 个连通区域").arg(filter->GetNumberOfExtractedRegions()));
                    modelTreeWidget->addDataObjectToModelTree(output, ItemSource::Algorithm);

                    // 优先按 RegionId（点标量）上色；否则退回第一个标量点/单元属性。
                    if (auto attrSet = output->GetAttributeSet(); attrSet && filter->GetColorRegions()) {
                        int activeIdx = attrSet->GetAttributeIndex(iGame::ConnectivityFilter::RegionIdName);
                        if (activeIdx < 0) {
                            int pointAttrIdx = -1;
                            int cellAttrIdx = -1;
                            for (int i = 0; i < static_cast<int>(attrSet->GetNumberOfAttributes()); ++i) {
                                auto& attr = attrSet->GetAttribute(i);
                                if (attr.IsNone() || attr.isDeleted || attr.type != IG_SCALAR) { continue; }
                                if (attr.attachmentType == IG_POINT && pointAttrIdx < 0) {
                                    pointAttrIdx = i;
                                } else if (attr.attachmentType == IG_CELL && cellAttrIdx < 0) {
                                    cellAttrIdx = i;
                                }
                            }
                            activeIdx = (pointAttrIdx >= 0) ? pointAttrIdx : cellAttrIdx;
                        }
                        if (activeIdx >= 0 && outDraw) {
                            // 显式按该属性的数据范围设置色表 range，保证热力图映射正确
                            // （不依赖 SetAttributeWithPointData 内部的 MTime 条件）。
                            auto& attr = attrSet->GetAttribute(activeIdx);
                            if (auto range = attr.GetDataRange()) {
                                double lo = range->GetValue(2);
                                double hi = range->GetValue(3);
                                if (hi <= lo) { hi = lo + 1.0; } // 单值退化范围，避免所有值落入同一点
                                if (auto cm = outDraw->GetColorMapper()) {
                                    cm->SetRangeStable(false);
                                    cm->SetRange(lo, hi);
                                }
                            }
                            if (auto scene2 = rendererWidget->GetScene()) {
                                outDraw->ViewCloudPicture(scene2, activeIdx, 0);
                            }
                        }
                    }

                    modelTreeWidget->updateCloudPicture();
                    rendererWidget->update();
                    dialog->close();
                });
            });
    // ---------- 带状面 (Ribbon)：追加在“开发中filter/第二批”菜单末尾 ----------
    QAction* ribbonAction = developingFiltersBatch2->addAction(QStringLiteral("带状面 (Ribbon)"));
    connect(ribbonAction, &QAction::triggered, this,
            [this](bool checked) {
                auto* scene = rendererWidget->GetScene();
                if (scene == nullptr || scene->GetCurrentModel() == nullptr) {
                    showDarkFramelessMessage(QStringLiteral("无可用模型"),
                                             QStringLiteral("请先加载并选择一个网格模型。"));
                    return;
                }
                auto model = scene->GetCurrentModel();
                auto obj = model->GetDataObject();
                if (obj == nullptr) {
                    showDarkFramelessMessage(QStringLiteral("无可用数据"),
                                             QStringLiteral("当前模型没有可用的网格数据。"));
                    return;
                }

                igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(QStringLiteral("带状面 (Ribbon)"));
                dialog->setFilterDescription(QStringLiteral(
                    "把线/折线沿“切向 × 线法向”构成的框架扫成有向带状面。<br>"
                    "半宽决定带宽：带宽 = 2 × 半宽；偏转角相对线法向旋转带面（0° 时带面垂直于线法向与切向）。<br>"
                    "输入需要包含线单元（IG_LINE / IG_POLY_LINE），其他单元类型会被忽略并在结果提示中说明。"));

                const int widthId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                         QStringLiteral("半宽 (Width)"), "0.5");
                const int angleId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                         QStringLiteral("偏转角/度 (Angle)"), "0");
                const int useDefId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_CHECK_BOX,
                        QStringLiteral("使用默认线法向 (Use Default Normal)"), "true");
                const int nxId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("默认法向 X (Normal X)"), "0");
                const int nyId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("默认法向 Y (Normal Y)"), "0");
                const int nzId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("默认法向 Z (Normal Z)"), "1");

                // 加宽面板，避免说明文字与标签过度换行
                dialog->setFixedWidth(660);

                auto* useDefCheck = qobject_cast<QCheckBox*>(dialog->getWidget(useDefId));
                if (useDefCheck != nullptr) {
                    useDefCheck->setToolTip(QStringLiteral(
                        "勾选时全部点使用下方默认法向；取消勾选则优先取输入的点法向（IG_NORMAL），"
                        "输入没有点法向时回退到默认法向并在结果提示中说明。"));
                }
                const QString normalEditTip = QStringLiteral(
                    "仅在勾选上方“使用默认线法向”后作为带面的定向依据；切线与该法向平行的那条折线会被跳过。");
                for (int id: {nxId, nyId, nzId}) {
                    if (QWidget* editor = dialog->getWidget(id)) { editor->setToolTip(normalEditTip); }
                }
                if (QWidget* angleEdit = dialog->getWidget(angleId)) {
                    angleEdit->setToolTip(QStringLiteral("单位度，取值 0–360；超出范围会被夹到区间内（与 VTK 一致）。"));
                }

                dialog->setApplyFunctor([this, dialog, obj, widthId, angleId, useDefId, nxId, nyId, nzId]() {
                    bool wok = false;
                    bool aok = false;
                    bool uok = false;
                    bool xok = false;
                    bool yok = false;
                    bool zok = false;

                    const double width = dialog->getDouble(widthId, wok);
                    const double angle = dialog->getDouble(angleId, aok);
                    const bool useDefaultNormal = dialog->getChecked(useDefId, uok);
                    const double nx = dialog->getDouble(nxId, xok);
                    const double ny = dialog->getDouble(nyId, yok);
                    const double nz = dialog->getDouble(nzId, zok);

                    if (!wok || width <= 0.0) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("半宽必须是大于 0 的数字（带宽 = 2 × 半宽）。"));
                        return;
                    }
                    if (!aok) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("偏转角不是有效数字。"));
                        return;
                    }
                    if (xok && yok && zok && nx == 0.0 && ny == 0.0 && nz == 0.0) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("默认法向不能是零向量。"));
                        return;
                    }

                    RibbonFilter::Pointer filter = RibbonFilter::New();
                    filter->SetInput(obj);
                    filter->SetWidth(width);
                    filter->SetAngle(angle);
                    filter->SetUseDefaultNormal(uok ? useDefaultNormal : true);
                    if (xok && yok && zok) { filter->SetDefaultNormal(Vector3d(nx, ny, nz)); }

                    if (!filter->Execute()) {
                        // 失败原因（无输入 / 无线单元 / 全被跳过 / 零宽度或零法向）由 filter 写在 m_Message 里
                        const std::string message = filter->GetMessage();
                        showDarkFramelessMessage(
                                QStringLiteral("带状面生成失败"),
                                message.empty() ? QStringLiteral("请检查输入是否包含线/折线单元（IG_LINE / IG_POLY_LINE）。")
                                                : QString::fromStdString(message));
                        return;
                    }

                    auto outMesh = DynamicCast<SurfaceMesh>(filter->GetOutput());
                    if (!outMesh) {
                        showDarkFramelessMessage(QStringLiteral("带状面生成失败"),
                                                 QStringLiteral("算法未产生有效结果。"));
                        return;
                    }

                    modelTreeWidget->addDataObjectToModelTree(outMesh, ItemSource::Algorithm);
                    rendererWidget->update();
                    // 非致命提示（忽略的非线单元、被跳过的折线）只在确实发生时弹一次，
                    // 与 Tube 一致：先弹模态提示再关闭面板，避免 close dock 触发布局重建后再开模态对话框
                    if (!filter->GetMessage().empty()) {
                        showDarkFramelessMessage(QStringLiteral("带状面生成完成"),
                                                 QString::fromStdString(filter->GetMessage()), true);
                    }
                    dialog->close();
                });
                dialog->show();
            });
    // 9.28 布置的第二批 filter：按上游约定追加在「开发中filter/第二批」末尾。
    // 这里只放代理入口，实际行为由「标准过滤器」里同 id 的 QAction 承担
    // （见 igQtMainWindowStandardFilters.cpp 的 filterPanel 懒加载面板）。
    static const DevelopmentEntry tensorBatch2Entries[] = {
        {"tensor_principal_invariants", "张量主值 (Tensor Principal Invariants)"},
    };
    for (const DevelopmentEntry& entry : tensorBatch2Entries) {
        addDevelopmentAction(developingFiltersBatch2, approvedFilter(entry.id),
                             QString::fromUtf8(entry.label));
    }



    // === Append Datasets Filter: 网格合并 (对标 ParaView Append Datasets) ===
    connect(developingFiltersBatch2->addAction(QStringLiteral("Append Datasets")),
            &QAction::triggered, this, [&](bool checked) {
        auto scene = rendererWidget->GetScene();
        if (!scene) return;

        // Capture the tree selection before opening the parameter panel so Apply
        // cannot pick up unrelated scene models or previous results.
        const auto inputs = modelTreeWidget->GetSelectedDataObjects();
        if (inputs.empty()) {
            showDarkFramelessMessage(QStringLiteral("提示"),
                                     QStringLiteral("请先在模型树中选中要合并的模型。\n"
                                                    "按住 Ctrl 或 Shift 点击可多选。"));
            return;
        }

        auto* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("Append Datasets"));
        dialog->setFilterDescription(QStringLiteral(
                "仅将打开面板时选中的网格合并为一个 UnstructuredMesh（保留原始单元类型）。\n"
                "按住 Ctrl 或 Shift 点击模型树可多选；单选时生成相同几何和属性的新模型，不合并重复点。\n"
                "默认不合并重复点（与 ParaView 一致）；需要消除接缝冗余顶点时勾选「合并重复顶点」，"
                "容差为 0 时仅合并完全重合的点。\n"
                "属性按交集规则保留：仅保留所有输入网格都具备的属性。"));
        const int mergeId = dialog->addParameter(
                igQtFilterDialogDockWidget::QT_CHECK_BOX,
                QStringLiteral("合并重复顶点 (Merge Points)"), "false");
        const int tolId = dialog->addParameter(
                igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("合并容差"), "0");
        dialog->show();

        dialog->setApplyFunctor([=, this]() {
            bool ok = false;
            const bool mergePoints = dialog->getChecked(mergeId, ok);
            float tolerance = (float) dialog->getDouble(tolId, ok);
            if (!ok) tolerance = 0.0f; // 解析失败时回退到 ParaView 默认值 0（精确合并）

            auto filter = AppendDatasetsFilter::New();
            filter->SetMergePoints(inputs.size() > 1 && mergePoints);
            filter->SetTolerance(tolerance);

            for (const auto& input : inputs) {
                filter->AddInput(input);
            }
            const int count = static_cast<int>(inputs.size());

            if (!filter->Execute()) {
                showDarkFramelessMessage(QStringLiteral("执行失败"),
                                         QString::fromStdString(filter->GetMessage()));
                return;
            }

            auto outMesh = DynamicCast<UnstructuredMesh>(filter->GetOutput());
            if (!outMesh) {
                showDarkFramelessMessage(QStringLiteral("执行失败"),
                                         QStringLiteral("合并未生成有效的非结构化网格。"));
                return;
            }
            outMesh->SetName("append_datasets_result");

            modelTreeWidget->addDataObjectToModelTree(outMesh, Algorithm);

            // 自动激活第一个标量属性，便于立即看到颜色映射
            auto attrSet = outMesh->GetAttributeSet();
            if (attrSet) {
                int activeIdx = -1;
                for (int i = 0; i < (int) attrSet->GetNumberOfAttributes(); i++) {
                    auto& attr = attrSet->GetAttribute(i);
                    if (attr.IsNone() || attr.isDeleted || attr.type != IG_SCALAR) continue;
                    activeIdx = i;
                    break;
                }
                if (activeIdx >= 0) {
                    outMesh->ViewCloudPicture(rendererWidget->GetScene(), activeIdx, 0);
                }
            }
            rendererWidget->update();
            showDarkFramelessMessage(QStringLiteral("合并完成"),
                                     QStringLiteral("已合并 %1 个网格为单一 UnstructuredMesh。").arg(count),
                                     true);
            dialog->close();
        });
    });


    // 新增「时间平移/缩放」菜单项
    QAction* temporalShiftScaleAction =
        developingFiltersBatch2->addAction(QStringLiteral("时间平移/缩放 (TemporalShiftScale)"));
    connect(temporalShiftScaleAction, &QAction::triggered, this, [this](bool) {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) {
            showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("当前没有打开模型。"));
            return;
        }
        auto data = model->GetDataObject();
        if (data == nullptr) {
            showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("当前模型没有数据。"));
            return;
        }
        auto frames = data->PeekTimeFrames();
        if (frames == nullptr || frames->GetTimeNum() == 0) {
            showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("当前模型没有时间序列。"));
            return;
        }

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("TemporalShiftScale - 时间平移/缩放"));
        dialog->setFilterDescription(QStringLiteral("t' = (t + Pre Shift) × Scale + Post Shift"));
        const int preShiftId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                    QStringLiteral("Pre Shift"), "0.0");
        const int scaleId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                 QStringLiteral("Scale"), "1.0");
        const int postShiftId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                     QStringLiteral("Post Shift"), "0.0");
        dialog->show();

        dialog->setApplyFunctor([=, this]() {
            bool ok = false;
            const float preShift = static_cast<float>(dialog->getDouble(preShiftId, ok));
            if (!ok) {
                showDarkFramelessMessage(QStringLiteral("参数错误"), QStringLiteral("Pre Shift 不是有效数字。"));
                return;
            }
            const float scale = static_cast<float>(dialog->getDouble(scaleId, ok));
            if (!ok) {
                showDarkFramelessMessage(QStringLiteral("参数错误"), QStringLiteral("Scale 不是有效数字。"));
                return;
            }
            const float postShift = static_cast<float>(dialog->getDouble(postShiftId, ok));
            if (!ok) {
                showDarkFramelessMessage(QStringLiteral("参数错误"), QStringLiteral("Post Shift 不是有效数字。"));
                return;
            }

            auto filter = iGame::TemporalShiftScaleFilter::New();
            filter->SetPreShift(preShift);
            filter->SetScale(scale);
            filter->SetPostShift(postShift);
            filter->SetInput(data);
            if (!filter->Execute()) {
                showDarkFramelessMessage(QStringLiteral("Warning"),
                                         QStringLiteral("执行失败：Scale 不能为 0，或输入没有时间序列。"));
                return;
            }
            auto outObj = filter->GetOutput();
            if (!outObj) {
                showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("输出对象为空。"));
                return;
            }
            modelTreeWidget->addDataObjectToModelTree(outObj, Algorithm);
            rendererWidget->update();
        });
    });
    // 保留指定时间步：打开左侧面板；结果由面板的信号回到这里加入模型树
    // 追加在「新增filter/第二批」菜单末尾
    connect(developingFiltersBatch2->addAction(QStringLiteral("保留指定时间步 (Extract Time Steps)")),
            &QAction::triggered, this, [this](bool) {
                if (m_extractTimeStepsWidget == nullptr) return;

                auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
                auto currentModel = scene ? scene->GetCurrentModel() : nullptr;
                if (!currentModel) {
                    showDarkFramelessMessage(QStringLiteral("保留指定时间步"), QStringLiteral("当前没有模型"));
                    return;
                }

                auto object = currentModel->GetDataObject();
                auto frames = object ? object->PeekTimeFrames() : nullptr;
                if (!frames || frames->GetTimeNum() == 0) {
                    showDarkFramelessMessage(
                            QStringLiteral("保留指定时间步"),
                            QStringLiteral("当前模型没有时间序列。请选择带时间序列的模型（例如打开的 .pvd 序列）。"));
                    return;
                }

                openLeftToolPanel(LeftToolPanelId::ExtractTimeSteps);
                m_extractTimeStepsWidget->SetOriginDataObject(object);
            });
    // 面板执行成功：把独立输出节点加入模型树
    connect(m_extractTimeStepsWidget, &igQtExtractTimeStepsWidget::ExtractTimeStepsApplied, this,
            [this](iGame::DataObject::Pointer output) {
                if (!output) return;
                modelTreeWidget->addDataObjectToModelTree(output, ItemSource::Algorithm);
                rendererWidget->update();
            });

    // === SurfaceVectors Filter: 将向量投影到表面（对标 ParaView Surface Vectors） ===
    connect(developingFiltersBatch2->addAction(QStringLiteral("Surface Vectors")), &QAction::triggered, this,
            [&](bool checked) {
                auto currentModel = rendererWidget->GetScene()->GetCurrentModel();
                if (!currentModel) {
                    showDarkFramelessMessage(QStringLiteral("无可用模型"),
                                             QStringLiteral("请先加载并选择一个网格模型。"));
                    return;
                }
                auto obj = currentModel->GetDataObject();
                if (!obj) {
                    showDarkFramelessMessage(QStringLiteral("无可用数据"),
                                             QStringLiteral("当前模型没有可用的网格数据。"));
                    return;
                }

                auto attrs = obj->GetAttributeSet();
                std::vector<IGsize> attrIndices;
                std::vector<QString> attrNames;
                if (attrs) {
                    for (IGsize i = 0; i < static_cast<IGsize>(attrs->GetNumberOfAttributes()); ++i) {
                        auto& attr = attrs->GetAttribute(i);
                        if (attr.isDeleted || !attr.pointer) continue;
                        if (attr.pointer->GetDimension() < 3) continue; // 只显示向量(>=3分量)
                        attrIndices.push_back(i);
                        const QString attach =
                                attr.attachmentType == IG_CELL ? QStringLiteral("Cell") : QStringLiteral("Point");
                        attrNames.push_back(
                                QStringLiteral("%1 (%2)").arg(QString::fromStdString(attr.pointer->GetName()), attach));
                    }
                }
                if (attrNames.empty()) {
                    showDarkFramelessMessage(QStringLiteral("Warning"),
                                             QStringLiteral("当前模型没有可用于投影的向量属性（需要3分量及以上）。"));
                    return;
                }

                auto* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(QStringLiteral("Surface Vectors"));
                dialog->setFilterDescription(
                        QStringLiteral("将指定向量投影到表面，结果写回同名数组（与 ParaView 一致）。"
                                       "支持表面网格与非结构化网格。"));

                const int vecAttrId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_COMBO_BOX, QStringLiteral("向量属性"), attrNames);
                const int modeId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_COMBO_BOX, QStringLiteral("约束模式"),
                        std::vector<QString>{
                                QStringLiteral("Parallel (切向: v-(v·n)·n)"),
                                QStringLiteral("Perpendicular (法向: (v·n)·n)"),
                                QStringLiteral("PerpendicularScale (标量: v·n)"),
                        });
                dialog->show();

                dialog->setApplyFunctor([=, this]() {
                    bool ok = false;
                    const int choice = dialog->getComboIndex(vecAttrId, ok);
                    if (!ok || choice < 0 || choice >= static_cast<int>(attrIndices.size())) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("请选择有效的向量属性。"));
                        return;
                    }
                    const int mode = dialog->getComboIndex(modeId, ok);
                    if (!ok || mode < 0 || mode > 2) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("请选择有效的约束模式。"));
                        return;
                    }

                    const auto& attr = obj->GetAttributeSet()->GetAttribute(
                            static_cast<IGsize>(attrIndices[static_cast<size_t>(choice)]));
                    // 输出副本中替换同名数组，输入模型保持不变。
                    const QString attrName = QString::fromStdString(attr.pointer->GetName());

                    SurfaceVectorsFilter::Pointer filter = SurfaceVectorsFilter::New();
                    filter->SetVectorAttributeByIndex(static_cast<int>(attrIndices[static_cast<size_t>(choice)]));
                    filter->SetConstraintMode(
                            static_cast<SurfaceVectorsFilter::ConstraintMode>(mode));
                    filter->SetInput(obj);

                    if (!filter->Execute()) {
                        showDarkFramelessMessage(
                                QStringLiteral("执行失败"),
                                QStringLiteral("向量投影失败，仅支持表面网格和非结构化网格。"));
                        return;
                    }

                    auto outObj = filter->GetOutput();
                    if (!outObj) {
                        showDarkFramelessMessage(QStringLiteral("执行失败"),
                                                 QStringLiteral("向量投影没有生成有效输出。"));
                        return;
                    }

                    modelTreeWidget->addDataObjectToModelTree(outObj, Algorithm);
                    rendererWidget->update();
                    showDarkFramelessMessage(
                            QStringLiteral("向量投影完成"),
                            QStringLiteral("已将向量按选定模式投影，结果写回属性 \"%1\"。")
                                    .arg(attrName),
                            true);
                    dialog->close();
                });
            });

    // === Compute Quartiles Filter: 计算四分位数 (对标 ParaView Compute Quartiles) ===
    connect(developingFiltersBatch2->addAction(QStringLiteral("Compute Quartiles")),
            &QAction::triggered, this, [&](bool checked) {
        auto currentModel = rendererWidget->GetScene()->GetCurrentModel();
        if (!currentModel) {
            showDarkFramelessMessage(QStringLiteral("无可用模型"),
                                     QStringLiteral("请先加载并选择一个网格模型。"));
            return;
        }
        auto obj = currentModel->GetDataObject();
        if (!obj) {
            showDarkFramelessMessage(QStringLiteral("无可用数据"),
                                     QStringLiteral("当前模型没有可用的网格数据。"));
            return;
        }

        auto attrs = obj->GetAttributeSet();
        if (!attrs || attrs->GetNumberOfAttributes() == 0) {
            showDarkFramelessMessage(QStringLiteral("无可用属性"),
                                     QStringLiteral("当前模型没有可统计的标量属性。"));
            return;
        }

        // 收集所有点标量属性 (dim==1) 供用户选择（与 ParaView 一致：仅点数据）
        std::vector<IGsize> attrIndices;
        std::vector<QString> attrNames;
        attrNames.push_back(QStringLiteral("全部点标量属性 (ParaView 默认行为)"));
        attrIndices.push_back((IGsize)-1); // 哨兵值：表示全部
        for (IGsize i = 0; i < static_cast<IGsize>(attrs->GetNumberOfAttributes()); ++i) {
            auto& attr = attrs->GetAttribute(i);
            if (attr.isDeleted || !attr.pointer) continue;
            if (attr.attachmentType != IG_POINT) continue; // vtkComputeQuartiles 只统计点数据数组
            if (attr.pointer->GetDimension() != 1) continue; // 只统计标量
            attrIndices.push_back(i);
            attrNames.push_back(QStringLiteral("%1 (Point)")
                                        .arg(QString::fromStdString(attr.pointer->GetName())));
        }
        if (attrNames.size() <= 1) {
            showDarkFramelessMessage(QStringLiteral("无可用属性"),
                                     QStringLiteral("当前模型没有点标量属性可用于计算四分位数。"));
            return;
        }

        auto* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("Compute Quartiles"));
        dialog->setFilterDescription(QStringLiteral(
                "计算点标量属性的四分位数统计：Min / Q1 / Median / Q3 / Max（Tukey hinges 定义）。\n"
                "只统计点数据数组，单元数据数组不参与（与 ParaView 一致）。\n"
                "输出一个 5 顶点的 PointSet，每个被统计属性 A 在输出上生成 \"A_Quartiles\" 数组（5 元素，对应 5 行）。"));
        const int attrId = dialog->addParameter(
                igQtFilterDialogDockWidget::QT_COMBO_BOX,
                QStringLiteral("标量属性"), attrNames);
        dialog->show();

        dialog->setApplyFunctor([=, this]() {
            bool ok = false;
            const int choice = dialog->getComboIndex(attrId, ok);
            if (!ok || choice < 0 || choice >= static_cast<int>(attrIndices.size())) {
                showDarkFramelessMessage(QStringLiteral("参数错误"),
                                         QStringLiteral("请选择有效的标量属性。"));
                return;
            }

            auto filter = ComputeQuartilesFilter::New();
            filter->SetInput(obj);

            const IGsize selectedIdx = attrIndices[static_cast<size_t>(choice)];
            if (selectedIdx == (IGsize)-1) {
                // 全部标量属性模式 (ParaView 默认)
                // 不调用 SetAttributeByName / SetAttributeByIndex，使用默认 m_ProcessAll=true
            } else {
                // 单属性模式
                auto& attr = obj->GetAttributeSet()->GetAttribute(selectedIdx);
                filter->SetAttributeByName(attr.pointer->GetName());
            }

            if (!filter->Execute()) {
                showDarkFramelessMessage(QStringLiteral("执行失败"),
                                         QString::fromStdString(filter->GetMessage()));
                return;
            }

            // 收集结果行
            QVector<QuartileReportRow> rows;
            for (const auto& r : filter->GetResults()) {
                rows.push_back({ QString::fromStdString(r.arrayName),
                                 r.attachmentType == IG_CELL ? QStringLiteral("cell")
                                                             : QStringLiteral("point"),
                                 r.min, r.q1, r.median, r.q3, r.max });
            }

            auto outObj = filter->GetOutput();
            if (outObj) {
                outObj->SetName("compute_quartiles_result");
                // 挂树会切换当前模型并触发"重看"逻辑，这里阻断信号避免重复弹窗
                QSignalBlocker blocker(modelTreeWidget);
                modelTreeWidget->addDataObjectToModelTree(outObj, Algorithm);
                rendererWidget->update();
            }

            showQuartilesReportDialog(this, rows);
            dialog->close();
        });
    });

    auto* extractSelection = developingFiltersBatch2->addAction(QStringLiteral("提取选中数据 (Extract Selection)"));
    extractSelection->setObjectName("actionExtractSelection");
    connect(extractSelection, &QAction::triggered, this, [this]() {
        for (auto* dialog : findChildren<igQtFilterDialogDockWidget*>()) {
            if (dialog->property("extractSelectionPanel").toBool()) {
                dialog->show();
                dialog->raise();
                dialog->activateWindow();
                return;
            }
        }
        auto scene = rendererWidget->GetScene();
        if (!scene || !scene->GetCurrentModel() || !scene->GetCurrentModel()->GetDataObject()) {
            showDarkFramelessMessage(QStringLiteral("提示"), QStringLiteral("请先打开并选择一个模型。"));
            return;
        }
        auto model = scene->GetCurrentModel();
        auto* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setProperty("extractSelectionPanel", true);
        dialog->setFilterTitle(QStringLiteral("提取选中数据"));
        dialog->setFilterDescription(QStringLiteral("输入：%1").arg(
            QString::fromStdString(model->GetDataObject()->GetName()).toHtmlEscaped()));
        dialog->setParameterLayoutVertical();
        const int modeId = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX,
            QStringLiteral("提取类型"),
            std::vector<QString>{QStringLiteral("选中的点"), QStringLiteral("选中的单元")});
        dialog->setMinimumWidth(360);
        dialog->resize(360, 280);
        dialog->setApplyFunctor([this, dialog, scene, model, modeId]() {
            if (scene->GetCurrentModel() != model) {
                showDarkFramelessMessage(QStringLiteral("提取选中数据"),
                    QStringLiteral("当前模型已切换，请关闭此窗口后重新打开。"));
                return;
            }
            bool ok = false;
            const int mode = dialog->getComboIndex(modeId, ok);
            if (!ok || mode < 0 || mode > 1) return;
            auto filter = ExtractSelectionFilter::New();
            filter->SetInput(model->GetDataObject());
            filter->SetSelectionType(mode == 0 ? IG_POINT : IG_CELL);
            if (!filter->Execute()) {
                showDarkFramelessMessage(QStringLiteral("提取失败"),
                    QStringLiteral("请确认已选择对应的点或单元。\n") + QString::fromStdString(filter->GetLastError()));
                return;
            }
            modelTreeWidget->addDataObjectToModelTree(filter->GetOutput(), Algorithm);
            modelTreeWidget->updateCloudPicture();
            rendererWidget->update();
            dialog->close();
        });
        dialog->show();
    });
}

void igQtMainWindow::initAllDockWidgetConnectWithAction() {
    auto* showModelTree = ui->menu_view->addAction(QStringLiteral("显示模型树"));
    showModelTree->setObjectName(QStringLiteral("actionShowModelTree"));
    connect(showModelTree, &QAction::triggered, this, [this]() {
        if (!modelTreeWidget || !rendererWidget) return;
        modelTreeWidget->setTreeDockCollapsed(false);
        modelTreeWidget->positionTreeDockToRendererCorner(rendererWidget);
    });
    // 显示并切换到对应 DockWidget / Tab
    auto showAndRaiseDock = [&](QDockWidget* dock) {
        if (!dock) return;
        dock->show();
        dock->raise();
        if (dock->widget()) dock->widget()->setFocus(Qt::OtherFocusReason);
    };

    connect(ui->action_IsShowColorBar, &QAction::triggered, this, &igQtMainWindow::updateColorBarShow);
    connect(ui->action_ExportAnimation, &QAction::triggered, this, [&](bool checked) { showAndRaiseDock(ui->dockWidget_Animation); });
    connect(ui->action_SearchInfo, &QAction::triggered, this, [&](bool checked) {
        if (ui->dockWidget_SearchInfo) {
            ui->dockWidget_SearchInfo->show();
            ui->dockWidget_SearchInfo->raise();
        }
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        ui->widget_SearchInfo->setCurrentModel(model);
    });
    connect(modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged, this, [this]() {
        if (!ui->dockWidget_SearchInfo || !ui->dockWidget_SearchInfo->isVisible()) return;
        QTimer::singleShot(0, this, [this]() {
            ui->widget_SearchInfo->setCurrentModel(rendererWidget->GetScene()->GetCurrentModel());
        });
    });
    connect(ui->action_Scalar, &QAction::triggered, this,
            [this](bool) { openLeftToolPanel(LeftToolPanelId::Scalar); });
    connect(ui->action_Vector, &QAction::triggered, this,
            [this](bool) { openLeftToolPanel(LeftToolPanelId::Vector); });
    connect(ui->action_Glyph, &QAction::triggered, this,
            [this](bool) { openLeftToolPanel(LeftToolPanelId::Vector); });
    connect(ui->action_Tensor, &QAction::triggered, this,
            [this](bool) { openLeftToolPanel(LeftToolPanelId::Tensor); });
    connect(ui->action_ParallelCoordinates, &QAction::triggered, this, [&](bool checked) {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        showAndRaiseDock(ui->dockWidget_ParallelCoordinatesField);
        ui->widget_ParallelCoordinatesField->SetParallelCoordinates(model);
    });

    //############# HIDE SOMETHING ST #############
    ui->action_ParallelCoordinates->setVisible(false);
    ui->action_SearchInfo->setVisible(true);
    //############# HIDE SOMETHING ED #############

    //############# TESTS ST #############
    {
        QAction* testAction{};
        testAction = new QAction(this);
        testAction->setObjectName(QString::fromUtf8("MeshSplit"));
        testAction->setText(QString::fromUtf8("MeshSplit"));
        ui->menu_filters->addAction(testAction);
        testAction->setVisible(false);
        connect(testAction, &QAction::triggered, this, [&](bool checked) {
#define TEST_MAP_BACK
#ifdef TEST_MAP_BACK
            auto model = rendererWidget->GetScene()->GetCurrentModel();
            if (!model || !model->GetDataObject()) {
                std::cout << "[Block Mapping Test] No model selected." << std::endl;
                return;
            }

            auto obj = iGame::FileIO::ReadFile("C:\\Users\\26280\\Downloads\\output_p600000_pr4000_t0.95_s42_npp.vtk");
            if (!obj) {
                std::cout << "[Block Mapping Test] Failed to read segment_result.vtk." << std::endl;
                return;
            }

            auto drawObj = DynamicCast<DrawObject>(model->GetDataObject());
            if (!drawObj) {
                std::cout << "[Block Mapping Test] Current model is not drawable." << std::endl;
                return;
            }
            drawObj->ConvertToDrawableData();
            auto surfaceMesh = DynamicCast<SurfaceMesh>(drawObj->GetRenderableObject(false));
            auto segmentedMesh = DynamicCast<UnstructuredMesh>(obj);
            if (!surfaceMesh || !segmentedMesh) {
                std::cout << "[Block Mapping Test] Unsupported original or segmented mesh type." << std::endl;
                return;
            }
            auto resultArray = BlockMapping::GetMappingBlockCellsArray(surfaceMesh, segmentedMesh);
            if (!resultArray) {
                std::cout << "[Block Mapping Test] Failed to map block IDs." << std::endl;
                return;
            }
            resultArray->SetName("block_id");
            auto dataObj = model->GetDataObject();
            dataObj->SetBlockMapping(resultArray);
            modelTreeWidget->updateAllAttriubute(dataObj);
            ui->widget_SearchInfo->setCurrentModel(model);
            std::cout << "[Block Mapping Test] Mapping complete. Cells: " << resultArray->GetNumberOfValues()
                      << std::endl;
#else
            // 测试P3SAM分割器
            auto model = rendererWidget->GetScene()->GetCurrentModel();
            if (!model) {
                std::cout << "[P3SAM Test] No model selected." << std::endl;
                return;
            }

            auto dataObj = model->GetDataObject();

            std::cout << "[P3SAM Test] Starting P3SAM segmentation..." << std::endl;
            P3SAMSegmenter::Pointer segmenter = P3SAMSegmenter::New();
            segmenter->SetInput(dataObj);
            segmenter->SetSimplificationRatio(0.1f);  // 简化到10%
            segmenter->SetPointNum(20000);
            segmenter->SetPromptNum(500);
            segmenter->SetSeed(42);
            segmenter->SetPostProcess(false);
            segmenter->SetTimeout(300000);  // 5分钟超时

            if (!segmenter->Execute()) {
                std::cout << "[P3SAM Test] Segmentation failed: "
                          << segmenter->GetErrorMessage() << std::endl;
                return;
            }

            modelTreeWidget->updateAllAttriubute(dataObj);
            std::cout << "[P3SAM Test] Segmentation complete! Parts: "
                      << segmenter->GetPartCount() << std::endl;
#endif // TEST_MAP_BACK
        });
    }
    //############# TESTS ED #############
    // PartSegmentation零件分割
    {
        QAction* partSegmentationAction{};
        partSegmentationAction = new QAction(this);
        partSegmentationAction->setObjectName(QString::fromUtf8("零件分割"));
        partSegmentationAction->setText(QString::fromUtf8("零件分割"));
        ui->menu_filters->addAction(partSegmentationAction);
        partSegmentationAction->setVisible(true);
        connect(partSegmentationAction, &QAction::triggered, this, [&](bool checked) {
            // 弹出 IP/Port 配置对话框
            QSettings settings("iGame", "iGameVis");
            QString savedHost = settings.value("P3SAM/host", "127.0.0.1").toString();
            int     savedPort = settings.value("P3SAM/port", 8765).toInt();

            igQtChromeFramelessDialog cfgDlg(this);
            cfgDlg.setDialogTitle(QStringLiteral("零件分割 - 服务器配置"));
            cfgDlg.setMaximizeEnabled(false);

            auto* body = new QWidget(cfgDlg.contentHost());
            body->setAttribute(Qt::WA_StyledBackground, true);
            body->setStyleSheet(
                "QWidget { background-color: transparent; color: #EAEAEA; }"
                "QLabel { color: #D8D8D8; }"
                "QLineEdit { background-color: #2A2A2A; color: #EAEAEA; border: 1px solid #3A3A3A;"
                "            padding: 4px 6px; border-radius: 3px; }"
                "QLineEdit:focus { border: 1px solid #5A7FA8; }"
                "QPushButton { background-color: #2A2A2A; color: #EAEAEA; border: 1px solid #3A3A3A;"
                "              padding: 6px 16px; border-radius: 4px; }"
                "QPushButton:hover { background-color: #3A3A3A; }"
                "QPushButton:pressed { background-color: #252526; }");

            auto* form = new QFormLayout(body);
            form->setContentsMargins(12, 12, 12, 12);
            form->setSpacing(10);
            auto* hostEdit = new QLineEdit(savedHost, body);
            auto* portEdit = new QLineEdit(QString::number(savedPort), body);
            portEdit->setValidator(new QIntValidator(1, 65535, body));
            form->addRow(QStringLiteral("服务器 IP："), hostEdit);
            form->addRow(QStringLiteral("端口："), portEdit);
            auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, body);
            form->addRow(btns);

            cfgDlg.setContentWidget(body);
            cfgDlg.resize(300, 160);

            bool accepted = false;
            QObject::connect(btns, &QDialogButtonBox::accepted, &cfgDlg, [&]() { accepted = true; cfgDlg.accept(); });
            QObject::connect(btns, &QDialogButtonBox::rejected, &cfgDlg, &QDialog::reject);
            cfgDlg.exec();
            if (!accepted) return;

            QString host = hostEdit->text().trimmed();
            int     port = portEdit->text().toInt();
            if (host.isEmpty()) host = "127.0.0.1";
            settings.setValue("P3SAM/host", host);
            settings.setValue("P3SAM/port", port);

            // 检查当前模型
            auto model = rendererWidget->GetScene()->GetCurrentModel();
            if (!model) {
                std::cout << "[PartSegmentation] No model selected." << std::endl;
                return;
            }

            auto dataObj = model->GetDataObject();

            std::cout << "[PartSegmentation] Starting P3SAM segmentation..." << std::endl;
            P3SAMSegmenter::Pointer segmenter = P3SAMSegmenter::New();
            segmenter->SetServerHost(host.toStdString());
            segmenter->SetServerPort(port);
            segmenter->SetInput(dataObj);
            segmenter->SetSimplificationRatio(0.1f);
            segmenter->SetPostProcess(false);
            segmenter->SetTimeout(300000);  // 5分钟超时

            if (!segmenter->Execute()) {
                std::cout << "[PartSegmentation] Segmentation failed: "
                          << segmenter->GetErrorMessage() << std::endl;
                return;
            }

            modelTreeWidget->updateAllAttriubute(dataObj);
            std::cout << "[PartSegmentation] Segmentation complete! Parts: "
                      << segmenter->GetPartCount() << std::endl;
        });
    }
    // 零件分割（自文件）
    {
        QAction* partSegmentationAction_fromFile{};
        partSegmentationAction_fromFile = new QAction(this);
        partSegmentationAction_fromFile->setObjectName(QString::fromUtf8("零件分割(自文件)"));
        partSegmentationAction_fromFile->setText(QString::fromUtf8("零件分割(自文件)"));
        ui->menu_filters->addAction(partSegmentationAction_fromFile);
        partSegmentationAction_fromFile->setVisible(true);
        connect(partSegmentationAction_fromFile, &QAction::triggered, this, [&](bool checked) {
            auto model = rendererWidget->GetScene()->GetCurrentModel();
            if (!model) {
                std::cout << "[PartSegFromFile] No model selected." << std::endl;
                return;
            }
            auto dataObj = model->GetDataObject();

            QString path = QFileDialog::getOpenFileName(
                this, QStringLiteral("选择分割结果文件"), QString(), "VTK Files (*.vtk);;All Files (*)");
            if (path.isEmpty()) return;

            DataObject::Pointer segmented = FileIO::ReadFile(path.toStdString());
            UnstructuredMesh::Pointer segMesh = DynamicCast<UnstructuredMesh>(segmented);
            if (!segMesh) {
                std::cout << "[PartSegFromFile] File is not an UnstructuredMesh: "
                          << path.toStdString() << std::endl;
                return;
            }

            IntArray::Pointer resultArray;
            IGenum type = dataObj->GetDataObjectType();
            switch (type) {
                case IG_SURFACE_MESH: {
                    SurfaceMesh::Pointer sm = DynamicCast<SurfaceMesh>(dataObj);
                    if (!sm) { std::cout << "[PartSegFromFile] Failed to cast to SurfaceMesh." << std::endl; return; }
                    resultArray = BlockMapping::GetMappingBlockCellsArray(sm, segMesh);
                    break;
                }
                case IG_UNSTRUCTURED_MESH: {
                    UnstructuredMesh::Pointer um = DynamicCast<UnstructuredMesh>(dataObj);
                    if (!um) { std::cout << "[PartSegFromFile] Failed to cast to UnstructuredMesh." << std::endl; return; }
                    resultArray = BlockMapping::GetMappingBlockCellsArray(um, segMesh);
                    break;
                }
                case IG_VOLUME_MESH: {
                    VolumeMesh::Pointer vm = DynamicCast<VolumeMesh>(dataObj);
                    if (!vm) { std::cout << "[PartSegFromFile] Failed to cast to VolumeMesh." << std::endl; return; }
                    resultArray = BlockMapping::GetMappingBlockCellsArray(vm, segMesh);
                    break;
                }
                default:
                    std::cout << "[PartSegFromFile] Unsupported mesh type." << std::endl;
                    return;
            }

            if (!resultArray) {
                std::cout << "[PartSegFromFile] Block mapping failed." << std::endl;
                return;
            }

            resultArray->SetName("part_id");
            dataObj->SetBlockMapping(resultArray);
            modelTreeWidget->updateAllAttriubute(dataObj);
            std::cout << "[PartSegFromFile] Done." << std::endl;
        });
    }
    // 零件聚焦弹窗
    {
        QAction* partFocusAction = new QAction(this);
        partFocusAction->setObjectName(QString::fromUtf8("零件聚焦"));
        partFocusAction->setText(QString::fromUtf8("零件聚焦"));
        ui->menu_filters->addAction(partFocusAction);
        partFocusAction->setVisible(true);
        connect(partFocusAction, &QAction::triggered, this, [&]() {
            if (!partFocusDialog) {
                partFocusDialog = new igQtChromeFramelessDialog(this);
                partFocusDialog->setDialogTitle(QStringLiteral("零件聚焦"));
                partFocusDialog->setMaximizeEnabled(false);
                partFocusWidget = new igQtPartFocusWidget(partFocusDialog->contentHost());
                connect(partFocusWidget, &igQtPartFocusWidget::SIGNAL_SelectedPartsChanged, this,
                        [this](const QVector<int>& partIds) {
                            auto model = rendererWidget->GetScene()->GetCurrentModel();
                            ui->widget_SearchInfo->setCurrentModel(model);
                            ui->widget_SearchInfo->setSelectedPartIds(partIds);
                        });
                partFocusDialog->setContentWidget(partFocusWidget);
                partFocusDialog->resize(340, 380);
            }
            partFocusWidget->SetScene(rendererWidget->GetScene(), rendererWidget);
            partFocusDialog->show();
            partFocusDialog->raise();
            partFocusDialog->activateWindow();
        });
    }
    // 报告生成
    {
        QAction* reportGenerateAction = new QAction(this);
        reportGenerateAction->setObjectName(QString::fromUtf8("报告生成"));
        reportGenerateAction->setText(QString::fromUtf8("报告生成"));
        ui->menu_help->addAction(reportGenerateAction);
        reportGenerateAction->setVisible(true);
        connect(reportGenerateAction, &QAction::triggered, this, [&]() {
            if (!reportGenerateDialog) {
                reportGenerateDialog = new igQtChromeFramelessDialog(this);
                reportGenerateDialog->setDialogTitle(QStringLiteral("报告生成"));
                reportGenerateDialog->setMaximizeEnabled(false);
                reportGenerateWidget = new igQtAttributeSelectWidget(reportGenerateDialog->contentHost());
                reportGenerateWidget->SetMaxSelectableCount(1);
                reportGenerateDialog->setContentWidget(reportGenerateWidget);
                reportGenerateDialog->resize(340, 380);
            }
            reportGenerateWidget->RefreshAttributeList();
            reportGenerateDialog->show();
            reportGenerateDialog->raise();
            reportGenerateDialog->activateWindow();
        });
    }
    connect(ui->widget_ParallelCoordinatesField, &igQtParallelCoordinatesWidget::SIGNAL_RefreshDataClicked, this,
            [&]() {
                auto model = rendererWidget->GetScene()->GetCurrentModel();
                if (model == nullptr) return;
                ui->widget_ParallelCoordinatesField->SetParallelCoordinates(model);
            });
    connect(ui->action_VariableCorrelation, &QAction::triggered, this, [&](bool checked) {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;

        // 使用动态属性存储对话框指针    // 匿名命名空间，只在当前cpp文件可见
        static igQtChromeFramelessDialog* dialog = nullptr;
        static igQtVariableCorrelationWidget* widget = nullptr;

        if (!dialog) {
            dialog = new igQtChromeFramelessDialog(this);
            dialog->setDialogTitle(QStringLiteral("变量相关性分析"));

            widget = new igQtVariableCorrelationWidget(dialog->contentHost());
            widget->GetUi()->splitter->setSizes({200, 300, 400});
            dialog->setContentWidget(widget);
            dialog->resize(900, 500);
            connect(widget, &igQtVariableCorrelationWidget::SIGNAL_RefreshDataClicked, this, [&]() {
                // 使用sender()获取信号发送者
                auto* senderWidget = qobject_cast<igQtVariableCorrelationWidget*>(sender());
                if (!senderWidget) return;
                auto model = rendererWidget->GetScene()->GetCurrentModel();
                if (model == nullptr) return;
                senderWidget->SetModel(model);
            });
        }

        widget->SetModel(model);
        dialog->show();
        dialog->raise();
        dialog->activateWindow();


        //ui->dockWidget_VariableCorrelationField->show();
        //ui->widget_VariableCorrelationField->SetModel(model);
    });

    connect(ui->widget_VariableCorrelationField, &igQtVariableCorrelationWidget::SIGNAL_RefreshDataClicked, this,
            [&]() {
                auto model = rendererWidget->GetScene()->GetCurrentModel();
                if (model == nullptr) return;
                ui->widget_VariableCorrelationField->SetModel(model);
            });
    connect(ui->action_VariableDensity, &QAction::triggered, this, [&](bool checked) {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        openLeftToolPanel(LeftToolPanelId::VariableDensity);
        ui->widget_VariableDensityField->SetModel(model);
    });

    connect(ui->widget_VariableDensityField, &igQtVariableDensityWidget::SIGNAL_RefreshDataClicked, this, [&]() {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        ui->widget_VariableDensityField->SetModel(model);
    });
    auto DataChangeFunc = [&](igQtMainWindow* mainWindow) {
        auto model = mainWindow->rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        mainWindow->openLeftToolPanel(LeftToolPanelId::DataChange);
        mainWindow->ui->widget_DataChangeField->InitRadialStyle(
                mainWindow->rendererWidget->GetScene()->GetInteractor());
        auto name = mainWindow->rendererWidget->GetScene()->GetInteractor()->SetSpecialInteractor(
                mainWindow->ui->widget_DataChangeField->GetRadialStyle());
        mainWindow->ui->widget_DataChangeField->SetInteractorName(name);
        mainWindow->ui->widget_DataChangeField->SetModel(model);
        mainWindow->ui->widget_DataChangeField->SetScene(mainWindow->rendererWidget->GetScene());
    };
    connect(ui->action_DataChange, &QAction::triggered, this, [&](bool checked) { DataChangeFunc(this); });
    connect(ui->widget_DataChangeField, &igQtDataChangeWidget::SIGNAL_RefreshDataClicked, this,
            [&]() { DataChangeFunc(this); });

    ui->action_ContextPreserving->setVisible(false);
    connect(ui->action_ContextPreserving, &QAction::triggered, this, [&](bool checked) {
        if (checked && !ui->dockWidget_ContextPreservingShowField->isVisible()) {
            auto model = rendererWidget->GetScene()->GetCurrentModel();
            if (model == nullptr) return;
            showAndRaiseDock(ui->dockWidget_ContextPreservingShowField);
            ui->widget_ContextPreservingShowField->SetContextPreserving(model);
        } else if (!checked && ui->dockWidget_ContextPreservingShowField->isVisible())
            ui->dockWidget_ContextPreservingShowField->hide();
    });
    connect(modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged, this, [&]() {
        if (ui->dockWidget_ContextPreservingShowField->isHidden()) return;
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) {
            ui->dockWidget_ContextPreservingShowField->hide();
            return;
        }
        ui->widget_ContextPreservingShowField->SetContextPreserving(model);
    });
    connect(ui->widget_ContextPreservingShowField, &igQtContextPreservingShowWidget::DrawUpdated, this,
            [&]() { rendererWidget->update(); });
    connect(ui->action_FlowField, &QAction::triggered, this,
            [this](bool) { openLeftToolPanel(LeftToolPanelId::Flow); });

//    connect(ui->action_FlowField_2, &QAction::triggered, this, [&](bool checked) {
//        ui->dockWidget_FlowField->show();
//        ui->widget_FlowField->updateVectorNameList();
//    });

    //  connect(ui->action_EditMode, &QAction::triggered, this, [&](bool checked)
    //  {
    //	ui->dockWidget_EditMode->show();
    //	});
    //  connect(ui->action_QualityDetection, &QAction::triggered, this, [&](bool
    //  checked) { 	ui->dockWidget_QualityDetection->show();
    //	});
    connect(ui->action_ContourExtract, &QAction::triggered, this, [this](bool) {
        openLeftToolPanel(LeftToolPanelId::ContourExtract);
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        if (!scene) return;
        auto CurrentModel = scene->GetCurrentModel();
        if (!CurrentModel) return;
        auto dataObject = CurrentModel->GetDataObject();
        if (!dataObject) return;
        ui->widget_ContourExtract->SetOriginDataObject(dataObject);
    });

    connect(ui->action_GenerateChart, &QAction::triggered, this, [&](bool checked) {
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        if (!scene) return;
        auto CurrentModel = scene->GetCurrentModel();
        if (!CurrentModel) return;
        auto dataObject = CurrentModel->GetDataObject();
        if (!dataObject) return;
        auto attributeSet = dataObject->GetAttributeSet();
        auto dataIndex = dataObject->GetAttributeIndex();
        auto attrDimension = dataObject->GetAttributeDimension();
        if (dataIndex < 0) { return; }
        auto array = attributeSet->GetAttribute(dataIndex).pointer;
        if (array == nullptr) return;
        ArrayObject::Pointer drawArray = nullptr;
        if (array->GetDimension() <= 1) {
            drawArray = array;
        } else {
            auto tmpArray = FloatArray::New();
            int size = array->GetNumberOfElements();
            tmpArray->Reserve(size);
            tmpArray->SetName(array->GetName());
            for (int i = 0; i < size; i++) { tmpArray->AddValue(array->GetElementValue(i, attrDimension)); }
            drawArray = tmpArray;
        }
        auto chart = new igQtCharts;
        chart->drawBarChart(drawArray);
        chart->exec();
    });
    auto DrawSurfaceMeshByPointer = [](SurfaceMesh::Pointer m, Painter3D* painter, const float color[3]) -> void {
        // 1. draw faces
        painter->SetPen(Pen::Style::NoPen);
        painter->SetBrush(color[0], color[1], color[2]);
        igIndex cell[32]{};
        for (int i = 0; i < m->GetNumberOfFaces(); i++) {
            int ncell = m->GetFacePointIds(i, cell);
            for (int j = 2; j < ncell; j++) {
                painter->DrawTriangle(m->GetPoint(cell[0]), m->GetPoint(cell[j - 1]), m->GetPoint(cell[j]));
            }
        }
        // 2. draw lines
        painter->SetPen(Color::Black);
        painter->SetBrush(Brush::Style::NoBrush);
        if (m->GetEdges() == nullptr) { m->BuildEdges(); }
        for (int i = 0; i < m->GetNumberOfEdges(); i++) {
            int ncell = m->GetEdgePointIds(i, cell);
            if (cell[0] < 0 || cell[1] < 0) {
                throw std::runtime_error("The index of the edge is negative.");
            } else {
                painter->DrawLine(m->GetPoint(cell[0]), m->GetPoint(cell[1]));
            }
        }
        painter->Modified();
    };

    auto AddClippingMeshToScene = [DrawSurfaceMeshByPointer](const std::string& mainName, SurfaceMesh::Pointer OV,
                                                             SurfaceMesh::Pointer t_IV, SurfaceMesh::Pointer OIV,
                                                             igQtModelDialogWidget* modelTreeWidget) {
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();

        const std::string OVName = "__" + mainName + "_OV";   // 临时模型
        const std::string IVName = "__" + mainName + "_IV";   // 临时模型
        const std::string OIVName = "__" + mainName + "_OIV"; // 临时模型
        const float OVColor[3]{1.f, 1.f, 1.f};
        const float IVColor[3]{1.f, 1.f, 0.f};
        const float OIVColor[3]{1.f, 1.f, 1.f};
        const float OIVAlpha = 0.2f;

        SurfaceMesh::Pointer IV = SurfaceMesh::New();
        OV->SetName(OVName);
        IV->SetName(IVName);
        OIV->SetName(OIVName);

        Model* IVModel{nullptr};
        bool exist[3]{false, false, false};

        auto modelList = scene->GetModelList();
        for (auto it = modelList->Begin(); it != modelList->End(); ++it) {
            auto id = it->first;
            auto model = it->second;

            if (model->GetDataObject()->GetName() == OVName) {
                auto model = scene->GetModelById(id);
                model->SetDataObject(OV);
                exist[0] = true;
            } else if (model->GetDataObject()->GetName() == IVName) {
                auto model = scene->GetModelById(id);
                model->SetDataObject(IV);
                exist[1] = true;
                IVModel = model;
            } else if (model->GetDataObject()->GetName() == OIVName) {
                auto model = scene->GetModelById(id);
                model->SetDataObject(OIV);
                exist[2] = true;
            }
        }
        if (!exist[0]) modelTreeWidget->addDataObjectToModelTree(OV, ItemSource::Algorithm);
        if (!exist[1]) {
            int id = modelTreeWidget->addDataObjectToModelTree(IV, ItemSource::Algorithm);
            IVModel = scene->GetModelById(id);
        }
        if (!exist[2]) modelTreeWidget->addDataObjectToModelTree(OIV, ItemSource::Algorithm);

        DrawSurfaceMeshByPointer(t_IV, IVModel->GetPainter3D(), IVColor);

        //OV->SetFaceColor(OVColor);
        OV->SetViewStyle(IG_SURFACE | IG_WIREFRAME);
        //IV->SetFaceColor(IVColor);
        //IV->SetViewStyle(IG_SURFACE | IG_WIREFRAME);
        //OIV->SetFaceColor(OIVColor);
        OIV->SetTransparency(OIVAlpha);
        OIV->SetViewStyle(IG_SURFACE);
    };

    connect(ui->action_BoxClipping_Better, &QAction::triggered, this, [&](bool checked) {
        if (!rendererWidget->GetScene()->GetCurrentModel()) {
            std::cout << "Need Input" << std::endl;
            return;
        }
        bool ok;
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        auto oldCurrentModel = scene->GetCurrentModel();
        auto dataObject = scene->GetCurrentModel()->GetDataObject();
        auto drawObject = DynamicCast<DrawObject>(dataObject);
        static std::vector<int> supportTypes = {IG_VOLUME_MESH, IG_UNSTRUCTURED_MESH, IG_STRUCTURED_MESH};
        if (std::find(supportTypes.begin(), supportTypes.end(), dataObject->GetDataObjectType()) ==
            supportTypes.end()) {
            std::cout << "This type of mesh can't be clipped" << std::endl;
            return;
        }

        auto inputMesh = DynamicCast<iGame::DataObject>(dataObject);
        auto box = inputMesh->GetBoundingBox();
        auto center = (box.min + box.max) * 0.5;
        auto size = box.max - box.min;

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("盒子切割（美观）"));
        int x_min_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "x_min(0..1)", "0.0");
        int y_min_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "y_min(0..1)", "0.0");
        int z_min_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "z_min(0..1)", "0.0");
        int x_max_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "x_max(0..1)", "0.5");
        int y_max_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "y_max(0..1)", "1.0");
        int z_max_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "z_max(0..1)", "1.0");
        int flip_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, "flip", "false");
        dialog->show();

        dialog->setApplyFunctor([=, this]() {
            bool ok;
            auto Clamp = [](double x, double l, double r) -> double {
                if (x < l) return l;
                if (x > r) return r;
                return x;
            };

            double x_min = box.min[0] + size[0] * Clamp(dialog->getDouble(x_min_id, ok), 0., 1.);
            double y_min = box.min[1] + size[1] * Clamp(dialog->getDouble(y_min_id, ok), 0., 1.);
            double z_min = box.min[2] + size[2] * Clamp(dialog->getDouble(z_min_id, ok), 0., 1.);
            double x_max = box.min[0] + size[0] * Clamp(dialog->getDouble(x_max_id, ok), 0., 1.);
            double y_max = box.min[1] + size[1] * Clamp(dialog->getDouble(y_max_id, ok), 0., 1.);
            double z_max = box.min[2] + size[2] * Clamp(dialog->getDouble(z_max_id, ok), 0., 1.);
            bool flip = dialog->getChecked(flip_id, ok);

            auto clipper = iGameVolumeMeshClipper::New();
            clipper->SetInput(0, dataObject);
            clipper->SetExtent(x_min, x_max, y_min, y_max, z_min, z_max, flip);
            clipper->Execute();
            auto OV = DynamicCast<SurfaceMesh>(clipper->GetOutput(0));
            auto IV = DynamicCast<SurfaceMesh>(clipper->GetOutput(1));
            auto OIV = DynamicCast<SurfaceMesh>(clipper->GetOutput(2));
            AddClippingMeshToScene(dataObject->GetName(), OV, IV, OIV, modelTreeWidget);
            drawObject->SetVisibility(false);
            scene->SetCurrentModel(oldCurrentModel);
            rendererWidget->update();
        });
    });

    connect(ui->action_PlaneClipping_Better, &QAction::triggered, this, [&](bool checked) {
        if (!rendererWidget->GetScene()->GetCurrentModel()) {
            std::cout << "Need Input" << std::endl;
            return;
        }
        bool ok;
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        auto oldCurrentModel = scene->GetCurrentModel();
        auto dataObject = scene->GetCurrentModel()->GetDataObject();
        auto drawObject = DynamicCast<DrawObject>(dataObject);
        static std::vector<int> supportTypes = {IG_VOLUME_MESH, IG_UNSTRUCTURED_MESH, IG_STRUCTURED_MESH};
        if (std::find(supportTypes.begin(), supportTypes.end(), dataObject->GetDataObjectType()) ==
            supportTypes.end()) {
            std::cout << "This type of mesh can't be clipped" << std::endl;
            return;
        }

        auto inputMesh = DynamicCast<iGame::DataObject>(dataObject);
        auto box = inputMesh->GetBoundingBox();
        auto center = (box.min + box.max) * 0.5;
        auto size = box.max - box.min;

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("平面切割（美观）"));
        int origin_x_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "origin_x(0..1)", "0.5");
        int origin_y_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "origin_y(0..1)", "0.5");
        int origin_z_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "origin_z(0..1)", "0.5");
        int normal_x_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "normal_x(-1..1)", "1.0");
        int normal_y_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "normal_y(-1..1)", "0.0");
        int normal_z_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "normal_z(-1..1)", "0.0");
        int flip_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, "flip", "false");
        dialog->show();

        dialog->setApplyFunctor([=, this]() {
            bool ok;
            auto Clamp = [](double x, double l, double r) -> double {
                if (x < l) return l;
                if (x > r) return r;
                return x;
            };

            double origin_x = box.min[0] + size[0] * Clamp(dialog->getDouble(origin_x_id, ok), 0., 1.);
            double origin_y = box.min[1] + size[1] * Clamp(dialog->getDouble(origin_y_id, ok), 0., 1.);
            double origin_z = box.min[2] + size[2] * Clamp(dialog->getDouble(origin_z_id, ok), 0., 1.);
            double normal_x = Clamp(dialog->getDouble(normal_x_id, ok), -1., 1.);
            double normal_y = Clamp(dialog->getDouble(normal_y_id, ok), -1., 1.);
            double normal_z = Clamp(dialog->getDouble(normal_z_id, ok), -1., 1.);
            bool flip = dialog->getChecked(flip_id, ok);
            if (normal_x == 0. && normal_y == 0. && normal_z == 0.) {
                std::cout << "Normal is a vector of zero" << std::endl;
                return;
            }

            auto clipper = iGameVolumeMeshClipper::New();
            clipper->SetInput(0, dataObject);
            clipper->SetPlane(origin_x, origin_y, origin_z, normal_x, normal_y, normal_z, flip);
            clipper->Execute();
            auto OV = DynamicCast<SurfaceMesh>(clipper->GetOutput(0));
            auto IV = DynamicCast<SurfaceMesh>(clipper->GetOutput(1));
            auto OIV = DynamicCast<SurfaceMesh>(clipper->GetOutput(2));
            AddClippingMeshToScene(dataObject->GetName(), OV, IV, OIV, modelTreeWidget);
            drawObject->SetVisibility(false);
            scene->SetCurrentModel(oldCurrentModel);
            rendererWidget->update();
        });
    });

    connect(ui->action_slice, &QAction::triggered, this, [this](bool) {
        const int sid = static_cast<int>(LeftToolPanelId::Slice);
        const int existing = m_leftToolTabByPanel[static_cast<size_t>(sid)];
        if (existing >= 0 && m_leftFieldDock && m_leftFieldDock->isVisible() && m_leftFieldTabs &&
            m_leftFieldTabs->currentIndex() == existing) {
            return;
        }
        openLeftToolPanel(LeftToolPanelId::Slice);
        if (!rendererWidget->GetScene() || !rendererWidget->GetScene()->GetCurrentModel()) return;
        auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        if (!obj) return;
        SliceWidget->SetOriginDataObject(obj);
        rendererWidget->getInteractor()->SetDataObject(obj);
        rendererWidget->getInteractor()->SetPainter3D(rendererWidget->GetScene()->GetCurrentModel()->GetPainter3D());
        rendererWidget->getInteractor()->RequestSlicingStyle(SliceWidget->GetSelection());
    });
    connect(SliceWidget, &igQtModelClipWidget::DrawClipModel, this,
            [&](DrawObject::Pointer mesh) { modelTreeWidget->addDataObjectToModelTree(mesh, ItemSource::Algorithm); });
    connect(SliceWidget, &igQtModelClipWidget::UpdateClipModel, this, [&]() {
        modelTreeWidget->updateCurrentModelInfo();
        rendererWidget->update();
    });
    connect(SliceWidget, &igQtModelClipWidget::ResetInteractor, this, [&]() {
        if (!rendererWidget->getInteractor()->IsBasicStyle()) {
            rendererWidget->getInteractor()->RequestBasicStyle();
            return;
        }
    });
    connect(ui->action_deformation, &QAction::triggered, this, [this](bool checked) {
        if (checked)
            openLeftToolPanel(LeftToolPanelId::Deformation);
        else
            closeLeftToolPanel(LeftToolPanelId::Deformation);
    });
}

void igQtMainWindow::closeHistogramParameters() {
    if (!m_histogramParametersDock) return;
    closeLeftToolPanel(LeftToolPanelId::HistogramParameters);
    auto* dialog = m_histogramParametersDock.data();
    m_histogramParametersDock = nullptr;
    m_histogramParameterContents = nullptr;
    dialog->close();
}

QDockWidget* igQtMainWindow::shellDockForLeftPanel(LeftToolPanelId id) const {
    switch (id) {
    case LeftToolPanelId::Scalar: return ui->dockWidget_ScalarField;
    case LeftToolPanelId::Vector: return ui->dockWidget_VectorField;
    case LeftToolPanelId::Tensor: return ui->dockWidget_TensorField;
    case LeftToolPanelId::Flow: return ui->dockWidget_FlowField;
    case LeftToolPanelId::ContourExtract: return ui->dockWidget_ContourExtract;
    case LeftToolPanelId::Slice: return SliceDockWidget;
    case LeftToolPanelId::Deformation: return DeformationDockWidget;
    case LeftToolPanelId::Selection: return ui->dockWidget_SelectionField;
    case LeftToolPanelId::VariableDensity: return ui->dockWidget_VariableDensityField;
    case LeftToolPanelId::DataChange: return ui->dockWidget_DataChangeField;
    case LeftToolPanelId::ExtractTimeSteps: return m_extractTimeStepsShell;
    case LeftToolPanelId::MedianFilter: return ui->dockWidget_MedianFilter;
    case LeftToolPanelId::ResampleToImage: return m_resampleToImageDock;
    case LeftToolPanelId::HistogramParameters: return m_histogramParametersDock.data();
    case LeftToolPanelId::HistogramSeries: return m_histogramSeriesDock;
    case LeftToolPanelId::Count: return nullptr;
    }
    return nullptr;
}

QWidget* igQtMainWindow::wrapContentInScrollArea(QWidget* content, QWidget* parent, bool centerFlowField) {
    if (!content) return nullptr;
    if (qobject_cast<QScrollArea*>(content)) return content;
    content->setMinimumHeight(0);
    content->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    auto* scroll = new QScrollArea(parent);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setWidget(content);
    if (centerFlowField) scroll->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    return scroll;
}

void igQtMainWindow::applyLeftToolStackVerticalSplit() {
    if (!m_leftFieldDock || !modelTreeWidget) return;
    QDockWidget* props = modelTreeWidget->getPropertiesDock();
    if (!props || !m_leftFieldDock->isVisible()) return;
    // 工具面板 : Properties = 1:1（事件循环跑完后再调，否则常不生效）
    resizeDocks({m_leftFieldDock, props}, {1, 1}, Qt::Vertical);
}

void igQtMainWindow::relocateContentToLeftTab(QDockWidget* shell, QWidget* inner, const QString& title, LeftToolPanelId id,
                                              bool centerFlowField) {
    const int pid = static_cast<int>(id);
    if (!m_leftFieldTabs || !m_leftFieldDock || !inner) return;
    if (m_leftToolTabByPanel[static_cast<size_t>(pid)] >= 0) {
        const int t = m_leftToolTabByPanel[static_cast<size_t>(pid)];
        if (t < m_leftFieldTabs->count()) m_leftFieldTabs->setCurrentIndex(t);
        m_leftFieldDock->show();
        m_leftFieldDock->raise();
        return;
    }
    if (shell) {
        QWidget* top = shell->widget();
        if (top == inner) {
            shell->setWidget(nullptr);
            removeDockWidget(shell);
            shell->hide();
        } else if (auto* sa = qobject_cast<QScrollArea*>(top)) {
            if (sa->widget() == inner) {
                sa->takeWidget();
                shell->setWidget(nullptr);
                delete sa;
                removeDockWidget(shell);
                shell->hide();
            }
        }
    }
    const bool firstTabInStack = (m_leftFieldTabs->count() == 0);
    QWidget* page = wrapContentInScrollArea(inner, m_leftFieldTabs, centerFlowField);
    const int idx = m_leftFieldTabs->addTab(page, title);
    m_leftToolTabByPanel[static_cast<size_t>(pid)] = idx;
    m_leftFieldDock->show();
    m_leftFieldDock->raise();
    m_leftFieldTabs->setCurrentIndex(idx);
    if (firstTabInStack) {
        QTimer::singleShot(0, this, [this]() { applyLeftToolStackVerticalSplit(); });
    }
}

void igQtMainWindow::openLeftToolPanel(LeftToolPanelId id) {
    switch (id) {
    case LeftToolPanelId::Scalar:
        relocateContentToLeftTab(ui->dockWidget_ScalarField, ui->widget_ScalarField, QStringLiteral("标量场"), id, false);
        break;
    case LeftToolPanelId::Vector:
        relocateContentToLeftTab(ui->dockWidget_VectorField, ui->widget_VectorField, QStringLiteral("矢量场"), id, false);
        ui->widget_VectorField->updateVectorNameList();
        break;
    case LeftToolPanelId::Tensor:
        relocateContentToLeftTab(ui->dockWidget_TensorField, ui->widget_TensorField, QStringLiteral("张量场"), id, false);
        ui->widget_TensorField->InitTensorWidget();
        break;
    case LeftToolPanelId::Flow:
        relocateContentToLeftTab(ui->dockWidget_FlowField, ui->widget_FlowField, QStringLiteral("流场"), id, true);
        ui->widget_FlowField->updateVectorNameList();
        break;
    case LeftToolPanelId::ContourExtract:
        relocateContentToLeftTab(ui->dockWidget_ContourExtract, ui->widget_ContourExtract, QStringLiteral("轮廓提取"), id, false);
        break;
    case LeftToolPanelId::Slice:
        relocateContentToLeftTab(SliceDockWidget, SliceWidget, QStringLiteral("网格切面"), id, false);
        break;
    case LeftToolPanelId::Deformation:
        relocateContentToLeftTab(DeformationDockWidget, DeformationWidget, QStringLiteral("结构形变"), id, false);
        break;
    case LeftToolPanelId::Selection:
        relocateContentToLeftTab(ui->dockWidget_SelectionField, ui->widget_SelectionField, QStringLiteral("选择"), id, false);
        break;
    case LeftToolPanelId::VariableDensity:
        relocateContentToLeftTab(ui->dockWidget_VariableDensityField, ui->widget_VariableDensityField,
                                 QStringLiteral("变量数据密度"), id, false);
        break;
    case LeftToolPanelId::DataChange:
        relocateContentToLeftTab(ui->dockWidget_DataChangeField, ui->widget_DataChangeField, QStringLiteral("路径图"), id,
                                 false);
        break;
    case LeftToolPanelId::ExtractTimeSteps:
        relocateContentToLeftTab(m_extractTimeStepsShell, m_extractTimeStepsWidget,
                                 QStringLiteral("保留指定时间步"), id, false);
        break;
    case LeftToolPanelId::MedianFilter:
        relocateContentToLeftTab(ui->dockWidget_MedianFilter, ui->widget_MedianFilter, QStringLiteral("中值滤波"), id, false);
        break;
    case LeftToolPanelId::ResampleToImage:
        relocateContentToLeftTab(m_resampleToImageDock, m_resampleToImageWidget, QStringLiteral("重采样到图像"), id, false);
        break;
    case LeftToolPanelId::HistogramParameters:
        relocateContentToLeftTab(m_histogramParametersDock.data(), m_histogramParameterContents,
                                 QStringLiteral("直方图参数"), id, false);
        break;
    case LeftToolPanelId::HistogramSeries:
        if (m_histogramChartWidget) {
            relocateContentToLeftTab(m_histogramSeriesDock, m_histogramChartWidget->settingsWidget(),
                                     QStringLiteral("直方图系列"), id, false);
            m_histogramChartWidget->settingsWidget()->show();
        }
        break;
    case LeftToolPanelId::Count:
        break;
    }
}

void igQtMainWindow::onLeftToolTabCloseRequested(int index) {
    if (index < 0) return;
    for (size_t i = 0; i < m_leftToolTabByPanel.size(); ++i) {
        if (m_leftToolTabByPanel[i] == index) {
            closeLeftToolPanel(static_cast<LeftToolPanelId>(i));
            return;
        }
    }
}

void igQtMainWindow::closeLeftToolPanel(LeftToolPanelId id) {
    const int pid = static_cast<int>(id);
    if (!m_leftFieldTabs) return;
    int idx = m_leftToolTabByPanel[static_cast<size_t>(pid)];
    if (idx < 0 || idx >= m_leftFieldTabs->count()) return;

    QWidget* page = m_leftFieldTabs->widget(idx);
    auto* scroll = qobject_cast<QScrollArea*>(page);
    QWidget* inner = scroll ? scroll->takeWidget() : nullptr;
    if (scroll) scroll->deleteLater();

    QDockWidget* shell = shellDockForLeftPanel(id);
    if (shell && inner) {
        shell->setWidget(inner);
        if (id == LeftToolPanelId::Deformation)
            addDockWidget(Qt::RightDockWidgetArea, shell);
        else
            addDockWidget(Qt::LeftDockWidgetArea, shell);
        shell->hide();
    }

    m_leftFieldTabs->removeTab(idx);
    m_leftToolTabByPanel[static_cast<size_t>(pid)] = -1;
    for (size_t i = 0; i < m_leftToolTabByPanel.size(); ++i) {
        int& t = m_leftToolTabByPanel[i];
        if (t == idx) t = -1;
        else if (t > idx) --t;
    }
    if (id == LeftToolPanelId::Slice && rendererWidget && rendererWidget->getInteractor() &&
        !rendererWidget->getInteractor()->IsBasicStyle()) {
        rendererWidget->getInteractor()->RequestBasicStyle();
    }
    if (id == LeftToolPanelId::Deformation && ui->action_deformation) ui->action_deformation->setChecked(false);
    if (id == LeftToolPanelId::Selection && ui->action_SelectView) ui->action_SelectView->setChecked(false);
    if (m_leftFieldTabs->count() == 0 && m_leftFieldDock) m_leftFieldDock->hide();
}

void igQtMainWindow::initAllMySignalConnections() {
    connect(fileLoader, &igQtFileLoader::RemoteCachedDatasetReattach, this,
            [this](DataObject::Pointer object) {
        if (!modelTreeWidget->getItemFromObject(object)) {
            modelTreeWidget->addDataObjectToModelTree(object, ItemSource::File);
        }
    }, Qt::DirectConnection);
    connect(fileLoader, &igQtFileLoader::RemoteCachedDatasetDetach, this,
            [this](DataObject::Pointer object, bool* detached) {
        auto* item = modelTreeWidget->getItemFromObject(object);
        if (!item) { return; }
        modelTreeWidget->setCurrentItem(item);
        modelTreeWidget->deleteCurrentModel();
        *detached = modelTreeWidget->getItemFromObject(object) == nullptr;
        igDebug("[RemoteMemoryCache] Detach tree verification: item_present={}",
                modelTreeWidget->getItemFromObject(object) != nullptr);
    }, Qt::DirectConnection);
    connect(fileLoader, &igQtFileLoader::RemoteRenderRequested,
            rendererWidget, &igQtRenderWidget::RequestCompletedFrame);
    connect(fileLoader, &igQtFileLoader::RemoteRenderCancelled,
            rendererWidget, &igQtRenderWidget::CancelCompletedFrame);
    connect(rendererWidget, &igQtRenderWidget::CompletedFrame,
            fileLoader, &igQtFileLoader::NotifyRemoteFrameCompleted);
    connect(fileLoader, &igQtFileLoader::RemoteCachedModelSelected, this,
            [this](DataObject::Pointer object) {
        auto item = modelTreeWidget->getItemFromObject(object);
        if (item) {
            item->setExpanded(true);
            // Selection only. Do NOT call viewAttribute()/FinishReading():
            // those would regenerate the complete scalar color arrays.
            modelTreeWidget->setCurrentItem(item);
        }
        rendererWidget->update();
        rendererWidget->getColorBarWidget()->update();
    });
    // connect(rendererWidget, &igQtModelDrawWidget::insertToModelListView,
    // ui->modelTreeView, &igQtModelListView::InsertModel);

    connect(fileLoader, &igQtFileLoader::NewModel, modelTreeWidget, &igQtModelDialogWidget::addDataObjectToModelTree);
    connect(fileLoader, &igQtFileLoader::FinishReading, this, &igQtMainWindow::updateRecentFilePaths);
    connect(ui->action_DeleteMesh, &QAction::triggered, modelTreeWidget, &igQtModelDialogWidget::deleteCurrentModel);
    connect(ui->action_DeleteMesh, &QAction::triggered, fileLoader, &igQtFileLoader::ReleaseDetachedRemoteGpuResources);

    connect(ui->action_DeleteMesh, &QAction::triggered, this, [&](bool){
        if (vortexMetricsLabel) {
            vortexMetricsLabel->clear();
            vortexMetricsLabel->hide();
        }
    });

    // connect(fileLoader, &igQtFileLoader::FinishReading, this,
    // &igQtMainWindow::updateViewStyleAndCloudPicture); connect(fileLoader,
    // &igQtFileLoader::FinishReading, this,
    // &igQtMainWindow::updateCurrentSceneWidget);

    connect(fileLoader, &igQtFileLoader::FinishReading, ui->widget_Animation, [&](){
        ui->widget_Animation->initAnimationComponents();
    });
    connect(fileLoader, &igQtFileLoader::FinishReading, DeformationWidget, [this](){
        DeformationWidget->updateInfo();
    });

    connect(fileLoader, &igQtFileLoader::FinishReading, this, [&]() {
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        if (!scene) return;

        auto model = scene->GetCurrentModel();
        if (!model) return;

        auto dataObject = model->GetDataObject();
        if (!dataObject) return;

        auto attributeSet = dataObject->GetAttributeSet();
        if (!attributeSet) return;

        auto allAttributes = attributeSet->GetAllAttributes();
        if (!allAttributes || allAttributes->GetNumberOfElements() == 0) return;

        auto drawObject = DynamicCast<DrawObject>(dataObject);
        if (drawObject) {
            auto item = modelTreeWidget->getItemFromObject(dataObject);
            if (item && item->childCount() > 0) {
                item->setExpanded(true);
                auto child = item->child(0);
                item->setCurrentChild(child);
                item->setSelected(false);
                item->viewAttribute(0, -1);
                child->setSelected(true);
                modelTreeWidget->setCurrentItem(child);
            }
        }
    });

    connect(ui->widget_FlowField, &igQtStreamTracerWidget::AddStreamObject, this, [&](iGame::DataObject::Pointer res) {
        streamTreeIndex=modelTreeWidget->addDataObjectToModelTree(res, ItemSource::Algorithm);
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        scene->GetCurrentModel()->SetViewWireframeSwitch(true);
    });
    connect(ui->widget_FlowField, &igQtStreamTracerWidget::UpdateStreamObject, this,
            [&](iGame::DataObject::Pointer res) {
                auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
                if (scene->GetCurrentModelID() == streamTreeIndex) {
                    modelTreeWidget->updateCurrentModelProperty();
                }
                modelTreeWidget->updateAllAttriubute(res);
                rendererWidget->update();
                auto drawObject = DynamicCast<DrawObject>(res);
                if (drawObject) {
                    auto item = modelTreeWidget->getItemFromObject(res);
                    if (item && item->childCount() > 0) {
                        item->setExpanded(true);
                        auto child = item->child(0);
                        item->setCurrentChild(child);
                        item->setSelected(false);
                        item->viewAttribute(0, -1);
                        child->setSelected(true);
                        modelTreeWidget->setCurrentItem(child);
                    }
                }
            });


    connect(fileLoader, &igQtFileLoader::AddFileToModelList, ui->modelTreeView, &igQtModelListView::AddModel);


    /* Animation signal connect BEGIN.*/
    connect(ui->widget_Animation, &igQtAnimationWidget::UpdateScene,
            this, &igQtMainWindow::UpdateRenderingWidget);
    // Update scalar view UI when animation frame changes (updates DataRange slider and info label)
    connect(ui->widget_Animation, &igQtAnimationWidget::AnimationFrameChanged,
            ui->widget_ScalarField, &igQtScalarViewWidget::showScalarView);

    connect(ui->widget_Animation, &igQtAnimationWidget::AnimationFrameChanged,
            this, [&](){
                ui->widget_VectorField->drawV();
            });

    connect(ui->widget_Animation, &igQtAnimationWidget::AnimationFrameChanged, this, [&](){
        SliceWidget->ClipModel();
//        SliceWidget->UpdateOriginDataObject()
    });
    connect(ui->widget_Animation, &igQtAnimationWidget::AnimationDataChanged,
            this, [this]() {
                auto* scene = rendererWidget ? rendererWidget->GetScene() : nullptr;
                auto model = scene ? scene->GetCurrentModel() : nullptr;
                auto data = model ? model->GetDataObject() : nullptr;
                if (data && modelTreeWidget) {
                    modelTreeWidget->refreshAnimationAttributes(data);
                }
            });
//    connect(ui->widget_Animation, &igQtAnimationWidget::AnimationFrameChanged,
//            DeformationWidget, &igQtDeformationWidget::updateInfo);

    //connect(ui->widget_QualityDetection,
    //&igQtQualityDetectionWidget::updateCurrentModelColor, rendererWidget,
    //&igQtModelDrawWidget::UpdateCurrentModel);
    connect(ui->widget_ScalarField, &igQtScalarViewWidget::changeColorBarShow, this,
            &igQtMainWindow::updateColorBarShow);
    /* Animation signal connect END.*/


    /* Model Tree signal connect BEGIN.*/
    // 色条按"选中优先"的数据对象工作：把模型树当前选中对象（多块组内为子块）推给
    // 色条控件后再刷新 —— 此前固定读当前模型根对象，组根未着色（属性下标 -1）
    // 会导致色条不更新（停留/回落默认范围）。
    connect(this->modelTreeWidget, &igQtModelDialogWidget::CloudPictureChanged, this, [this]() {
        auto currentObj = this->modelTreeWidget->GetCurrentDataObject();
        ui->widget_ScalarField->setCurrentDataObject(currentObj);
        ui->widget_ScalarField->showScalarView();
        // 左侧色条图例是场景上的覆盖 widget（igQtColorBarWidget），它独立读 scene 的
        // 当前模型根对象 —— 同样必须改成"选中优先"，否则子块着色时它仍显示 0-255。
        if (this->rendererWidget && this->rendererWidget->getColorBarWidget()) {
            this->rendererWidget->getColorBarWidget()->setCurrentDataObject(currentObj);
            this->rendererWidget->getColorBarWidget()->update();
        }
    });
    // Update Deformation Info when model is deleted
    connect(this->modelTreeWidget, &igQtModelDialogWidget::ModelDeleted,
            DeformationWidget, &igQtDeformationWidget::updateInfo);
    connect(this->modelTreeWidget, &igQtModelDialogWidget::ModelDeleted,
            ui->widget_Animation, &igQtAnimationWidget::initAnimationComponents);
    // 删除高程输出节点时自动关闭右侧 Elevation 参数面板
    connect(this->modelTreeWidget, &igQtModelDialogWidget::ModelDeleted,
            ElevationFilterPanel, &igQtElevationFilterPanel::onModelDeleted);

    // Update animation controls when model changes
    connect(this->modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged,
            ui->widget_Animation, &igQtAnimationWidget::initAnimationComponents);
    // Update Deformation Info when model changes
    connect(this->modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged,
            DeformationWidget, &igQtDeformationWidget::updateInfo);

    // Refresh the stream-tracer vector list when the current model changes.
    // 只在面板可见时刷新：pickSourceModel/isUsableSource 会遍历全部单元，
    // 模型树点击很频繁，大网格上无谓地跑一遍代价不小。
    connect(this->modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged, this, [this]() {
        if (!ui->widget_FlowField || !ui->widget_FlowField->isVisible()) return;
        ui->widget_FlowField->refreshVectorCombo();
    });

    // Rebind the contour-extraction panel when the current model changes
    connect(this->modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged, this, [this]() {
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        if (!scene) return;
        auto model = scene->GetCurrentModel();
        if (!model) return;
        auto dataObject = model->GetDataObject();
        if (!dataObject) return;
        ui->widget_ContourExtract->SetOriginDataObject(dataObject);
    });

    /* Model Tree signal connect END.*/

    connect(ui->widget_ScalarField, &igQtScalarViewWidget::ChangeShowColorManager, this, [&]() {
        if (this->ColorManagerWidget->isHidden()) {
            // 打开前先按"选中优先"把目标对象推给色条编辑器（多块组内选中子块时为子块），
            // 再同步色条（无模型会提示）—— 否则色条编辑会绑到组根 mapper 上，
            // 编辑结果写不回子块。
            this->ColorManagerWidget->setCurrentDataObject(this->modelTreeWidget->GetCurrentDataObject());
            this->ColorManagerWidget->resetColorRange();
            this->ColorManagerWidget->show();
            this->ColorManagerWidget->raise();
            this->ColorManagerWidget->activateWindow();
        } else {
            this->ColorManagerWidget->hide();
        }
    });

    connect(this->ColorManagerWidget, &igQtColorManagerWidget::UpdateColorBarFinished, this, [&]() {
        // SetColorMap + Modified 后刷新顶点色与图例
        ui->widget_ScalarField->updateDrawStyle();
        if (this->rendererWidget && this->rendererWidget->getColorBarWidget()) {
            this->rendererWidget->getColorBarWidget()->update();
        }
        if (this->rendererWidget) { this->rendererWidget->update(); }
    });

    connect(ui->widget_VectorField, &igQtVectorWidget::DrawDireVector, this, [&](iGame::DataObject::Pointer res) {
        modelTreeWidget->addDataObjectToModelTree(res, ItemSource::Algorithm);
    });
    connect(ui->widget_VectorField, &igQtVectorWidget::UpdateDireVector, this, [&](iGame::DataObject::Pointer res) {
        res->Modified();
        modelTreeWidget->updateItemName(res);
        rendererWidget->update();
    });
    connect(ui->widget_TensorField, &igQtTensorWidget::DrawTensorGlyphs, this, [&](iGame::DataObject::Pointer res) {
        modelTreeWidget->addDataObjectToModelTree(res, ItemSource::Algorithm);
    });
    connect(ui->widget_TensorField, &igQtTensorWidget::UpdateTensorGlyphs, this,
            [&](iGame::DataObject::Pointer res) { rendererWidget->update(); });
    connect(ui->widget_TensorField, &igQtTensorWidget::UpdateAttributes, this,
            [&](iGame::DataObject::Pointer res) { modelTreeWidget->updateAllAttriubute(res); });

    connect(ui->widget_ContourExtract, &igQtContourExtractWidget::DrawContourModel, this,
            [&](iGame::DataObject::Pointer res) {
                modelTreeWidget->addDataObjectToModelTree(res, ItemSource::Algorithm);
            });
    connect(ui->widget_ContourExtract, &igQtContourExtractWidget::UpdateContourModel, this,
            [&](DataObject::Pointer mesh) {
                modelTreeWidget->updateCurrentModelInfo();
                rendererWidget->update();
            });
    connect(ui->widget_MedianFilter, &igQtMedianFilterWidget::DrawMedianModel, this,
            [&](iGame::DataObject::Pointer res) {
                modelTreeWidget->addDataObjectToModelTree(res, ItemSource::Algorithm);
            });
    connect(ui->widget_MedianFilter, &igQtMedianFilterWidget::UpdateMedianModel, this,
            [&](DataObject::Pointer mesh) {
                modelTreeWidget->updateCurrentModelInfo();
                rendererWidget->update();
            });
    // reset clipping
    connect(ui->action_ResetClipping, &QAction::triggered, this, [&](bool checked) {
        if (!rendererWidget->GetScene()->GetCurrentModel()) {
            std::cout << "Need Input" << std::endl;
            return;
        }
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        auto inputMesh = DynamicCast<iGame::DrawObject>(scene->GetCurrentModel()->GetDataObject());
        if (!inputMesh->GetClipped()) {
            std::cout << "This type of mesh can't be clipped" << std::endl;
            return;
        }

        inputMesh->GetClipper()->DisableAll();
        inputMesh->SetVisibility(true);

        const std::string OVName = "__" + inputMesh->GetName() + "_OV";   // 临时模型
        const std::string IVName = "__" + inputMesh->GetName() + "_IV";   // 临时模型
        const std::string OIVName = "__" + inputMesh->GetName() + "_OIV"; // 临时模型
        bool exist[3]{false, false, false};
        //for (auto& [id, model]: scene->GetModelList()) {
        //    if (model->GetDataObject()->GetName() == OVName) {
        //        auto model = scene->GetModelById(id);
        //        model->GetDataObject()->SetVisibility(false);
        //    } else if (model->GetDataObject()->GetName() == IVName) {
        //        auto model = scene->GetModelById(id);
        //        model->GetDataObject()->SetVisibility(false);
        //    } else if (model->GetDataObject()->GetName() == OIVName) {
        //        auto model = scene->GetModelById(id);
        //        model->GetDataObject()->SetVisibility(false);
        //    }
        //}
        auto modelList = scene->GetModelList();
        for (auto it = modelList->Begin(); it != modelList->End(); ++it) {
            auto id = it->first;
            auto model = it->second;

            auto drawObject = DynamicCast<DrawObject>(model->GetDataObject());

            if (drawObject->GetName() == OVName) {
                drawObject->SetVisibility(false);
            } else if (drawObject->GetName() == IVName) {
                drawObject->SetVisibility(false);
            } else if (drawObject->GetName() == OIVName) {
                drawObject->SetVisibility(false);
            }
        }

        rendererWidget->update();
    });


    // box clipping
    connect(ui->action_BoxClipping, &QAction::triggered, this, [&](bool checked) {
        if (!rendererWidget->GetScene()->GetCurrentModel()) {
            std::cout << "Need Input" << std::endl;
            return;
        }
        bool ok;

        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        auto inputMesh = DynamicCast<iGame::DrawObject>(scene->GetCurrentModel()->GetDataObject());
        if (!inputMesh->GetClipped()) {
            std::cout << "This type of mesh can't be clipped" << std::endl;
            return;
        }

        auto box = inputMesh->GetBoundingBox();
        auto center = (box.min + box.max) * 0.5;
        auto size = box.max - box.min;

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("盒子切割"));
        int x_min_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "x_min(0..1)", "0.0");
        int y_min_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "y_min(0..1)", "0.0");
        int z_min_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "z_min(0..1)", "0.0");
        int x_max_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "x_max(0..1)", "0.5");
        int y_max_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "y_max(0..1)", "1.0");
        int z_max_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "z_max(0..1)", "1.0");
        int flip_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, "flip", "false");
        dialog->show();

        dialog->setApplyFunctor([=, this]() {
            bool ok;
            auto Clamp = [](double x, double l, double r) -> double {
                if (x < l) return l;
                if (x > r) return r;
                return x;
            };

            auto clipper = inputMesh->GetClipper();

            auto& cbox = clipper->m_Box;
            cbox.m_Use = true;

            cbox.m_Bmin[0] = box.min[0] + size[0] * Clamp(dialog->getDouble(x_min_id, ok), 0., 1.);
            cbox.m_Bmin[1] = box.min[1] + size[1] * Clamp(dialog->getDouble(y_min_id, ok), 0., 1.);
            cbox.m_Bmin[2] = box.min[2] + size[2] * Clamp(dialog->getDouble(z_min_id, ok), 0., 1.);
            cbox.m_Bmax[0] = box.min[0] + size[0] * Clamp(dialog->getDouble(x_max_id, ok), 0., 1.);
            cbox.m_Bmax[1] = box.min[1] + size[1] * Clamp(dialog->getDouble(y_max_id, ok), 0., 1.);
            cbox.m_Bmax[2] = box.min[2] + size[2] * Clamp(dialog->getDouble(z_max_id, ok), 0., 1.);
            cbox.m_Flip = dialog->getChecked(flip_id, ok);

            clipper->Modified();

            rendererWidget->update();
        });
    });


    // plane clipping
    connect(ui->action_PlaneClipping, &QAction::triggered, this, [&](bool checked) {
        if (!rendererWidget->GetScene()->GetCurrentModel()) {
            std::cout << "Need Input" << std::endl;
            return;
        }
        bool ok;

        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();

        auto inputMesh = DynamicCast<iGame::DrawObject>(scene->GetCurrentModel()->GetDataObject());
        if (!inputMesh->GetClipped()) {
            std::cout << "This type of mesh can't be clipped" << std::endl;
            return;
        }

        auto box = inputMesh->GetBoundingBox();
        auto center = (box.min + box.max) * 0.5;
        auto size = box.max - box.min;

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("平面切割"));
        int origin_x_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "origin_x(0..1)", "0.5");
        int origin_y_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "origin_y(0..1)", "0.5");
        int origin_z_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "origin_z(0..1)", "0.5");
        int normal_x_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "normal_x(-1..1)", "1.0");
        int normal_y_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "normal_y(-1..1)", "0.0");
        int normal_z_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "normal_z(-1..1)", "0.0");
        int flip_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, "flip", "false");
        dialog->show();

        dialog->setApplyFunctor([=, this]() {
            bool ok;
            auto Clamp = [](double x, double l, double r) -> double {
                if (x < l) return l;
                if (x > r) return r;
                return x;
            };

            auto clipper = inputMesh->GetClipper();

            auto& cplane = clipper->m_Plane;
            cplane.m_Use = true;

            cplane.m_Origin[0] = box.min[0] + size[0] * Clamp(dialog->getDouble(origin_x_id, ok), 0., 1.);
            cplane.m_Origin[1] = box.min[1] + size[1] * Clamp(dialog->getDouble(origin_y_id, ok), 0., 1.);
            cplane.m_Origin[2] = box.min[2] + size[2] * Clamp(dialog->getDouble(origin_z_id, ok), 0., 1.);
            cplane.m_Normal[0] = Clamp(dialog->getDouble(normal_x_id, ok), -1., 1.);
            cplane.m_Normal[1] = Clamp(dialog->getDouble(normal_y_id, ok), -1., 1.);
            cplane.m_Normal[2] = Clamp(dialog->getDouble(normal_z_id, ok), -1., 1.);
            cplane.m_Flip = dialog->getChecked(flip_id, ok);
            if (cplane.m_Normal[0] == 0. && cplane.m_Normal[1] == 0. && cplane.m_Normal[2] == 0.) {
                std::cout << "Normal is a vector of zero" << std::endl;
                return;
            }

            clipper->Modified();

            rendererWidget->update();
        });
    });
}
void igQtMainWindow::updateRecentFilePaths() {
    if (!ui || !ui->menu_RecentFiles || !fileLoader) return;

    QMenu* menu = ui->menu_RecentFiles;
    const QList<QAction*> oldActions = menu->actions();
    for (QAction* a : oldActions) {
        if (a && a->property("igRecentFileEntry").toBool()) menu->removeAction(a);
    }

    const QList<QAction*> list = fileLoader->GetRecentActionList();
    for (int i = list.size() - 1; i >= 0; --i) {
        QAction* a = list.at(i);
        if (!a) continue;
        if (menu->actions().contains(a)) continue;
        a->setProperty("igRecentFileEntry", true);
        menu->addAction(a);
    }
}

void igQtMainWindow::applyTopMenuButtonStyle() {
    const bool menuLight = isLightStyle(m_styleMode);
    if (m_brandBox) {
        m_brandBox->setStyleSheet(menuLight
                ? QStringLiteral("QWidget#TitleBrandBox { background-color: rgba(0,0,0,0.05); border: 1px solid rgba(0,0,0,0.10); border-radius: 6px; }")
                : QStringLiteral("QWidget#TitleBrandBox { background-color: rgba(255,255,255,0.06); border: 1px solid rgba(255,255,255,0.10); border-radius: 6px; }"));
    }
    if (ui && ui->menuBar) {
        ui->menuBar->setStyleSheet(menuLight
                ? QStringLiteral("QMenuBar { background: transparent; color: #1F2A3A; border: none; }"
                                 "QMenuBar::item { background: transparent; padding: 3px 10px; }"
                                 "QMenuBar::item:selected { background-color: rgba(0,0,0,0.08); border-radius: 4px; }")
                : QStringLiteral("QMenuBar { background: transparent; color: #D4D4D4; border: none; }"
                                 "QMenuBar::item { background: transparent; padding: 3px 10px; }"
                                 "QMenuBar::item:selected { background-color: rgba(255,255,255,0.12); border-radius: 4px; }"));
    }
}


void igQtMainWindow::updateColorBarShow() {
    auto colorBar = this->rendererWidget->getColorBarWidget();
    if (!colorBar) { return; }
    colorBar->update();
    if (colorBar->isHidden()) {
        colorBar->show();
    } else {
        colorBar->hide();
    }
}

void igQtMainWindow::initAllSources() {
    // connect(ui->action_LineSource, &QAction::triggered, this, [&]() {
    //	UnstructuredMesh::Pointer newLinePointSet = UnstructuredMesh::New();
    //	newLinePointSet->SetViewStyle(IG_POINTS);
    //	newLinePointSet->AddPoint(Point(0.f, 0.f, 0.f));
    //	newLinePointSet->AddPoint(Point(1.f, 1.0f, 1.f));
    //	igIndex cell[1] = { 0 };
    //	newLinePointSet->AddCell(cell, 1, IG_VERTEX);
    //	cell[0] = 1;
    //	newLinePointSet->AddCell(cell, 1, IG_VERTEX);
    //	auto curScene = SceneManager::Instance()->GetCurrentScene();

    //	LineTypePointsSource::Pointer lineSource = LineTypePointsSource::New();

    //	lineSource->SetInput(newLinePointSet);
    //	lineSource->SetResolution(20);
    //	lineSource->GetOutput()->SetName("lineSource");

    //	auto model = curScene->CreateModel(lineSource->GetOutput());
    //	modelTreeWidget->addModelToModelTree(model);
    //	auto interactor = LineSourceInteractor::New();

    //	//        auto interactor = PointDragInteractor::New();
    //	interactor->SetPointSet(DynamicCast<PointSet>(SceneManager::Instance()
    //		->GetCurrentScene()
    //		->GetCurrentModel()
    //		->GetDataObject()));

    //	rendererWidget->ChangeInteractor(interactor);
    //	});
}

void igQtMainWindow::initAllInteractor() {
    connect(ui->action_SelectView, &QAction::triggered, this, [this](bool checked) {
        if (checked) {
            openLeftToolPanel(LeftToolPanelId::Selection);
            if (ui->widget_SelectionField) ui->widget_SelectionField->setFocus(Qt::OtherFocusReason);
        } else {
            closeLeftToolPanel(LeftToolPanelId::Selection);
        }
    });
    connect(ui->widget_SelectionField, &igQtSelectionWidget::Signal_SetSelectionStationChanged, this, [&]() {
        if (!iGame::SelectionParameter::Instance().GetInSelection()) {
            rendererWidget->ChangeInteractorStyle(Interactor::BasicStyle);
            auto removeBoxFunc = [&]() {
                auto scene = rendererWidget->GetScene();
                SelectionParameter::Instance().SetHaveBox(false);
                scene->GetInteractor()->RemoveSepcialInteractor("SelectBox");
                rendererWidget->update();
            };
            removeBoxFunc();
            return;
        }
        auto selectionStation = iGame::SelectionParameter::Instance().GetSelectionStation();
        switch (selectionStation) {
            case iGame::SelectionParameter::SelectionStation::NONE_SELECTION:
                ui->widget_SelectionField->SetVariableNames({});
                break;
            case iGame::SelectionParameter::SelectionStation::POINT_SELECTION: {
                auto model = rendererWidget->GetScene()->GetCurrentModel();
                if (model == nullptr) {
                    ui->widget_SelectionField->SetVariableNames({});
                } else {
                    auto attrs = model->GetDataObject()->GetAttributeSet()->GetAllAttributes();
                    auto variableNames = CtxPresObjData_Main::GenerateVariableNames(attrs, IG_POINT);
                    ui->widget_SelectionField->SetVariableNames(variableNames);
                }
            } break;
            case iGame::SelectionParameter::SelectionStation::CELL_SELECTION: {
                auto model = rendererWidget->GetScene()->GetCurrentModel();
                if (model == nullptr) {
                    ui->widget_SelectionField->SetVariableNames({});
                } else {
                    auto attrs = model->GetDataObject()->GetAttributeSet()->GetAllAttributes();
                    auto variableNames = CtxPresObjData_Main::GenerateVariableNames(attrs, IG_CELL);
                    ui->widget_SelectionField->SetVariableNames(variableNames);
                }
            } break;
            default:
                break;
        }

        static auto PreVisitFunc = [](iGame::Model::Pointer model) {
            if (model == nullptr) return;
            auto dataObj = model->GetDataObject();
            if (dataObj == nullptr) return;
            auto type = dataObj->GetDataObjectType();
            switch (type) {
                case IG_SURFACE_MESH:
                case IG_STRUCTURED_MESH:
                case IG_VOLUME_MESH: {
                    auto buildAdjacencyRelationFilter = BuildAdjacencyRelationFilter::New();
                    buildAdjacencyRelationFilter->SetInput(dataObj);
                    buildAdjacencyRelationFilter->Execute();
                } break;
                case IG_UNSTRUCTURED_MESH: {
                    auto mesh = DynamicCast<UnstructuredMesh>(dataObj);
                    if (mesh == nullptr) return;
                    auto selection = mesh->GetSelection();
                    if (selection == nullptr) return;
                    auto& cellFaceExtracter = selection->GetCellFaceExtracter();
                    cellFaceExtracter.PreVisit(mesh);
                } break;
                default:
                    return;
            }
        };

        switch (selectionStation) {
            case iGame::SelectionParameter::SelectionStation::NONE_SELECTION:
                rendererWidget->ChangeInteractorStyle(Interactor::BasicStyle);
                break;
            case iGame::SelectionParameter::SelectionStation::POINT_SELECTION: {
                rendererWidget->ChangeInteractorStyle(Interactor::SinglePointSelectionStyle);
                auto model = rendererWidget->GetScene()->GetCurrentModel();
                PreVisitFunc(model);
            } break;
            case iGame::SelectionParameter::SelectionStation::CELL_SELECTION: {
                rendererWidget->ChangeInteractorStyle(Interactor::SingleFaceSelectionStyle);
                auto model = rendererWidget->GetScene()->GetCurrentModel();
                PreVisitFunc(model);
            } break;
            default:
                break;
        }
    });
    //######### View Cloud Change ST #########
    connect(this->modelTreeWidget, &igQtModelDialogWidget::CloudPictureChanged, this, [&]() {
        // 变量选择器按"选中优先"取对象：多块组内选中子块时，变量列表/当前变量
        // 应基于子块（此前读当前模型根对象，组根属性下标为 -1 会直接早退、不更新）。
        auto dataObj = this->modelTreeWidget->GetCurrentDataObject();
        if (dataObj == nullptr) return;
        auto attrSet = dataObj->GetAttributeSet();
        if (attrSet == nullptr) return;
        auto allAttr = attrSet->GetAllAttributes();
        if (allAttr == nullptr) return;
        auto currentAttributeIndex = dataObj->GetCurrentAttributeIndex();
        if (currentAttributeIndex < 0 || allAttr->Size() <= currentAttributeIndex) return;
        auto& currentAttr = allAttr->GetElement(currentAttributeIndex);
        auto dataType = currentAttr.GetAttachmentType();
        auto currentAttributeDim = dataObj->GetCurrentAttributeDimension();
        int variableIndex = 0;
        for (int attrIndex = 0; attrIndex < currentAttributeIndex; attrIndex++) {
        //for (int attrIndex = 0; attrIndex < allAttr->Size(); attrIndex++) {
            auto& attr = allAttr->GetElement(attrIndex);
            if (attr.attachmentType != dataType) continue;
            auto dim = attr.pointer->GetDimension();
            variableIndex += ((dim == 1) ? 1 : dim + 1);
        }
        //if the attribute is not scalar, Extra plus one
        variableIndex += ((currentAttr.pointer->GetDimension() == 1) ? currentAttributeDim : currentAttributeDim + 1);

        auto variableNames = CtxPresObjData_Main::GenerateVariableNames(allAttr, dataType);
        ui->widget_SelectionField->SetVariableNames(variableNames);

        ui->widget_SelectionField->SetCurrentVariable(dataType, variableIndex);
        });
    
    //######### View Cloud Change ED #########
    connect(ui->widget_FlowField, &igQtStreamTracerWidget::SetSelectItemShow, this, [&](bool visiable) {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        auto selection = model->GetSelection();
        if (selection == nullptr) return;
        selection->SetSelectItemVisable(visiable);
        rendererWidget->update();
    });
    connect(ui->widget_SelectionField, &igQtSelectionWidget::SetSelectItemShow, this, [&](bool visiable) {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        auto selection = model->GetSelection();
        if (selection == nullptr) return;
        selection->SetSelectItemVisable(visiable);
        rendererWidget->update();
    });
    connect(ui->widget_SelectionField, &igQtSelectionWidget::SetClearSelection, this, [&]() {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        auto selection = model->GetSelection();
        if (selection == nullptr) return;
        selection->Reset();
        rendererWidget->update();
    });
    connect(ui->widget_SelectionField, &igQtSelectionWidget::SetClearBox, this, [&]() {
        auto scene = rendererWidget->GetScene();
        SelectionParameter::Instance().SetHaveBox(false);
        scene->GetInteractor()->RemoveSepcialInteractor("SelectBox");
        rendererWidget->update();
    });
    connect(ui->widget_SelectionField, &igQtSelectionWidget::SetBoxSettingDialog, this, [&]() {
        auto scene = rendererWidget->GetScene();
        auto interactor = scene->GetInteractor();
        if (!SelectionParameter::Instance().GetHaveBox()) return;
        auto basicStyle = interactor->GetSpecialInteractor("SelectBox");
        if (basicStyle == nullptr) return;
        auto boxStyle = DynamicCast<iGame::BoxStyle>(basicStyle);
        if (boxStyle == nullptr) return;
        auto dynamicBox = boxStyle->GetBox();
        if (dynamicBox == nullptr) return;
        ui->widget_SelectionField->SetInitBoxSettingDialog(rendererWidget);
    });
    connect(ui->widget_FlowField, &igQtStreamTracerWidget::SetUseBox, this, [&](Model::Pointer model) {
        // model = rendererWidget-> GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        auto dataObj = model->GetDataObject();
        if (dataObj == nullptr) return;
        auto selection = model->GetSelection();
        if (selection == nullptr) return;
        auto scene = rendererWidget->GetScene();
        auto interactor = scene->GetInteractor();
        if (!SelectionParameter::Instance().GetHaveBox()) return;
        auto basicStyle = interactor->GetSpecialInteractor("SelectBox");
        if (basicStyle == nullptr) return;
        auto boxStyle = DynamicCast<iGame::BoxStyle>(basicStyle);
        if (boxStyle == nullptr) return;
        auto dynamicBox = boxStyle->GetBox();
        if (dynamicBox == nullptr) return;
        auto faces = dynamicBox->GetAllFaces();
        auto meshType = dataObj->GetDataObjectType();
        switch (meshType) {
            case IG_SURFACE_MESH: {
                auto mesh = DynamicCast<SurfaceMesh>(dataObj);
                mesh->RequestEditStatus();
                auto pointIds = iGame::SingleSelectionStyle::GetPointsInBox(faces, mesh, SelectionParameter::Instance().GetSelectOnlySelectSeeAbleCells());
                selection->SelectionCallBackEvent(IG_POINT, pointIds,  Selection::Operate::Add);
            } break;
            case IG_VOLUME_MESH: {
                auto mesh = DynamicCast<VolumeMesh>(dataObj);
                mesh->RequestEditStatus();
                auto pointIds = iGame::SingleSelectionStyle::GetPointsInBox(faces, mesh, SelectionParameter::Instance().GetSelectOnlySelectSeeAbleCells());
                selection->SelectionCallBackEvent(IG_POINT, pointIds,Selection::Operate::Add);
            } break;
            case IG_UNSTRUCTURED_MESH: {
                auto mesh = DynamicCast<UnstructuredMesh>(dataObj);
                    auto pointIds = iGame::SingleSelectionStyle::GetPointsInBox(
                            faces, mesh, SelectionParameter::Instance().GetSelectOnlySelectSeeAbleCells());
                    selection->SelectionCallBackEvent(IG_POINT, pointIds, Selection::Operate::Add );
            } break;
            default:
                return;
        }
        rendererWidget->update();
    });
    connect(ui->widget_SelectionField, &igQtSelectionWidget::SetUseBox, this, [&]() {
        if (!SelectionParameter::Instance().GetInSelection()) return;
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        auto dataObj = model->GetDataObject();
        if (dataObj == nullptr) return;
        auto selection = model->GetSelection();
        if (selection == nullptr) return;
        auto scene = rendererWidget->GetScene();
        auto interactor = scene->GetInteractor();
        if (!SelectionParameter::Instance().GetHaveBox()) return;
        auto basicStyle = interactor->GetSpecialInteractor("SelectBox");
        if (basicStyle == nullptr) return;
        auto boxStyle = DynamicCast<iGame::BoxStyle>(basicStyle);
        if (boxStyle == nullptr) return;
        auto dynamicBox = boxStyle->GetBox();
        if (dynamicBox == nullptr) return;
        auto faces = dynamicBox->GetAllFaces();
        auto meshType = dataObj->GetDataObjectType();
        switch (meshType) {
            case IG_SURFACE_MESH: {
                auto mesh = DynamicCast<SurfaceMesh>(dataObj);
                mesh->RequestEditStatus();
                if (iGame::SelectionParameter::Instance().GetSelectionStation() ==
                    iGame::SelectionParameter::SelectionStation::CELL_SELECTION) {
                    auto cellIds = iGame::SingleSelectionStyle::GetCellsInBox(
                            faces, mesh, SelectionParameter::Instance().GetSelectOnlySelectSeeAbleCells());
                    selection->SelectionCallBackEvent(IG_CELL, cellIds,
                                                      SelectionParameter::Instance().GetSelectOrUnSelect()
                                                              ? Selection::Operate::Add
                                                              : Selection::Operate::Remove);
                } else {
                    auto pointIds = iGame::SingleSelectionStyle::GetPointsInBox(
                            faces, mesh, SelectionParameter::Instance().GetSelectOnlySelectSeeAbleCells());
                    selection->SelectionCallBackEvent(IG_POINT, pointIds,
                                                      SelectionParameter::Instance().GetSelectOrUnSelect()
                                                              ? Selection::Operate::Add
                                                              : Selection::Operate::Remove);
                }
            } break;
            case IG_STRUCTURED_MESH:
            case IG_VOLUME_MESH: {
                auto mesh = DynamicCast<VolumeMesh>(dataObj);
                mesh->RequestEditStatus();
                if (iGame::SelectionParameter::Instance().GetSelectionStation() ==
                    iGame::SelectionParameter::SelectionStation::CELL_SELECTION) {
                    auto cellIds = iGame::SingleSelectionStyle::GetCellsInBox(
                            faces, mesh, SelectionParameter::Instance().GetSelectOnlySelectSeeAbleCells());
                    selection->SelectionCallBackEvent(IG_CELL, cellIds,
                                                      SelectionParameter::Instance().GetSelectOrUnSelect()
                                                              ? Selection::Operate::Add
                                                              : Selection::Operate::Remove);
                } else {
                    auto pointIds = iGame::SingleSelectionStyle::GetPointsInBox(
                            faces, mesh, SelectionParameter::Instance().GetSelectOnlySelectSeeAbleCells());
                    selection->SelectionCallBackEvent(IG_POINT, pointIds,
                                                      SelectionParameter::Instance().GetSelectOrUnSelect()
                                                              ? Selection::Operate::Add
                                                              : Selection::Operate::Remove);
                }
            } break;
            case IG_UNSTRUCTURED_MESH: {
                auto mesh = DynamicCast<UnstructuredMesh>(dataObj);
                if (iGame::SelectionParameter::Instance().GetSelectionStation() ==
                    iGame::SelectionParameter::SelectionStation::CELL_SELECTION) {
                    auto cellIds = iGame::SingleSelectionStyle::GetCellsInBox(
                            faces, mesh, SelectionParameter::Instance().GetSelectOnlySelectSeeAbleCells());
                    selection->SelectionCallBackEvent(IG_CELL, cellIds,
                                                      SelectionParameter::Instance().GetSelectOrUnSelect()
                                                              ? Selection::Operate::Add
                                                              : Selection::Operate::Remove);
                } else {
                    auto pointIds = iGame::SingleSelectionStyle::GetPointsInBox(
                            faces, mesh, SelectionParameter::Instance().GetSelectOnlySelectSeeAbleCells());
                    selection->SelectionCallBackEvent(IG_POINT, pointIds,
                                                      SelectionParameter::Instance().GetSelectOrUnSelect()
                                                              ? Selection::Operate::Add
                                                              : Selection::Operate::Remove);
                }
            } break;
            default:
                return;
        }
        rendererWidget->update();
    });

    connect(ui->widget_SelectionField, &igQtSelectionWidget::Hided, this, [this]() {
        if (m_leftToolTabByPanel[static_cast<size_t>(LeftToolPanelId::Selection)] >= 0)
            closeLeftToolPanel(LeftToolPanelId::Selection);
        ui->action_SelectView->setChecked(false);
        auto scene = rendererWidget->GetScene();
        SelectionParameter::Instance().SetHaveBox(false);
        scene->GetInteractor()->RemoveSepcialInteractor("SelectBox");
        ui->widget_SelectionField->PreventSignalSend(true);
        ui->widget_SelectionField->SetDefaultSelectionButton();
        ui->widget_SelectionField->PreventSignalSend(false);
        rendererWidget->update();
    });

    connect(modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged, this, [&]() {
        ui->widget_SelectionField->PreventSignalSend(true);
        ui->widget_SelectionField->SetDefaultSelectionButton();
        ui->widget_SelectionField->PreventSignalSend(false);
        //####### ATTENTION #######
        auto attenetionFunc = [&]() {
            ui->widget_SelectionField->SetNoAttention();
            auto model = rendererWidget->GetScene()->GetCurrentModel();
            if (model == nullptr) return;
            auto dataObj = model->GetDataObject();
            if (dataObj == nullptr) return;
            auto attributeSet = dataObj->GetAttributeSet();
            if (attributeSet == nullptr) return;
            bool haveNoPointAttr = (attributeSet->GetAllPointAttributes()->GetNumberOfElements() == 0);
            bool haveNoCellAttr = (attributeSet->GetAllCellAttributes()->GetNumberOfElements() == 0);
            if (haveNoPointAttr && haveNoCellAttr) {
                ui->widget_SelectionField->SetAllAttention();
            } else if (haveNoPointAttr) {
                ui->widget_SelectionField->SetPointAttention();
            } else if (haveNoCellAttr) {
                ui->widget_SelectionField->SetCellAttention();
            }
        };
        attenetionFunc();
        //####### SelectFunc #######
        auto selectFunc = [&]() {
            auto model = rendererWidget->GetScene()->GetCurrentModel();
            if (model == nullptr) return;
            auto selection = model->GetSelection();
            if (selection == nullptr) return;
            ui->widget_SelectionField->SetBoxInitCallBackFunc(selection);
        };
        selectFunc();
        return;
        //auto radius = ui->widget_SelectionField->GetSelectionRadius();
        //auto selectionStation = ui->widget_SelectionField->GetSelectionStation();
        //auto selectOrUnSelect = ui->widget_SelectionField->GetSelectOrUnSelect();
        //switch (selectionStation) {
        //    case SelectionStation::NONE_SELECTION:
        //        rendererWidget->ChangeInteractorStyle(Interactor::BasicStyle);
        //        break;
        //    case SelectionStation::POINT_SELECTION:
        //        rendererWidget->ChangeInteractorStyle(Interactor::SinglePointSelectionStyle, radius, selectOrUnSelect);
        //        break;
        //    case SelectionStation::CELL_SELECTION:
        //        rendererWidget->ChangeInteractorStyle(Interactor::SingleFaceSelectionStyle, radius, selectOrUnSelect);
        //        break;
        //    default:
        //        break;
        //}
        //auto visiable = ui->widget_SelectionField->GetSelectItemShow();
        //auto model = rendererWidget->GetScene()->GetCurrentModel();
        //if (model == nullptr) return;
        //if (visiable) model->GetPainter3D()->ShowAll();
        //else
        //    model->GetPainter3D()->HideAll();
    });
    connect(ui->widget_ContextPreservingShowField, &igQtContextPreservingShowWidget::Hided, this,
            [&]() { ui->action_ContextPreserving->setChecked(false); });


    ui->action_select_point->setVisible(false);
    ui->action_select_face->setVisible(false);
    connect(ui->action_select_point, &QAction::triggered, this, [&](bool checked) {
        if (ui->action_select_point->isChecked()) {
            if (ui->action_select_face->isChecked()) { ui->action_select_face->setChecked(false); }
            rendererWidget->ChangeInteractorStyle(Interactor::SinglePointSelectionStyle);
        } else {
            rendererWidget->ChangeInteractorStyle(Interactor::BasicStyle);
        }
    });


    connect(ui->action_select_face, &QAction::triggered, this, [&](bool checked) {
        if (ui->action_select_face->isChecked()) {
            if (ui->action_select_point->isChecked()) { ui->action_select_point->setChecked(false); }
            rendererWidget->ChangeInteractorStyle(Interactor::SingleFaceSelectionStyle);
        } else {
            rendererWidget->ChangeInteractorStyle(Interactor::BasicStyle);
        }
    });

    connect(ui->action_drag_point, &QAction::triggered, this, [&](bool checked) {
        if (ui->action_drag_point->isChecked()) {
            rendererWidget->ChangeInteractorStyle(Interactor::DragPointStyle);
        } else {
            rendererWidget->ChangeInteractorStyle(Interactor::BasicStyle);
        }
    });
}

void igQtMainWindow::UpdateRenderingWidget() { rendererWidget->update(); }


QString igQtMainWindow::LoadExternalFonts() {
    int fontId = QFontDatabase::addApplicationFont(":/Styles/Styles/SourceHanSansCN-Normal.otf");
    if (fontId == -1) {
        qWarning() << "Failed to load font from resource :/Styles/SourceHanSansCN-Normal.otf";
        return QString();
    }

    const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
    if (families.isEmpty()) {
        qWarning() << "No font families found in loaded font.";
        return QString();
    }

    const QString family = families.first();
    qDebug() << "Loaded font family:" << family;

    QFont     appFont(family);
    appFont.setPointSize(12);

    QApplication::setFont(appFont);

    return family;
}

QString igQtMainWindow::styleSheetForMode(int mode) const {
    if (mode == 12) return loadQssResource(QStringLiteral(":/Styles/iGameVisFloatingDark.qss"));
    if (mode == 13) return loadQssResource(QStringLiteral(":/Styles/iGameVisFloatingLight.qss"));
    if (mode == 14) return loadQssResource(QStringLiteral(":/Styles/iGameVisFloatingGraphiteModern.qss"));
    if (mode == 15) return loadQssResource(QStringLiteral(":/Styles/iGameVisFloatingMatteGraphite.qss"));
    return m_originalStyleSheet;
}

QString igQtMainWindow::styleToggleButtonQss() const {
    const int fam = styleColorFamily(m_styleMode);
    if (fam == 1) {
        return QStringLiteral(
                "QPushButton#StyleToggleButton {"
                " background-color: rgba(56, 189, 248, 0.12);"
                " color: #7FD4FF;"
                " border: 1px solid rgba(56, 189, 248, 0.45);"
                " border-radius: 8px;"
                " font-size: 10pt;"
                " padding: 0 12px;"
                "}"
                "QPushButton#StyleToggleButton:hover {"
                " background-color: rgba(56, 189, 248, 0.22);"
                " border-color: rgba(56, 189, 248, 0.70);"
                "}");
    }
    if (fam == 2) {
        return QStringLiteral(
                "QPushButton#StyleToggleButton {"
                " background-color: rgba(37, 99, 235, 0.10);"
                " color: #2563EB;"
                " border: 1px solid rgba(37, 99, 235, 0.45);"
                " border-radius: 8px;"
                " font-size: 10pt;"
                " padding: 0 12px;"
                "}"
                "QPushButton#StyleToggleButton:hover {"
                " background-color: rgba(37, 99, 235, 0.18);"
                " border-color: rgba(37, 99, 235, 0.70);"
                "}");
    }
    if (fam == 4) {
        return QStringLiteral(
                "QPushButton#StyleToggleButton {"
                " background-color: rgba(77, 208, 225, 0.14);"
                " color: #5CE1F0;"
                " border: 1px solid rgba(77, 208, 225, 0.48);"
                " border-radius: 8px;"
                " font-size: 10pt;"
                " padding: 0 12px;"
                "}"
                "QPushButton#StyleToggleButton:hover {"
                " background-color: rgba(77, 208, 225, 0.24);"
                " border-color: rgba(77, 208, 225, 0.72);"
                "}");
    }
    if (fam == 6 || fam == 8) {
        return QStringLiteral(
                "QPushButton#StyleToggleButton {"
                " background-color: rgba(77, 208, 225, 0.14);"
                " color: #5CE1F0;"
                " border: 1px solid rgba(77, 208, 225, 0.48);"
                " border-radius: 8px;"
                " font-size: 10pt;"
                " padding: 0 12px;"
                "}"
                "QPushButton#StyleToggleButton:hover {"
                " background-color: rgba(77, 208, 225, 0.24);"
                " border-color: rgba(77, 208, 225, 0.72);"
                "}");
    }
    if (fam == 9) {
        return QStringLiteral(
                "QPushButton#StyleToggleButton {"
                " background-color: rgba(108, 142, 174, 0.12);"
                " color: #9FB6C9;"
                " border: 1px solid rgba(108, 142, 174, 0.38);"
                " border-radius: 6px;"
                " font-size: 10pt;"
                " padding: 0 12px;"
                "}"
                "QPushButton#StyleToggleButton:hover {"
                " background-color: rgba(108, 142, 174, 0.22);"
                " border-color: rgba(108, 142, 174, 0.60);"
                "}");
    }
    if (fam == 10) {
        return QStringLiteral(
                "QPushButton#StyleToggleButton {"
                " background-color: rgba(42, 48, 58, 0.30);"
                " color: #A2A8B0;"
                " border: 1px solid #2C3038;"
                " border-radius: 4px;"
                " font-size: 10pt;"
                " padding: 0 12px;"
                "}"
                "QPushButton#StyleToggleButton:hover {"
                " background-color: #2A303A;"
                " border-color: #3A414C;"
                "}");
    }
    if (fam == 11 || fam == 12) {
        return QStringLiteral(
                "QPushButton#StyleToggleButton {"
                " background-color: #252526;"
                " color: #858585;"
                " border: 1px solid #2D2D30;"
                " border-radius: 4px;"
                " font-size: 10pt;"
                " padding: 0 12px;"
                "}"
                "QPushButton#StyleToggleButton:hover {"
                " background-color: #2A2A2C;"
                " border-color: #37373D;"
                "}");
    }
    return QStringLiteral(
            "QPushButton#StyleToggleButton {"
            " background-color: #2A2A2A;"
            " color: #CCCCCC;"
            " border: 1px solid #3C3C3C;"
            " border-radius: 6px;"
            " font-size: 10pt;"
            " padding: 0 10px;"
            "}"
            "QPushButton#StyleToggleButton:hover {"
            " background-color: #3A3A3A;"
            " border-color: #555555;"
            "}");
}

QString igQtMainWindow::styleModeDisplayName(int mode) const {
    if (mode == 12) return QStringLiteral("✦ 深灰");
    if (mode == 13) return QStringLiteral("✦ 浅白");
    if (mode == 14) return QStringLiteral("✦ 石墨");
    if (mode == 15) return QStringLiteral("✦ 哑光");
    return QStringLiteral("✦ 深灰");
}

void igQtMainWindow::createStyleMenu() {
    if (m_styleMenu) return;
    m_styleMenu = new QMenu(m_styleToggleButton);
    static const int kStyleModes[] = {12, 13, 14, 15};
    for (const int mode : kStyleModes) {
        QAction* act = m_styleMenu->addAction(styleModeDisplayName(mode));
        act->setCheckable(true);
        act->setData(mode);
        connect(act, &QAction::triggered, this, [this, mode]() {
            applyStyleMode(mode);
            QSettings settings(QStringLiteral("iGame"), QStringLiteral("iGameVis"));
            settings.setValue(QStringLiteral("ui/styleMode"), mode);
        });
    }
    m_styleToggleButton->setMenu(m_styleMenu);
}

void igQtMainWindow::updateTitleBarIcons() {
    const bool light = isLightStyle(m_styleMode);
    if (m_btnMinimize) {
        m_btnMinimize->setIcon(QIcon(light ? QStringLiteral(":/Ticon/Icons/window_minimize_dark.svg")
                                           : QStringLiteral(":/Ticon/Icons/window_minimize_white.svg")));
    }
    updateMaximizeButtonIcon();
}

void igQtMainWindow::applyStyleMode(int mode) {
    mode = normalizeStyleMode(qBound(0, mode, 15));
    if (styleSheetForMode(mode).isEmpty()) {
        mode = kFallbackStyleMode;
    }

    if (m_styleMode == 6 && mode != 6 && modelTreeWidget) {
        applyWorkspaceLayout(false);
    }
    if (isFloatingCardStyle(m_styleMode) && !isFloatingCardStyle(mode)) {
        if (modelTreeWidget && modelTreeWidget->isTreeDockCollapsed()) {
            modelTreeWidget->setTreeDockCollapsed(false);
        }
        applyFloatingCards(false);
    }
    if ((m_styleMode == 7 || m_styleMode == 8) && mode != 7 && mode != 8) {
        applyViewRail(false);
    }
    m_styleMode = mode;
    igQtRenderWidget::setGlobalStyleMode(m_styleMode);
    this->setStyleSheet(styleSheetForMode(mode));

    if (m_styleToggleButton) {
        m_styleToggleButton->setText(styleModeDisplayName(mode));
        m_styleToggleButton->setStyleSheet(styleToggleButtonQss());
    }
    applyTopMenuButtonStyle();
    QString accent = QStringLiteral("#4DD0E1");
    switch (styleColorFamily(mode)) {
        case 0:  accent = QStringLiteral("#3C3C3C"); break;
        case 1:  accent = QStringLiteral("#38BDF8"); break;
        case 2:  accent = QStringLiteral("#2563EB"); break;
        case 3:  accent = QStringLiteral("#4A4E54"); break;
        case 9:  accent = QStringLiteral("#6C8EAE"); break;
        case 10: accent = QStringLiteral("#3A414C"); break;
        case 11: accent = QStringLiteral("#37373D"); break;
        case 12: accent = QStringLiteral("#37373D"); break;
        default: accent = QStringLiteral("#4DD0E1"); break;
    }
    const bool isLight = isLightStyle(mode);

    if (m_logoIconLabel) {
        QString grad;
        if (isLight) {
            grad = QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #FFFFFF, stop:1 #E9EFF9)");
        } else if (styleColorFamily(mode) == 10) {
            grad = QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #2A303A, stop:1 #1C1F25)");
        } else if (styleColorFamily(mode) >= 11) {
            grad = QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #2A2A2C, stop:1 #1E1E1E)");
        } else {
            grad = QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #4DD0E1, stop:1 #2B7CD3)");
        }
        m_logoIconLabel->setStyleSheet(
                QStringLiteral("QLabel#AppLogoLabel { background: %1; border-radius: 8px; padding: 3px; }")
                        .arg(grad));
    }

    if (m_projectChip) {
        if (isLight) {
            m_projectChip->setStyleSheet(
                    "QLabel#ProjectChip { color: #1F2A3A; background-color: rgba(37,99,235,0.08);"
                    " border: 1px solid rgba(37,99,235,0.30); border-radius: 999px; padding: 3px 10px; font-size: 10pt; }");
        } else {
            m_projectChip->setStyleSheet(
                    QStringLiteral("QLabel#ProjectChip { color: #B8C0CA; background-color: rgba(255,255,255,0.05);"
                                   " border: 1px solid %1; border-radius: 999px; padding: 3px 10px; font-size: 10pt; }")
                            .arg(accent));
        }
    }

    if (m_titleAccentLine) {
        m_titleAccentLine->setStyleSheet(
                QStringLiteral("QFrame#TitleBarAccentLine { background-color: %1; }").arg(accent));
    }

    if (m_rightDivider) {
        m_rightDivider->setStyleSheet(
                isLight
                        ? "QFrame#RightDivider { background-color: rgba(0,0,0,0.12); }"
                        : "QFrame#RightDivider { background-color: rgba(255,255,255,0.12); }");
    }
    if (m_styleMenu) {
        const QList<QAction*> acts = m_styleMenu->actions();
        for (QAction* act : acts) {
            act->setChecked(act->data().toInt() == mode);
        }
    }
    updateTitleBarIcons();
    if (rendererWidget) rendererWidget->applyThemeBackground();
    if (isFloatingCardStyle(m_styleMode)) applyFloatingCardPalette();
    if (modelTreeWidget) modelTreeWidget->refreshStyle();

    if (mode == 6 && modelTreeWidget) {
        applyWorkspaceLayout(true);
    }
    if (mode == 12) {
        applyFloatingCards(true);
    }
    if (mode == 7 || mode == 8) {
        applyViewRail(true);
    }

    const bool wrappersExist = (this->findChild<QToolBar*>(QStringLiteral("wrapper_toolBar_meshfile")) != nullptr);
    if (wrappersExist && !m_toolbarRebuilding) {
        rebuildToolbarRow(m_currentToolbarIconSize);
        relayoutToolbarWrappers();
    }

    {
        const QWidgetList tops = QApplication::topLevelWidgets();
        for (QWidget* top : tops) {
            if (!top) continue;
            QList<QWidget*> widgets = top->findChildren<QWidget*>();
            widgets.prepend(top);
            for (QWidget* w : widgets) {
                if (!w) continue;
                if (!w->property("igPanelBaseQss").toString().isEmpty()) {
                    igQtPanelTheme::refresh(w);
                }
            }
            if (top != this) {
                QEvent styleEvent(QEvent::StyleChange);
                QCoreApplication::sendEvent(top, &styleEvent);
            }
        }
    }
}

void igQtMainWindow::applyFloatingCardPalette() {
    if (!isFloatingCardStyle(m_styleMode)) return;
    const FloatingCardPalette cardPal = floatingCardPalette(m_styleMode);
    const QString backdrop = QString::fromLatin1(cardPal.backdrop);

    if (rendererWidget) {
        QPalette rendererPal = rendererWidget->palette();
        rendererPal.setColor(QPalette::Window, QColor(backdrop));
        rendererWidget->setPalette(rendererPal);
        rendererWidget->setStyleSheet(QStringLiteral("background-color: %1;").arg(backdrop));
    }
    if (m_floatingCardWidget) {
        m_floatingCardWidget->setStyleSheet(
                QStringLiteral("QWidget#FloatingCard { background-color: %1; border: 1px solid %2;"
                               " border-radius: 8px; }")
                        .arg(QString::fromLatin1(cardPal.cardBg), QString::fromLatin1(cardPal.cardBorder)));
    }
}

void igQtMainWindow::applyFloatingCards(bool enabled) {
    if (enabled && m_centralCardContainer) {
        applyFloatingCardPalette();
        return;
    }
    if (!enabled && !m_centralCardContainer) return;

    if (enabled) {
        if (!m_centralCardContainer && rendererWidget) {
            auto* container = new QWidget(this);
            container->setObjectName(QStringLiteral("CentralCardContainer"));
            container->setAttribute(Qt::WA_StyledBackground, true);
            auto* lay = new QVBoxLayout(container);
            lay->setContentsMargins(11, 6, 11, 6);
            lay->setSpacing(0);
            lay->addWidget(rendererWidget);
            m_centralCardContainer = container;
            this->setCentralWidget(m_centralCardContainer);
        }
        if (rendererWidget) {
            rendererWidget->setAutoFillBackground(true);
            applyFloatingCardPalette();
            applyMainWindowRoundedMask(rendererWidget, 8);
        }
        if (modelTreeWidget) {
            QDockWidget* props = modelTreeWidget->getPropertiesDock();
            if (!props) return;
            m_propertiesOriginalMinWidth = props->minimumWidth();
            props->setMinimumWidth(200);
            {
                auto* emptyTitle = new QWidget(props);
                emptyTitle->setFixedHeight(0);
                props->setTitleBarWidget(emptyTitle);
            }
            QWidget* original = props->widget();
            m_floatingCardOriginalWidget = original;
            if (original) {
                props->setAttribute(Qt::WA_TranslucentBackground, true);
                props->setAutoFillBackground(false);
                props->setStyleSheet(
                        "QDockWidget#LayerPropertiesDock { background-color: transparent; border: none; }");
                auto* outer = new QWidget(props);
                outer->setObjectName(QStringLiteral("FloatingCardOuter"));
                outer->setAttribute(Qt::WA_StyledBackground, true);
                outer->setAttribute(Qt::WA_TranslucentBackground, true);
                outer->setAutoFillBackground(false);
                auto* outerLayout = new QVBoxLayout(outer);
                outerLayout->setContentsMargins(11, 6, 11, 6);
                outerLayout->setSpacing(0);
                original->setParent(outer);
                original->setObjectName(QStringLiteral("FloatingCard"));
                original->setContentsMargins(0, 0, 0, 0);
                outerLayout->addWidget(original);
                props->setWidget(outer);
                m_floatingCardWidget = original;
                applyFloatingCardPalette();
                igQtDetachRoundedCorners(original);
            }
            m_floatingCardDock = props;
            QTimer::singleShot(0, this, [this, props]() {
                if (props) this->resizeDocks({props}, {210}, Qt::Horizontal);
            });
            if (QDockWidget* treeDock = modelTreeWidget->getTreeDock()) {
                m_floatingTreeDock = treeDock;
                applyMainWindowRoundedMask(treeDock, 8);

                QWidget* originalTree = treeDock->widget();
                m_floatingTreeOriginalWidget = originalTree;

                if (originalTree) {
                    auto* treeOuter = new QWidget(treeDock);
                    treeOuter->setObjectName(QStringLiteral("FloatingTreeOuter"));
                    treeOuter->setAttribute(Qt::WA_TranslucentBackground, true);
                    auto* treeLayout = new QVBoxLayout(treeOuter);
                    treeLayout->setContentsMargins(0, 20, 0, 0);
                    treeLayout->setSpacing(0);
                    originalTree->setParent(treeOuter);
                    originalTree->setContentsMargins(0, 0, 0, 0);
                    treeLayout->addWidget(originalTree);
                    treeDock->setWidget(treeOuter);
                    m_floatingTreeWrapper = treeOuter;
                }
            }
        }

    } else {
        if (m_centralCardContainer && rendererWidget) {
            QWidget* old = m_centralCardContainer;
            m_centralCardContainer = nullptr;
            this->setCentralWidget(rendererWidget);
            old->deleteLater();
        }
        if (rendererWidget) {
            rendererWidget->setCornerCover(0, QColor());
            rendererWidget->setMask(QRegion());
        }        if (m_floatingCardWidget) {
            igQtDetachRoundedCorners(m_floatingCardWidget);
            m_floatingCardWidget->setMask(QRegion());
            m_floatingCardWidget = nullptr;
        }
        if (m_floatingCardDock) {
            m_floatingCardDock->setMask(QRegion());
            m_floatingCardDock = nullptr;
        }
        if (m_floatingTreeDock) {
            QDockWidget* treeDock = m_floatingTreeDock;
            igQtDetachRoundedCorners(treeDock);
            if (m_floatingTreeOriginalWidget) {
                QWidget* original = m_floatingTreeOriginalWidget;
                QWidget* wrapper = treeDock->widget();
                original->setContentsMargins(0, 0, 0, 0);
                original->setParent(treeDock);
                treeDock->setWidget(original);
                if (wrapper && wrapper != original) wrapper->deleteLater();
                m_floatingTreeOriginalWidget = nullptr;
                m_floatingTreeWrapper = nullptr;
            }
            treeDock->setMask(QRegion());
            m_floatingTreeDock = nullptr;
        }
        if (modelTreeWidget) {
            QDockWidget* props = modelTreeWidget->getPropertiesDock();
            if (props) {
                const int restoreW = m_propertiesOriginalMinWidth > 0 ? m_propertiesOriginalMinWidth : 220;
                m_propertiesOriginalMinWidth = 0;
                props->setMinimumWidth(restoreW);
                if (m_floatingCardOriginalWidget) {
                    QWidget* original = m_floatingCardOriginalWidget;
                    QWidget* wrapper = props->widget();
                    original->setObjectName(QString());
                    original->setContentsMargins(0, 0, 0, 0);
                    original->setStyleSheet(QString());
                    original->setParent(props);
                    props->setWidget(original);
                    if (wrapper && wrapper != original) wrapper->deleteLater();
                    m_floatingCardOriginalWidget = nullptr;
                } else if (QWidget* w = props->widget()) {
                    w->setObjectName(QString());
                    w->setContentsMargins(0, 0, 0, 0);
                    w->setStyleSheet(QString());
                }
                props->setStyleSheet(QString());
                props->setAttribute(Qt::WA_TranslucentBackground, false);
                props->setAutoFillBackground(true);
                props->setTitleBarWidget(nullptr);
                QTimer::singleShot(0, this, [this, props, restoreW]() {
                    if (props) this->resizeDocks({props}, {restoreW}, Qt::Horizontal);
                });
            }
        }
    }
}

void igQtMainWindow::applyWorkspaceLayout(bool enabled) {
    if (!modelTreeWidget) return;

    QDockWidget* propertiesDock = modelTreeWidget->getPropertiesDock();
    if (!propertiesDock) return;

    const QList<QDockWidget*> analysisDocks = {
            ui->dockWidget_ScalarField,
            ui->dockWidget_VectorField,
            ui->dockWidget_FlowField,
            ui->dockWidget_TensorField,
            ui->dockWidget_ParallelCoordinatesField,
            ui->dockWidget_VariableCorrelationField,
            ui->dockWidget_VariableDensityField,
            ui->dockWidget_DataChangeField,
            ui->dockWidget_SelectionField,
            ui->dockWidget_ContextPreservingShowField,
            ui->dockWidget_QualityDetection,
            ui->dockWidget_EditMode,
            ui->dockWidget_ModelList,
            ui->dockWidget_ContourExtract
    };

    if (enabled) {
        for (QDockWidget* d : analysisDocks) {
            if (d) this->addDockWidget(Qt::RightDockWidgetArea, d);
        }
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_ParallelCoordinatesField);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_VariableCorrelationField);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_VariableDensityField);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_DataChangeField);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_ContextPreservingShowField);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_QualityDetection);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_EditMode);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_ModelList);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_ContourExtract);

        this->addDockWidget(Qt::LeftDockWidgetArea, propertiesDock);
        if (m_leftFieldDock) this->addDockWidget(Qt::LeftDockWidgetArea, m_leftFieldDock);
        this->splitDockWidget(m_leftFieldDock, propertiesDock, Qt::Vertical);

        QTimer::singleShot(0, this, [this, propertiesDock]() {
            if (ui->dockWidget_SelectionField) {
                this->resizeDocks({ui->dockWidget_SelectionField}, {300}, Qt::Horizontal);
            }
            if (propertiesDock) {
                this->resizeDocks({propertiesDock}, {260}, Qt::Horizontal);
            }
        });

        modelTreeWidget->positionTreeDockToRendererCorner(rendererWidget);
    } else {
        for (QDockWidget* d : analysisDocks) {
            if (d) this->addDockWidget(Qt::LeftDockWidgetArea, d);
        }
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_ParallelCoordinatesField);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_VariableCorrelationField);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_VariableDensityField);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_DataChangeField);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_ContextPreservingShowField);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_QualityDetection);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_EditMode);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_ModelList);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_ContourExtract);

        this->addDockWidget(Qt::LeftDockWidgetArea, m_leftFieldDock);
        this->addDockWidget(Qt::LeftDockWidgetArea, propertiesDock);
        this->splitDockWidget(m_leftFieldDock, propertiesDock, Qt::Vertical);

        if (rendererWidget) {
            modelTreeWidget->positionTreeDockToRendererCorner(rendererWidget);
        }
    }
}

void igQtMainWindow::applyViewRail(bool enabled) {
    if (!rendererWidget) return;

    if (enabled && !m_viewDock) {
        m_viewDock = new QDockWidget(QStringLiteral("视图"), this);
        m_viewDock->setObjectName(QStringLiteral("ViewDock"));
        m_viewDock->setAllowedAreas(Qt::RightDockWidgetArea);
        m_viewDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
        m_viewDock->setMinimumWidth(40);
        m_viewDock->setMaximumWidth(42);

        QWidget* railWidget = new QWidget(m_viewDock);
        QVBoxLayout* railLayout = new QVBoxLayout(railWidget);
        railLayout->setContentsMargins(2, 2, 2, 2);
        railLayout->setSpacing(3);

        auto addRailButton = [&](QAction* act, const QString& tip) {
            if (!act) return;
            QToolButton* b = new QToolButton(railWidget);
            b->setDefaultAction(act);
            b->setIconSize(QSize(18, 18));
            b->setToolButtonStyle(Qt::ToolButtonIconOnly);
            b->setAutoRaise(true);
            b->setFocusPolicy(Qt::NoFocus);
            b->setFixedSize(26, 26);
            b->setToolTip(tip);
            b->setCursor(Qt::PointingHandCursor);
            b->setStyleSheet(
                    "QToolButton { border-radius: 0; }"
                    "QToolButton:hover { background-color: rgba(255,255,255,0.10); }");
            railLayout->addWidget(b, 0, Qt::AlignHCenter);
        };

        addRailButton(ui->action_ShowOrientationAxes, QStringLiteral("定向轴"));
        addRailButton(ui->action_ResetCameraView, QStringLiteral("重置视角"));
        addRailButton(ui->action_UseOrthographic, QStringLiteral("正交投影"));
        addRailButton(ui->action_setViewToIsometric, QStringLiteral("等轴测视图"));
        addRailButton(ui->action_setViewToPositiveX, QStringLiteral("+X 方向"));
        addRailButton(ui->action_setViewToNegativeX, QStringLiteral("-X 方向"));
        addRailButton(ui->action_setViewToPositiveY, QStringLiteral("+Y 方向"));
        addRailButton(ui->action_setViewToNegativeY, QStringLiteral("-Y 方向"));
        addRailButton(ui->action_setViewToPositiveZ, QStringLiteral("+Z 方向"));
        addRailButton(ui->action_setViewToNegativeZ, QStringLiteral("-Z 方向"));
        addRailButton(ui->action_rotateNinetyCounterClockwise, QStringLiteral("逆时针 90°"));
        addRailButton(ui->action_rotateNinetyClockwise, QStringLiteral("顺时针 90°"));
        addRailButton(ui->action_ShowCenter, QStringLiteral("显示中心"));
        addRailButton(ui->action_PickCenter, QStringLiteral("选择中心"));

        m_viewDock->setWidget(railWidget);
        this->addDockWidget(Qt::RightDockWidgetArea, m_viewDock);
    }
    if (m_viewDock) {
        m_viewDock->setVisible(enabled);
        if (enabled) updateViewRailPosition();
    }
}

void igQtMainWindow::updateViewRailPosition() {
    if (m_viewDock && m_viewDock->isVisible()) {
        QTimer::singleShot(0, this, [this]() {
            if (m_viewDock) this->resizeDocks({m_viewDock}, {40}, Qt::Horizontal);
        });
    }
}

QString igQtMainWindow::toolbarButtonQss(int fontPx) const {
    const int fam = styleColorFamily(m_styleMode);
    if (fam == 1) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 8px; margin: 0; padding: 2px; font-size: %1px; color: #B8C4D4; }"
                "QToolButton:hover { background-color: rgba(56, 189, 248, 0.10); border-color: rgba(56, 189, 248, 0.30); color: #E8EEF7; }"
                "QToolButton:pressed { background-color: rgba(56, 189, 248, 0.18); }"
                "QToolButton:checked { background-color: rgba(56, 189, 248, 0.16); border-color: rgba(56, 189, 248, 0.45); color: #38BDF8; }")
                .arg(fontPx);
    }
    if (fam == 2) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 8px; margin: 0; padding: 2px; font-size: %1px; color: #33405B; }"
                "QToolButton:hover { background-color: rgba(37, 99, 235, 0.08); border-color: rgba(37, 99, 235, 0.30); color: #1F2A3A; }"
                "QToolButton:pressed { background-color: rgba(37, 99, 235, 0.16); }"
                "QToolButton:checked { background-color: rgba(37, 99, 235, 0.14); border-color: rgba(37, 99, 235, 0.45); color: #2563EB; }")
                .arg(fontPx);
    }
    if (fam == 4) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 6px; margin: 0; padding: 1px; font-size: %1px; color: #B8C0CA; }"
                "QToolButton:hover { background-color: rgba(77, 208, 225, 0.14); border-color: rgba(77, 208, 225, 0.36); color: #FFFFFF; }"
                "QToolButton:pressed { background-color: rgba(77, 208, 225, 0.26); }"
                "QToolButton:checked { background-color: rgba(77, 208, 225, 0.20); border-color: rgba(77, 208, 225, 0.52); color: #5CE1F0; }")
                .arg(fontPx);
    }
    if (fam == 6 || fam == 8) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 6px; margin: 0; padding: 1px; font-size: %1px; color: #B8C0CA; }"
                "QToolButton:hover { background-color: rgba(77, 208, 225, 0.14); border-color: rgba(77, 208, 225, 0.36); color: #FFFFFF; }"
                "QToolButton:pressed { background-color: rgba(77, 208, 225, 0.26); }"
                "QToolButton:checked { background-color: rgba(77, 208, 225, 0.20); border-color: rgba(77, 208, 225, 0.52); color: #5CE1F0; }")
                .arg(fontPx);
    }
    if (fam == 9) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 5px; margin: 0; padding: 5px; font-size: %1px; color: #C3CBD5; }"
                "QToolButton:hover { background-color: rgba(108, 142, 174, 0.10); border-color: rgba(108, 142, 174, 0.24); color: #E9EDF2; }"
                "QToolButton:pressed { background-color: rgba(108, 142, 174, 0.18); }"
                "QToolButton:checked { background-color: rgba(108, 142, 174, 0.16); border-color: rgba(108, 142, 174, 0.40); color: #9FB6C9; }")
                .arg(fontPx);
    }
    if (fam == 10) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 4px; margin: 0; padding: 5px; font-size: %1px; color: #A2A8B0; }"
                "QToolButton:hover { background-color: #242830; border-color: #2A303A; color: #E2E4E8; }"
                "QToolButton:pressed { background-color: #2A303A; }"
                "QToolButton:checked { background-color: #2A303A; border-color: #3A414C; color: #E2E4E8; }")
                .arg(fontPx);
    }
    if (fam == 11 || fam == 12) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 4px; margin: 0; padding: 5px; font-size: %1px; color: #858585; }"
                "QToolButton:hover { background-color: #2A2A2C; border-color: #37373D; color: #CCCCCC; }"
                "QToolButton:pressed { background-color: #404045; }"
                "QToolButton:checked { background-color: #37373D; border-color: #404045; color: #CCCCCC; }")
                .arg(fontPx);
    }
    return QStringLiteral(
            "QToolButton { border: none; margin: 0; padding: 1px; font-size: %1px; }"
            "QToolButton:hover { background-color: #3A3A3A; border-radius: 2px; }"
            "QToolButton:pressed { background-color: #4A4A4A; }")
            .arg(fontPx);
}

QString igQtMainWindow::twoRowGridButtonQss() const {
    const int fam = styleColorFamily(m_styleMode);
    if (fam == 1) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 6px; margin: 0; padding: 0; }"
                "QToolButton:hover { background-color: rgba(56, 189, 248, 0.10); border-color: rgba(56, 189, 248, 0.30); }"
                "QToolButton:pressed { background-color: rgba(56, 189, 248, 0.18); }"
                "QToolButton:checked { background-color: rgba(56, 189, 248, 0.16); border-color: rgba(56, 189, 248, 0.45); }");
    }
    if (fam == 2) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 6px; margin: 0; padding: 0; }"
                "QToolButton:hover { background-color: rgba(37, 99, 235, 0.08); border-color: rgba(37, 99, 235, 0.30); }"
                "QToolButton:pressed { background-color: rgba(37, 99, 235, 0.16); }"
                "QToolButton:checked { background-color: rgba(37, 99, 235, 0.14); border-color: rgba(37, 99, 235, 0.45); }");
    }
    if (fam == 4) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 6px; margin: 0; padding: 0; }"
                "QToolButton:hover { background-color: rgba(77, 208, 225, 0.14); border-color: rgba(77, 208, 225, 0.36); }"
                "QToolButton:pressed { background-color: rgba(77, 208, 225, 0.26); }"
                "QToolButton:checked { background-color: rgba(77, 208, 225, 0.20); border-color: rgba(77, 208, 225, 0.52); }");
    }
    if (fam == 6 || fam == 8) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 6px; margin: 0; padding: 0; }"
                "QToolButton:hover { background-color: rgba(77, 208, 225, 0.14); border-color: rgba(77, 208, 225, 0.36); }"
                "QToolButton:pressed { background-color: rgba(77, 208, 225, 0.26); }"
                "QToolButton:checked { background-color: rgba(77, 208, 225, 0.20); border-color: rgba(77, 208, 225, 0.52); }");
    }
    if (fam == 9) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 5px; margin: 0; padding: 3px; }"
                "QToolButton:hover { background-color: rgba(108, 142, 174, 0.10); border-color: rgba(108, 142, 174, 0.24); }"
                "QToolButton:pressed { background-color: rgba(108, 142, 174, 0.18); }"
                "QToolButton:checked { background-color: rgba(108, 142, 174, 0.16); border-color: rgba(108, 142, 174, 0.40); }");
    }
    if (fam == 10) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 4px; margin: 0; padding: 3px; }"
                "QToolButton:hover { background-color: #242830; border-color: #2A303A; }"
                "QToolButton:pressed { background-color: #2A303A; }"
                "QToolButton:checked { background-color: #2A303A; border-color: #3A414C; }");
    }
    if (fam == 11 || fam == 12) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 4px; margin: 0; padding: 3px; }"
                "QToolButton:hover { background-color: #2A2A2C; border-color: #37373D; }"
                "QToolButton:pressed { background-color: #404045; }"
                "QToolButton:checked { background-color: #37373D; border-color: #404045; }");
    }
    return QStringLiteral(
            "QToolButton { border: none; margin: 0; padding: 0; }"
            "QToolButton:hover { background-color: #3A3A3A; border-radius: 2px; }"
            "QToolButton:pressed { background-color: #4A4A4A; }");
}

QString igQtMainWindow::toolbarItemQss() const {
    const int fam = styleColorFamily(m_styleMode);
    QString hover;
    switch (fam) {
        case 1:  hover = QStringLiteral("rgba(56, 189, 248, 0.10)"); break;
        case 2:  hover = QStringLiteral("rgba(37, 99, 235, 0.08)"); break;
        case 4:  hover = QStringLiteral("rgba(77, 208, 225, 0.14)"); break;
        case 6:  hover = QStringLiteral("rgba(77, 208, 225, 0.14)"); break;
        case 8:  hover = QStringLiteral("rgba(77, 208, 225, 0.14)"); break;
        case 9:  hover = QStringLiteral("rgba(108, 142, 174, 0.10)"); break;
        case 10: hover = QStringLiteral("#242830"); break;
        case 11: hover = QStringLiteral("#2A2A2C"); break;
        case 12: hover = QStringLiteral("#2A2A2C"); break;
        default: hover = QStringLiteral("#3A3A3A"); break;
    }
    return QStringLiteral(
            "QWidget#toolbarButtonItem { background: transparent; border: none; border-radius: 4px; }"
            "QWidget#toolbarButtonItem:hover { background-color: %1; }")
            .arg(hover);
}

QString igQtMainWindow::toolbarTitleLabelQss() const {
    const int fam = styleColorFamily(m_styleMode);
    if (fam == 1) {
        return QStringLiteral(
                "QLabel { color: #8A99AC; padding: 3px 12px; background-color: rgba(255, 255, 255, 0.03); "
                "border: 1px solid rgba(148, 163, 184, 0.18); border-radius: 6px; font-family: 'PingFang SC'; }");
    }
    if (fam == 2) {
        return QStringLiteral(
                "QLabel { color: #5A6577; padding: 3px 12px; background-color: rgba(0, 0, 0, 0.03); "
                "border: 1px solid #D9DEE7; border-radius: 6px; font-family: 'PingFang SC'; }");
    }
    if (fam == 4) {
        return QStringLiteral(
                "QLabel { color: #5E6670; padding: 2px 8px; background-color: rgba(77, 208, 225, 0.06); "
                "border: 1px solid rgba(77, 208, 225, 0.24); border-radius: 6px; font-family: 'PingFang SC'; }");
    }
    if (fam == 6 || fam == 8) {
        return QStringLiteral(
                "QLabel { color: #5E6670; padding: 2px 8px; background-color: rgba(77, 208, 225, 0.06); "
                "border: 1px solid rgba(77, 208, 225, 0.24); border-radius: 6px; font-family: 'PingFang SC'; }");
    }
    if (fam == 9) {
        return QStringLiteral(
                "QLabel { color: #8B96A3; padding: 2px 4px; background-color: transparent; "
                "border: none; border-radius: 0; font-family: 'PingFang SC'; }");
    }
    if (fam == 10) {
        return QStringLiteral(
                "QLabel { color: #7E858E; padding: 2px 4px; background-color: transparent; "
                "border: none; border-radius: 0; font-family: 'PingFang SC'; }");
    }
    if (fam == 11 || fam == 12) {
        return QStringLiteral(
                "QLabel { color: #858585; padding: 2px 4px; background-color: transparent; "
                "border: none; border-radius: 0; font-family: 'PingFang SC'; }");
    }
    return QStringLiteral(
            "QLabel { color: #9A9A9A; padding: 3px 12px; background-color: rgba(255, 255, 255, 0.03); "
            "border: 1px solid #3C3C3C; border-radius: 6px; font-family: 'PingFang SC'; }");
}

QString igQtMainWindow::toolbarCaptionLabelQss(int fontPx) const {
    const int fam = styleColorFamily(m_styleMode);
    if (fam == 1) {
        return QStringLiteral(
                "QLabel { color: #B8C4D4; padding: 0; background-color: transparent; border: none; "
                "font-family: 'PingFang SC'; font-size: %1px; }")
                .arg(fontPx);
    }
    if (fam == 2) {
        return QStringLiteral(
                "QLabel { color: #4A5568; padding: 0; background-color: transparent; border: none; "
                "font-family: 'PingFang SC'; font-size: %1px; }")
                .arg(fontPx);
    }
    if (fam == 4) {
        return QStringLiteral(
                "QLabel { color: #B8C0CA; padding: 0; background-color: transparent; border: none; "
                "font-family: 'PingFang SC'; font-size: %1px; }")
                .arg(fontPx);
    }
    if (fam == 6 || fam == 8) {
        return QStringLiteral(
                "QLabel { color: #B8C0CA; padding: 0; background-color: transparent; border: none; "
                "font-family: 'PingFang SC'; font-size: %1px; }")
                .arg(fontPx);
    }
    if (fam == 9) {
        return QStringLiteral(
                "QLabel { color: #C7D0DA; padding: 0; background-color: transparent; border: none; "
                "font-family: 'PingFang SC'; font-size: %1px; }")
                .arg(fontPx);
    }
    if (fam == 10) {
        return QStringLiteral(
                "QLabel { color: #9AA1AA; padding: 0; background-color: transparent; border: none; "
                "font-family: 'PingFang SC'; font-size: %1px; }")
                .arg(fontPx);
    }
    if (fam == 11 || fam == 12) {
        return QStringLiteral(
                "QLabel { color: #858585; padding: 0; background-color: transparent; border: none; "
                "font-family: 'PingFang SC'; font-size: %1px; }")
                .arg(fontPx);
    }
    return QStringLiteral(
            "QLabel { color: #D2D2D2; padding: 0; background-color: transparent; border: none; "
            "font-family: 'PingFang SC'; font-size: %1px; }")
            .arg(fontPx);
}

QString igQtMainWindow::toolbarSeamColor() const {
    const int fam = styleColorFamily(m_styleMode);
    switch (fam) {
        case 0:  return QStringLiteral("#3C3C3C");
        case 1:  return QStringLiteral("#2E3D52");
        case 2:  return QStringLiteral("#CBD2DC");
        case 4:  return QStringLiteral("#343B43");
        case 5:  return QStringLiteral("#3A414D");
        case 6:  return QStringLiteral("#343B43");
        case 8:  return QStringLiteral("#343B43");
        case 9:  return QStringLiteral("#2B2F36");
        case 10: return QStringLiteral("#2C3038");
        case 11: return QStringLiteral("#2D2D30");
        case 12: return QStringLiteral("#2D2D30");
        default: return QStringLiteral("#3E4550");
    }
}

QString igQtMainWindow::toolbarAccentColor() const {
    const int fam = styleColorFamily(m_styleMode);
    switch (fam) {
        case 0:  return QStringLiteral("#007ACC");
        case 1:  return QStringLiteral("#38BDF8");
        case 2:  return QStringLiteral("#2563EB");
        case 3:  return QStringLiteral("#4A4E54");
        case 7:  return QStringLiteral("#4A4E54");
        case 5:  return QStringLiteral("#60CDFF");
        case 9:  return QStringLiteral("#3F5568");
        case 10: return QStringLiteral("#2A303A");
        case 11: return QStringLiteral("#37373D");
        case 12: return QStringLiteral("#37373D");
        default: return QStringLiteral("#4DD0E1");
    }
}

void igQtMainWindow::rebuildActionsAsTwoRowWidget(QToolBar* toolbar, const QList<QAction*>& targetActions,
                                                  int columns, QAction* insertBefore) {
    if (!toolbar || targetActions.isEmpty())
        return;

    // 1. 先移除目标action（原逻辑保留）
    for (QAction* act : targetActions) {
        if (act)
            toolbar->removeAction(act);
    }

    // 2. 创建容器和布局（原逻辑保留，微调尺寸计算）
    QWidget* container = new QWidget(toolbar);
    QGridLayout* grid = new QGridLayout(container);
    QSize iconSize = toolbar->iconSize();
    int gridSpacing = qMax(2, iconSize.height() / 12);
    if (isModernDenseStyle(m_styleMode)) gridSpacing += 2;
    grid->setSpacing(gridSpacing);
    grid->setContentsMargins(0, 0, 0, 0);

    // 两行视图按钮：小图标时按钮也相应做小，避免网格占用过多宽度
    const int targetIcon = qMax(12, qMin(16, static_cast<int>(iconSize.height() * 0.45)));
    const int rowHeight = targetIcon + 6 + (isModernDenseStyle(m_styleMode) ? 2 : 0);
    const int containerHeight = 2 * rowHeight + gridSpacing;
    QSize btnSize(rowHeight, rowHeight);

    // 【修改1】放宽尺寸约束，避免被父布局挤压
    container->setMinimumHeight(containerHeight);
    container->setMinimumWidth(3 * rowHeight + 2 * gridSpacing); // 去掉fixedHeight，改用minimumHeight
    container->setObjectName("twoRowViewGrid");

    // 3. 构建两行按钮（原逻辑保留）
    int row = 0, col = 0;
    for (QAction* act : targetActions) {
        if (!act)
            continue;
        QToolButton* btn = new QToolButton(container);
        btn->setDefaultAction(act);
        btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        btn->setIconSize(QSize(targetIcon, targetIcon));
        btn->setAutoRaise(true);
        btn->setFocusPolicy(Qt::NoFocus);
        btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        btn->setMinimumSize(btnSize);
        btn->setMaximumSize(btnSize);
        btn->setStyleSheet(twoRowGridButtonQss());
        grid->addWidget(btn, row, col, Qt::AlignVCenter | Qt::AlignHCenter);
        if (++col >= columns) {
            col = 0;
            ++row;
        }
    }

    // 4. 添加QWidgetAction到toolbar（原逻辑保留）
    QWidgetAction* widgetAction = new QWidgetAction(toolbar);
    widgetAction->setDefaultWidget(container);
    if (insertBefore && toolbar->actions().contains(insertBefore))
        toolbar->insertAction(insertBefore, widgetAction);
    else
        toolbar->addAction(widgetAction); // 【修改2】改用addAction，避免insert位置异常

}

void igQtMainWindow::addToolbarTitle(QToolBar* toolbar, const QString& title, int iconSizePx) {
    if (!toolbar)
        return;

    Qt::ToolBarArea area = this->toolBarArea(toolbar);
    if (area == Qt::NoToolBarArea)
        area = Qt::TopToolBarArea; // 重建时原 toolbar 已不在 QMainWindow 管理下，统一回到顶部区域
    const QSize iconSize(iconSizePx, iconSizePx);
    ToolbarSpacingMetrics spacing = metricsForIconSize(iconSize.width());
    if (isModernDenseStyle(m_styleMode)) {
        spacing.btnGap += 2;
        spacing.edgeMargin += 2;
        spacing.buttonPadding += 3;
        spacing.bottomMargin += 2;
    }
    const QList<QAction*> actions = toolbar->actions();
    const int fontPx = toolbarButtonFontPixel(iconSizePx);

    QFont titleFont(QStringLiteral("PingFang SC"));
    titleFont.setPointSize(titlePointSizeForIcon(iconSizePx));
    const bool compact = (m_styleMode == 6);
    const int titleTextH = compact ? 0 : QFontMetrics(titleFont).height();

    QWidget* container = new QWidget(this);
    container->setObjectName("toolbarContainer_" + toolbar->objectName());
    container->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    container->setAttribute(Qt::WA_StyledBackground, true);

    QWidget* topRow = new QWidget(container);
    topRow->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QHBoxLayout* hLayout = new QHBoxLayout(topRow);
    hLayout->setContentsMargins(spacing.edgeMargin, 0, spacing.edgeMargin, 0);
    hLayout->setSpacing(spacing.btnGap);
    hLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    for (QAction* act : actions) {
        if (!act) continue;

        if (QWidgetAction* wa = qobject_cast<QWidgetAction*>(act)) {
            QWidget* w = wa->defaultWidget();
            if (w) {
                w->setParent(topRow);
                w->setVisible(true);
                w->show();
                w->setMinimumSize(w->minimumSizeHint());
                hLayout->addWidget(w, 0, Qt::AlignLeft | Qt::AlignVCenter);
                continue;
            }
        }

        if (compact) {
            QToolButton* b = new QToolButton(topRow);
            b->setDefaultAction(act);
            b->setIconSize(iconSize);
            b->setToolButtonStyle(Qt::ToolButtonIconOnly);
            b->setAutoRaise(true);
            b->setFocusPolicy(Qt::NoFocus);
            b->setFixedSize(iconSize.width(), iconSize.height());
            b->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            b->setToolTip(act->text());
            b->setStyleSheet(toolbarButtonQss(fontPx));
            hLayout->addWidget(b, 0, Qt::AlignLeft | Qt::AlignVCenter);
            continue;
        }

        QFont btnFont(QStringLiteral("PingFang SC"));
        btnFont.setPixelSize(fontPx);
        const QFontMetrics fm(btnFont);
        const int textMaxW = qMax(kToolbarButtonTextMinWidth, iconSize.width() + 16);
        const QString rawText = act->text();
        const QString wrappedText = wrapToolbarButtonText(rawText, textMaxW, fm);
        const int textLines = wrappedText.isEmpty() ? 1 : wrappedText.count(QLatin1Char('\n')) + 1;
        // 按钮宽度按「换行后最宽的一行」计算，避免换行后按钮仍然过宽
        int widestTextW = fm.horizontalAdvance(rawText);
        if (wrappedText.contains(QLatin1Char('\n'))) {
            widestTextW = 0;
            const QStringList textLinesList = wrappedText.split(QLatin1Char('\n'));
            for (const QString& line : textLinesList) {
                widestTextW = qMax(widestTextW, fm.horizontalAdvance(line));
            }
        }
        const int btnW = qMax(iconSize.width(), widestTextW) + 2 * spacing.buttonPadding;
        const int textLineH = qMax(fm.height(), fm.lineSpacing());
        const int captionH = textLines * textLineH + 2;

        QWidget* item = new QWidget(topRow);
        QVBoxLayout* itemLayout = new QVBoxLayout(item);
        itemLayout->setContentsMargins(spacing.buttonPadding, 0, spacing.buttonPadding, 0);
        itemLayout->setSpacing(2);
        itemLayout->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

        QToolButton* b = new QToolButton(item);
        b->setDefaultAction(act);
        b->setIconSize(iconSize);
        b->setToolButtonStyle(Qt::ToolButtonIconOnly);
        b->setAutoRaise(true);
        b->setFocusPolicy(Qt::NoFocus);
        b->setFixedSize(iconSize.width(), iconSize.height());
        b->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        b->setStyleSheet(twoRowGridButtonQss());
        itemLayout->addWidget(b, 0, Qt::AlignHCenter);

        QLabel* caption = new QLabel(wrappedText, item);
        caption->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        caption->setFixedSize(qMax(iconSize.width(), widestTextW), captionH);
        caption->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        caption->setStyleSheet(toolbarCaptionLabelQss(fontPx));
        itemLayout->addWidget(caption, 0, Qt::AlignHCenter);

        item->setFixedSize(btnW, iconSize.height() + captionH + 2);
        item->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        item->setObjectName(QStringLiteral("toolbarButtonItem"));
        item->setAttribute(Qt::WA_StyledBackground, true);
        item->setStyleSheet(toolbarItemQss());
        item->setCursor(Qt::PointingHandCursor);
        item->setToolTip(act->text());
        item->setProperty("igToolbarButton", QVariant::fromValue<QObject*>(b));
        b->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        caption->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        item->installEventFilter(this);

        hLayout->addWidget(item, 0, Qt::AlignLeft | Qt::AlignVCenter);
    }

    // 容器高度：按钮换行后变高，用 topRow 的实际 sizeHint 兜底
    const int totalH = qMax(iconSize.height() + spacing.verticalGap + titleTextH + spacing.bottomMargin,
                            topRow->sizeHint().height() + spacing.verticalGap + titleTextH + spacing.bottomMargin);
    container->setMinimumSize(100, totalH);

    this->removeToolBar(toolbar);
    toolbar->hide();

    QVBoxLayout* vLayout = new QVBoxLayout(container);
    const int frameInset = 4;
    vLayout->setContentsMargins(spacing.edgeMargin + frameInset, frameInset,
                                spacing.edgeMargin + frameInset, frameInset + 2);
    vLayout->setSpacing(2);
    vLayout->setSizeConstraint(QLayout::SetFixedSize);

    QLabel* titleLabel = nullptr;
    if (!compact) {
        titleLabel = new QLabel(title, container);
        titleLabel->setObjectName("toolbarTitle_" + toolbar->objectName());
        titleLabel->setFont(titleFont);
        titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        titleLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        titleLabel->setStyleSheet(toolbarTitleLabelQss());
        vLayout->addWidget(titleLabel, 0, Qt::AlignLeft);
    }

    vLayout->addStretch(1);
    vLayout->addWidget(topRow, 0, Qt::AlignHCenter);
    vLayout->addStretch(1);

    QFrame* bottomLine = new QFrame(container);
    bottomLine->setObjectName("toolbarAccentLine_" + toolbar->objectName());
    bottomLine->setFixedHeight(2);
    bottomLine->setFrameShape(QFrame::NoFrame);
    bottomLine->setAttribute(Qt::WA_StyledBackground, true);
    bottomLine->setStyleSheet(
            QStringLiteral("QFrame { background-color: %1; }").arg(toolbarAccentColor()));
    vLayout->addWidget(bottomLine, 0);

    QToolBar* wrapper = new QToolBar(this);
    wrapper->setObjectName("wrapper_" + toolbar->objectName());
    wrapper->setWindowTitle(title);
    wrapper->setMovable(true);
    wrapper->setFloatable(true);
    wrapper->setMinimumHeight(totalH);
    wrapper->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QWidgetAction* wrapperAction = new QWidgetAction(wrapper);
    wrapperAction->setDefaultWidget(container);
    wrapper->addAction(wrapperAction);
    wrapper->setMinimumWidth(container->sizeHint().width() + spacing.btnGap * 2);

    this->addToolBar(area, wrapper);
}

void igQtMainWindow::relayoutToolbarWrappers() {
    if (m_toolbarRebuilding) return;

    const QStringList orderedNames = {
            "wrapper_toolBar_meshfile",
            "wrapper_toolBar_3",
            "wrapper_toolBar_2",
            "wrapper_toolBar_4"
    };

    auto collectWrappers = [&]() -> QList<QToolBar*> {
        QList<QToolBar*> out;
        for (const QString& name : orderedNames) {
            if (QToolBar* tb = this->findChild<QToolBar*>(name)) {
                out.push_back(tb);
            }
        }
        return out;
    };

    QList<QToolBar*> wrappers = collectWrappers();
    if (wrappers.isEmpty()) return;

    const int availableWidth = qMax(320, this->width() - 12);

    // 从当前已拟合的尺寸继续缩/放，避免每次 resize 都先重置回首选再重新放大（导致反复重建）
    int iconSize = m_currentToolbarIconSize;

    // 2) 宽度拟合：整行放不下时逐档缩小图标并重建，直到放得下或到图标下限
    while (iconSize > kToolbarIconMin && measureToolbarRowWidth() > availableWidth) {
        iconSize = qMax(kToolbarIconMin, iconSize - 4);
        rebuildToolbarRow(iconSize);
        wrappers = collectWrappers();
    }

    // 3) 空间充足时按填充比例放大图标/字号，避免宽屏下右侧太空
    const int targetWidth = qMax(320, static_cast<int>(availableWidth * kToolbarFillRatio));
    while (iconSize < kToolbarIconMax && measureToolbarRowWidth() < targetWidth) {
        iconSize = qMin(kToolbarIconMax, iconSize + 2);
        rebuildToolbarRow(iconSize);
        wrappers = collectWrappers();
    }

    // 每次重排前先清掉舊分行點，避免重複斷行導致排版漂移
    for (const QString& name : orderedNames) {
        if (QToolBar* tb = this->findChild<QToolBar*>(name)) {
            this->removeToolBarBreak(tb);
        }
    }

    const ToolbarSpacingMetrics spacing = metricsForIconSize(m_currentToolbarIconSize);
    const int gap = isModernDenseStyle(m_styleMode)
            ? 2
            : spacing.groupGap;
    const int rowGap = spacing.rowGap;
    int usedWidth = 0;

    for (QToolBar* tb : wrappers) {
        if (!tb) continue;
        const int needWidth = qMax(tb->minimumWidth(), tb->sizeHint().width());
        bool startsNewRow = false;
        if (usedWidth > 0 && (usedWidth + needWidth) > availableWidth) {
            this->insertToolBarBreak(tb);
            startsNewRow = true;
            usedWidth = 0;
        }
        const bool firstInRow = (usedWidth == 0);
        const QString seam = toolbarSeamColor();
        if (isModernDenseStyle(m_styleMode)) {
            tb->setStyleSheet(QStringLiteral("#%1 { margin-top: %2px; border: none; border-top: 1px solid %3; %4 }")
                                      .arg(tb->objectName())
                                      .arg(startsNewRow ? rowGap : 0)
                                      .arg(seam)
                                      .arg(firstInRow
                                               ? QStringLiteral("")
                                               : QStringLiteral("border-left: 1px solid %1;").arg(seam)));
        } else {
            tb->setStyleSheet(QStringLiteral("#%1 { margin-top: %2px; border: none; border-top: 1px solid %3; }")
                                      .arg(tb->objectName())
                                      .arg(startsNewRow ? rowGap : 0)
                                      .arg(seam));
        }
        usedWidth += needWidth + gap;
    }
}


void igQtMainWindow::rebuildToolbarRow(int iconSize) {
    if (m_toolbarRebuilding) return;
    m_toolbarRebuilding = true;

    const int originalIconSize = iconSize;
    if (m_styleMode == 7 || m_styleMode == 8) {
        iconSize = qMin(iconSize, 28);
    }

    // 1. 先摘掉 toolBar_4 上残留的两行网格 QWidgetAction（此刻旧容器还活着，可安全比对）
    const QList<QAction*> t4Actions = ui->toolBar_4->actions();
    for (QAction* a : t4Actions) {
        if (auto* wa = qobject_cast<QWidgetAction*>(a)) {
            if (wa->defaultWidget() && wa->defaultWidget()->objectName() == QStringLiteral("twoRowViewGrid")) {
                ui->toolBar_4->removeAction(wa);
                wa->deleteLater();
                break;
            }
        }
    }

    // 2. 删除旧 wrapper 工具栏（连带旧容器与旧按钮）
    removeToolbarWrappers();

    // 3. 把新的图标尺寸应用到原始 QToolBar（两行网格与标题容器都按它重建）
    for (QToolBar* tb : this->findChildren<QToolBar*>()) {
        if (tb->objectName().startsWith("wrapper_"))
            continue;
        tb->setIconSize(QSize(iconSize, iconSize));
        tb->setMinimumHeight(iconSize + qMax(6, iconSize / 6));
    }

    // 4. 重建 toolBar_4 的 3×2 轴方向网格
    rebuildActionsAsTwoRowWidget(
            ui->toolBar_4,
            {
                    ui->action_setViewToPositiveX,
                    ui->action_setViewToNegativeX,
                    ui->action_setViewToPositiveY,
                    ui->action_setViewToNegativeY,
                    ui->action_setViewToPositiveZ,
                    ui->action_setViewToNegativeZ
            },
            3,
            ui->action_rotateNinetyCounterClockwise
    );

    addToolbarTitle(ui->toolBar_meshfile, QStringLiteral("文件与输出"), iconSize);
    addToolbarTitle(ui->toolBar_3, QStringLiteral("可视化"), iconSize);
    addToolbarTitle(ui->toolBar_2, QStringLiteral("选择与编辑"), iconSize);
    if (m_styleMode != 7 && m_styleMode != 8) {
        addToolbarTitle(ui->toolBar_4, QStringLiteral("视图设置"), iconSize);
    } else {
        this->removeToolBar(ui->toolBar_4);
        ui->toolBar_4->hide();
    }

    {
        const QStringList names = {QStringLiteral("toolBar_meshfile"), QStringLiteral("toolBar_3"),
                                   QStringLiteral("toolBar_2"), QStringLiteral("toolBar_4")};
        int maxH = 0;
        QList<QWidget*> containers;
        for (const QString& n : names) {
            if (QWidget* c = this->findChild<QWidget*>(QStringLiteral("toolbarContainer_") + n)) {
                containers.push_back(c);
                maxH = qMax(maxH, c->sizeHint().height());
            }
        }
        // 容器内部 vLayout 带 SetFixedSize 约束，会按自身 sizeHint 反压高度，
        // 必须先解除该约束，统一 setFixedHeight 才能让四组底部分隔线对齐。
        for (QWidget* c : containers) {
            if (maxH > 0) {
                if (QLayout* lay = c->layout()) { lay->setSizeConstraint(QLayout::SetDefaultConstraint); }
                c->setFixedHeight(maxH);
            }
        }
    }

    m_currentToolbarIconSize = originalIconSize;
    m_toolbarRebuilding = false;
}

void igQtMainWindow::removeToolbarWrappers() {
    const QStringList orderedNames = {
            "wrapper_toolBar_meshfile",
            "wrapper_toolBar_3",
            "wrapper_toolBar_2",
            "wrapper_toolBar_4"
    };
    for (const QString& name : orderedNames) {
        if (QToolBar* tb = this->findChild<QToolBar*>(name)) {
            this->removeToolBar(tb);
            delete tb;
        }
    }
}

int igQtMainWindow::measureToolbarRowWidth() const {
    const QStringList orderedNames = {
            "wrapper_toolBar_meshfile",
            "wrapper_toolBar_3",
            "wrapper_toolBar_2",
            "wrapper_toolBar_4"
    };
    const ToolbarSpacingMetrics spacing = metricsForIconSize(m_currentToolbarIconSize);
    const int gap = isModernDenseStyle(m_styleMode)
            ? 2
            : spacing.groupGap;
    int used = 0;
    int count = 0;
    for (const QString& name : orderedNames) {
        if (const QToolBar* tb = this->findChild<QToolBar*>(name)) {
            used += qMax(tb->minimumWidth(), tb->sizeHint().width());
            if (count > 0) used += gap;
            ++count;
        }
    }
    return used;
}

QString igQtMainWindow::wrapToolbarButtonText(const QString& text, int maxWidth, const QFontMetrics& fm) const {
    if (text.isEmpty() || maxWidth <= 0) return text;
    const int n = text.size();
    if (n <= 1 || fm.horizontalAdvance(text) <= maxWidth) return text;

    const auto lineWidth = [&](int from, int count) {
        return fm.horizontalAdvance(text.mid(from, count));
    };

    int bestScore = std::numeric_limits<int>::max();
    int bestImbalance = std::numeric_limits<int>::max();
    int bestB1 = -1;
    int bestB2 = -1;

    for (int b1 = 1; b1 < n; ++b1) {
        const int w1 = lineWidth(0, b1);
        const int w2 = lineWidth(b1, n - b1);
        if (w1 > maxWidth || w2 > maxWidth) continue;
        const int score = qMax(w1, w2);
        const int imbalance = qAbs(w1 - w2);
        if (score < bestScore || (score == bestScore && imbalance < bestImbalance)) {
            bestScore = score;
            bestImbalance = imbalance;
            bestB1 = b1;
            bestB2 = n;
        }
    }

    if (n >= 3) {
        for (int b1 = 1; b1 < n - 1; ++b1) {
            const int w1 = lineWidth(0, b1);
            if (w1 > maxWidth) continue;
            for (int b2 = b1 + 1; b2 < n; ++b2) {
                const int w2 = lineWidth(b1, b2 - b1);
                const int w3 = lineWidth(b2, n - b2);
                if (w2 > maxWidth || w3 > maxWidth) continue;
                const int score = qMax(w1, qMax(w2, w3));
                const int imbalance = qMax(w1, qMax(w2, w3)) - qMin(w1, qMin(w2, w3));
                if (score < bestScore || (score == bestScore && imbalance < bestImbalance)) {
                    bestScore = score;
                    bestImbalance = imbalance;
                    bestB1 = b1;
                    bestB2 = b2;
                }
            }
        }
    }

    if (bestB1 > 0) {
        if (bestB2 >= n) {
            return text.left(bestB1) + QLatin1Char('\n') + text.mid(bestB1);
        }
        return text.left(bestB1) + QLatin1Char('\n') + text.mid(bestB1, bestB2 - bestB1) + QLatin1Char('\n') + text.mid(bestB2);
    }

    QStringList lines;
    int pos = 0;
    while (pos < n) {
        int end = pos + 1;
        while (end < n && lineWidth(pos, end - pos + 1) <= maxWidth) ++end;
        lines.append(text.mid(pos, end - pos));
        pos = end;
        if (lines.size() >= kToolbarButtonTextMaxLines && pos < n) {
            break;
        }
    }
    if (pos < n) lines.append(text.mid(pos));
    return lines.join(QStringLiteral("\n"));
}


void igQtMainWindow::UpdateIcons()
{
    // 依螢幕寬度與 DPI 動態縮放，避免高縮放或低解析度下 toolbar 後段按鈕被擠壓。
    const int iconSize = resolveToolbarIconSizeForWidget(this);

    for (QToolBar* tb : this->findChildren<QToolBar*>()) {
        if (tb->objectName().startsWith("wrapper_"))
            continue;
        tb->setIconSize(QSize(iconSize, iconSize));
        tb->setMinimumHeight(iconSize + qMax(6, iconSize / 6));
    }


}

void igQtMainWindow::hookResponsiveEvents() {
    if (m_ResponsiveHooked) return;
    m_ResponsiveHooked = true;

    // 防抖 timer：合并 100ms 内的多次 resize 触发。
    m_ResizeDebounceTimer = new QTimer(this);
    m_ResizeDebounceTimer->setSingleShot(true);
    connect(m_ResizeDebounceTimer, &QTimer::timeout, this,
            [this]() { applyResponsiveToolbarLayout(); });

    // 屏幕切换（拖到不同显示器）→ 重新排。
    if (auto wh = this->windowHandle()) {
        connect(wh, &QWindow::screenChanged, this,
                [this](QScreen*) { applyResponsiveToolbarLayout(); });
    }
}

void igQtMainWindow::applyResponsiveToolbarLayout() {
    // 重建式拟合：由 relayoutToolbarWrappers 统一决定图标尺寸（缩小/放大/换行）
    relayoutToolbarWrappers();
}

void igQtMainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    // 窗口首次真正显示后，屏幕 handle 才稳定；此时挂 hook + 跑一次布局，
    // 修正启动阶段按"主屏宽度"猜的档位。
    hookResponsiveEvents();
    applyResponsiveToolbarLayout();
    updateViewRailPosition();
}
