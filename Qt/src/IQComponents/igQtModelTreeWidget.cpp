#include <IQComponents/igQtModelTreeWidget.h>
#include <QAbstractItemView>
#include <QAction>
#include <QMenu>
#include <QHeaderView>
#include <QEvent>
#include <QStyle>
#include <QStyleOptionViewItem>

#include "iGameSceneManager.h"

ModelTreeWidgetItem::ModelTreeWidgetItem(QTreeWidget* parent) : QTreeWidgetItem(parent), visibility(true) {
    QWidget* buttonWidget = new QWidget(parent);
    buttonWidget->setStyleSheet(QStringLiteral("background-color: transparent; border: none;"));
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
    layout->addStretch();
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
    // Reflect the style chosen by the reader/filter without changing the model.
    auto drawObject = DynamicCast<iGame::DrawObject>(model->GetDataObject());
    const auto viewStyle = drawObject ? drawObject->GetViewStyle() : 0;
    view_points->setChecked((viewStyle & IG_POINTS) != 0);
    view_wireframe->setChecked((viewStyle & IG_WIREFRAME) != 0);
    view_fill->setChecked((viewStyle & IG_SURFACE) != 0);
    view_pickedItem->setChecked(true);
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
    // 以「模型在画面上是否真的可见」为准，而不是本节点自己的 visibility 成员：
    //  1) 节点成员只在 show()/hide() 时更新，一旦显隐从其他路径被改变就会过时；
    //     直接读渲染对象的真实状态则永远自洽（组内子块是原模型的深拷贝克隆，
    //     显隐各自独立，同样按真实状态读即可）。
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
    widget->setStyleSheet(QStringLiteral("background-color: transparent; border: none;"));
    comboBox = new MComboBox(this, widget);
    auto* comboLayout = new QHBoxLayout(widget);
    comboLayout->setContentsMargins(0, 0, 0, 0);
    comboLayout->addWidget(comboBox);
    comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    comboBox->setStyleSheet("QComboBox { background-color: transparent; }");

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

void igQtModelTreeWidget::setLeftColumnPercent(int percent) {
    m_leftPercent = qBound(30, percent, 60);
    m_lastLeft = -1;
    m_lastRight = -1;
    applyColumnProportions();
}

void igQtModelTreeWidget::resizeEvent(QResizeEvent* event) {
    QTreeWidget::resizeEvent(event);
    applyColumnProportions();
}

void igQtModelTreeWidget::showEvent(QShowEvent* event) {
    QTreeWidget::showEvent(event);
    applyColumnProportions();
}

void igQtModelTreeWidget::applyColumnProportions() {
    if (!header() || columnCount() < 2) return;
    const int sbW = style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, this);
    const int vw = qMax(220, width() - sbW);
    int leftW = qRound(vw * m_leftPercent / 100.0);
    const int minLeft = 108;
    const int maxLeft = qMax(minLeft, vw - 140);
    leftW = qBound(minLeft, leftW, maxLeft);
    const int rightW = vw - leftW;
    if (leftW == m_lastLeft && rightW == m_lastRight) return;
    m_lastLeft = leftW;
    m_lastRight = rightW;
    setColumnWidth(0, leftW);
    setColumnWidth(1, rightW);
}

ModelTreeWidgetItem* igQtModelTreeWidget::getItem(const QPoint& p) const {
    return dynamic_cast<ModelTreeWidgetItem*>(itemAt(p));
}
QTreeWidgetItem* igQtModelTreeWidget::getChild(const QPoint& p) const {
    return dynamic_cast<QTreeWidgetItem*>(itemAt(p));
}

