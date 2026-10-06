#include <IQComponents/igQtModelTreeWidget.h>
#include <QAbstractItemView>
#include <QAction>
#include <QMenu>
#include <QHeaderView>

#include "iGameSceneManager.h"

ModelTreeWidgetItem::ModelTreeWidgetItem(QTreeWidget* parent) : QTreeWidgetItem(parent), visibility(true) {
    QWidget* buttonWidget = new QWidget(parent);
    QHBoxLayout* layout = new QHBoxLayout(buttonWidget);

    view_bbox = new HoverButton(buttonWidget);
    view_points = new HoverButton(buttonWidget);
    view_wireframe = new HoverButton(buttonWidget);
    view_fill = new HoverButton(buttonWidget);
    view_pickedItem = new HoverButton(buttonWidget);

    view_bbox->setIcon(QIcon(":/Ticon/Icons/select/bbox.png"));
    view_points->setIcon(QIcon(":/Ticon/Icons/select/points.png"));
    view_wireframe->setIcon(QIcon(":/Ticon/Icons/select/wireframe.png"));
    view_fill->setIcon(QIcon(":/Ticon/Icons/select/fill.png"));
    view_pickedItem->setIcon(QIcon(":/Ticon/Icons/select/selected.png"));

    layout->setSpacing(0);
    layout->addStretch();
    layout->addWidget(view_bbox);
    layout->addWidget(view_points);
    layout->addWidget(view_wireframe);
    layout->addWidget(view_fill);
    layout->addWidget(view_pickedItem);
    layout->setContentsMargins(0, 2, 2, 2);

    parent->setItemWidget(this, 1, buttonWidget);

    view_wireframe->setChecked(false);
    show();

    view_bbox->setConcernFunctor(&ModelTreeWidgetItem::showBoundingBox, this);
    view_bbox->setCancelFunctor(&ModelTreeWidgetItem::hideBoundingBox, this);

    view_points->setConcernFunctor(&ModelTreeWidgetItem::showPoints, this);
    view_points->setCancelFunctor(&ModelTreeWidgetItem::hidePoints, this);

    view_wireframe->setConcernFunctor(&ModelTreeWidgetItem::showWireframe, this);
    view_wireframe->setCancelFunctor(&ModelTreeWidgetItem::hideWireframe, this);

    view_fill->setConcernFunctor(&ModelTreeWidgetItem::showFill, this);
    view_fill->setCancelFunctor(&ModelTreeWidgetItem::hideFill, this);

    view_pickedItem->setConcernFunctor(&ModelTreeWidgetItem::showPickedItem, this);
    view_pickedItem->setCancelFunctor(&ModelTreeWidgetItem::hidePickedItem, this);
    this->parent = parent;
}
iGame::Model* ModelTreeWidgetItem::getModel() { return this->model; }

void ModelTreeWidgetItem::setModel(iGame::Model* model) {
    this->model = model;
    view_fill->setChecked(true);
    view_pickedItem->setChecked(true);
    //    view_wireframe->setChecked(true);
    //    showWireframe();
    showFill();
    showPickedItem();
}

void ModelTreeWidgetItem::setName(const QString& name) {
    setText(0, name);
    setToolTip(0, name);
}

void ModelTreeWidgetItem::changeVisibility() {
    if (getVisibility()) {
        hide();
    } else {
        show();
    }
}
void ModelTreeWidgetItem::changeVisibility(bool vis) {
    if (!vis) {
        hide();
    } else {
        show();
    }
}
void ModelTreeWidgetItem::viewAttribute(int index, int dim) {
    model->ViewCloudPicture(index, dim);
    Q_EMIT dynamic_cast<igQtModelTreeWidget*>(this->parent)->ViewCloudPicture();
}

void ModelTreeWidgetItem::setCurrentChild(QTreeWidgetItem* child) { current_child = child; }
QTreeWidgetItem* ModelTreeWidgetItem::getCurrentChild() { return current_child; }

