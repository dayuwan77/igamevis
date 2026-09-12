#pragma once

#include <ui_PointVolumeInterpolator.h>

#include "iGameDataObject.h"
#include "iGamePointSet.h"

#include <functional>

// 点体积插值面板：把当前点云/数据集上的点属性按核函数插值到规则体网格。
class igQtPointVolumeInterpolatorWidget : public QWidget {
    Q_OBJECT

public:
    using OutputCallback = std::function<void(iGame::DataObject::Pointer)>;

    explicit igQtPointVolumeInterpolatorWidget(QWidget* parent = nullptr);
    ~igQtPointVolumeInterpolatorWidget() override = default;

    // 执行成功后把输出体网格交回主窗口（加入模型树/刷新渲染）。
    void SetOutputCallback(OutputCallback cb) { m_OutputCallback = std::move(cb); }

public slots:
    // 从当前场景模型刷新：模型名 + 点数
    void RefreshModel();
    // 按面板参数执行插值
    void Apply();

public:
    // 把输入对象解析为点集（任何网格都是 PointSet 子类），失败返回 nullptr
    static iGame::PointSet::Pointer ResolvePointSet(iGame::DataObject::Pointer obj);

    Ui::PointVolumeInterpolatorWidget* ui;
    iGame::PointSet::Pointer m_PointSet{nullptr};
    OutputCallback m_OutputCallback;
};