QRect igQtModelTreeWidget::eyeHitRect(const QTreeWidgetItem* item) const {
    if (!item) return QRect();
    const QRect cell = visualItemRect(item);
    if (cell.isEmpty()) return QRect();

    QStyleOptionViewItem opt;
    opt.initFrom(this);
    opt.rect = cell;
    opt.features = QStyleOptionViewItem::HasDecoration | QStyleOptionViewItem::HasDisplay;
    opt.decorationPosition = QStyleOptionViewItem::Left;
    opt.decorationAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    opt.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    opt.text = item->text(0);
    opt.icon = item->icon(0);
    opt.decorationSize = iconSize();
    opt.font = font();
    opt.fontMetrics = QFontMetrics(opt.font);
    const QRect textRect = style()->subElementRect(QStyle::SE_ItemViewItemText, &opt, this);
    if (textRect.isValid() && !textRect.isEmpty() && textRect.left() > cell.left() + 2) {
        return QRect(cell.left(), cell.top(), textRect.left() - cell.left(), cell.height());
    }

    const QSize sz = item->icon(0).actualSize(iconSize());
    return QRect(cell.left() + 4, cell.top() + (cell.height() - sz.height()) / 2, sz.width(), sz.height());
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
    // 多选时以 currentItem（最后点击 / 键盘移动到的那个）为准 —— selectedItems() 的返回顺序
    // 依赖内部选择模型的实现细节，不保证与用户点击顺序一致。
    if (auto* current = currentItem()) {
        if (auto* modelItem = dynamic_cast<ModelTreeWidgetItem*>(current)) {
            if (modelItem->getModel() && modelItem->getModel()->GetDataObject()) {
                return modelItem->getModel()->GetDataObject();
            }
        } else if (auto* subItem = dynamic_cast<SubObjectTreeWidgetItem*>(current)) {
            if (subItem->getDataObject()) { return subItem->getDataObject(); }
        } else if (dynamic_cast<SubAttribTreeWidgetItem*>(current) ||
                   dynamic_cast<AttribTreeWidgetItem*>(current)) {
            // 属性行作为 current 时，上下文 = 该属性行所属的数据对象：
            //   子块属性行 → 所属子块；顶层模型属性行 → 模型根对象。
            // 不能靠 selectedItems() 回落：点击属性行会 setCurrentItem(属性行)，
            // 单选模式下数据对象行的选中被清掉，回落会错误地拿到模型根 ——
            // 信息框 / 属性面板 / 色条因此显示根模型而非子块。
            for (QTreeWidgetItem* p = current->parent(); p; p = p->parent()) {
                if (auto* subItem = dynamic_cast<SubObjectTreeWidgetItem*>(p)) {
                    if (subItem->getDataObject()) { return subItem->getDataObject(); }
                    break;
                }
                if (auto* modelItem = dynamic_cast<ModelTreeWidgetItem*>(p)) {
                    if (modelItem->getModel() && modelItem->getModel()->GetDataObject()) {
                        return modelItem->getModel()->GetDataObject();
                    }
                    break;
                }
            }
        }
    }
    // currentItem 不是数据对象行（如属性行）时，回落为任意一个选中的数据对象。
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
        const QRect iconItem = visualItemRect(item);
        const QRect eyeRect = eyeHitRect(item);

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

        if (clickedOnIndicator) {
        } else if (eyeRect.contains(event->pos())) {
            item->changeVisibility();
            // 切换后同步【全树】图标：顶层模型的显隐会递归作用到多块子块（Model::Hide()），
            // 展开出的子块行图标必须跟着刷新。syncVisibilityIcons() 统一按各对象真实的
            // GetVisibility() 重刷图标，只刷图标、不传播状态（组内克隆与原模型显隐互相独立）。
            syncVisibilityIcons();
            call = false;
        } else if (currentItem() != item) { // Check operation - only when clicking on the model itself
            // 点击顶层模型行：上下文切回该模型的根对象。
            // 即使场景当前模型没变也要发信号刷新 —— 之前可能选中了子块，属性面板 /
            // 信息框停在该子块的属性值上，需要按"选中优先、回落根"切回模型根对象。
            // 顺序：先更新选中状态（本行成为 current），再发信号 —— 信号处理器
            // 读取的是更新后的选中状态（GetCurrentDataObject）。
            item->setSelected(true);
            setCurrentItem(item);
            iGame::SceneManager::Instance()->GetCurrentScene()->SetCurrentModel(item->getModel());
            emit ChangeCurrentModel(item->getModel());

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
                const QRect eyeRect = eyeHitRect(sub);
                int subLevel = 0;
                for (QTreeWidgetItem* p = sub->parent(); p; p = p->parent()) { ++subLevel; }
                const QRect subIndicator(0, eyeRect.top(), indentation() * (subLevel + 1), eyeRect.height());
                if (!subIndicator.contains(event->pos()) && eyeRect.contains(event->pos())) {
                    sub->changeVisibility();
                    // 子块的显隐会影响其所属容器（组）的图标判定 —— 容器不绘制自身，
                    // 图标以【子树中是否还有可见节点】为准（见 ModelTreeWidgetItem::getVisibility）。
                    // 这里统一重刷全树图标，让容器行与各子块行都反映真实状态。
                    syncVisibilityIcons();
                    call = false;
                } else {
                    // 点击子块行：上下文切到该子块（属性面板 / 信息框 / filter 输入源
                    // 均按"选中优先、回落根"取对象）。
                    // 顺序：先更新选中状态（子块行成为 current），再发信号 ——
                    // 信号处理器读取的是更新后的选中状态；若先发信号，取到的还是
                    // 上一次的选中（原实现即因此无法让属性面板切到子块）。
                    // 嵌套多块时子块行可能挂在另一个子块行下 —— 向上找所属的顶层模型行。
                    ModelTreeWidgetItem* rootModelItem = nullptr;
                    for (QTreeWidgetItem* p = sub->parent(); p; p = p->parent()) {
                        if (auto* modelItem = dynamic_cast<ModelTreeWidgetItem*>(p)) {
                            rootModelItem = modelItem;
                            break;
                        }
                    }
                    if (rootModelItem) {
                        sub->setSelected(true);
                        setCurrentItem(sub);
                        iGame::SceneManager::Instance()->GetCurrentScene()->SetCurrentModel(rootModelItem->getModel());
                        emit ChangeCurrentModel(rootModelItem->getModel());
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

                // 点击子块属性行：与子块行 / 顶层属性行分支同规则 —— 先更新选中状态
                // （本行成为 current），再发信号。该分支此前不更新选中态、且 call=false
                // 跳过了基类处理，导致属性行背景永不点亮（高亮停留在子模型行）。
                sa->setSelected(true);
                setCurrentItem(sa);
                // 上下文切到该子块所属的顶层模型（嵌套多块时逐级向上找模型行）。
                ModelTreeWidgetItem* rootModelItem = nullptr;
                for (QTreeWidgetItem* p = parent->parent(); p; p = p->parent()) {
                    if (auto* modelItem = dynamic_cast<ModelTreeWidgetItem*>(p)) {
                        rootModelItem = modelItem;
                        break;
                    }
                }
                if (rootModelItem) {
                    iGame::SceneManager::Instance()->GetCurrentScene()->SetCurrentModel(rootModelItem->getModel());
                    emit ChangeCurrentModel(rootModelItem->getModel());
                }

                int dim = sa->currentIndex();
                if (dim == -1) dim = 0;
                // For single-component fields: use component 0
                // For multi-component fields: index 0=Magnitude(-1), 1=x(0), 2=y(1), etc.
                int actualDim = (sa->getDimension() == 1) ? 0 : (dim - 1);
                sa->viewAttribute(actualDim);
                Q_EMIT ViewCloudPicture(); // 刷新标量图例（与顶层属性行分支一致）
                call = false;
            }
        } else {
            // Top-level attribute item under model
            int index = child->data(0, Qt::UserRole).toInt();
            ModelTreeWidgetItem* parent = dynamic_cast<ModelTreeWidgetItem*>(child->parent());
            if (parent) {
                // 点击模型的属性行：上下文切回该模型的根对象（此前可能选中了子块，
                // 属性面板 / 信息框停在该子块的属性值上）。
                // 顺序：先更新选中状态再发信号 —— 该属性行成为 current，但它本身
                // 不是数据对象，信号处理器按"选中优先、回落根"取到模型根对象。
                child->setSelected(true);
                setCurrentItem(child);
                iGame::SceneManager::Instance()->GetCurrentScene()->SetCurrentModel(parent->getModel());
                emit ChangeCurrentModel(parent->getModel());

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
