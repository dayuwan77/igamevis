#include "IQWidgets/igQtResampleToImageWidget.h"

#include "ui_igQtResampleToImage.h"

#include <Convert/iGameResampleToImageFilter.h>
#include <iGameModel.h>
#include <iGamePointSet.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>

#include <QApplication>
#include <QCheckBox>
#include <QDockWidget>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>

#include <string>

namespace {

// 大尺寸提醒阈值：格点数超过 256^3，或点数据预估内存超过 256 MB 时二次确认。
constexpr IGsize kLargeGridPointsWarning = 256ull * 256ull * 256ull;
constexpr double kLargeMemoryWarningMB = 256.0;

} // namespace

igQtResampleToImageWidget::igQtResampleToImageWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::igQtResampleToImage) {
    ui->setupUi(this);
    initUI();
    initConnections();
}

igQtResampleToImageWidget::~igQtResampleToImageWidget() { delete ui; }

QDockWidget* igQtResampleToImageWidget::createDockWidget(QWidget* parent) {
    auto* dockWidget = new QDockWidget(QStringLiteral("重采样到图像"), parent);
    dockWidget->setObjectName(QStringLiteral("dockWidget_ResampleToImage"));
    dockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    dockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    dockWidget->setWidget(new igQtResampleToImageWidget(dockWidget));
    return dockWidget;
}

void igQtResampleToImageWidget::initUI() {
    for (auto* field : {ui->spinBox_DimX, ui->spinBox_DimY, ui->spinBox_DimZ}) {
        field->setMinimumWidth(72);
        field->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    }
    for (auto* field : {ui->lineEdit_XMin, ui->lineEdit_XMax, ui->lineEdit_YMin,
                        ui->lineEdit_YMax, ui->lineEdit_ZMin, ui->lineEdit_ZMax}) {
        field->setMinimumWidth(64);
        field->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    }
    refreshInputState();
}

void igQtResampleToImageWidget::initConnections() {
    connect(ui->pushButton_Run, &QPushButton::clicked, this, &igQtResampleToImageWidget::onRunClicked);
    connect(ui->pushButton_Close, &QPushButton::clicked, this, [this]() { emit closeRequested(); });

    // 不使用输入包围盒时才允许编辑采样范围
    connect(ui->checkBox_UseInputBounds, &QCheckBox::toggled, this, [this](bool useInputBounds) {
        const bool manual = !useInputBounds;
        ui->lineEdit_XMin->setEnabled(manual);
        ui->lineEdit_XMax->setEnabled(manual);
        ui->lineEdit_YMin->setEnabled(manual);
        ui->lineEdit_YMax->setEnabled(manual);
        ui->lineEdit_ZMin->setEnabled(manual);
        ui->lineEdit_ZMax->setEnabled(manual);
    });
    const bool manual = !ui->checkBox_UseInputBounds->isChecked();
    ui->lineEdit_XMin->setEnabled(manual);
    ui->lineEdit_XMax->setEnabled(manual);
    ui->lineEdit_YMin->setEnabled(manual);
    ui->lineEdit_YMax->setEnabled(manual);
    ui->lineEdit_ZMin->setEnabled(manual);
    ui->lineEdit_ZMax->setEnabled(manual);
}

void igQtResampleToImageWidget::setCurrentModel(iGame::Model* model) {
    m_currentModel = model;
    m_input = (model != nullptr) ? model->GetDataObject() : nullptr;

    // 与 filter 内部一致：统一转换到 UnstructuredMesh（只在切换模型时做一次）
    m_meshData = nullptr;
    if (m_input != nullptr) {
        auto volume = iGame::DynamicCast<iGame::VolumeMesh>(m_input);
        if (volume && volume->GetIsPolyhedronType()) {
            m_meshData = volume;
            refreshInputState();
            return;
        }
        auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(m_input);
        if (mesh == nullptr) mesh = iGame::UnstructuredMesh::TransDataObjToUnstructuredMesh(m_input);
        if (mesh != nullptr) m_meshData = mesh;
    }
    refreshInputState();
}

