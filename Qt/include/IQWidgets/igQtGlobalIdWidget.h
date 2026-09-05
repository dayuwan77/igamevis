#pragma once

#include <QWidget>

class QComboBox;
class QDockWidget;
class QLabel;
class QPushButton;

namespace iGame
{
class ArrayObject;
class DataObject;
class Model;
}

namespace Ui
{
class igQtGlobalId;
}

class igQtGlobalIdWidget : public QWidget {
    Q_OBJECT

public:
    explicit igQtGlobalIdWidget(QWidget* parent = nullptr);
    ~igQtGlobalIdWidget() override;

    static QDockWidget* createDockWidget(QWidget* parent);

    void setCurrentModel(iGame::Model* model);
    void resetOffsets();

signals:
    void cancelRequested();

private slots:
    void generateGlobalIds();
    void cancel();

private:
    Ui::igQtGlobalId* ui;
    iGame::Model* m_currentModel{nullptr};
    iGame::DataObject* m_currentModelData{nullptr};

    QWidget* m_paginationWidget{nullptr};
    QPushButton* m_previousPageButton{nullptr};
    QPushButton* m_nextPageButton{nullptr};
    QComboBox* m_pageSizeComboBox{nullptr};
    QLabel* m_pageInfoLabel{nullptr};
    quint64 m_currentPage{0};
    int m_currentDataType{0}; // 0: points, 1: cells

    void initUI();
    void initConnections();
    void refreshResults();
    void renderCurrentPage();

    iGame::ArrayObject* globalIdArray(int dataType) const;
    QString globalIdText(iGame::ArrayObject* array, quint64 index) const;
};
