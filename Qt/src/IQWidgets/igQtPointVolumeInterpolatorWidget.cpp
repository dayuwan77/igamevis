#include "IQWidgets/igQtPointVolumeInterpolatorWidget.h"

#include "Interpolation/iGamePointVolumeInterpolatorFilter.h"
#include "iGameAttributeSet.h"
#include "iGameModel.h"
#include "iGameScene.h"
#include "iGameSceneManager.h"
#include "iGameStructuredMesh.h"
#include "iGameType.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

#include <algorithm>
#include <string>
#include <vector>

namespace {
iGame::PointKernelType KernelFromIndex(int index) {
    switch (index) {
        case 1: return iGame::PointKernelType::Gaussian;
        case 2: return iGame::PointKernelType::Shepard;
        case 3: return iGame::PointKernelType::Linear;
        default: return iGame::PointKernelType::Voronoi;
    }
}
iGame::PointKernelFootprint FootprintFromIndex(int index) {
    return (index == 1) ? iGame::PointKernelFootprint::NClosest
                        : iGame::PointKernelFootprint::Radius;
}
iGame::PointNullPointsStrategy NullStrategyFromIndex(int index) {
    switch (index) {
        case 0: return iGame::PointNullPointsStrategy::MaskPoints;
        case 2: return iGame::PointNullPointsStrategy::ClosestPoint;
        default: return iGame::PointNullPointsStrategy::NullValue;
    }
}

// 输出格点硬上限提示阈值（与 Apply 中的确认阈值一致）
constexpr long long kGridWarnThreshold = 2000000;
} // namespace

