#pragma once

#include <ui_VolumeInterpolator.h>

#include "iGameDataObject.h"
#include "iGameVolumeMesh.h"

#include <vector>

// 体采样面板：对当前体网格模型，在指定坐标点处做插值（仅支持四面体/六面体单元）。
class igQtVolumeInterpolatorWidget : public QWidget {
    Q_OBJECT

public:
    explicit igQtVolumeInterpolatorWidget(QWidget* parent = nullptr);
    ~igQtVolumeInterpolatorWidget() override = default;

public slots:
    // 从当前场景模型刷新：模型名 + 点属性列表
    void RefreshModel();
    // 对输入坐标执行体采样插值
    void Probe();

public:
    // 把输入对象解析为体网格（支持 VolumeMesh / UnstructuredMesh 转换），失败返回 nullptr
    static iGame::VolumeMesh::Pointer ResolveVolumeMesh(iGame::DataObject::Pointer obj);
    // 检查体网格是否只包含四面体 / 六面体单元（其余类型不支持插值，需弹窗报警）
    static bool CheckCellTypesSupported(iGame::VolumeMesh* mesh);

    Ui::VolumeInterpolatorWidget* ui;
    iGame::DataObject::Pointer m_DataObject{nullptr};
    iGame::VolumeMesh::Pointer m_VolumeMesh{nullptr};
    std::vector<int> m_AttributeIndices;
};
