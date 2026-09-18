#include"IQWidgets/igQtResampleToLineWidget.h"
#include"ModelSurface/iGameModelGeometryFilter.h"

#include "IQComponents/igQtModelDialogWidget.h"
#include "iGameSceneManager.h"
#include"iGameThreadPool.h"
#include <QRegularExpression>
#include <QRegularExpressionValidator>

igQtResampleToLine::igQtResampleToLine(igQtModelDialogWidget* modelTreeWidget, QWidget* parent) : QWidget(parent), ui(new Ui::ResampleToLineWidget) {
    ui->setupUi(this);
    m_ModelTreeWidget = modelTreeWidget;
    connect(ui->pushButton, &QPushButton::clicked, this, [&]() {
        this->UpdateLine();
        this->ResampleToLine();
    });
    connect(ui->point1_x, &QLineEdit::textChanged, this,[&]() {
        this->UpdateLine();
        this->DrawLine(m_orig, m_target);
    });
    connect(ui->point1_y, &QLineEdit::textChanged, this,[&]() {
        this->UpdateLine();
        this->DrawLine(m_orig, m_target);
    });
    connect(ui->point1_z, &QLineEdit::textChanged, this,[&]() {
        this->UpdateLine();
        this->DrawLine(m_orig, m_target);
    });
    connect(ui->point2_x, &QLineEdit::textChanged, this,[&]() {
        this->UpdateLine();
        this->DrawLine(m_orig, m_target);
    });
    connect(ui->point2_y, &QLineEdit::textChanged, this,[&]() {
        this->UpdateLine();
        this->DrawLine(m_orig, m_target);
    });
    connect(ui->point2_z, &QLineEdit::textChanged, this,[&]() {
        this->UpdateLine();
        this->DrawLine(m_orig, m_target);
    });
    m_Selection = GetSelection();
    QRegularExpression rx("-?\\d*\\.?\\d+");
    ui->point1_x->setValidator(new QRegularExpressionValidator(rx, this));
    ui->point1_y->setValidator(new QRegularExpressionValidator(rx, this));
    ui->point1_z->setValidator(new QRegularExpressionValidator(rx, this));
    ui->point2_x->setValidator(new QRegularExpressionValidator(rx, this));
    ui->point2_y->setValidator(new QRegularExpressionValidator(rx, this));
    ui->point2_z->setValidator(new QRegularExpressionValidator(rx, this));
    QRegularExpression rx_2("^[0-9]+$");
    ui->resolution->setValidator(new QRegularExpressionValidator(rx_2, this));
    ui->lineEdit_8->setValidator(new QRegularExpressionValidator(rx_2, this));

}

void igQtResampleToLine::DrawLine(float o[3], float t[3]) { 
    iGame::SurfaceMesh::Pointer line = iGame::SurfaceMesh::New();
    line->AddPoint(o);
    line->AddPoint(t);
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    auto model = scene->GetCurrentModel();
    auto painter = model->GetPainter3D();
    painter->SetPen(Color::White);
    painter->SetBrush(0, 255, 0);
    line->BuildEdges();
    for (int i = 0; i < line->GetNumberOfPoints() - 1; i++) { painter->DrawLine(line->GetPoint(i), line->GetPoint(i + 1)); }
    painter->Modified();
}


void igQtResampleToLine::SetLine(float o[3], float t[3]) { 
    QSignalBlocker blocker1(ui->point1_x);
    QSignalBlocker blocker2(ui->point1_y);
    QSignalBlocker blocker3(ui->point1_z);
    QSignalBlocker blocker4(ui->point2_x);
    QSignalBlocker blocker5(ui->point2_y);
    QSignalBlocker blocker6(ui->point2_z);
    this->m_orig[0] = o[0];
    this->m_orig[1] = o[1];
    this->m_orig[2] = o[2];
    this->m_target[0] = t[0]; 
    this->m_target[1] = t[1]; 
    this->m_target[2] = t[2]; 
    ui->point1_x->setText(QString::number(o[0]));
    ui->point1_y->setText(QString::number(o[1]));
    ui->point1_z->setText(QString::number(o[2]));
    ui->point2_x->setText(QString::number(t[0]));
    ui->point2_y->setText(QString::number(t[1]));
    ui->point2_z->setText(QString::number(t[2]));
    
}

