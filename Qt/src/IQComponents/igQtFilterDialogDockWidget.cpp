//
// Created by m_ky on 2024/5/22.
//

/**
 * @class   igQtFilterDialogDockWidget
 * @brief   igQtFilterDialogDockWidget's brief
 */
#include <IQComponents/igQtFilterDialogDockWidget.h>
#include <QColor>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMouseEvent>
#include <QPainterPath>
#include <QPalette>
#include <QPolygon>
#include <QPushButton>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTimer>
#include <iostream>

namespace
{
// Custom title bar for frameless floating filter dock (drag + close), aligned with igQtModelDialogWidget::DockTitleBar.
class FilterFrameTitleBar final : public QWidget {
public:
    explicit FilterFrameTitleBar(igQtFilterDialogDockWidget* dock, const QString& title, QWidget* parent = nullptr)
        : QWidget(parent), m_dock(dock) {
        setObjectName(QStringLiteral("FilterFrameTitleBar"));
        setAttribute(Qt::WA_StyledBackground, true);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setFixedHeight(32);

        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(10, 0, 6, 0);
        layout->setSpacing(8);

        m_titleLabel = new QLabel(title, this);
        m_titleLabel->setObjectName(QStringLiteral("IgQtFilterTitleLabel"));
        m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        layout->addWidget(m_titleLabel);

        auto* closeBtn = new QPushButton(this);
        closeBtn->setObjectName(QStringLiteral("FilterFrameTitleCloseButton"));
        closeBtn->setFixedSize(24, 24);
        closeBtn->setFlat(true);
        closeBtn->setFocusPolicy(Qt::NoFocus);
        closeBtn->setIcon(QIcon(QStringLiteral(":/Ticon/Icons/dock_close_white.svg")));
        closeBtn->setIconSize(QSize(16, 16));
        layout->addWidget(closeBtn);

        if (m_dock) {
            connect(closeBtn, &QPushButton::clicked, m_dock, &igQtFilterDialogDockWidget::close);
        }
    }

    void setTitle(const QString& t) {
        if (m_titleLabel) m_titleLabel->setText(t);
    }

protected:
    void mousePressEvent(QMouseEvent* e) override {
        if (!m_dock) return QWidget::mousePressEvent(e);
        if (e->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragOffset = e->globalPos() - m_dock->frameGeometry().topLeft();
            e->accept();
            return;
        }
        QWidget::mousePressEvent(e);
    }

    void mouseMoveEvent(QMouseEvent* e) override {
        if (!m_dock) return QWidget::mouseMoveEvent(e);
        if (m_dragging && (e->buttons() & Qt::LeftButton)) {
            m_dock->move(e->globalPos() - m_dragOffset);
            e->accept();
            return;
        }
        QWidget::mouseMoveEvent(e);
    }

    void mouseReleaseEvent(QMouseEvent* e) override {
        if (e->button() == Qt::LeftButton) {
            m_dragging = false;
            e->accept();
            return;
        }
        QWidget::mouseReleaseEvent(e);
    }

private:
    igQtFilterDialogDockWidget* m_dock = nullptr;
    QLabel* m_titleLabel = nullptr;
    bool m_dragging = false;
    QPoint m_dragOffset;
};
} // namespace

