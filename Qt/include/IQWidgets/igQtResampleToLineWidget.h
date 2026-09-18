#pragma once
#include "ResampleToLine/iGameResampleToLine.h"

#include "IQCore/igQtMainWindow.h"
#include "IQComponents/igQtModelTreeWidget.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameSelection.h"
#include "Core/Interactor/iGameSlicingStyle.h"
#include "iGameSelection.h"

#include <ui_ResampleToLine.h>

class igQtModelDialogWidget;

class igQtResampleToLine : public QWidget {
	Q_OBJECT

public:
    igQtResampleToLine(igQtModelDialogWidget* modelTreeWidget, QWidget* parent = nullptr);

public slots:

	void SetLine(float o[3], float t[3]);

	void SetLine(iGame::Vector3d orig, iGame::Vector3d target);

	void DrawLine(float o[3], float t[3]);

	void UpdateLine();

	void ResampleToLine();

	void SetOriginDataObject(iGame::DataObject::Pointer m_d);

	void UpdateOriginDataObject(iGame::DataObject::Pointer _origin_ptr);


	iGame::LineSelection::Pointer GetSelection();

signals:
    void DrawLine(iGame::DrawObject::Pointer);
    void UpdateLineModel(iGame::DrawObject::Pointer);

	void ResetInteractor();
protected:
private:
    Ui::ResampleToLineWidget* ui;

	iGame::LineSelection::Pointer m_Selection;
    float m_orig[3] = {-1, 0, 0};
    float m_target[3] = {1, 0, 0};
	float resolution = 40;
    float dist = 1e-6f;

    iGame::DataObject::Pointer m_OriginDataObject{nullptr};
    iGame::UnstructuredMesh::Pointer m_ResultMesh{nullptr};
    igQtModelDialogWidget* m_ModelTreeWidget{nullptr};
	
	unsigned long m_OriginObserverTag{0};
    unsigned long m_ResultObserverTag{0};

	
	
};