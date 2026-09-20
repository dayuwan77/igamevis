/**
 * @class   igQtMainWindow
 * @brief   igQtMainWindow's brief
 */

#ifndef IGAMEVIS_IGQTMAINWINDOW_H
#define IGAMEVIS_IGQTMAINWINDOW_H

#define QT_NO_OPENGL
#include <ui_iGameQtMainWindow.h>
#if __linux__
#include <QTabWidget>
#include <QtGui>
#else
#include <QtGUI/QtGui>
#include <QtWidgets/Qtabwidget.h>
#endif
#include <IQCore/igQtExportModule.h>
#include <QtWidgets/QMainWindow>
#include <QResizeEvent>
#include <QShowEvent>
#include <QRect>
#include <QTimer>
#include <array>
#include <AxisAlignedReflection/iGameAxisAlignedReflectionFilter.h>
#include <MyFilter/iGameExtractCellsByTypeFilter.h>
#include <iGameScene.h>
#undef QT_NO_OPENGL

class igQtModelDrawWidget;
class igQtFileLoader;
class igQtColorManagerWidget;
class igQtFilterDialogDockWidget;
class igQtSliceWidget;
class igQtProgressBarWidget;
class igQtModelDialogWidget;
class igQtModelClipWidget;
class igQtDeformationWidget;
class igQtAiChatWidget;
class igQtCommandManager;
class igQtChromeFramelessDialog;
class igQtPartFocusWidget;
class igQtGlobalIdWidget;
class igQtTriangleStripWidget;
class igQtExtractCellsByTypeWidget;
class igQtAxisAlignedReflectionWidget;
class igQtPointAndCellIdsWidget;


class IG_QT_MODULE_EXPORT igQtMainWindow : public QMainWindow {
    Q_OBJECT
public:
    /// 左侧「工具」Tab 面板项（扩展：追加枚举值 + cpp 中 switch + 菜单/工具栏连接）
    enum class LeftToolPanelId : int {
        Scalar = 0,
        Vector,
        Tensor,
        Flow,
        ContourExtract,
        ExtractEdges,
        CountCellVertices,
        Slice,
        Deformation,
        Selection,
        VariableDensity,
        DataChange,
        MergeVectorComponents,
        ExtractComponent,
        ExtractCellsByType,
        GenerateProcessIds,
        Count
    };

    igQtMainWindow(QWidget* parent = Q_NULLPTR);
    ~igQtMainWindow() override;

public:
    void initAllUnDefinedComponents();
    void initToolbarComponent();
    void initAllComponents();
    void initAllDockWidgetConnectWithAction();
    void initAllMySignalConnections();
    void initAllFilters();
    void initAllSources();
    void initAllInteractor();
    void initArgs(const QStringList& args);
    void updateVortexMetricsLabelPos();

    /** 将已登记的面板迁入左侧 QTabWidget 并显示（已存在则仅切换 Tab） */
    void openLeftToolPanel(LeftToolPanelId id);
    /** 从左侧 Tab 移除并还原到原 QDockWidget（供可勾选动作关闭等） */
    void closeLeftToolPanel(LeftToolPanelId id);

public:
    igQtModelDrawWidget* rendererWidget;
    igQtFileLoader* fileLoader;
    igQtModelDialogWidget* modelTreeWidget;

    igQtColorManagerWidget* ColorManagerWidget;
    igQtFilterDialogDockWidget* filterDialogDockWidget;
    QDockWidget* SliceDockWidget;
    QDockWidget* ContourDockWidget;
    igQtModelClipWidget* SliceWidget;
    QDockWidget* DeformationDockWidget;
    igQtDeformationWidget* DeformationWidget;

    igQtProgressBarWidget* progressBarWidget;
    QComboBox* viewStyleCombox;
    QComboBox* attributeViewIndexCombox;
    QComboBox* attributeViewDimCombox;
    
    // AI Chat DockWidget
    QDockWidget* aiChatDockWidget;
    igQtAiChatWidget* aiChatWidget;

    // Command Manager for MCP Server (端口 12345)
    igQtCommandManager* commandManager;

    // 零件聚焦弹窗
    igQtChromeFramelessDialog* partFocusDialog{nullptr};
    igQtPartFocusWidget* partFocusWidget{nullptr};

    // 全局 ID 生成与 Local/Global 对照结果
    QDockWidget* GlobalIdDockWidget{nullptr};
    igQtGlobalIdWidget* GlobalIdWidget{nullptr};

    QDockWidget* TriangleStripDockWidget{nullptr};
    igQtTriangleStripWidget* TriangleStripWidget{nullptr};
    // 轴对齐反射面板
    QDockWidget* AxisAlignedReflectionDockWidget{nullptr};
    igQtAxisAlignedReflectionWidget* AxisAlignedReflectionWidget{nullptr};
    iGame::AxisAlignedReflectionFilter::Pointer m_axisAlignedReflectionFilter;
    iGame::Model::Pointer m_axisAlignedReflectionModel;
    int m_axisAlignedReflectionCount{0};
    // 点与单元 ID 参数面板
    QDockWidget* PointAndCellIdsDockWidget{nullptr};
    igQtPointAndCellIdsWidget* PointAndCellIdsWidget{nullptr};

private slots:
    void updateRecentFilePaths();
    void updateColorBarShow();