namespace {
/**
 * 子树中是否存在【可见】的绘制对象。
 *
 * 用于容器模型（多块）的眼睛图标判定：Model::Draw() 对容器【只绘制子块、不绘制自身】
 * （见 iGameModel.cpp 的 `if (!HasSubDataObject()) draw(自己) else for(子块) draw(子块)`），
 * 所以容器"在画面上有没有东西"完全取决于它的子树，而不是它自己的可见性标志。
 */
bool AnySubDataObjectVisible(const iGame::DataObject::Pointer& obj) {
    if (!obj) { return false; }
    for (auto it = obj->SubDataObjectIteratorBegin(); it != obj->SubDataObjectIteratorEnd(); ++it) {
        auto sub = it->second;
        if (!sub) { continue; }
        auto draw = DynamicCast<iGame::DrawObject>(sub);
        if (draw && draw->GetVisibility()) { return true; }
        // 嵌套多块：继续向下找
        if (sub->HasSubDataObject() && AnySubDataObjectVisible(sub)) { return true; }
    }
    return false;
}
} // namespace

bool ModelTreeWidgetItem::getVisibility() const {
    // 以「模型在画面上是否真的可见」为准，而不是本节点自己的 visibility 成员。
    //
    // 两层原因：
    //  1) 同一个 DataObject 可能同时挂在顶层模型节点和某个容器模型的子块节点下，
    //     两处共享同一份可见性。若这里读各自的成员变量就会状态漂移 ——
    //     在子块节点处隐藏后，顶层节点仍以为自己是"亮"的，用户点它的眼睛得点两次才生效。
    //  2) 容器模型（多块）不绘制自身，只绘制子块。因此它的图标必须看【子树】：
    //     隐藏任一个子块后，容器的图标要跟着变，否则就会出现
    //     "子块眼睛灭了、外层图标还亮着"的不一致。
    if (model) {
        if (auto root = DynamicCast<iGame::DrawObject>(model->GetDataObject())) {
            if (root->HasSubDataObject()) {
                // 容器：自身可见 且 子树中存在可见节点
                return root->GetVisibility() && AnySubDataObjectVisible(model->GetDataObject());
            }
            return root->GetVisibility();
        }
    }
    return visibility;
}

void ModelTreeWidgetItem::refreshVisibilityIcon() {
    this->setIcon(0, getVisibility() ? igQtModelTreeIcons::EyeOpen() : igQtModelTreeIcons::EyeClose());
}

void ModelTreeWidgetItem::show() {
    visibility = true;
    this->setIcon(0, igQtModelTreeIcons::EyeOpen());
    if (!model) { return; }

    model->Show();
    update();
}

void ModelTreeWidgetItem::hide() {
    visibility = false;
    this->setIcon(0, igQtModelTreeIcons::EyeClose());
    if (!model) { return; }

    model->Hide();
    update();
}

void ModelTreeWidgetItem::showBoundingBox() {
    model->SetBoundingBoxSwitch(true);
    update();
}
void ModelTreeWidgetItem::hideBoundingBox() {
    model->SetBoundingBoxSwitch(false);
    update();
}

void ModelTreeWidgetItem::showPoints() {
    model->SetViewPointsSwitch(true);
    update();
}
void ModelTreeWidgetItem::hidePoints() {
    model->SetViewPointsSwitch(false);
    update();
}

void ModelTreeWidgetItem::showWireframe() {
    model->SetViewWireframeSwitch(true);
    update();
}
void ModelTreeWidgetItem::hideWireframe() {
    model->SetViewWireframeSwitch(false);
    update();
}

void ModelTreeWidgetItem::showFill() {
    model->SetViewFillSwitch(true);
    update();
}
void ModelTreeWidgetItem::hideFill() {
    model->SetViewFillSwitch(false);
    update();
}

void ModelTreeWidgetItem::showPickedItem() {
    model->SetPickedItemSwitch(true);
    update();
}
void ModelTreeWidgetItem::hidePickedItem() {
    model->SetPickedItemSwitch(false);
    update();
}

void ModelTreeWidgetItem::update() {
    if (model) { model->Update(); }
}

AttribTreeWidgetItem::AttribTreeWidgetItem(int index, QTreeWidget* treeview, ModelTreeWidgetItem* parent)
    : index(index), QTreeWidgetItem(parent), parent(parent) {

    QWidget* widget = new QWidget(treeview);
    comboBox = new MComboBox(this, widget);
    comboBox->setStyleSheet("QComboBox { background-color: transparent; }"
                            "QComboBox QAbstractItemView { background-color: white; }");

    setDimension(1);

    treeview->setItemWidget(this, 1, widget);

    hide();
}

