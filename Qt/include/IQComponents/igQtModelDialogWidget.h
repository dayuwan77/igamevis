#pragma once

#include <iGameSceneManager.h>

#include <IQComponents/igQtModelTreeWidget.h>
#include <IQComponents/igQtPropertyTreeWidget.h>
#include <IQCore/igQtExportModule.h>

#include <ui_layerDialog.h>

#include <Plugin/qtpropertybrowser/qteditorfactory.h>
#include <Plugin/qtpropertybrowser/qttreepropertybrowser.h>
#include <Plugin/qtpropertybrowser/qtvariantproperty.h>
#include <QMouseEvent>
#include <QObject>
#include <QString>
#include <QTreeWidget>
#include <functional>
#include <iostream>
#include <vector>

class QDockWidget;

class IG_QT_MODULE_EXPORT igQtModelDialogWidget : public QObject {
    Q_OBJECT
public:
    igQtModelDialogWidget(QWidget* parent);
    ~igQtModelDialogWidget() override = default;

    /** 上半部分：圖層/模型樹，可單獨拖出懸浮 */
    QDockWidget* getTreeDock() const { return m_treeDock; }
    /** 下半部分：屬性 / 模型資訊 */
    QDockWidget* getPropertiesDock() const { return m_propertiesDock; }

    /**
     * 就地转换后把该模型改名（同步改 DataObject 自己的名字与树上那一行）：
     * 名字冲突时自动 _2/_3（检查时排除本行自己）。返回最终使用的名字。
     */
    QString renameModelRow(iGame::DataObject::Pointer obj, const QString& newName);


public slots:
    int addModelToModelTree(iGame::Model::Pointer model);
    ModelTreeWidgetItem* getItemFromObject(iGame::DataObject::Pointer obj);
    void updateAllAttriubute(iGame::DataObject::Pointer obj);
    void refreshAnimationAttributes(iGame::DataObject::Pointer obj);
    void updateItemName(iGame::DataObject::Pointer obj);
    int addDataObjectToModelTree(iGame::DataObject::Pointer obj, ItemSource source);
    /** 就地刷新属性行的挂载类型图标/提示（只改图标和提示，不重建行，避免丢掉子块行） */
    void refreshAttributeBadges(iGame::DataObject::Pointer obj);
    int updateCurrentModelInfo();
    void updateCurrentModelProperty(iGame::Model* model);
    void updateCurrentModelProperty();
    int updateCloudPicture();
    void deleteCurrentModel();
    /** 主题切换后刷新模型树标题栏配色（文字、图标、背景） */
    void refreshStyle();
    void onPropertyChanged(QtProperty* property, const QVariant& value);
    iGame::Model* GetCurrentModel();
    void setCurrentItem(QTreeWidgetItem* item) {
        if (modelTreeWidget) modelTreeWidget->setCurrentItem(item);
    }
    void positionTreeDockToRendererCorner(QWidget* rendererWidget);

    void setTreeDockCollapsed(bool collapsed);
    bool isTreeDockCollapsed() const { return m_treeCollapsed; }

    /**
     * 【单输入 filter 用】当前选中的单个数据对象。
     *
     * - 选中顶层模型  -> 该模型的 DataObject
     * - 选中多块子块  -> 该子块的 DataObject（不是父模型）
     * - 无任何选中    -> 回退为当前模型的 DataObject（保持既有行为）
     */
    iGame::DataObject::Pointer GetCurrentDataObject();

    /**
     * 【多输入 filter 用】当前选中的一组数据对象（如多选合组）。
     *
     * - 顶层模型行与多块子块行都会被映射为各自的 DataObject
     * - 属性行会被忽略
     * - 未多选时最多返回 1 个；需 Ctrl / Shift 配合多选
     */
    std::vector<iGame::DataObject::Pointer> GetSelectedDataObjects();

    /**
     * 刷新模型树所有节点的眼睛图标，使其与各自数据的【真实可见性】一致。
     *
     * 适用场景：同一个 DataObject 可能同时挂在「顶层模型节点」和
     * 「某个组合模型的子块节点」下（如 GroupDatasets 的输出），
     * 两处共享同一份可见性 —— 任一处变化后调用本方法即可消除图标不一致。
     */
    void RefreshModelTreeVisibilityIcons();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    void CurrendModelChanged();
    void CloudPictureChanged();
    void ModelDeleted(const std::string& modelName);  // Emitted when model is deleted
    void Update();

private:
    void rebuildAttributes(iGame::DataObject::Pointer obj, bool preserveDisplay);
    //iGame::Model* currentModel;

    igQtModelTreeWidget* modelTreeWidget;
    QtTreePropertyBrowser* propertyWidget;
    QTabWidget* tabWidget;

    QtVariantPropertyManager* propertyManager;
    QtVariantEditorFactory* editFactory;

    QtProperty* objectGroup;
    QtVariantProperty* prop_PointSize;
    QtVariantProperty* pror_LineWidth;
    QtVariantProperty* prop_Transparency;

    Ui::LayerDialog* ui;
    QDockWidget* m_treeDock = nullptr;       // 上半
    QDockWidget* m_propertiesDock = nullptr; // 下半
    static bool m_AutoAccelerate;

    bool m_treeCollapsed = false;
    QRect m_treeGeomBeforeCollapse;
    QSize m_treeMinBeforeCollapse;
    QWidget* m_treeTitleBar = nullptr;
    QWidget* m_collapsedBlock = nullptr;
    QString m_treeDockSavedStyleSheet;
    bool m_blockDragActive = false;
    bool m_blockDragged = false;
    QRect m_blockRectAtCollapse;
    QPoint m_blockDragOffset;
    std::function<void(bool)> m_setCollapseVisible;
    void refreshCollapsedBlockStyle();
};