igQtPointVolumeInterpolatorWidget::igQtPointVolumeInterpolatorWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::PointVolumeInterpolatorWidget) {
    ui->setupUi(this);

    setStyleSheet(
        "QWidget { background-color: transparent; color: #EAEAEA; font-size: 13px; }"
        "QLabel { color: #D8D8D8; background: transparent; font-size: 13px; }"
        "QGroupBox { color: #D8D8D8; border: 1px solid #3A3A3A; border-radius: 4px; "
        "margin-top: 8px; padding-top: 8px; font-size: 14px; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 0 4px; }"
        "QLineEdit, QComboBox, QPlainTextEdit { background-color: #2A2A2A; color: #EAEAEA; "
        "border: 1px solid #3A3A3A; padding: 3px 6px; border-radius: 3px; font-size: 13px; }"
        "QComboBox::drop-down { border-left: 1px solid #3A3A3A; }"
        "QComboBox QAbstractItemView { background-color: #2A2A2A; color: #EAEAEA; "
        "selection-background-color: #3A3A3A; font-size: 13px; }"
        "QCheckBox { color: #D8D8D8; background: transparent; font-size: 13px; }"
        "QPushButton { background-color: #2A2A2A; color: #EAEAEA; border: 1px solid #3A3A3A; "
        "padding: 6px 12px; border-radius: 4px; font-size: 13px; }"
        "QPushButton:hover { background-color: #3A3A3A; }"
        "QPushButton:pressed { background-color: #252526; }"
        "QPushButton:disabled { color: #6A6A6A; }"
        "QListWidget { background-color: #2A2A2A; color: #EAEAEA; border: 1px solid #3A3A3A; "
        "border-radius: 3px; font-size: 13px; }"
        "QListWidget::item { color: #EAEAEA; }"
        "QListWidget::item:selected { background-color: #3A3A3A; }"
        "QScrollBar:vertical { background: #252526; width: 10px; margin: 0; }"
        "QScrollBar::handle:vertical { background: #4A4A4A; border-radius: 5px; min-height: 20px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }");

    ui->comboBox_Kernel->addItem(QStringLiteral("Voronoi (最近点)"));
    ui->comboBox_Kernel->addItem(QStringLiteral("Gaussian (高斯)"));
    ui->comboBox_Kernel->addItem(QStringLiteral("Shepard (逆距离)"));
    ui->comboBox_Kernel->addItem(QStringLiteral("Linear (等权平均)"));
    ui->comboBox_Kernel->setCurrentIndex(0);

    ui->comboBox_Footprint->addItem(QStringLiteral("Radius (半径)"));
    ui->comboBox_Footprint->addItem(QStringLiteral("N Closest (最近 N)"));
    ui->comboBox_Footprint->setCurrentIndex(0);

    ui->comboBox_NullStrategy->addItem(QStringLiteral("Mask Points"));
    ui->comboBox_NullStrategy->addItem(QStringLiteral("Null Value"));
    ui->comboBox_NullStrategy->addItem(QStringLiteral("Closest Point"));
    ui->comboBox_NullStrategy->setCurrentIndex(1);

    ui->lineEdit_Radius->setText(QStringLiteral("1.0"));
    ui->lineEdit_NumberOfPoints->setText(QStringLiteral("8"));
    ui->lineEdit_Sharpness->setText(QStringLiteral("2.0"));
    ui->lineEdit_Power->setText(QStringLiteral("2.0"));
    ui->lineEdit_NullValue->setText(QStringLiteral("0"));

    QRegularExpression numberRx(QStringLiteral("-?\\d*\\.?\\d+"));
    for (QLineEdit* e : {ui->lineEdit_Radius, ui->lineEdit_NumberOfPoints,
                         ui->lineEdit_Sharpness, ui->lineEdit_Power,
                         ui->lineEdit_NullValue}) {
        e->setValidator(new QRegularExpressionValidator(numberRx, this));
    }

    ui->lineEdit_ResX->setText(QStringLiteral("64"));
    ui->lineEdit_ResY->setText(QStringLiteral("64"));
    ui->lineEdit_ResZ->setText(QStringLiteral("64"));
    QRegularExpression intRx(QStringLiteral("\\d+"));
    for (QLineEdit* e : {ui->lineEdit_ResX, ui->lineEdit_ResY, ui->lineEdit_ResZ}) {
        e->setValidator(new QRegularExpressionValidator(intRx, this));
    }

    // 采样包围盒输入框（默认跟随输入包围盒，故先禁用）
    for (QLineEdit* e : {ui->lineEdit_XMin, ui->lineEdit_XMax, ui->lineEdit_YMin,
                         ui->lineEdit_YMax, ui->lineEdit_ZMin, ui->lineEdit_ZMax}) {
        e->setValidator(new QRegularExpressionValidator(numberRx, this));
    }
    auto setBoundsEnabled = [this](bool enabled) {
        for (QLineEdit* e : {ui->lineEdit_XMin, ui->lineEdit_XMax, ui->lineEdit_YMin,
                             ui->lineEdit_YMax, ui->lineEdit_ZMin, ui->lineEdit_ZMax}) {
            e->setEnabled(enabled);
        }
        for (QLabel* l : {ui->label_XBounds, ui->label_YBounds, ui->label_ZBounds}) {
            l->setEnabled(enabled);
        }
    };
    setBoundsEnabled(!ui->checkBox_UseInputBounds->isChecked());
    connect(ui->checkBox_UseInputBounds, &QCheckBox::toggled, this, [this, setBoundsEnabled](bool checked) {
        setBoundsEnabled(!checked);
        if (checked) RefreshBoundsFromInput();
    });

    // 参数联动：核 / 邻域 / 空邻域策略决定哪些参数可用
    connect(ui->comboBox_Kernel, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &igQtPointVolumeInterpolatorWidget::UpdateParameterEnabled);
    connect(ui->comboBox_Footprint, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &igQtPointVolumeInterpolatorWidget::UpdateParameterEnabled);
    connect(ui->comboBox_NullStrategy, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &igQtPointVolumeInterpolatorWidget::UpdateParameterEnabled);

    // 分辨率 -> 实时格点数
    for (QLineEdit* e : {ui->lineEdit_ResX, ui->lineEdit_ResY, ui->lineEdit_ResZ}) {
        connect(e, &QLineEdit::textChanged, this,
                [this](const QString&) { UpdateGridCount(); });
    }

    // 数组勾选 -> 已选计数
    connect(ui->listWidget_Arrays, &QListWidget::itemChanged, this,
            &igQtPointVolumeInterpolatorWidget::UpdateArrayCount);
    connect(ui->pushButton_SelectAll, &QPushButton::clicked, this, [this] {
        for (int i = 0; i < ui->listWidget_Arrays->count(); ++i) {
            ui->listWidget_Arrays->item(i)->setCheckState(Qt::Checked);
        }
        UpdateArrayCount();
    });
    connect(ui->pushButton_ClearAll, &QPushButton::clicked, this, [this] {
        for (int i = 0; i < ui->listWidget_Arrays->count(); ++i) {
            ui->listWidget_Arrays->item(i)->setCheckState(Qt::Unchecked);
        }
        UpdateArrayCount();
    });

    connect(ui->pushButton_Apply, &QPushButton::clicked,
            this, &igQtPointVolumeInterpolatorWidget::Apply);

    UpdateParameterEnabled();
    UpdateGridCount();
    UpdateArrayCount();
}