void igQtResampleToLine::SetLine(iGame::Vector3d orig, iGame::Vector3d target) { 
    float o[3], t[3];
    o[0] = orig[0];
    o[1] = orig[1];
    o[2] = orig[2];
    t[0] = target[0];
    t[1] = target[1];
    t[2] = target[2];
    SetLine(o, t);

}

void igQtResampleToLine::UpdateLine() { 
    this->m_orig[0] = ui->point1_x->text().toFloat();
    this->m_orig[1] = ui->point1_y->text().toFloat();
    this->m_orig[2] = ui->point1_z->text().toFloat();
    this->m_target[0] = ui->point2_x->text().toFloat();
    this->m_target[1] = ui->point2_y->text().toFloat();
    this->m_target[2] = ui->point2_z->text().toFloat();

    m_Selection->Orig[0] = m_orig[0];
    m_Selection->Orig[1] = m_orig[1];
    m_Selection->Orig[2] = m_orig[2];
    m_Selection->Target[0] = m_target[0];
    m_Selection->Target[1] = m_target[1];
    m_Selection->Target[2] = m_target[2];
    m_Selection->UpdateLine();

}

void igQtResampleToLine::SetOriginDataObject(iGame::DataObject::Pointer m_d) {
    
    if (m_OriginDataObject && m_OriginObserverTag) { 
        m_OriginDataObject->RemoveObserver(m_OriginObserverTag);
        m_OriginObserverTag = 0;
    }
    if (m_ResultMesh && m_ResultObserverTag) { 
        m_ResultMesh = iGame::UnstructuredMesh::New();
        m_ResultObserverTag = 0;
    }

    this->m_OriginDataObject = m_d;
    m_ResultMesh = iGame::UnstructuredMesh::New();
    m_ResultMesh->SetName("ResampleToLine");
    DrawLine(m_ResultMesh);

    //监听
    m_ResultObserverTag = m_ResultMesh->AddObserver(iGame::Command::DeleteEvent, [&]() -> void {
        // 移除原始模型上的 Observer
        if (m_OriginDataObject && m_OriginObserverTag) {
            m_OriginDataObject->RemoveObserver(m_OriginObserverTag);
            m_OriginObserverTag = 0;
        }
        m_ResultObserverTag = 0;
        this->m_OriginDataObject = nullptr;
        this->m_ResultMesh = nullptr;
        this->parentWidget()->hide();
        ResetInteractor();
    });
}

iGame::LineSelection::Pointer igQtResampleToLine::GetSelection() {
    if (m_Selection == nullptr) {
        m_Selection = iGame::LineSelection::New();
        m_Selection->SetSelectionCallBackEvent(
                [&](IGenum itemType, const std::vector<igIndex>& ids, iGame::Selection::Operate ope) {
                    if (itemType != IG_CHANGE) return;
                    SetLine(m_Selection->Orig, m_Selection->Target);
                },
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    }
    return m_Selection;
}

void igQtResampleToLine::ResampleToLine() {
    if (!this->m_OriginDataObject) return;
    iGame::ResampleToLine::Pointer filter = iGame::ResampleToLine::New();
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    //auto modelTreeWidget = iGame::SceneManager
    filter->SetInput(m_OriginDataObject);
    filter->setOrigTarget(m_orig, m_target, resolution);
    if (filter->Execute()) { iGame::UnstructuredMesh::Pointer res = DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput(0));
        res->SetName(res->GetName());
        if (res != nullptr) { 
            int id = m_ModelTreeWidget->addDataObjectToModelTree(res, Algorithm);
            
            m_ResultMesh = res;
            m_ResultMesh->ConvertToDrawableData();
            DrawLine(m_ResultMesh);
            
        }
    }

}

void igQtResampleToLine::UpdateOriginDataObject(iGame::DataObject::Pointer _origin_ptr) {
    m_OriginDataObject = _origin_ptr;
}

