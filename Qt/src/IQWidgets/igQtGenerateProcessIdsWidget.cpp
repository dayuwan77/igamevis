#include "IQWidgets/igQtGenerateProcessIdsWidget.h"
#include "iGameFilterIncludes.h"

#include <QMessageBox>

igQtGenerateProcessIdsWidget::igQtGenerateProcessIdsWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::GenerateProcessIdsWidget) {
    ui->setupUi(this);
    connect(ui->btnApply, &QPushButton::clicked, this, &igQtGenerateProcessIdsWidget::Apply);
}

void igQtGenerateProcessIdsWidget::SetOriginDataObject(iGame::DataObject::Pointer data) {
    m_OriginDataObject = data;
}

void igQtGenerateProcessIdsWidget::Apply() {
    if (m_OriginDataObject == nullptr) {
        QMessageBox::warning(this, QStringLiteral("生成进程ID"), QStringLiteral("请先选择一个模型。"));
        return;
    }
    auto filter = iGame::GenerateProcessIdsFilter::New();
    filter->SetInput(m_OriginDataObject);
    filter->SetGeneratePointData(ui->checkBox_PointData->isChecked());
    filter->SetGenerateCellData(ui->checkBox_CellData->isChecked());
    if (!filter->Execute()) {
        QMessageBox::warning(this, QStringLiteral("生成进程ID失败"), QString::fromStdString(filter->GetMessage()));
        return;
    }
    Q_EMIT UpdateProcessIdsModel(m_OriginDataObject);
}
