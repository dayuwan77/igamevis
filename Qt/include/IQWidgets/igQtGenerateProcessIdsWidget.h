/**
 * @class   igQtGenerateProcessIdsWidget
 * @brief   生成进程ID filter 的工具面板（对齐轮廓提取的交互模式）
 */

#pragma once
#include <ui_GenerateProcessIdsWidget.h>
#include "iGameDataObject.h"

class igQtGenerateProcessIdsWidget : public QWidget {
    Q_OBJECT

public:
    igQtGenerateProcessIdsWidget(QWidget* parent = nullptr);

public slots:
    void SetOriginDataObject(iGame::DataObject::Pointer data);
    void Apply();

signals:
    void UpdateProcessIdsModel(iGame::DataObject::Pointer);

private:
    Ui::GenerateProcessIdsWidget* ui;
    iGame::DataObject::Pointer m_OriginDataObject{nullptr};
};
