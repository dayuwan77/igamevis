#include "IQWidgets/igQtCountCellVerticesWidget.h"

#include "iGameAttributeSet.h"
#include "iGameFlatArray.h"

#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QTextStream>

igQtCountCellVerticesWidget::igQtCountCellVerticesWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::CountCellVertices) {
    ui->setupUi(this);

    m_Filter = iGame::CountCellVerticesFilter::New();

    // 按钮 → 槽
    connect(ui->btnExecute, &QPushButton::clicked, this, &igQtCountCellVerticesWidget::ExecuteCount);
    connect(ui->btnExportCSV, &QPushButton::clicked, this, &igQtCountCellVerticesWidget::ExportCSV);

    // 表格初始化：两列表头 + 可排序 + 拉伸
    QStringList headers;
    headers << QStringLiteral("单元编号") << QStringLiteral("顶点数");
    ui->tableWidget->setHorizontalHeaderLabels(headers);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->verticalHeader()->setVisible(false);   // 隐藏行号列，更紧凑
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);  // 只读表格

    // 深色主题适配：程序全局是深色样式，表格必须显式配深底浅字，
    // 否则交替行会出现"白底白字"看不清（QTableWidget 默认底色是白的）。
    ui->tableWidget->setStyleSheet(R"(
        QTableWidget {
            background-color: #2b2b2b;
            alternate-background-color: #3a3a3a;
            color: #e8e8e8;
            gridline-color: #4a4a4a;
            selection-background-color: #4a6fa5;
            selection-color: #ffffff;
        }
        QHeaderView::section {
            background-color: #3a3a3a;
            color: #e8e8e8;
            border: none;
            padding: 4px;
        }
        QTableCornerButton::section { background-color: #3a3a3a; }
    )");
    // 深色交替行（#2b2b2b / #3a3a3a）都配浅色文字，保证每行都可读
    ui->tableWidget->setAlternatingRowColors(true);
}

void igQtCountCellVerticesWidget::SetOriginDataObject(iGame::DataObject::Pointer obj) {
    this->m_OriginDataObject = obj;
    m_Counts = nullptr;  // 换了模型，旧统计结果作废
}

// ------------------------------------------------------------------
// 按名字从属性集里找 cell_vertex_count 数组
// ------------------------------------------------------------------
iGame::ArrayObject::Pointer
igQtCountCellVerticesWidget::FindCountArray(iGame::DataObject::Pointer obj) {
    if (!obj) { return nullptr; }
    auto attrs = obj->GetAttributeSet();
    if (!attrs) { return nullptr; }

    // 遍历属性集，按名字匹配（AttributeSet 属性包含：指针、类型、挂载点、名字）
    auto all = attrs->GetAllAttributes();
    for (int i = 0; i < static_cast<int>(all->GetNumberOfElements()); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || !attr.pointer) { continue; }
        if (std::string(attr.pointer->GetName()) == "cell_vertex_count") {
            return attr.pointer;
        }
    }
    return nullptr;
}

// ------------------------------------------------------------------
// 「执行」：跑 Filter → 填表格
// ------------------------------------------------------------------
void igQtCountCellVerticesWidget::ExecuteCount() {
    if (!m_OriginDataObject) {
        QMessageBox::warning(this, "提示", "请先选中一个模型。");
        return;
    }

    // 标准 Filter 调用：New() → SetInput → Execute
    // 简单任务不生成新网格，cell_vertex_count 属性直接挂在输入模型上
    m_Filter->SetInput(m_OriginDataObject);
    if (!m_Filter->Execute()) {
        QMessageBox::critical(this, "执行失败", "CountCellVerticesFilter 执行失败，请查看日志。");
        return;
    }

    // 从输出（= 输入模型）取回属性数组
    m_Counts = FindCountArray(m_OriginDataObject);
    if (!m_Counts) {
        QMessageBox::critical(this, "执行失败", "未找到 cell_vertex_count 属性数组。");
        return;
    }

    FillTable(m_Counts);
}

// ------------------------------------------------------------------
// 填充表格 + 更新摘要
// ------------------------------------------------------------------
void igQtCountCellVerticesWidget::FillTable(iGame::ArrayObject::Pointer counts) {
    IGsize n = counts->GetNumberOfValues();  // 数组长度 = 单元数
    ui->tableWidget->setRowCount(static_cast<int>(n));

    // 统计顶点数范围（min~max）用于摘要
    IGsize minV = (n > 0) ? static_cast<IGsize>(counts->GetValue(0)) : 0;
    IGsize maxV = minV;
    for (IGsize i = 0; i < n; ++i) {
        IGsize v = static_cast<IGsize>(counts->GetValue(i));
        if (v < minV) { minV = v; }
        if (v > maxV) { maxV = v; }

        // 单元格：单元编号 | 顶点数（注意 setItem 接管内存，每次 new 一个）
        auto* idItem = new QTableWidgetItem(QString::number(static_cast<long long>(i)));
        auto* cntItem = new QTableWidgetItem(QString::number(static_cast<long long>(v)));
        idItem->setTextAlignment(Qt::AlignCenter);
        cntItem->setTextAlignment(Qt::AlignCenter);
        ui->tableWidget->setItem(static_cast<int>(i), 0, idItem);
        ui->tableWidget->setItem(static_cast<int>(i), 1, cntItem);
    }

    // 摘要：共 N 个单元，顶点数范围 min ~ max
    ui->lblSummary->setText(QStringLiteral("共 %1 个单元，顶点数范围 %2 ~ %3")
                                .arg(static_cast<long long>(n))
                                .arg(static_cast<long long>(minV))
                                .arg(static_cast<long long>(maxV)));
}

// ------------------------------------------------------------------
// 「导出CSV」：把表格存成 .csv 文件（答辩可展示数据）
// ------------------------------------------------------------------
void igQtCountCellVerticesWidget::ExportCSV() {
    if (!m_Counts || m_Counts->GetNumberOfValues() == 0) {
        QMessageBox::warning(this, "提示", "请先点击「执行」生成统计数据。");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, "导出统计结果为 CSV", "cell_vertex_count.csv", "CSV 文件 (*.csv)");
    if (filePath.isEmpty()) { return; }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法写入文件: " + filePath);
        return;
    }

    QTextStream out(&file);
    out << "cell_id,vertex_count\n";   // 表头
    IGsize n = m_Counts->GetNumberOfValues();
    for (IGsize i = 0; i < n; ++i) {
        out << i << ',' << static_cast<long long>(m_Counts->GetValue(i)) << '\n';
    }
    file.close();

    QMessageBox::information(this, "导出成功", "已导出到:\n" + filePath);
}