void AttribTreeWidgetItem::setDimension(int length) {
    comboBox->clear();
    m_Dimension = length;

    // For Dimension=1, show only the single dimension value, not magnitude
    if (length == 1) {
        comboBox->addItem(QString::fromStdString("x"));
        comboBox->setCurrentIndex(0);
        return;
    }

    // For Dimension>=2, show magnitude first, then individual dimensions
    comboBox->addItem("magnitude");
    if (length < 4) {
        if (length > 0) comboBox->addItem(QString::fromStdString("x"));
        if (length > 1) comboBox->addItem(QString::fromStdString("y"));
        if (length > 2) comboBox->addItem(QString::fromStdString("z"));
    } else {
        for (int i = 0; i < length; i++) { comboBox->addItem(QString::fromStdString("D" + std::to_string(i))); }
    }
    comboBox->setCurrentIndex(0);
}

igQtModelTreeWidget::igQtModelTreeWidget(QWidget* parent) : QTreeWidget(parent) {
    // Keep eliding ("xxxx...") but show full name via tooltip.
    setTextElideMode(Qt::ElideRight);

    // 允许 Ctrl / Shift 多选：供「多输入 filter」（如多选合组）通过
    // getSelectedDataObjects() 取用。单选语义不变（普通点击仍会清掉其他选中）。
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    if (header()) {
        header()->setStretchLastSection(false);
        header()->setSectionResizeMode(QHeaderView::Interactive);
    }
}

ModelTreeWidgetItem* igQtModelTreeWidget::getItem(const QPoint& p) const {
    return dynamic_cast<ModelTreeWidgetItem*>(itemAt(p));
}
QTreeWidgetItem* igQtModelTreeWidget::getChild(const QPoint& p) const {
    return dynamic_cast<QTreeWidgetItem*>(itemAt(p));
}

std::vector<iGame::DataObject::Pointer> igQtModelTreeWidget::getSelectedDataObjects() const {
    std::vector<iGame::DataObject::Pointer> result;
    for (QTreeWidgetItem* it: selectedItems()) {
        if (auto* modelItem = dynamic_cast<ModelTreeWidgetItem*>(it)) {
            // 顶层模型行
            if (modelItem->getModel() && modelItem->getModel()->GetDataObject()) {
                result.push_back(modelItem->getModel()->GetDataObject());
            }
        } else if (auto* subItem = dynamic_cast<SubObjectTreeWidgetItem*>(it)) {
            // 多块装配体的子块行：返回【子块本身】，而不是父模型
            if (subItem->getDataObject()) { result.push_back(subItem->getDataObject()); }
        }
        // AttribTreeWidgetItem / SubAttribTreeWidgetItem 是属性行，不是数据对象，忽略
    }
    return result;
}

iGame::DataObject::Pointer igQtModelTreeWidget::getSingleSelectedDataObject() const {
    auto objs = getSelectedDataObjects();
    return objs.empty() ? nullptr : objs.front();
}

void igQtModelTreeWidget::syncVisibilityIcons() {
    // 顶层模型节点：每个都按自己 DataObject 的真实可见性刷新
    for (int i = 0; i < topLevelItemCount(); ++i) {
        auto* item = dynamic_cast<ModelTreeWidgetItem*>(topLevelItem(i));
        if (!item) { continue; }
        item->refreshVisibilityIcon();

        // 该模型下的多块子块节点（SyncIconWithVisibility 会递归到更深层）
        for (int j = 0; j < item->childCount(); ++j) {
            if (auto* sub = dynamic_cast<SubObjectTreeWidgetItem*>(item->child(j))) {
                sub->SyncIconWithVisibility(true);
            }
        }
    }
}

//void igQtModelTreeWidget::setCurrentModelItem(ModelTreeWidgetItem* item) {
//    currentModelItem = item;
//    //std::cout << "change\n";
//}

//ModelTreeWidgetItem* igQtModelTreeWidget::getCurrentModelItem() { return currentModelItem; }

//void igQtModelTreeWidget::setCurrentModel(ModelTreeWidgetItem* item) {
//    if (currentModel) {
//        auto* current = dynamic_cast<AttribTreeWidgetItem*>(
//                currentModel->getCurrentChild());
//        if (current) { current->hide(); }
//        currentModel->setCurrentChild(nullptr);
//    }
//    currentModel = item;
//}