igQtFilterDialogDockWidget::igQtFilterDialogDockWidget(QWidget* parent, bool framelessWhenFloating)
    : QDockWidget(parent), ui(new Ui::FilterDockDialog), m_framelessWhenFloating(framelessWhenFloating) {
    ui->setupUi(this);

    connect(ui->applyButton, &QPushButton::clicked, this, &igQtFilterDialogDockWidget::apply);
    connect(ui->closeButton, &QPushButton::clicked, this, &igQtFilterDialogDockWidget::close);

    gridLayout = new QGridLayout();
    gridLayout->setSpacing(4);
    gridLayout->setColumnStretch(0, 1);
    gridLayout->setColumnStretch(1, 1);

    ui->verticalLayout->setAlignment(Qt::AlignTop);
    ui->verticalLayout->setMargin(20);
    ui->verticalLayout->addLayout(gridLayout);

    index = 0;

    if (m_framelessWhenFloating) {
        setObjectName(QStringLiteral("IgQtFilterFramelessDock"));
        setAttribute(Qt::WA_TranslucentBackground, false);
        setAutoFillBackground(true);
        {
            QPalette pal = palette();
            pal.setColor(QPalette::Window, QColor(0x1E, 0x1E, 0x1E));
            setPalette(pal);
        }
        ui->dockWidgetContents->setAttribute(Qt::WA_StyledBackground, true);
        auto* titleBar = new FilterFrameTitleBar(this, windowTitle(), this);
        m_customFrameTitleBar = titleBar;
        setTitleBarWidget(titleBar);

        connect(this, &QDockWidget::topLevelChanged, this, [this](bool floating) { applyFramelessForFloating(floating); });
        QTimer::singleShot(0, this, [this] {
            if (isFloating()) applyFramelessForFloating(true);
        });
    }
}

igQtFilterDialogDockWidget::~igQtFilterDialogDockWidget() {}

void igQtFilterDialogDockWidget::resizeEvent(QResizeEvent* event) {
    QDockWidget::resizeEvent(event);
    updateFramelessRoundedMask();
}

void igQtFilterDialogDockWidget::showEvent(QShowEvent* event) {
    QDockWidget::showEvent(event);
    updateFramelessRoundedMask();
}

void igQtFilterDialogDockWidget::updateFramelessRoundedMask() {
    if (!m_framelessWhenFloating) return;
    if (!isFloating()) {
        clearMask();
        return;
    }
    const QRect r = rect();
    if (r.width() < 2 || r.height() < 2) return;
    constexpr int kRadiusPx = 10;
    const int rad = qMin(kRadiusPx, qMin(r.width(), r.height()) / 2);
    QPainterPath path;
    path.addRoundedRect(r, rad, rad);
    const QPolygon poly = path.toFillPolygon().toPolygon();
    setMask(QRegion(poly));
}

void igQtFilterDialogDockWidget::applyFramelessForFloating(bool floating) {
    if (!m_framelessWhenFloating) return;
    if (floating) {
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
        /* 使用不透明绘制 + setMask 圆角即可；若改为 true，需保证子控件铺满背景 */
        setAttribute(Qt::WA_TranslucentBackground, false);
    } else {
        setWindowFlags(Qt::Widget);
        clearMask();
    }
    if (isVisible()) show();
    updateFramelessRoundedMask();
}

void igQtFilterDialogDockWidget::apply() {
    applyFunctor();
}

void igQtFilterDialogDockWidget::close() {
    hide();
    delete this;
}

void igQtFilterDialogDockWidget::setFilterTitle(const QString& title) {
    setWindowTitle(title);
    if (m_customFrameTitleBar) {
        if (auto* lab = m_customFrameTitleBar->findChild<QLabel*>(QStringLiteral("IgQtFilterTitleLabel"))) lab->setText(title);
    }
}

void igQtFilterDialogDockWidget::setFilterDescription(const QString& text) {
    ui->filterInfoLabel->setText("    " + text);
}

int igQtFilterDialogDockWidget::addParameter(WidgetType type, const QString& title, const QString& defaultValue) {

    QWidget* widget = nullptr;

    switch (type) {
    case QT_LINE_EDIT: {
        QLineEdit* line = new QLineEdit(this);
        line->setText(defaultValue);
        widget = line;
    } break;
    case QT_CHECK_BOX: {
        QCheckBox* check = new QCheckBox(this);
        bool value = defaultValue == "true" ? true : false;
        check->setChecked(value);
        widget = check;
    } break;
    case QT_COMBO_BOX: {
        QComboBox* comboBox = new QComboBox;
        comboBox->addItem(defaultValue);
        widget = comboBox;
    } break;
    default:
        break;
    }

    if (widget == nullptr) {
        return (-1);
    }

    QLabel* label = new QLabel(this);
    label->setText(title);

    Item item{ title, { defaultValue }, type, widget };
    itemMap[index] = item;

    return addParameter(label, widget);
}

