#include <IQWidgets/igQtConnectedSurfacePropertiesWidget.h>

#include <DataProcessing/ConnectedSurfaceProperties/iGameConnectedSurfacePropertiesFilter.h>
#include <IQComponents/Dialog/igQtDarkFramelessMessage.h>
#include <IQComponents/igQtModelDialogWidget.h>
#include <IQWidgets/igQtModelDrawWidget.h>

#include <QCheckBox>
#include <QGroupBox>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTabWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <type_traits>

using namespace iGame;

namespace {

QString NumberText(double value) {
    return std::isfinite(value) ? QString::number(value, 'g', 17) : QStringLiteral("NaN");
}

template<typename TArray>
QString ArrayRangeText(const typename TArray::Pointer& array) {
    using TValue = std::remove_cv_t<std::remove_pointer_t<decltype(array->RawPointer())>>;
    if (array.IsNull() || array->GetNumberOfElements() == 0) return QStringLiteral("—");
    auto valueText = [](TValue value) {
        if constexpr (std::is_integral_v<TValue> && std::is_signed_v<TValue>)
            return QString::number(static_cast<qlonglong>(value));
        if constexpr (std::is_integral_v<TValue> && std::is_unsigned_v<TValue>)
            return QString::number(static_cast<qulonglong>(value));
        return NumberText(static_cast<double>(value));
    };

    QStringList ranges;
    for (int component = 0; component < array->GetDimension(); ++component) {
        bool found = false;
        TValue minimum{};
        TValue maximum{};
        for (IGsize element = 0; element < array->GetNumberOfElements(); ++element) {
            const TValue value = array->RawPointer(element)[component];
            if constexpr (std::is_floating_point_v<TValue>) {
                if (!std::isfinite(static_cast<double>(value))) continue;
            }
            if (!found) {
                minimum = maximum = value;
                found = true;
            } else {
                minimum = std::min(minimum, value);
                maximum = std::max(maximum, value);
            }
        }
        ranges << (found ? QStringLiteral("[%1, %2]").arg(valueText(minimum), valueText(maximum))
                         : QStringLiteral("[NaN, NaN]"));
    }
    return ranges.join(QStringLiteral(" "));
}

} // namespace

igQtConnectedSurfacePropertiesWidget::igQtConnectedSurfacePropertiesWidget(
        igQtModelDrawWidget* rendererWidget, igQtModelDialogWidget* modelTreeWidget,
        Model::Pointer sourceModel, QWidget* parent)
        : QDialog(parent), m_RendererWidget(rendererWidget), m_ModelTreeWidget(modelTreeWidget),
          m_SourceModel(sourceModel) {
    setModal(false);
    setWindowModality(Qt::NonModal);
    setObjectName(QStringLiteral("ConnectedSurfacePropertiesWidget"));
    setWindowTitle(QStringLiteral("计算连通表面属性"));
    setMinimumWidth(640);
    resize(700, 560);

    setSourceModel(sourceModel);
    if (!m_RendererWidget || !m_ModelTreeWidget || m_InputMesh.IsNull()) return;
    buildUi();
}

bool igQtConnectedSurfacePropertiesWidget::setSourceModel(Model::Pointer sourceModel) {
    m_SourceModel = sourceModel;
    m_InputMesh = m_SourceModel
            ? DynamicCast<SurfaceMesh>(m_SourceModel->GetDataObject())
            : SurfaceMesh::Pointer{};
    m_LatestOutput = nullptr;
    if (m_ArrayTable) m_ArrayTable->setRowCount(0);
    if (m_ObjectTable) m_ObjectTable->setRowCount(0);
    if (m_NumberOfObjectsLabel) m_NumberOfObjectsLabel->setText(QStringLiteral("—"));
    if (m_AllValidLabel) {
        m_AllValidLabel->setText(QStringLiteral("—"));
        m_AllValidLabel->setStyleSheet(QString{});
    }
    if (m_TotalAreaLabel) m_TotalAreaLabel->setText(QStringLiteral("—"));
    if (m_TotalVolumeLabel) m_TotalVolumeLabel->setText(QStringLiteral("—"));
    if (m_StatusLabel) m_StatusLabel->setText(QStringLiteral("等待执行；结果将生成独立节点。"));
    return !m_InputMesh.IsNull();
}