iGame::PointSet::Pointer igQtPointVolumeInterpolatorWidget::ResolvePointSet(
        iGame::DataObject::Pointer obj) {
    if (obj == nullptr) return nullptr;
    return iGame::DynamicCast<iGame::PointSet>(obj);
}

void igQtPointVolumeInterpolatorWidget::RefreshBoundsFromInput() {
    if (m_PointSet == nullptr) return;
    const auto& box = m_PointSet->GetBoundingBox();
    ui->lineEdit_XMin->setText(QString::number(box.min[0]));
    ui->lineEdit_XMax->setText(QString::number(box.max[0]));
    ui->lineEdit_YMin->setText(QString::number(box.min[1]));
    ui->lineEdit_YMax->setText(QString::number(box.max[1]));
    ui->lineEdit_ZMin->setText(QString::number(box.min[2]));
    ui->lineEdit_ZMax->setText(QString::number(box.max[2]));
}

void igQtPointVolumeInterpolatorWidget::UpdateParameterEnabled() {
    const int kernel = ui->comboBox_Kernel->currentIndex();
    const int footprint = ui->comboBox_Footprint->currentIndex();
    const int nullStrategy = ui->comboBox_NullStrategy->currentIndex();

    const bool radiusUsed = (footprint == 0);
    const bool nUsed = (footprint == 1);
    const bool sharpnessUsed = (kernel == 1);
    const bool powerUsed = (kernel == 2);
    const bool nullValueUsed = (nullStrategy == 1);

    ui->lineEdit_Radius->setEnabled(radiusUsed);
    ui->label_Radius->setEnabled(radiusUsed);
    ui->lineEdit_NumberOfPoints->setEnabled(nUsed);
    ui->label_NumberOfPoints->setEnabled(nUsed);
    ui->lineEdit_Sharpness->setEnabled(sharpnessUsed);
    ui->label_Sharpness->setEnabled(sharpnessUsed);
    ui->lineEdit_Power->setEnabled(powerUsed);
    ui->label_Power->setEnabled(powerUsed);
    ui->lineEdit_NullValue->setEnabled(nullValueUsed);
    ui->label_NullValue->setEnabled(nullValueUsed);
}

void igQtPointVolumeInterpolatorWidget::UpdateGridCount() {
    bool ax = false, ay = false, az = false;
    const long long rx = ui->lineEdit_ResX->text().toLongLong(&ax);
    const long long ry = ui->lineEdit_ResY->text().toLongLong(&ay);
    const long long rz = ui->lineEdit_ResZ->text().toLongLong(&az);
    if (!ax || !ay || !az || rx < 1 || ry < 1 || rz < 1) {
        ui->label_GridCount->setText(QStringLiteral("格点数：-"));
        return;
    }
    const long long total = rx * ry * rz;
    ui->label_GridCount->setText(
            QStringLiteral("格点数：%1%2")
                    .arg(total)
                    .arg(total > kGridWarnThreshold
                                 ? QStringLiteral("（超过 2e6，可能较慢/占内存）")
                                 : QString()));
}

