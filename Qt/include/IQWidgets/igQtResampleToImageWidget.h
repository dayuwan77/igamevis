#pragma once

// ============================================================================
// igQtResampleToImageWidget —「重采样到图像 (Resample To Image)」参数面板
//
// 对应 VTK vtkResampleToImage / ParaView 的 Resample To Image：
//   - 参数：采样维度（各轴格点数）、是否使用输入包围盒、显式采样范围、
//           遇到不支持的单元类型是否终止、是否对 ID/整型数组禁用线性插值；
//   - 仅展示用户参数；规模过大时的二次确认和执行失败提示保持不变。
// ============================================================================

#include <IQCore/igQtExportModule.h>

#include <QWidget>

#include <Convert/iGameResampleToImageFilter.h>
#include <iGameDataObject.h>

class QDockWidget;

namespace iGame {
class Model;
}

namespace Ui {
class igQtResampleToImage;
}

class IG_QT_MODULE_EXPORT igQtResampleToImageWidget : public QWidget {
    Q_OBJECT

public:
    explicit igQtResampleToImageWidget(QWidget* parent = nullptr);
    ~igQtResampleToImageWidget() override;

    static QDockWidget* createDockWidget(QWidget* parent);

    /// 打开面板 / 切换模型时调用：绑定输入并刷新执行按钮状态。
    void setCurrentModel(iGame::Model* model);

signals:
    void closeRequested();
    void resultReady(iGame::DataObject::Pointer result);

private slots:
    void onRunClicked();
    void refreshInputState();

private:
    void initUI();
    void initConnections();
    bool buildFilter(iGame::ResampleToImageFilter::Pointer& filter, QString& error);

    Ui::igQtResampleToImage* ui{nullptr};
    iGame::Model* m_currentModel{nullptr};
    iGame::DataObject::Pointer m_input;
    iGame::DataObject::Pointer m_meshData; ///< 适配后的网格（用于判断面板是否可执行）
};