void igQtModelTreeWidget::mousePressEvent(QMouseEvent* event) {
    bool call = true;
    ModelTreeWidgetItem* item = getItem(event->pos());
    QTreeWidgetItem* child = nullptr;

    if (item) {
        // Gets the position of the click and the position of the icon
        QRect iconItem = visualItemRect(item);
        QSize iconSize = item->icon(0).actualSize(QSize(20, 24));
        QRect iconRect(iconItem.left() + 4, iconItem.top() + (iconItem.height() - iconSize.height()) / 2,
                       iconSize.width(), iconSize.height());

        // Check if click is on the expand/collapse indicator (branch arrow)
        int indentation_level = 0;
        QTreeWidgetItem* parentItem = static_cast<QTreeWidgetItem*>(item)->parent();
        while (parentItem) {
            indentation_level++;
            parentItem = parentItem->parent();
        }
        int indicatorWidth = indentation() * (indentation_level + 1);
        QRect indicatorRect(0, iconItem.top(), indicatorWidth, iconItem.height());
        bool clickedOnIndicator = indicatorRect.contains(event->pos());

        if (event->button() == Qt::RightButton) {
            QMenu menu(this);

            // 菜单项 1：设置旋转中心
            QAction* setCenterAction = menu.addAction(QString::fromUtf8("设置旋转中心为当前模型"));
            connect(setCenterAction, &QAction::triggered, this, [item]() {
                auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
                if (scene && item->getModel()) { scene->ResetCameraView(item->getModel()->GetDataObject()); }
            });

            // 菜单项 2：构建渲染加速结构
            QAction* buildAccelAction = menu.addAction(QString::fromUtf8("构建渲染加速结构"));
            connect(buildAccelAction, &QAction::triggered, this, [item]() {
                auto model = item->getModel();
                if (model && model->GetDataObject()) {
                    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
                    auto drawObj = iGame::DynamicCast<iGame::DrawObject>(model->GetDataObject());
                    if (drawObj) {
                        drawObj->SetAccelerationOption(true);
                        scene->Update();
                    }
                }
            });

            // 菜单项3：关闭加速结构
            QAction* disableAccelAction = menu.addAction(QString::fromUtf8("关闭渲染加速结构"));
            connect(disableAccelAction, &QAction::triggered, this, [item]() {
                auto model = item->getModel();
                if (model && model->GetDataObject()) {
                    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
                    auto drawObj = iGame::DynamicCast<iGame::DrawObject>(model->GetDataObject());
                    if (drawObj) {
                        drawObj->SetAccelerationOption(false);
                        scene->Update();
                    }
                }
            });

            // 菜单项4：开启/关闭Meshlet可视化
            QAction* meshletRenderingAction = menu.addAction(QString::fromUtf8("开启/关闭Meshlet可视化"));
            connect(meshletRenderingAction, &QAction::triggered, this, [item]() {
                auto model = item->getModel();
                if (model && model->GetDataObject()) {
                    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
                    auto drawObj = iGame::DynamicCast<iGame::DrawObject>(model->GetDataObject());
                    if (drawObj) {
                        bool lastOption = drawObj->GetRenderWithMeshlet();
                        drawObj->SetRenderWithMeshlet(!lastOption);
                        scene->Update();
                    }
                }
            });

            // 弹出菜单
            menu.exec(viewport()->mapToGlobal(event->pos()));
        }

        // Determine if the icon area has been clicked
        if (iconRect.contains(event->pos())) {
            item->changeVisibility();
            // 同一个 DataObject 可能同时挂在【顶层模型节点】和【组合模型的子块节点】下，
            // 两者共享同一份可见性 —— 因此每次切换后同步【全树】图标，避免两处自相矛盾。
            syncVisibilityIcons();
            call = false;
        } else if (clickedOnIndicator) {
            // Clicked on expand/collapse indicator, only handle expand/collapse, don't change attribute display
            // Just let the base class handle the expand/collapse
        } else if (currentItem() != item) { // Check operation - only when clicking on the model itself
            auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
            if (item->getModel() != scene->GetCurrentModel()) {
                scene->SetCurrentModel(item->getModel());
            }
            // 无论是否与当前模型相同都要通知：删除模型后场景当前模型可能已被 Scene::RemoveModel
            // 自动切到别的模型，此时若只在"模型不同"时发信号，左侧模型信息会一直停在被删模型上
            emit ChangeCurrentModel(item->getModel());

            item->setSelected(true);
            item->getModel()->ViewCloudPicture(-1);
            Q_EMIT ViewCloudPicture();
            auto* current = dynamic_cast<AttribTreeWidgetItem*>(item->getCurrentChild());
            if (current) { current->hide(); }
            item->setCurrentChild(nullptr);

            //if (currentModelItem != item) { this->setCurrentModelItem(item); }
        }


    } else if ((child = getChild(event->pos())) && child) {
        // Sub-data object item
        if (auto* sub = dynamic_cast<SubObjectTreeWidgetItem*>(child)) {
            // Right-click: set rotation center to sub-block bbox center
            if (event->button() == Qt::RightButton) {
                QMenu menu(this);
                QAction* setCenterAction = menu.addAction(QString::fromUtf8("设置旋转中心为当前子块"));
                connect(setCenterAction, &QAction::triggered, this, [sub]() {
                    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
                    if (!scene) { return; }
                    auto draw = DynamicCast<iGame::DrawObject>(sub->getDataObject());
                    if (!draw) { return; }
                    scene->ResetCameraView(sub->getDataObject());
                    scene->Update();
                });
                menu.exec(viewport()->mapToGlobal(event->pos()));
            } else {
                // Left click: eye icon toggle or select parent model
                QRect iconItem = visualItemRect(sub);
                QSize iconSize = sub->icon(0).actualSize(QSize(20, 24));
                QRect iconRect(iconItem.left() + 4, iconItem.top() + (iconItem.height() - iconSize.height()) / 2,
                               iconSize.width(), iconSize.height());
                if (iconRect.contains(event->pos())) {
                    sub->changeVisibility();
                    // 子块与顶层原模型往往指向同一个 DataObject（共享指针）——
                    // 这里必须同步【全树】图标，否则会出现"子块眼睛灭了、原模型眼睛还亮着"。
                    syncVisibilityIcons();
                    call = false;
                } else {
                    if (auto* parent = dynamic_cast<ModelTreeWidgetItem*>(sub->parent())) {
                        if (currentItem() != parent) {
                            iGame::SceneManager::Instance()->GetCurrentScene()->SetCurrentModel(parent->getModel());
                            emit ChangeCurrentModel(parent->getModel());
                        }
                    }

                    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
                    if (!scene) { return; }
                    auto draw = DynamicCast<iGame::DrawObject>(sub->getDataObject());
                    if (!draw) { return; }
                    draw->ViewCloudPicture(scene, -1);
                    Q_EMIT ViewCloudPicture();
                }
            }
        } else if (auto* sa = dynamic_cast<SubAttribTreeWidgetItem*>(child)) {
            // Handle sub-attribute selection display and apply
            auto* parent = dynamic_cast<SubObjectTreeWidgetItem*>(sa->parent());
            if (parent) {
                // Hide previous
                if (auto* current = dynamic_cast<SubAttribTreeWidgetItem*>(parent->getCurrentChild())) {
                    current->hide();
                }
                // Show current
                sa->show();
                parent->setCurrentChild(sa);
                int dim = sa->currentIndex();
                if (dim == -1) dim = 0;
                // For single-component fields: use component 0
                // For multi-component fields: index 0=Magnitude(-1), 1=x(0), 2=y(1), etc.
                int actualDim = (sa->getDimension() == 1) ? 0 : (dim - 1);
                sa->viewAttribute(actualDim);
                call = false;
            }
        } else {
            // Top-level attribute item under model
            int index = child->data(0, Qt::UserRole).toInt();
            ModelTreeWidgetItem* parent = dynamic_cast<ModelTreeWidgetItem*>(child->parent());
            if (parent) {
                if (parent->getModel() != iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()) {
                    iGame::SceneManager::Instance()->GetCurrentScene()->SetCurrentModel(parent->getModelId());

                    emit ChangeCurrentModel(parent->getModel());
                }
                AttribTreeWidgetItem* current{nullptr};
                if (parent->getCurrentChild()) {
                    current = dynamic_cast<AttribTreeWidgetItem*>(parent->getCurrentChild());
                }

                if (current) { current->hide(); }
                AttribTreeWidgetItem* c = dynamic_cast<AttribTreeWidgetItem*>(child);
                if (c) {
                    c->show();
                    parent->setCurrentChild(child);

                    int dim = c->currentIndex();
                    if (dim == -1) { dim = 0; }
                    // For single-component fields: use component 0
                    // For multi-component fields: index 0=Magnitude(-1), 1=x(0), 2=y(1), etc.
                    int actualDim = (c->getDimension() == 1) ? 0 : (dim - 1);
                    c->viewAttribute(actualDim);
                    Q_EMIT ViewCloudPicture();
                }
            }
        }
    }
    if (call) {
        // Call the base class's mousePressEvent to ensure that other events continue to be handled
        QTreeWidget::mousePressEvent(event);
    }
}