void igQtPointVolumeInterpolatorWidget::UpdateArrayCount() {
    const int total = ui->listWidget_Arrays->count();
    int checked = 0;
    for (int i = 0; i < total; ++i) {
        if (ui->listWidget_Arrays->item(i)->checkState() == Qt::Checked) ++checked;
    }
    ui->label_ArrayCount->setText(QStringLiteral("已选 %1/%2").arg(checked).arg(total));
}

void igQtPointVolumeInterpolatorWidget::RefreshModel() {
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    if (scene == nullptr) return;
    auto model = scene->GetCurrentModel();
    auto dataObject = model ? model->GetDataObject() : nullptr;

    m_PointSet = ResolvePointSet(dataObject);

    if (m_PointSet == nullptr) {
        ui->listWidget_Arrays->clear();
        ui->label_ModelName->setText(QStringLiteral("未选择模型（请选择一个点云/网格模型）"));
        ui->plainTextEdit_Result->setPlainText(QStringLiteral("结果："));
        UpdateArrayCount();
        return;
    }

    RefreshBoundsFromInput();

    // 插值数组列表（默认全选）
    ui->listWidget_Arrays->clear();
    if (m_PointSet->GetAttributeSet()) {
        auto all = m_PointSet->GetAttributeSet()->GetAllAttributes();
        for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
            auto& a = all->GetElement(i);
            if (a.isDeleted || !a.pointer) continue;
            if (a.attachmentType != IG_POINT) continue;
            auto* item = new QListWidgetItem(QString::fromStdString(a.pointer->GetName()),
                                             ui->listWidget_Arrays);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Checked);
        }
    }
    UpdateArrayCount();

    const IGsize points = m_PointSet->GetNumberOfPoints();
    ui->label_ModelName->setText(
            QString::fromStdString(m_PointSet->GetName()) +
            QStringLiteral("\n点数：%1").arg(static_cast<qulonglong>(points)));
}

