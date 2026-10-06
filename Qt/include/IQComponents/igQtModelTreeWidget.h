#pragma once

#include <iGameModel.h>
#include <iGameSceneManager.h>

#include <IQComponents/igQtComponents.h>
#include <IQCore/igQtExportModule.h>

#include <QComboBox>
#include <QDockWidget>
#include <QIcon>
#include <QMouseEvent>
#include <QObject>
#include <QPushButton>
#include <QTreeWidget>
#include <qboxlayout.h>

#include <iostream>
#include <vector>

class igQtModelTreeWidget; // forward declaration for dynamic_cast in SubAttribTreeWidgetItem

namespace igQtModelTreeIcons
{
inline const QIcon& EyeOpen() {
    static const QIcon icon(":/Ticon/Icons/select/eye-open.png");
    return icon;
}

inline const QIcon& EyeClose() {
    static const QIcon icon(":/Ticon/Icons/select/eye-close.png");
    return icon;
}

inline const QIcon& Point() {
    static const QIcon icon(":/Ticon/Icons/select/point.png");
    return icon;
}

inline const QIcon& Cell() {
    static const QIcon icon(":/Ticon/Icons/select/hex.png");
    return icon;
}
} // namespace igQtModelTreeIcons

class IG_QT_MODULE_EXPORT ModelTreeWidgetItem : public QTreeWidgetItem {
public:
    ModelTreeWidgetItem(QTreeWidget* parent = nullptr);

    iGame::Model* getModel();

    void setModel(iGame::Model* model);

    void setName(const QString& name);

    void changeVisibility();

    void changeVisibility(bool);

    void viewAttribute(int index, int dim = -1);

    void setCurrentChild(QTreeWidgetItem* child);
    QTreeWidgetItem* getCurrentChild();

    /**
     * 按模型的【实际可见性】刷新眼睛图标；只改图标，不改变任何状态。
     *
     * 存在的意义：同一个 DataObject 可能同时挂在「顶层模型节点」和
     * 「某个组合模型的子块节点」下，两处共享同一份可见性 ——
     * 在任一处切换后，另一处的图标必须跟着刷新才不会自相矛盾。
     */
    void refreshVisibilityIcon();

    int getModelId() const { return modelId; }
    void setModelId(int id) { modelId = id; }

protected:
    bool getVisibility() const;

    void show();
    void hide();

    void showBoundingBox();
    void hideBoundingBox();

    void showPoints();
    void hidePoints();

    void showWireframe();
    void hideWireframe();

    void showFill();
    void hideFill();

    void showPickedItem();
    void hidePickedItem();

    void update();

private:
    bool visibility;
    HoverButton* view_bbox;
    HoverButton* view_points;
    HoverButton* view_wireframe;
    HoverButton* view_fill;
    HoverButton* view_pickedItem;

    int modelId;
    iGame::Model* model{nullptr};
    QTreeWidget* parent{nullptr};
    QTreeWidgetItem* current_child{nullptr};
};

class IG_QT_MODULE_EXPORT AttribTreeWidgetItem : public QTreeWidgetItem {
public:
    AttribTreeWidgetItem(int index, QTreeWidget* treeview = nullptr, ModelTreeWidgetItem* parent = nullptr);

    void setDimension(int length);
    int getDimension() const { return m_Dimension; }

    /** 该行对应 AttributeSet 里的下标（供转换后原地刷新挂载类型图标用） */
    int attributeIndex() const { return index; }

    int currentIndex() const { return comboBox->currentIndex(); }
    void show() { comboBox->show(); }
    void hide() { comboBox->hide(); }
    MComboBox* get() { return comboBox; }
    void viewAttribute(int dim) { parent->viewAttribute(index, dim); }

private:
    int index;
    int m_Dimension{1};
    MComboBox* comboBox;
    ModelTreeWidgetItem* parent;
};

// Sub-data object tree item to represent DataObject hierarchy
class IG_QT_MODULE_EXPORT SubObjectTreeWidgetItem : public QTreeWidgetItem {
public:
    SubObjectTreeWidgetItem(QTreeWidgetItem* parent = nullptr) : QTreeWidgetItem(parent) {}

    void setDataObject(iGame::DataObject::Pointer obj) { m_DataObject = obj; }
    iGame::DataObject::Pointer getDataObject() const { return m_DataObject; }

    void setName(const QString& name) {
        setText(0, name);
        setToolTip(0, name);
    }

    bool getVisibility() const {
        auto draw = DynamicCast<iGame::DrawObject>(m_DataObject);
        return draw ? draw->GetVisibility() : true;
    }

    void changeVisibility() {
        if (getVisibility()) hide();
        else
            show();
    }

    void changeVisibility(bool vis) {
        if (!vis) hide();
        else
            show();
    }

    void show() {
        auto draw = DynamicCast<iGame::DrawObject>(m_DataObject);
        if (draw) { draw->SetVisibility(true); }
        SyncIconWithVisibility(true);
        updateScene();
    }

    void hide() {
        auto draw = DynamicCast<iGame::DrawObject>(m_DataObject);
        if (draw) { draw->SetVisibility(false); }
        SyncIconWithVisibility(true);
        updateScene();
    }

    void setCurrentChild(QTreeWidgetItem* c) { m_CurrentChild = c; }
    QTreeWidgetItem* getCurrentChild() const { return m_CurrentChild; }

