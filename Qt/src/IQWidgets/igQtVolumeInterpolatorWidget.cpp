#include "IQWidgets/igQtVolumeInterpolatorWidget.h"

#include "Interpolation/iGamePointVolumeInterpolatorFilter.h"
#include "iGameModel.h"
#include "iGameScene.h"
#include "iGameSceneManager.h"
#include "iGameType.h"
#include "iGameUnstructuredMesh.h"

#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

igQtVolumeInterpolatorWidget::igQtVolumeInterpolatorWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::VolumeInterpolatorWidget) {
    ui->setupUi(this);

    // 与 igQtChromeFramelessDialog 深色底相适配，避免主窗口全局样式把文字/按钮设为纯白导致看不清
    setStyleSheet(
        "QWidget { background-color: transparent; color: #EAEAEA; }"
        "QLabel { color: #D8D8D8; background: transparent; }"
        "QGroupBox { color: #D8D8D8; border: 1px solid #3A3A3A; border-radius: 4px; margin-top: 8px; padding-top: 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 0 4px; }"
        "QLineEdit, QComboBox { background-color: #2A2A2A; color: #EAEAEA; border: 1px solid #3A3A3A; padding: 3px 6px; border-radius: 3px; }"
        "QComboBox::drop-down { border-left: 1px solid #3A3A3A; }"
        "QComboBox QAbstractItemView { background-color: #2A2A2A; color: #EAEAEA; selection-background-color: #3A3A3A; }"
        "QPushButton { background-color: #2A2A2A; color: #EAEAEA; border: 1px solid #3A3A3A; padding: 6px 12px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #3A3A3A; }"
        "QPushButton:pressed { background-color: #252526; }");

    QRegularExpression rx("-?\\d*\\.?\\d+");
    ui->lineEdit_X->setValidator(new QRegularExpressionValidator(rx, this));
    ui->lineEdit_Y->setValidator(new QRegularExpressionValidator(rx, this));
    ui->lineEdit_Z->setValidator(new QRegularExpressionValidator(rx, this));
    ui->lineEdit_X->setText("0");
    ui->lineEdit_Y->setText("0");
    ui->lineEdit_Z->setText("0");

    connect(ui->pushButton_Probe, &QPushButton::clicked, this, &igQtVolumeInterpolatorWidget::Probe);
}

void igQtVolumeInterpolatorWidget::RefreshModel() {
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    if (scene == nullptr) return;
    auto model = scene->GetCurrentModel();
    m_DataObject = model ? model->GetDataObject() : nullptr;

    ui->comboBox_Attribute->blockSignals(true);
    ui->comboBox_Attribute->clear();
    m_AttributeIndices.clear();
    m_VolumeMesh = nullptr;

    if (m_DataObject == nullptr) {
        ui->label_ModelName->setText(QStringLiteral("未选择模型"));
        ui->label_Result->setText(QStringLiteral("结果："));
        ui->comboBox_Attribute->blockSignals(false);
        return;
    }

    m_VolumeMesh = ResolveVolumeMesh(m_DataObject);
    if (m_VolumeMesh == nullptr) {
        ui->label_ModelName->setText(
            QString::fromStdString(m_DataObject->GetName()) + QStringLiteral("\n（不是体网格，不支持体采样）"));
        ui->comboBox_Attribute->blockSignals(false);
        return;
    }
    ui->label_ModelName->setText(QString::fromStdString(m_DataObject->GetName()));

    // 只列出点属性：插值按点索引取值，单元属性可能越界导致崩溃
    auto attrSet = m_VolumeMesh->GetAttributeSet();
    if (attrSet != nullptr) {
        const size_t numAttrs = attrSet->GetNumberOfAttributes();
        for (size_t i = 0; i < numAttrs; ++i) {
            auto& attr = attrSet->GetAttribute(i);
            if (attr.isDeleted || !attr.pointer) continue;
            if (attr.attachmentType != IG_POINT) continue;
            ui->comboBox_Attribute->addItem(QString::fromStdString(attr.pointer->GetName()));
            m_AttributeIndices.push_back(static_cast<int>(i));
        }
    }
    ui->comboBox_Attribute->blockSignals(false);
}