void igQtPointVolumeInterpolatorWidget::Apply() {
    if (m_PointSet == nullptr) {
        QMessageBox::warning(this, QStringLiteral("点体积插值"),
                             QStringLiteral("请先选择一个点云/网格模型。"));
        return;
    }

    bool okRadius = false, okN = false, okSharp = false, okPower = false, okNull = false;
    bool okRx = false, okRy = false, okRz = false;
    const double radius = ui->lineEdit_Radius->text().toDouble(&okRadius);
    const int numberOfPoints = ui->lineEdit_NumberOfPoints->text().toInt(&okN);
    const double sharpness = ui->lineEdit_Sharpness->text().toDouble(&okSharp);
    const double power = ui->lineEdit_Power->text().toDouble(&okPower);
    const double nullValue = ui->lineEdit_NullValue->text().toDouble(&okNull);
    const int resX = ui->lineEdit_ResX->text().toInt(&okRx);
    const int resY = ui->lineEdit_ResY->text().toInt(&okRy);
    const int resZ = ui->lineEdit_ResZ->text().toInt(&okRz);
    const bool useInputBounds = ui->checkBox_UseInputBounds->isChecked();
    bool okXMin = false, okXMax = false, okYMin = false, okYMax = false;
    bool okZMin = false, okZMax = false;
    const double xMin = ui->lineEdit_XMin->text().toDouble(&okXMin);
    const double xMax = ui->lineEdit_XMax->text().toDouble(&okXMax);
    const double yMin = ui->lineEdit_YMin->text().toDouble(&okYMin);
    const double yMax = ui->lineEdit_YMax->text().toDouble(&okYMax);
    const double zMin = ui->lineEdit_ZMin->text().toDouble(&okZMin);
    const double zMax = ui->lineEdit_ZMax->text().toDouble(&okZMax);
    if (!okRx || !okRy || !okRz ||
        (!useInputBounds && (!okXMin || !okXMax || !okYMin || !okYMax || !okZMin || !okZMax))) {
        QMessageBox::warning(this, QStringLiteral("点体积插值"),
                             QStringLiteral("参数格式无效，请输入有效数字。"));
        return;
    }

    const long long gridPoints = static_cast<long long>(resX) * resY * resZ;
    if (gridPoints > kGridWarnThreshold) {
        const auto answer = QMessageBox::question(
                this, QStringLiteral("点体积插值"),
                QStringLiteral("输出格点约 %1 个，可能较慢/占内存，是否继续？").arg(gridPoints));
        if (answer != QMessageBox::Yes) return;
    }

    const int kernelIndex = ui->comboBox_Kernel->currentIndex();
    const int footprintIndex = ui->comboBox_Footprint->currentIndex();
    auto filter = iGame::PointVolumeInterpolatorFilter::New();
    filter->SetInput(m_PointSet);
    filter->SetKernelType(KernelFromIndex(kernelIndex));
    filter->SetKernelFootprint(FootprintFromIndex(footprintIndex));
    filter->SetRadius(okRadius && radius > 0.0 ? radius : 1.0);
    filter->SetNumberOfPoints(okN && numberOfPoints > 0 ? numberOfPoints : 1);
    filter->SetSharpness(okSharp ? sharpness : 2.0);
    filter->SetPowerParameter(okPower ? power : 2.0);
    filter->SetNullPointsStrategy(NullStrategyFromIndex(ui->comboBox_NullStrategy->currentIndex()));
    filter->SetNullValue(okNull ? nullValue : 0.0);
    filter->SetUseInputBounds(useInputBounds);
    if (!useInputBounds) {
        filter->SetSamplingBounds(xMin, xMax, yMin, yMax, zMin, zMax);
    }
    filter->SetResolution(resX, resY, resZ);

    // 勾选要插值的数组（全选 <=> 不限制）
    std::vector<std::string> selectedArrays;
    for (int i = 0; i < ui->listWidget_Arrays->count(); ++i) {
        auto* item = ui->listWidget_Arrays->item(i);
        if (item->checkState() == Qt::Checked) {
            selectedArrays.push_back(item->text().toStdString());
        }
    }
    if (selectedArrays.empty()) {
        QMessageBox::warning(this, QStringLiteral("点体积插值"),
                             QStringLiteral("请至少勾选一个要插值的点属性数组。"));
        return;
    }
    if (selectedArrays.size() != static_cast<size_t>(ui->listWidget_Arrays->count())) {
        filter->SetInterpolateArrayNames(selectedArrays);
    }

    if (!filter->Execute()) {
        QMessageBox::warning(this, QStringLiteral("点体积插值"),
                             QStringLiteral("执行失败：%1").arg(QString::fromStdString(filter->GetMessage())));
        return;
    }

    auto output = iGame::DynamicCast<iGame::StructuredMesh>(filter->GetOutput());
    if (output == nullptr) {
        QMessageBox::warning(this, QStringLiteral("点体积插值"),
                             QStringLiteral("执行无输出。"));
        return;
    }

    if (m_OutputCallback) { m_OutputCallback(output); }

    igIndex* dims = output->GetDimensionSize();
    IGsize hits = 0;
    auto attrSet = output->GetAttributeSet();
    if (attrSet != nullptr) {
        const int maskIndex = attrSet->GetAttributeIndex(
                iGame::PointVolumeInterpolatorFilter::ValidPointsMaskName);
        if (maskIndex >= 0) {
            auto& maskAttr = attrSet->GetAttribute(maskIndex);
            if (maskAttr.pointer) {
                for (IGsize i = 0; i < maskAttr.pointer->GetNumberOfElements(); ++i) {
                    if (maskAttr.pointer->GetElementValue(i, 0) != 0.0) ++hits;
                }
            }
        }
    }
    ui->plainTextEdit_Result->setPlainText(
            QStringLiteral("输出体网格：%1 x %2 x %3\n点数：%4，命中：%5")
                    .arg(static_cast<qulonglong>(dims[0]))
                    .arg(static_cast<qulonglong>(dims[1]))
                    .arg(static_cast<qulonglong>(dims[2]))
                    .arg(static_cast<qulonglong>(output->GetNumberOfPoints()))
                    .arg(static_cast<qulonglong>(hits)));
}
