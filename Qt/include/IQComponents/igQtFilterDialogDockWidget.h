//
// Created by m_ky on 2024/5/22.
//

/**
 * @class   igQtFilterDialogDockWidget
 * @brief   igQtFilterDialogDockWidget's brief
 */
#pragma once

#include <ui_filterDialog.h>
#include <IQCore/igQtExportModule.h>
#include <qaction.h>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <array>
#include <functional>

class QResizeEvent;
class QShowEvent;

class IG_QT_MODULE_EXPORT igQtFilterDialogDockWidget : public QDockWidget {
    Q_OBJECT
public:
    /// When \p framelessWhenFloating is true, floating windows use no system frame and a custom title bar
    /// (same pattern as igQtModelDialogWidget layer docks). \p allowedAreas in filterDialog.ui is already NoDockWidgetArea.
    explicit igQtFilterDialogDockWidget(QWidget* parent = Q_NULLPTR, bool framelessWhenFloating = false);
    ~igQtFilterDialogDockWidget() override;

    enum WidgetType{ 
        QT_LINE_EDIT = 1,
        QT_CHECK_BOX,
        QT_COMBO_BOX,
    };

    void apply();
    void close();

    void setFilterTitle(const QString& title);
    void setFilterDescription(const QString& text);
    int addParameter(WidgetType type, const QString& title,
                     const QString& defaultValue);
    int addParameter(WidgetType type, const QString& title,
        const std::vector<QString>& defaultValue);

    // 三分量向量参数：一行内并排 X/Y/Z 三个输入框（避免每分量各占一行的臃肿比例）
    int addVectorParameter(const QString& title,
                           const QString& x, const QString& y, const QString& z);
    double getVectorComponent(int i, int component, bool& ok) const;
    void setVectorComponent(int i, int component, const QString& text) const;
    // 获取向量某个分量的输入框（用于连接实时预览等）
    QLineEdit* getVectorEdit(int i, int component) const;
    // 启用/禁用整个向量输入（预设已决定方向时置灰，仅“自定义”时可编辑）
    void setVectorEnabled(int i, bool enabled) const;
    // 调整两列比例（标签列 : 输入列）；默认 1:1，建议表单用 0:1 让输入框占满剩余宽度
    void setParameterColumnStretch(int labelStretch, int valueStretch);

    double getDouble(int i, bool& ok) {
        Item& item = itemMap[i];
        double value{};
        switch (item.type) 
        {
            case QT_LINE_EDIT:
            {
                QLineEdit* line = dynamic_cast<QLineEdit*>(item.widget);
                value = line->text().toDouble(&ok);
            } 
            break;
            default:
                break;
        }
        return value;
    }

    int getInt(int i, bool& ok) {
        Item& item = itemMap[i];
        int value{};
        switch (item.type) {
            case QT_LINE_EDIT: {
                QLineEdit* line = dynamic_cast<QLineEdit*>(item.widget);
                value = line->text().toInt(&ok);
            } break;
            default:
                break;
        }
        return value;
    }

    bool getChecked(int i, bool& ok) {
        Item& item = itemMap[i];
        bool value{};
        ok = false;
        switch (item.type) {
            case QT_CHECK_BOX: {
                QCheckBox* check = dynamic_cast<QCheckBox*>(item.widget);
                value = check->isChecked();
                ok = true;
            } break;
            default:
                break;
        }
        return value;
    }

    int getComboIndex(int i, bool& ok) {
        Item& item = itemMap[i];
        int value{};
        ok = false;
        switch (item.type) {
        case QT_COMBO_BOX: {
            QComboBox* check = dynamic_cast<QComboBox*>(item.widget);
            value = check->currentIndex();
            ok = true;
        } break;
        default:
            break;
        }
        return value;
    }

	QWidget* getWidget(int i) {
		auto item = itemMap.find(i);
		return item == itemMap.end() ? nullptr : item->second.widget;
	}

    template<typename Functor, typename... Args>
    void setApplyFunctor(Functor&& functor, Args&&... args) {
        applyFunctor = std::bind(functor, args...);
    }

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void applyFramelessForFloating(bool floating);
    void updateFramelessRoundedMask();
    int addParameter(QLabel* label, QWidget* value);
    QWidget* m_customFrameTitleBar = nullptr;
    bool m_framelessWhenFloating = false;
    Ui::FilterDockDialog* ui;
    QGridLayout* gridLayout;

    struct Item {
        QString title;
        std::vector<QString> value;
        WidgetType type;
        QWidget* widget;
    };

    std::function<void()> applyFunctor;
    std::map<int, Item> itemMap;
    std::map<int, std::array<QLineEdit*, 3>> vectorItemMap;
    int index;
};

