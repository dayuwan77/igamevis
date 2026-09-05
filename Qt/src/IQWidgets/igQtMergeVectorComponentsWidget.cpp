#include "IQWidgets/igQtMergeVectorComponentsWidget.h"

#include <QComboBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QString>
#include <string>
#include <vector>

#include "ui_MergeVectorComponents.h"

igQtMergeVectorComponentsWidget::igQtMergeVectorComponentsWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::MergeVectorComponents) {
    ui->setupUi(this);
    // 黑底白字主题
    this->setStyleSheet(QStringLiteral(
        "QWidget { background-color: #000000; color: #FFFFFF; }"
        "QComboBox, QLineEdit { background-color: #1a1a1a; color: #FFFFFF; border: 1px solid #3a3a3a; padding: 2px; }"
        "QComboBox QAbstractItemView { background-color: #1a1a1a; color: #FFFFFF; selection-background-color: #3a3a3a; }"
        "QPushButton { background-color: #2a2a2a; color: #FFFFFF; border: 1px solid #3a3a3a; padding: 4px 8px; }"
        "QPushButton:hover { background-color: #3a3a3a; }"
        "QLabel { background-color: transparent; color: #FFFFFF; }"
    ));
    ui->comboBox_DataType->addItem(QStringLiteral("点数据"));
    ui->comboBox_DataType->addItem(QStringLiteral("面数据"));
    ui->lineEdit_OutputName->setText(QStringLiteral("vector"));
    connect(ui->comboBox_DataType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &igQtMergeVectorComponentsWidget::OnDataTypeChanged);
    connect(ui->btnExecute, &QPushButton::clicked, this, &igQtMergeVectorComponentsWidget::OnExecute);
}

igQtMergeVectorComponentsWidget::~igQtMergeVectorComponentsWidget() {
    delete ui;
}

void igQtMergeVectorComponentsWidget::SetOriginDataObject(iGame::DataObject::Pointer data) {
    m_OriginDataObject = data;
    PopulateComponents();
}

void igQtMergeVectorComponentsWidget::OnDataTypeChanged() {
    PopulateComponents();
}

void igQtMergeVectorComponentsWidget::PopulateComponents() {
    for (auto* cb : {ui->comboBox_X, ui->comboBox_Y, ui->comboBox_Z}) {
        cb->clear();
    }
    if (!m_OriginDataObject) return;
    auto attrs = m_OriginDataObject->GetAttributeSet();
    if (!attrs) return;
    const bool isPoint = ui->comboBox_DataType->currentIndex() == 0;
    auto buf = isPoint ? attrs->GetAllPointAttributes() : attrs->GetAllCellAttributes();
    if (!buf) return;
    for (int i = 0; i < buf->GetNumberOfElements(); ++i) {
        auto& a = buf->GetElement(i);
        if (a.type != IG_SCALAR || !a.pointer || a.pointer->GetDimension() != 1) continue;
        const QString name = QString::fromStdString(a.pointer->GetName());
        ui->comboBox_X->addItem(name);
        ui->comboBox_Y->addItem(name);
        ui->comboBox_Z->addItem(name);
    }
}

void igQtMergeVectorComponentsWidget::OnExecute() {
    if (!m_OriginDataObject) {
        QMessageBox::warning(this, QStringLiteral("合并标量数组为向量"),
                             QStringLiteral("请先加载模型。"));
        return;
    }
    if (ui->comboBox_X->count() == 0) {
        QMessageBox::warning(this, QStringLiteral("合并标量数组为向量"),
                             QStringLiteral("请加载模型/当前数据类型下没有可用的标量数组。"));
        return;
    }
    std::vector<std::string> names = {
        ui->comboBox_X->currentText().toStdString(),
        ui->comboBox_Y->currentText().toStdString(),
        ui->comboBox_Z->currentText().toStdString(),
    };
    const bool isPoint = ui->comboBox_DataType->currentIndex() == 0;
    const QString out = ui->lineEdit_OutputName->text().trimmed();

    auto filter = iGame::MergeVectorComponentsFilter::New();
    filter->SetInput(m_OriginDataObject);
    filter->SetComponentArrayNames(names);
    filter->SetAttachmentType(isPoint ? IG_POINT : IG_CELL);
    filter->SetOutputVectorName(out.isEmpty() ? std::string("vector") : out.toStdString());
    if (!filter->Execute()) {
        QMessageBox::warning(this, QStringLiteral("合并标量数组为向量"),
                             QString::fromStdString(filter->GetMessage()));
        return;
    }
    QMessageBox::information(this, QStringLiteral("合并标量数组为向量"), QStringLiteral("Algorithm execution completed."));
}
