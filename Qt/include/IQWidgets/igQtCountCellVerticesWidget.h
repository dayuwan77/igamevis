#pragma once

// —— 简单任务 Filter 头文件 ——
#include "CountCellVertices/iGameCountCellVerticesFilter.h"
#include "iGamePointSet.h"

#include <ui_CountCellVertices.h>

/**
 * @class igQtCountCellVerticesWidget
 * @brief "统计单元顶点数" 面板控件（简单任务 #5 的 GUI 集成）。
 *
 * 【职责】
 *   把 CountCellVerticesFilter 包装成可视化面板：
 *   点「执行」→ 跑 Filter（给网格挂 cell_vertex_count 属性）→
 *   把每个单元的顶点数以表格形式列出来（模仿 ParaView SpreadSheet View 的体验）。
 *
 * 【布局】
 *   顶部： [执行] [导出CSV] 按钮
 *   中间： 统计摘要（共 N 个单元，顶点数范围）
 *   主体： QTableWidget 表格（单元编号 | 顶点数）
 */
class igQtCountCellVerticesWidget : public QWidget {

    Q_OBJECT

public:
    igQtCountCellVerticesWidget(QWidget* parent = nullptr);

public slots:
    /// 「执行」按钮：运行 CountCellVerticesFilter 并填充表格
    void ExecuteCount();

    /// 「导出CSV」按钮：把表格数据保存为 .csv 文件
    void ExportCSV();

    /// 由主窗口调用：记录当前选中的输入模型
    void SetOriginDataObject(iGame::DataObject::Pointer obj);

private:
    /// 从结果属性集里按名字找 cell_vertex_count 数组（找不到返回空）
    iGame::ArrayObject::Pointer FindCountArray(iGame::DataObject::Pointer obj);

    /// 把数组填进表格并更新摘要
    void FillTable(iGame::ArrayObject::Pointer counts);

    Ui::CountCellVertices* ui;

    iGame::DataObject::Pointer m_OriginDataObject{ nullptr };   // 选中的输入模型
    iGame::CountCellVerticesFilter::Pointer m_Filter{ nullptr }; // 简单任务 Filter 实例
    iGame::ArrayObject::Pointer m_Counts{ nullptr };             // 最近一次统计结果（供导出用）
};
