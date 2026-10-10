#pragma once

#include <IQCore/igQtExportModule.h>

#include <MyFilter/iGameValidateCellsFilter.h>

#include <QDialog>

#include <vector>

class QLabel;
class QTableWidget;
class igQtModelDrawWidget;

namespace iGame {
class Model;
}

/**
 * @brief “单元几何校验”结果汇总面板。
 *
 * 面板展示已检查、有效、无效、未支持的数量，并按 ValidityState 位解析为
 * 人类可读的错误类别。点击任一类别只高亮该类别对应的单元；详情表支持单击
 * 单个单元高亮。
 */
class IG_QT_MODULE_EXPORT igQtValidateCellsResultDialog : public QDialog {
    Q_OBJECT

public:
    igQtValidateCellsResultDialog(iGame::ValidateCellsFilter::Pointer filter,
                                  iGame::Model* model,
                                  igQtModelDrawWidget* rendererWidget,
                                  QWidget* parent = nullptr);

private slots:
    void onCategoryCellClicked(int row, int column);
    void onDetailCellClicked(int row, int column);
    void onHighlightAllInvalid();

private:
    void buildUi();
    void highlightCellIds(const std::vector<igIndex>& ids);
    void highlightAllInvalid();

    iGame::ValidateCellsFilter::Pointer m_Filter{};
    iGame::Model* m_Model{nullptr};
    igQtModelDrawWidget* m_RendererWidget{nullptr};

    QLabel* m_CheckedValue{nullptr};
    QLabel* m_ValidValue{nullptr};
    QLabel* m_InvalidValue{nullptr};
    QLabel* m_UnsupportedValue{nullptr};
    QTableWidget* m_CategoryTable{nullptr};
    QTableWidget* m_DetailTable{nullptr};
};