    //void ChangeViewStyle();
    //void ChangeScalarView();
    //void ChangeScalarViewDim();
    //void updateViewStyleAndCloudPicture();
    //void updateCurrentDataObject();
    //void updateCurrentSceneWidget();

    void UpdateRenderingWidget();
    //void changePointSelectionInteractor();
    //void changePointsSelectionInteractor();
    //void changeFaceSelectionInteractor();
    //void changeFacesSelectionInteractor();
    void UpdateIcons();
    QString LoadExternalFonts();


private:
    Ui::MainWindow* ui;
    QLabel* vortexMetricsLabel = nullptr;
    // 自定义标题栏相关
    QWidget* m_titleBar = nullptr;
    QLabel* m_titleLabel = nullptr;
    QPushButton* m_btnMinimize = nullptr;
    QPushButton* m_btnMaximize = nullptr;
    QPushButton* m_btnClose = nullptr;
    bool m_titleBarDragging = false;
    QPoint m_dragOffset;
    bool m_isMinimizing = false;
    bool m_isRestoringFromMaximized = false;
    QRect m_geometryBeforeMinimize;
    QRect m_normalGeometry;

    // 左侧工具 Tab（按需添加；下方 Properties 常驻）
    QDockWidget* m_leftFieldDock = nullptr;
    QTabWidget* m_leftFieldTabs = nullptr;
    // 按单元类型提取：左侧面板 + 壳 Dock + 常驻 filter
    // 首次提取生成独立新模型 ExtractCellsByType_n（不覆盖输入模型）；
    // 改勾选重提取时，原地更新该新模型（模型树不新增节点）
    QDockWidget* m_extractCellsByTypeShell = nullptr;
    igQtExtractCellsByTypeWidget* m_extractCellsByTypeWidget = nullptr;
    iGame::ExtractCellsByTypeFilter::Pointer m_extractCellsByTypeFilter;
    iGame::Model::Pointer m_extractCellsByTypeModel;
    std::array<int, static_cast<size_t>(LeftToolPanelId::Count)> m_leftToolTabByPanel{
        {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

    void relocateContentToLeftTab(QDockWidget* shell, QWidget* inner, const QString& title, LeftToolPanelId id,
                                  bool centerFlowField);
    QWidget* wrapContentInScrollArea(QWidget* content, QWidget* parent, bool centerFlowField);
    QDockWidget* shellDockForLeftPanel(LeftToolPanelId id) const;
    void onLeftToolTabCloseRequested(int index);
    /** 工具面板与 Properties 垂直比例（需在工具 Dock 已 show 后调用） */
    void applyLeftToolStackVerticalSplit();

    /** 与菜单「算法处理 / 特征提取」等一致：无边框 QMessageBox + 暗色圆角边框。 */
    void showDarkFramelessMessage(const QString& title, const QString& text, bool useInformationIcon = false);

    void rebuildActionsAsTwoRowWidget(QToolBar* toolbar, const QList<QAction*>& targetActions, int columns,
                                      QAction* insertBefore = nullptr);
    void addToolbarTitle(QToolBar* toolbar, const QString& title);
    void relayoutToolbarWrappers();
    void initCustomTitleBar();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void changeEvent(QEvent* event) override;
    int streamTreeIndex = -1;

private:
    // 响应式 toolbar 布局：把耗时的重排合并（避免每 1px 拖动都触发全量刷新）
    QTimer* m_ResizeDebounceTimer{nullptr};
    bool m_ResponsiveHooked{false};
    void hookResponsiveEvents();
    // 依当前窗口宽度/屏幕 DPI 挑选最大能一行装下的 iconSize，然后应用到全部工具栏
    void applyResponsiveToolbarLayout();
    // 内部帮手：按给定 iconSize 刷新普通 toolbar、两行按钮容器、带标题容器
    void applyToolbarIconSize(int iconSize);
    // 内部帮手：估计所有 wrapper toolbar 总宽（当前布局下）
    int totalWrapperWidth() const;

private:
    void minimizeWithAnimation();
    void toggleMaximizeRestore();
    void updateMaximizeButtonIcon();

    // IsoVolume 时序: 由框架动画控件驱动时保存信号连接;
    // 重复执行 IsoVolume 前先断开, 避免连接叠加导致一次切帧重跑多次。
    QMetaObject::Connection m_IsoAnimFrameConn;

    // IsoVolume 时序会话的源容器(只有多帧 .pvd 提取过才有效)。
    // 打开动画面板时用它把"当前模型"切回去 —— 框架动画面板绑定当前模型,
    // 若它停在静态结果上则只有 1 帧, 无法拖动/播放。
    iGame::DataObject::Pointer m_IsoAnimContainerObj;

    // 时序会话中"当前这一帧"的等值体结果: 源数据被隐藏后, 用户切换属性着色时
    // 用它把同一套着色同步刷新到结果上。
    iGame::DataObject::Pointer m_IsoAnimResultObj;

    // 「选中谁就播谁」: true = 播等值体结果(每帧重跑滤镜); false = 播源数据原始帧。
    // 由模型树的选择决定, 见 CurrendModelChanged 处理。
    bool m_IsoAnimShowResult{true};
    // 选择处理里会回头改"场景当前模型", 可能再次触发 CurrendModelChanged, 用它防重入。
    bool m_IsoAnimSelecting{false};
    QMetaObject::Connection m_IsoAnimSelectConn;
};


#endif //IGAMEVIS_IGQTMAINWINDOW_H