void igQtConnectedSurfacePropertiesWidget::buildUi() {
    setStyleSheet(QStringLiteral(R"(
        QDialog { background: #1f2024; color: #ececec; }
        QLabel, QCheckBox { color: #e7e7e7; font-size: 12px; }
        QGroupBox { color: #f0f0f0; font-size: 13px; font-weight: 600;
                    border: 1px solid #565b66; border-radius: 4px; margin-top: 8px; padding-top: 6px; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }
        QLineEdit { min-height: 23px; color: #f4f4f4; background: #292b30;
                    border: 1px solid #686d78; border-radius: 4px; padding: 1px 6px; font-size: 12px; }
        QLineEdit:disabled { color: #8f949e; background: #25262a; }
        QPushButton { min-height: 26px; color: #f4f4f4; background: #343942;
                      border: 1px solid #697181; border-radius: 4px; padding: 1px 9px; font-size: 12px; }
        QPushButton:hover { background: #465365; }
        QPushButton:pressed { background: #2a7bc8; }
        QTableWidget { color: #eeeeee; background: #25272c; alternate-background-color: #2c2f35;
                       border: 1px solid #565b66; gridline-color: #4c5059; font-size: 12px; }
        QHeaderView::section { color: #f2f2f2; background: #373b43; border: 0;
                               border-right: 1px solid #555a64; border-bottom: 1px solid #555a64;
                               padding: 3px; font-size: 12px; font-weight: 600; }
        QTabWidget::pane { border: 1px solid #565b66; border-radius: 4px; }
        QTabBar::tab { color: #d8dce3; background: #2b2e34; border: 1px solid #565b66;
                       padding: 6px 14px; }
        QTabBar::tab:selected { color: white; background: #3a4658; }
    )"));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 7, 10, 9);
    layout->setSpacing(4);

    auto* description = new QLabel(
            QStringLiteral("按共享边识别连通表面，计算逐面面积/体积贡献及逐对象有效性、面积、体积和质心。"), this);
    description->setWordWrap(true);
    description->setStyleSheet(QStringLiteral("color: #d5d9df;"));
    description->setToolTip(
            QStringLiteral("计算方式与 ParaView Connected Surface Properties 一致。只有闭合且每条边恰由"
                           "两个面使用的对象才会标记为有效。"));
    layout->addWidget(description);

    auto* optionsGroup = new QGroupBox(QStringLiteral("参数"), this);
    auto* optionsLayout = new QHBoxLayout(optionsGroup);
    optionsLayout->setContentsMargins(8, 7, 8, 5);
    optionsLayout->setSpacing(6);
    m_SkipIdentificationCheck = new QCheckBox(
            QStringLiteral("跳过识别/有效性检查"), optionsGroup);
    m_SkipIdentificationCheck->setToolTip(
            QStringLiteral("Skip Object Identification：仅在已有同长度的 64 位整数单元数组时使用。"));
    m_ObjectIdsNameEdit = new QLineEdit(QStringLiteral("ObjectIds"), optionsGroup);
    m_ObjectIdsNameEdit->setEnabled(false);
    m_ObjectIdsNameEdit->setToolTip(
            QStringLiteral("数组缺失、类型不符或长度不一致时改用自动识别。"));
    optionsLayout->addWidget(m_SkipIdentificationCheck);
    optionsLayout->addWidget(new QLabel(QStringLiteral("ID 数组"), optionsGroup));
    optionsLayout->addWidget(m_ObjectIdsNameEdit, 1);
    layout->addWidget(optionsGroup);
    connect(m_SkipIdentificationCheck, &QCheckBox::toggled, m_ObjectIdsNameEdit, &QLineEdit::setEnabled);

    auto* summaryGroup = new QGroupBox(QStringLiteral("汇总结果"), this);
    auto* summaryLayout = new QGridLayout(summaryGroup);
    summaryLayout->setContentsMargins(8, 7, 8, 5);
    summaryLayout->setHorizontalSpacing(18);
    summaryLayout->setVerticalSpacing(2);
    m_NumberOfObjectsLabel = new QLabel(QStringLiteral("—"), summaryGroup);
    m_AllValidLabel = new QLabel(QStringLiteral("—"), summaryGroup);
    m_TotalAreaLabel = new QLabel(QStringLiteral("—"), summaryGroup);
    m_TotalVolumeLabel = new QLabel(QStringLiteral("—"), summaryGroup);
    const QString summaryTitles[4]{QStringLiteral("对象数"), QStringLiteral("全部有效"),
                                   QStringLiteral("总面积"), QStringLiteral("有效总体积")};
    QLabel* summaryValues[4]{m_NumberOfObjectsLabel, m_AllValidLabel,
                             m_TotalAreaLabel, m_TotalVolumeLabel};
    for (int column = 0; column < 4; ++column) {
        auto* title = new QLabel(summaryTitles[column], summaryGroup);
        title->setStyleSheet(QStringLiteral("color: #aeb4bd;"));
        summaryValues[column]->setStyleSheet(QStringLiteral("font-size: 13px; font-weight: 600;"));
        summaryLayout->addWidget(title, 0, column);
        summaryLayout->addWidget(summaryValues[column], 1, column);
        summaryLayout->setColumnStretch(column, 1);
    }
    layout->addWidget(summaryGroup);

    auto* resultTabs = new QTabWidget(this);
    auto* arraysPage = new QWidget(resultTabs);
    auto* arraysLayout = new QVBoxLayout(arraysPage);
    arraysLayout->setContentsMargins(4, 4, 4, 4);
    m_ArrayTable = new QTableWidget(0, 5, arraysPage);
    m_ArrayTable->setHorizontalHeaderLabels({QStringLiteral("名称"), QStringLiteral("位置"),
                                             QStringLiteral("类型"), QStringLiteral("元素数"),
                                             QStringLiteral("范围")});
    m_ArrayTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ArrayTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ArrayTable->setAlternatingRowColors(true);
    m_ArrayTable->verticalHeader()->setDefaultSectionSize(23);
    m_ArrayTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_ArrayTable->horizontalHeader()->setStretchLastSection(true);
    arraysLayout->addWidget(m_ArrayTable);
    resultTabs->addTab(arraysPage, QStringLiteral("7 项数组"));

    auto* objectsPage = new QWidget(resultTabs);
    auto* objectsLayout = new QVBoxLayout(objectsPage);
    objectsLayout->setContentsMargins(4, 4, 4, 4);
    m_ObjectTable = new QTableWidget(0, 7, objectsPage);
    m_ObjectTable->setHorizontalHeaderLabels({QStringLiteral("Object ID"), QStringLiteral("有效"),
                                              QStringLiteral("面积"), QStringLiteral("体积"),
                                              QStringLiteral("质心 X"), QStringLiteral("质心 Y"),
                                              QStringLiteral("质心 Z")});
    m_ObjectTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ObjectTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ObjectTable->setAlternatingRowColors(true);
    m_ObjectTable->verticalHeader()->setDefaultSectionSize(23);
    m_ObjectTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_ObjectTable->horizontalHeader()->setStretchLastSection(true);
    objectsLayout->addWidget(m_ObjectTable);
    resultTabs->addTab(objectsPage, QStringLiteral("对象明细"));
    layout->addWidget(resultTabs, 1);

    m_StatusLabel = new QLabel(QStringLiteral("等待执行；结果将生成独立节点。"), this);
    m_StatusLabel->setWordWrap(true);
    m_StatusLabel->setStyleSheet(QStringLiteral("color: #77bfff;"));
    layout->addWidget(m_StatusLabel);
    auto* executeButton = new QPushButton(QStringLiteral("执行并生成输出"), this);
    layout->addWidget(executeButton);
    connect(executeButton, &QPushButton::clicked, this,
            [this]() { executeFilter(); });
}

void igQtConnectedSurfacePropertiesWidget::executeFilter() {
    if (m_InputMesh.IsNull()) {
        showMessage(QStringLiteral("输入模型已不可用，请关闭面板后重新选择表面网格。"));
        return;
    }

    auto filter = ConnectedSurfacePropertiesFilter::New();
    filter->SetInput(m_InputMesh);
    filter->SetSkipObjectIdentification(m_SkipIdentificationCheck->isChecked());
    filter->SetObjectIdsArrayName(m_ObjectIdsNameEdit->text().trimmed().toStdString());
    if (!filter->Execute()) {
        showMessage(QString::fromStdString(filter->GetLastError()));
        return;
    }
    m_LatestOutput = DynamicCast<SurfaceMesh>(filter->GetOutput());
    if (m_LatestOutput.IsNull()) {
        showMessage(QStringLiteral("未生成有效输出。"));
        return;
    }

    m_ModelTreeWidget->addDataObjectToModelTree(m_LatestOutput, Algorithm);
    if (m_SourceModel) m_SourceModel->SetVisibility(false);
    m_RendererWidget->update();

    m_NumberOfObjectsLabel->setText(QString::number(filter->GetNumberOfObjects()));
    m_AllValidLabel->setText(filter->GetAllValid() ? QStringLiteral("是") : QStringLiteral("否"));
    m_AllValidLabel->setStyleSheet(filter->GetAllValid()
            ? QStringLiteral("color: #78d994; font-size: 13px; font-weight: 600;")
            : QStringLiteral("color: #ffb36b; font-size: 13px; font-weight: 600;"));
    m_TotalAreaLabel->setText(NumberText(filter->GetTotalArea()));
    m_TotalVolumeLabel->setText(NumberText(filter->GetTotalVolume()));

    const auto objectIds = filter->GetObjectIds();
    const auto faceAreas = filter->GetAreas();
    const auto faceVolumes = filter->GetVolumes();
    const auto validity = filter->GetObjectValidity();
    const auto areas = filter->GetObjectAreas();
    const auto volumes = filter->GetObjectVolumes();
    const auto centroids = filter->GetObjectCentroids();
    m_ArrayTable->setRowCount(0);
    auto addArrayRow = [this](const QString& name, const QString& location, const QString& type,
                              IGsize elements, const QString& range) {
        const int row = m_ArrayTable->rowCount();
        m_ArrayTable->insertRow(row);
        m_ArrayTable->setItem(row, 0, new QTableWidgetItem(name));
        m_ArrayTable->setItem(row, 1, new QTableWidgetItem(location));
        m_ArrayTable->setItem(row, 2, new QTableWidgetItem(type));
        m_ArrayTable->setItem(row, 3, new QTableWidgetItem(QString::number(elements)));
        m_ArrayTable->setItem(row, 4, new QTableWidgetItem(range));
    };
    addArrayRow(QStringLiteral("ObjectIds"), QStringLiteral("单元数据"), QStringLiteral("long long"),
                objectIds->GetNumberOfElements(), ArrayRangeText<LongLongArray>(objectIds));
    addArrayRow(QStringLiteral("Areas"), QStringLiteral("单元数据"), QStringLiteral("double"),
                faceAreas->GetNumberOfElements(), ArrayRangeText<DoubleArray>(faceAreas));
    addArrayRow(QStringLiteral("Volumes"), QStringLiteral("单元数据"), QStringLiteral("double"),
                faceVolumes->GetNumberOfElements(), ArrayRangeText<DoubleArray>(faceVolumes));
    addArrayRow(QStringLiteral("ObjectValidity"), QStringLiteral("字段数据"),
                QStringLiteral("int（对应 uchar）"), validity->GetNumberOfElements(),
                ArrayRangeText<IntArray>(validity));
    addArrayRow(QStringLiteral("ObjectAreas"), QStringLiteral("字段数据"), QStringLiteral("double"),
                areas->GetNumberOfElements(), ArrayRangeText<DoubleArray>(areas));
    addArrayRow(QStringLiteral("ObjectVolumes"), QStringLiteral("字段数据"), QStringLiteral("double"),
                volumes->GetNumberOfElements(), ArrayRangeText<DoubleArray>(volumes));
    addArrayRow(QStringLiteral("ObjectCentroids"), QStringLiteral("字段数据"), QStringLiteral("double[3]"),
                centroids->GetNumberOfElements(), ArrayRangeText<DoubleArray>(centroids));

    m_ObjectTable->setRowCount(filter->GetNumberOfObjects());
    for (int objectId = 0; objectId < filter->GetNumberOfObjects(); ++objectId) {
        const bool valid = validity->RawPointer()[objectId] != 0;
        m_ObjectTable->setItem(objectId, 0, new QTableWidgetItem(QString::number(objectId)));
        m_ObjectTable->setItem(objectId, 1, new QTableWidgetItem(valid ? QStringLiteral("是") : QStringLiteral("否")));
        m_ObjectTable->setItem(objectId, 2, new QTableWidgetItem(NumberText(areas->RawPointer()[objectId])));
        m_ObjectTable->setItem(objectId, 3, new QTableWidgetItem(NumberText(volumes->RawPointer()[objectId])));
        for (int component = 0; component < 3; ++component) {
            m_ObjectTable->setItem(objectId, 4 + component,
                                   new QTableWidgetItem(NumberText(centroids->RawPointer(objectId)[component])));
        }
    }

    QString modeText = QStringLiteral("自动识别");
    if (m_SkipIdentificationCheck->isChecked()) {
        modeText = filter->GetUsedSuppliedObjectIds()
                ? QStringLiteral("使用现有 64 位 ObjectIds")
                : QStringLiteral("指定数组不可用，已改用自动识别");
    }
    QString validityHint;
    if (!filter->GetAllValid()) {
        validityHint = QStringLiteral(" | 开放/非流形：体积和质心可能无实体意义");
    }
    m_StatusLabel->setText(
            QStringLiteral("完成：%1 个对象 | %2 | 已生成独立输出%3")
                    .arg(filter->GetNumberOfObjects()).arg(modeText, validityHint));
}

void igQtConnectedSurfacePropertiesWidget::showMessage(const QString& text, bool information) {
    igQtShowDarkFramelessMessage(this, QStringLiteral("计算连通表面属性"), text, information);
}