void igQtResampleToImageWidget::refreshInputState() {
    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(m_meshData);
    auto polyhedra = iGame::DynamicCast<iGame::VolumeMesh>(m_meshData);
    ui->pushButton_Run->setEnabled(m_input && (mesh || (polyhedra && polyhedra->GetIsPolyhedronType())));
}

bool igQtResampleToImageWidget::buildFilter(iGame::ResampleToImageFilter::Pointer& filter,
                                            QString& error) {
    if (m_input == nullptr) {
        error = QStringLiteral("请先选择一个模型。");
        return false;
    }

    filter = iGame::ResampleToImageFilter::New();
    filter->SetInput(m_input);
    filter->SetSamplingDimensions(ui->spinBox_DimX->value(), ui->spinBox_DimY->value(),
                                  ui->spinBox_DimZ->value());
    filter->SetUseInputBounds(ui->checkBox_UseInputBounds->isChecked());
    filter->SetFailOnUnsupportedCells(ui->checkBox_FailOnUnsupported->isChecked());
    filter->SetDisableInterpolationForDiscreteArrays(ui->checkBox_DisableDiscreteInterp->isChecked());

    if (!ui->checkBox_UseInputBounds->isChecked()) {
        bool okXMin = true, okXMax = true, okYMin = true, okYMax = true, okZMin = true, okZMax = true;
        const double x0 = ui->lineEdit_XMin->text().toDouble(&okXMin);
        const double x1 = ui->lineEdit_XMax->text().toDouble(&okXMax);
        const double y0 = ui->lineEdit_YMin->text().toDouble(&okYMin);
        const double y1 = ui->lineEdit_YMax->text().toDouble(&okYMax);
        const double z0 = ui->lineEdit_ZMin->text().toDouble(&okZMin);
        const double z1 = ui->lineEdit_ZMax->text().toDouble(&okZMax);
        if (!(okXMin && okXMax && okYMin && okYMax && okZMin && okZMax)) {
            error = QStringLiteral("采样范围必须是合法数字。");
            return false;
        }
        if (!(x1 > x0) || !(y1 > y0) || !(z1 > z0)) {
            error = QStringLiteral("采样范围要求每个轴都满足 max > min。");
            return false;
        }
        filter->SetSamplingBounds(x0, x1, y0, y1, z0, z1);
    }
    return true;
}

void igQtResampleToImageWidget::onRunClicked() {
    iGame::ResampleToImageFilter::Pointer filter;
    QString error;
    if (!buildFilter(filter, error)) {
        QMessageBox::warning(this, QStringLiteral("重采样到图像"), error);
        return;
    }

    // 执行前显示规模并对大尺寸二次确认
    IGsize gridPoints = 0;
    IGsize gridCells = 0;
    double memoryMB = 0.0;
    if (filter->EstimateOutputSize(gridPoints, gridCells, memoryMB) &&
        (gridPoints > kLargeGridPointsWarning || memoryMB > kLargeMemoryWarningMB)) {
        const auto answer = QMessageBox::question(
                this, QStringLiteral("重采样到图像"),
                QStringLiteral("输出规模较大：\n\n%1 格点 / %2 单元\n点数据内存量级约 %3 MB\n\n"
                               "是否继续执行？")
                        .arg(static_cast<qlonglong>(gridPoints))
                        .arg(static_cast<qlonglong>(gridCells))
                        .arg(memoryMB, 0, 'f', 1),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes) return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    const bool ok = filter->Execute();
    QApplication::restoreOverrideCursor();

    const QString message = QString::fromStdString(filter->GetMessage()).trimmed();
    if (!ok) {
        QMessageBox::warning(this, QStringLiteral("重采样到图像"),
                             message.isEmpty() ? QStringLiteral("执行失败。") : message);
        refreshInputState();
        return;
    }

    iGame::DataObject::Pointer result = filter->GetOutput(0);
    if (result == nullptr) {
        QMessageBox::warning(this, QStringLiteral("重采样到图像"), QStringLiteral("执行成功但没有输出。"));
        return;
    }
    result->SetName(m_input->GetName() + std::string("_image"));
    emit resultReady(result);

    refreshInputState();
}