void igQtVolumeInterpolatorWidget::Probe() {
    if (m_VolumeMesh == nullptr) {
        QMessageBox::warning(this, QStringLiteral("体采样"), QStringLiteral("请先选择体网格模型。"));
        return;
    }

    bool okX = false, okY = false, okZ = false;
    float x = ui->lineEdit_X->text().toFloat(&okX);
    float y = ui->lineEdit_Y->text().toFloat(&okY);
    float z = ui->lineEdit_Z->text().toFloat(&okZ);
    if (!okX || !okY || !okZ) {
        QMessageBox::warning(this, QStringLiteral("体采样"), QStringLiteral("采样点坐标格式无效，请输入有效数字。"));
        return;
    }

    const int comboIndex = ui->comboBox_Attribute->currentIndex();
    if (comboIndex < 0 || static_cast<size_t>(comboIndex) >= m_AttributeIndices.size()) {
        QMessageBox::warning(this, QStringLiteral("体采样"), QStringLiteral("请选择要插值的点属性。"));
        return;
    }

    // 插值仅支持四面体/六面体，其他单元类型（棱柱、金字塔、多面体等）直接弹窗报警
    if (!CheckCellTypesSupported(m_VolumeMesh.get())) {
        QMessageBox::warning(this, QStringLiteral("体采样"),
                             QStringLiteral("当前体网格包含非四面体/六面体单元（如棱柱、金字塔、多面体等）。\n"
                                            "体采样插值仅支持四面体（Tetra）和六面体（Hexahedron）单元，"
                                            "请先将体网格转换为四面体或六面体后再进行采样。"));
        return;
    }

    // 构造单点查询点集
    auto querySet = iGame::PointSet::New();
    querySet->AddPoint(iGame::Point(x, y, z));

    auto filter = iGame::PointVolumeInterpolatorFilter::New();
    filter->SetInput(0, m_VolumeMesh);
    filter->SetInput(1, querySet);
    filter->SetAttributeByIndex(m_AttributeIndices[comboIndex]);
    if (!filter->Execute()) {
        QMessageBox::warning(this, QStringLiteral("体采样"), QStringLiteral("体采样执行失败，请检查体网格与属性数据。"));
        return;
    }

    auto output = filter->GetOutput(0);
    if (output == nullptr) {
        QMessageBox::warning(this, QStringLiteral("体采样"), QStringLiteral("体采样无输出结果。"));
        return;
    }
    auto attrSet = output->GetAttributeSet();
    if (attrSet == nullptr) return;

    const int hitIndex = attrSet->GetAttributeIndex(iGame::PointVolumeInterpolatorFilter::HitMaskName);
    if (hitIndex < 0) return;
    const int valueIndex = (hitIndex == 0) ? 1 : 0;
    const size_t numAttrs = attrSet->GetNumberOfAttributes();
    if (static_cast<size_t>(hitIndex) >= numAttrs || static_cast<size_t>(valueIndex) >= numAttrs) return;

    auto& valueAttr = attrSet->GetAttribute(valueIndex);
    auto& hitAttr = attrSet->GetAttribute(hitIndex);
    if (!valueAttr.pointer || !hitAttr.pointer) return;

    const int dim = valueAttr.pointer->GetDimension();
    const int hit = static_cast<int>(hitAttr.pointer->GetElementValue(0, 0));

    QString text = QStringLiteral("采样点：(%1, %2, %3)\n").arg(x).arg(y).arg(z);
    if (hit == 1) {
        QStringList vals;
        for (int d = 0; d < dim; ++d) {
            vals << QString::number(valueAttr.pointer->GetElementValue(0, d));
        }
        text += QStringLiteral("命中：是\n")
                + QStringLiteral("属性：%1\n").arg(QString::fromStdString(valueAttr.pointer->GetName()))
                + QStringLiteral("插值结果：(%1)").arg(vals.join(QStringLiteral(", ")));
    } else {
        text += QStringLiteral("命中：否（采样点在体网格外，未插值）");
    }
    ui->label_Result->setText(text);
}

iGame::VolumeMesh::Pointer igQtVolumeInterpolatorWidget::ResolveVolumeMesh(iGame::DataObject::Pointer obj) {
    if (obj == nullptr) return nullptr;
    auto volumeMesh = iGame::DynamicCast<iGame::VolumeMesh>(obj);
    if (volumeMesh) return volumeMesh;
    auto unstructured = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);
    if (unstructured) return unstructured->TransferToVolumeMesh();
    return nullptr;
}

bool igQtVolumeInterpolatorWidget::CheckCellTypesSupported(iGame::VolumeMesh* mesh) {
    if (mesh == nullptr) return false;
    const IGsize n = mesh->GetNumberOfVolumes();
    for (IGsize i = 0; i < n; ++i) {
        auto vol = mesh->GetVolume(i);
        if (vol == nullptr) continue;
        const IGenum type = vol->GetCellType();
        if (type != iGame::IG_TETRA && type != iGame::IG_HEXAHEDRON) return false;
    }
    return true;
}