int igQtFilterDialogDockWidget::addParameter(WidgetType type, const QString& title, const std::vector<QString>& defaultValue) {

    QWidget* widget = nullptr;

    switch (type) {
    case QT_LINE_EDIT: {
        if (defaultValue.size() != 1) {
            std::cout << "Only one parameter is required" << std::endl;
            return -1;
        }
        QLineEdit* line = new QLineEdit(this);
        line->setText(defaultValue[0]);
        widget = line;
    } break;
    case QT_CHECK_BOX: {
        if (defaultValue.size() != 1) {
            std::cout << "Only one parameter is required" << std::endl;
            return -1;
        }
        QCheckBox* check = new QCheckBox(this);
        bool value = defaultValue[0] == "true" ? true : false;
        check->setChecked(value);
        widget = check;
    } break;
    case QT_COMBO_BOX: {
        if (defaultValue.empty()) {
            std::cout << "At least one parameter is required" << std::endl;
            return -1;
        }
        QComboBox* comboBox = new QComboBox;
        for (const auto& item : defaultValue) {
            comboBox->addItem(item);
        }
        widget = comboBox;
    } break;
    default:
        break;
    }

    if (widget == nullptr) {
        return (-1);
    }

    QLabel* label = new QLabel(this);
    label->setText(title);

    Item item{ title, defaultValue, type, widget };
    itemMap[index] = item;

    return addParameter(label, widget);
}

int igQtFilterDialogDockWidget::addParameter(QLabel* label, QWidget* value) {
    label->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    label->setMinimumHeight(20);
    value->setMinimumHeight(20);

    gridLayout->addWidget(label, index, 0);
    gridLayout->addWidget(value, index, 1);

    return index++;
}

int igQtFilterDialogDockWidget::addVectorParameter(const QString& title,
                                                   const QString& x, const QString& y, const QString& z) {
    auto* container = new QWidget(this);
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    const QString defaults[3] = {x, y, z};
    const QString axisNames[3] = {QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("Z")};
    std::array<QLineEdit*, 3> edits{};
    for (int c = 0; c < 3; ++c) {
        auto* axisLabel = new QLabel(axisNames[c], container);
        axisLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        auto* edit = new QLineEdit(container);
        edit->setText(defaults[c]);
        layout->addWidget(axisLabel);
        layout->addWidget(edit, 1);
        edits[c] = edit;
    }

    QLabel* label = new QLabel(this);
    label->setText(title);
    const int id = addParameter(label, container);
    vectorItemMap[id] = edits;
    return id;
}

double igQtFilterDialogDockWidget::getVectorComponent(int i, int component, bool& ok) const {
    ok = false;
    auto it = vectorItemMap.find(i);
    if (it == vectorItemMap.end() || component < 0 || component > 2) return 0.0;
    return it->second[component]->text().toDouble(&ok);
}

void igQtFilterDialogDockWidget::setVectorComponent(int i, int component, const QString& text) const {
    auto it = vectorItemMap.find(i);
    if (it == vectorItemMap.end() || component < 0 || component > 2) return;
    it->second[component]->setText(text);
}

QLineEdit* igQtFilterDialogDockWidget::getVectorEdit(int i, int component) const {
    auto it = vectorItemMap.find(i);
    if (it == vectorItemMap.end() || component < 0 || component > 2) return nullptr;
    return it->second[component];
}

void igQtFilterDialogDockWidget::setVectorEnabled(int i, bool enabled) const {
    auto it = vectorItemMap.find(i);
    if (it == vectorItemMap.end()) return;
    for (QLineEdit* edit : it->second) {
        if (edit) edit->setEnabled(enabled);
    }
}

void igQtFilterDialogDockWidget::setParameterColumnStretch(int labelStretch, int valueStretch) {
    gridLayout->setColumnStretch(0, labelStretch);
    gridLayout->setColumnStretch(1, valueStretch);
}
