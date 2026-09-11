#include <IQWidgets/igQtValidateCellsResultDialog.h>

#include <IQWidgets/igQtModelDrawWidget.h>

#include <iGameModel.h>
#include <iGameSelection.h>

#include <QAbstractItemView>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

using namespace iGame;

namespace {

QString FlagText(unsigned short flag) {
    return QStringLiteral("0x%1").arg(static_cast<int>(flag), 2, 16, QLatin1Char('0'));
}

}  // namespace

igQtValidateCellsResultDialog::igQtValidateCellsResultDialog(
        ValidateCellsFilter::Pointer filter,
        Model* model,
        igQtModelDrawWidget* rendererWidget,
        QWidget* parent)
    : QDialog(parent),
      m_Filter(std::move(filter)),
      m_Model(model),
      m_RendererWidget(rendererWidget) {
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(false);
    setWindowModality(Qt::NonModal);
    setWindowTitle(QStringLiteral("单元几何校验结果"));
    setMinimumWidth(720);
    resize(760, 680);

    buildUi();
    highlightAllInvalid();
}

void igQtValidateCellsResultDialog::buildUi() {
    setStyleSheet(QStringLiteral(R"(
        QDialog { background-color: #23262b; }
        QLabel { color: #e3e7ee; font-size: 13px; }
        QGroupBox { color: #d8dde5; border: 1px solid #4b5059; border-radius: 5px;
                    margin-top: 12px; padding-top: 8px; font-size: 13px; font-weight: 600; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }
        QPushButton { min-height: 28px; color: #f4f4f4; background: #343942;
                      border: 1px solid #697181; border-radius: 4px; padding: 2px 12px; font-size: 13px; }
        QPushButton:hover { background: #465365; }
        QPushButton:pressed { background: #2a7bc8; }
        QTableWidget { color: #eeeeee; background: #25272c; alternate-background-color: #2c2f35;
                       border: 1px solid #565b66; gridline-color: #4c5059; font-size: 13px; }
        QTableWidget::item:selected { background: #2a78be; color: white; }
        QHeaderView::section { color: #f2f2f2; background: #373b43; border: 0;
                               border-right: 1px solid #555a64; border-bottom: 1px solid #555a64;
                               padding: 5px; font-size: 13px; font-weight: 600; }
    )"));

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(14, 12, 14, 12);
    mainLayout->setSpacing(8);

    auto* title = new QLabel(QStringLiteral("运行结果汇总"), this);
    title->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: 700; color: #ffffff;"));
    mainLayout->addWidget(title);

    auto* summaryGroup = new QGroupBox(QStringLiteral("数量统计"), this);
    auto* summaryLayout = new QHBoxLayout(summaryGroup);
    summaryLayout->setContentsMargins(10, 10, 10, 10);
    summaryLayout->setSpacing(10);

    auto makeSummaryItem = [&](const QString& caption, QLabel*& valueLabel) {
        auto* card = new QFrame(summaryGroup);
        card->setObjectName(QStringLiteral("summaryCard"));
        card->setStyleSheet(QStringLiteral(
                "QFrame#summaryCard { background: #2c2f35; border: 1px solid #4b5059;"
                " border-radius: 6px; }"));

        auto* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(10, 8, 10, 8);
        cardLayout->setSpacing(3);

        auto* captionLabel = new QLabel(caption, card);
        captionLabel->setAlignment(Qt::AlignCenter);
        captionLabel->setStyleSheet(QStringLiteral("color: #b9c0cb; font-size: 12px;"));

        valueLabel = new QLabel(QStringLiteral("0"), card);
        valueLabel->setAlignment(Qt::AlignCenter);
        valueLabel->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 18px; font-weight: 700;"));

        cardLayout->addWidget(captionLabel);
        cardLayout->addWidget(valueLabel);
        summaryLayout->addWidget(card, 1);
    };

    makeSummaryItem(QStringLiteral("已检查单元数"), m_CheckedValue);
    makeSummaryItem(QStringLiteral("有效单元数"), m_ValidValue);
    makeSummaryItem(QStringLiteral("无效单元数"), m_InvalidValue);
    makeSummaryItem(QStringLiteral("未支持单元数"), m_UnsupportedValue);
    mainLayout->addWidget(summaryGroup);

    if (m_Filter && m_Filter->GetUnsupportedCellCount() > 0) {
        auto* unsupportedHint = new QLabel(
                QStringLiteral("检测到 %1 个未支持的单元类型，已单独列出，不参与几何无效判定。")
                        .arg(m_Filter->GetUnsupportedCellCount()),
                this);
        unsupportedHint->setWordWrap(true);
        unsupportedHint->setStyleSheet(QStringLiteral("color: #f0b45c; font-size: 13px;"));
        mainLayout->addWidget(unsupportedHint);
    }

    if (m_Filter) {
        m_CheckedValue->setText(QString::number(m_Filter->GetCheckedCellCount()));
        m_ValidValue->setText(QString::number(m_Filter->GetValidCellCount()));
        m_InvalidValue->setText(QString::number(m_Filter->GetInvalidCellCount()));
        m_UnsupportedValue->setText(QString::number(m_Filter->GetUnsupportedCellCount()));
    }

    auto* categoryGroup = new QGroupBox(QStringLiteral("错误类别（点击类别只高亮对应单元）"), this);
    auto* categoryLayout = new QVBoxLayout(categoryGroup);
    m_CategoryTable = new QTableWidget(0, 3, categoryGroup);
    m_CategoryTable->setHorizontalHeaderLabels({QStringLiteral("类别"), QStringLiteral("数量"),
                                                QStringLiteral("状态值")});
    m_CategoryTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_CategoryTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_CategoryTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_CategoryTable->verticalHeader()->setVisible(false);
    m_CategoryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_CategoryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_CategoryTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_CategoryTable->setAlternatingRowColors(true);
    categoryLayout->addWidget(m_CategoryTable);
    mainLayout->addWidget(categoryGroup, 1);

    if (m_Filter) {
        const unsigned short flags[] = {
                Validity_WrongNumberOfPoints,
                Validity_IntersectingEdges,
                Validity_IntersectingFaces,
                Validity_NoncontiguousEdges,
                Validity_Nonconvex,
                Validity_FacesAreOrientedIncorrectly,
                Validity_UnsupportedCellType};

        auto addCategoryRow = [&](unsigned short flag, const QString& name, int count,
                                  const QString& stateText) {
            const int row = m_CategoryTable->rowCount();
            m_CategoryTable->insertRow(row);

            auto* nameItem = new QTableWidgetItem(name);
            nameItem->setData(Qt::UserRole, static_cast<int>(flag));
            auto* countItem = new QTableWidgetItem(QString::number(count));
            countItem->setTextAlignment(Qt::AlignCenter);
            auto* stateItem = new QTableWidgetItem(stateText);
            stateItem->setTextAlignment(Qt::AlignCenter);

            m_CategoryTable->setItem(row, 0, nameItem);
            m_CategoryTable->setItem(row, 1, countItem);
            m_CategoryTable->setItem(row, 2, stateItem);
        };

        const int totalInvalid = m_Filter->GetInvalidCellCount() + m_Filter->GetUnsupportedCellCount();
        addCategoryRow(0, QStringLiteral("全部非有效单元（含未支持）"), totalInvalid, QStringLiteral("—"));
        for (unsigned short flag : flags) {
            addCategoryRow(flag, QString::fromStdString(m_Filter->GetValidityFlagName(flag)),
                           static_cast<int>(m_Filter->GetCellIdsWithFlag(flag).size()),
                           FlagText(flag));
        }
    }

    connect(m_CategoryTable, &QTableWidget::cellClicked, this,
            &igQtValidateCellsResultDialog::onCategoryCellClicked);

    auto* detailGroup = new QGroupBox(QStringLiteral("问题单元明细"), this);
    auto* detailLayout = new QVBoxLayout(detailGroup);
    m_DetailTable = new QTableWidget(0, 3, detailGroup);
    m_DetailTable->setHorizontalHeaderLabels({QStringLiteral("单元编号"), QStringLiteral("问题说明"),
                                              QStringLiteral("状态值")});
    m_DetailTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_DetailTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_DetailTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_DetailTable->verticalHeader()->setVisible(false);
    m_DetailTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_DetailTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_DetailTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_DetailTable->setAlternatingRowColors(true);
    detailLayout->addWidget(m_DetailTable);
    mainLayout->addWidget(detailGroup, 1);

    if (m_Filter) {
        const auto& states = m_Filter->GetValidityStates();
        for (size_t cellId = 0; cellId < states.size(); ++cellId) {
            const unsigned short state = states[cellId];
            if (state == Validity_Valid) {
                continue;
            }

            const int row = m_DetailTable->rowCount();
            m_DetailTable->insertRow(row);

            auto* idItem = new QTableWidgetItem(QString::number(static_cast<qlonglong>(cellId)));
            idItem->setData(Qt::UserRole, static_cast<qlonglong>(cellId));
            idItem->setTextAlignment(Qt::AlignCenter);
            auto* textItem = new QTableWidgetItem(
                    QString::fromStdString(m_Filter->GetValidityStateText(state)));
            auto* stateItem = new QTableWidgetItem(FlagText(state));
            stateItem->setTextAlignment(Qt::AlignCenter);

            m_DetailTable->setItem(row, 0, idItem);
            m_DetailTable->setItem(row, 1, textItem);
            m_DetailTable->setItem(row, 2, stateItem);
        }
    }

    connect(m_DetailTable, &QTableWidget::cellClicked, this,
            &igQtValidateCellsResultDialog::onDetailCellClicked);

    auto* buttonLayout = new QHBoxLayout;
    auto* allButton = new QPushButton(QStringLiteral("高亮全部非有效单元"), this);
    auto* closeButton = new QPushButton(QStringLiteral("关闭"), this);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(allButton);
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    connect(allButton, &QPushButton::clicked, this,
            &igQtValidateCellsResultDialog::onHighlightAllInvalid);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
}

void igQtValidateCellsResultDialog::highlightCellIds(const std::vector<igIndex>& ids) {
    if (m_Model == nullptr) {
        return;
    }

    auto selection = m_Model->GetSelection();
    if (selection == nullptr) {
        return;
    }

    selection->ClearSelections();
    if (!ids.empty()) {
        selection->SelectionCallBackEvent(IG_CELL, ids, Selection::Operate::Add);
        selection->SetSelectItemVisable(true);
    } else {
        selection->SetSelectItemVisable(false);
    }

    if (m_RendererWidget != nullptr) {
        m_RendererWidget->update();
    }
}

void igQtValidateCellsResultDialog::highlightAllInvalid() {
    if (!m_Filter) {
        return;
    }

    std::vector<igIndex> ids = m_Filter->GetInvalidCellIds();
    const auto& unsupported = m_Filter->GetUnsupportedCellIds();
    ids.insert(ids.end(), unsupported.begin(), unsupported.end());
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    highlightCellIds(ids);
}

void igQtValidateCellsResultDialog::onCategoryCellClicked(int row, int column) {
    Q_UNUSED(column);
    if (m_CategoryTable == nullptr || m_CategoryTable->item(row, 0) == nullptr) {
        return;
    }

    const unsigned short flag = static_cast<unsigned short>(
            m_CategoryTable->item(row, 0)->data(Qt::UserRole).toInt());
    if (flag == 0) {
        highlightAllInvalid();
        return;
    }

    if (!m_Filter) {
        highlightCellIds({});
        return;
    }
    highlightCellIds(m_Filter->GetCellIdsWithFlag(flag));
}

void igQtValidateCellsResultDialog::onDetailCellClicked(int row, int column) {
    Q_UNUSED(column);
    if (m_DetailTable == nullptr || m_DetailTable->item(row, 0) == nullptr) {
        return;
    }

    const igIndex cellId = static_cast<igIndex>(
            m_DetailTable->item(row, 0)->data(Qt::UserRole).toLongLong());
    highlightCellIds({cellId});
}

void igQtValidateCellsResultDialog::onHighlightAllInvalid() {
    highlightAllInvalid();
}