    // Update only icons to reflect current visibility status (no data changes)
    void SyncIconWithVisibility(bool deep = true) {
        auto draw = DynamicCast<iGame::DrawObject>(m_DataObject);
        if (draw) {
            this->setIcon(0, draw->GetVisibility() ? igQtModelTreeIcons::EyeOpen()
                                                   : igQtModelTreeIcons::EyeClose());
        }
        if (deep) {
            for (int i = 0; i < childCount(); ++i) {
                if (auto* sub = dynamic_cast<SubObjectTreeWidgetItem*>(child(i))) { sub->SyncIconWithVisibility(true); }
            }
        }
    }

private:
    void updateScene() {
        // Find parent model item and trigger scene update
        QTreeWidgetItem* p = parent();
        while (p) {
            if (auto* mi = dynamic_cast<ModelTreeWidgetItem*>(p)) {
                if (mi->getModel()) mi->getModel()->Update();
                break;
            }
            p = p->parent();
        }
    }

    iGame::DataObject::Pointer m_DataObject{nullptr};
    QTreeWidgetItem* m_CurrentChild{nullptr};
};

// Attribute item for sub-data objects
class IG_QT_MODULE_EXPORT SubAttribTreeWidgetItem : public QTreeWidgetItem {
public:
    SubAttribTreeWidgetItem(int index, QTreeWidget* treeview = nullptr, SubObjectTreeWidgetItem* parent = nullptr)
        : QTreeWidgetItem(parent), m_Index(index), m_Tree(treeview), m_Parent(parent) {
        QWidget* widget = new QWidget(treeview);
        widget->setStyleSheet(QStringLiteral("background-color: transparent; border: none;"));
        m_Combo = new MComboBox(this, widget);
        auto* comboLayout = new QHBoxLayout(widget);
        comboLayout->setContentsMargins(0, 0, 0, 0);
        comboLayout->addWidget(m_Combo);
        m_Combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        m_Combo->setStyleSheet("QComboBox { background-color: transparent; }");
        setDimension(1);
        treeview->setItemWidget(this, 1, widget);
        hide();
    }


    void setDimension(int length) {
        m_Combo->clear();
        m_Dimension = length;
        
        // For Dimension=1, show only the single dimension value, not magnitude
        if (length == 1) {
            m_Combo->addItem(QString::fromStdString("x"));
            m_Combo->setCurrentIndex(0);
            return;
        }
        
        // For Dimension>=2, show magnitude first, then individual dimensions
        m_Combo->addItem("magnitude");
        if (length < 4) {
            if (length > 0) m_Combo->addItem(QString::fromStdString("x"));
            if (length > 1) m_Combo->addItem(QString::fromStdString("y"));
            if (length > 2) m_Combo->addItem(QString::fromStdString("z"));
        } else {
            for (int i = 0; i < length; i++) { m_Combo->addItem(QString::fromStdString("D" + std::to_string(i))); }
        }
        m_Combo->setCurrentIndex(0);
    }

    int getDimension() const { return m_Dimension; }
    /** 该行对应（子块）AttributeSet 里的下标 */
    int attributeIndex() const { return m_Index; }

    int currentIndex() const { return m_Combo->currentIndex(); }
    void show() { m_Combo->show(); }
    void hide() { m_Combo->hide(); }

    void viewAttribute(int dim) {
        if (!m_Parent) return;
        auto obj = m_Parent->getDataObject();
        auto draw = DynamicCast<iGame::DrawObject>(obj);
        if (draw) {
            auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
            draw->ViewCloudPicture(scene, m_Index, dim);
            if (scene) scene->Update();
        }
    }

private:
    int m_Index;
    int m_Dimension{1};
    MComboBox* m_Combo{nullptr};
    QTreeWidget* m_Tree{nullptr};
    SubObjectTreeWidgetItem* m_Parent{nullptr};
};

class IG_QT_MODULE_EXPORT igQtModelTreeWidget : public QTreeWidget {
    Q_OBJECT

public:
    igQtModelTreeWidget(QWidget* parent = nullptr);

    ModelTreeWidgetItem* getItem(const QPoint& p) const;
    QTreeWidgetItem* getChild(const QPoint& p) const;
    void setLeftColumnPercent(int percent);

    /**
     * 当前选中的【一组】数据对象。
     *
     * 映射规则：
     *   - 顶层模型行        -> model->GetDataObject()
     *   - 多块装配体的子块行 -> sub->getDataObject()   （子块本身，而不是它的父模型）
     *   - 属性行（Point/Cell 属性）-> 忽略
     *
     * 供【多输入 filter】（如多选合组）使用。需要 Ctrl / Shift 配合多选。
     */
    std::vector<iGame::DataObject::Pointer> getSelectedDataObjects() const;

    /**
     * 当前选中的【单个】数据对象（取选中集合的第一个）；无选中时返回 nullptr。
     *
     * 供【单输入 filter】使用：选中多块子块时会返回该子块本身。
     */
    iGame::DataObject::Pointer getSingleSelectedDataObject() const;

    /**
     * 按【每个节点的真实可见性】刷新整棵树的眼睛图标。
     *
     * 为什么需要全树刷新：同一个 DataObject 可能同时出现在
     *   · 顶层模型节点            ModelTreeWidgetItem
     *   · 某个组合模型的子块节点   SubObjectTreeWidgetItem
     * 两处指向同一个对象、共享同一份可见性。在任意一处切换显隐后，
     * 另一处的图标如果不刷新，就会出现「这里隐藏了，那边眼睛还亮着」。
     *
     * 这是通用规则，不涉及任何「组合模型」的特殊语义。
     */
    void syncVisibilityIcons();

    //void setCurrentModelItem(ModelTreeWidgetItem* item);
    //ModelTreeWidgetItem* getCurrentModelItem();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

signals:
    void ChangeCurrentModel(iGame::Model* model);
    void ViewCloudPicture();

private:
    void applyColumnProportions();
    QRect eyeHitRect(const QTreeWidgetItem* item) const;
    //ModelTreeWidgetItem* currentModelItem{nullptr};
    int m_leftPercent = 36;
    int m_lastLeft = -1;
    int m_lastRight = -1;
};
