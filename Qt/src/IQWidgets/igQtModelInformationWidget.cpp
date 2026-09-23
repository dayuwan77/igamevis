#include "IQWidgets/igQtModelInformationWidget.h"
#include "iGameSceneManager.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
#include <filesystem>

namespace {
/** 在路径分隔符后插入零宽空格，便于 QLabel 在窄 dock 内按段换行（路径通常不含空格）。 */
QString pathForLabelWrap(const QString& path) {
    if (path.isEmpty() || path == QLatin1String("(n/a)")) return path;
    QString out;
    out.reserve(path.size() + path.size() / 8 + 8);
    for (QChar c : path) {
        out.append(c);
        if (c == QLatin1Char('/') || c == QLatin1Char('\\')) out.append(QChar(0x200B));
    }
    return out;
}
} // namespace

igQtModelInformationWidget::igQtModelInformationWidget(QWidget* parent) : QWidget(parent) {
    // Layout for the main widget
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    // Creating frames and adding them to the main layout
    this->scrollArea = new QScrollArea(this);
    this->scrollArea->setWidgetResizable(true);
    this->scrollArea->setFrameShape(QFrame::NoFrame);
    this->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    this->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    this->informationFrame = new QFrame();
    this->informationFrame->setFrameShape(QFrame::StyledPanel);
    this->frameLayout = new QVBoxLayout(this->informationFrame);
    this->frameLayout->setContentsMargins(8, 8, 8, 8);
    this->frameLayout->setSpacing(6);
    this->scrollArea->setWidget(this->informationFrame);
    mainLayout->addWidget(this->scrollArea);
    this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
}

void igQtModelInformationWidget::CreateDataObjectLayoutInfo(iGame::DataObject::Pointer obj, QFormLayout* formLayout) {
    if (!formLayout) {
        return;
    }
    switch (obj->GetDataObjectType()) {
        case IG_SURFACE_MESH: {
            auto mesh = iGame::DynamicCast<iGame::SurfaceMesh>(obj);
            createPropertyLabel(formLayout, QStringLiteral("类型"), QStringLiteral("表面网格"));
            createPropertyLabel(formLayout, QStringLiteral("面数"), QString::number(mesh->GetNumberOfFaces()));
            createPropertyLabel(formLayout, QStringLiteral("点数"), QString::number(mesh->GetNumberOfPoints()));
        } break;
        case IG_VOLUME_MESH: {
            auto mesh = iGame::DynamicCast<iGame::VolumeMesh>(obj);
            createPropertyLabel(formLayout, QStringLiteral("类型"), QStringLiteral("体网格"));
            createPropertyLabel(formLayout, QStringLiteral("体单元数"), QString::number(mesh->GetNumberOfVolumes()));
            createPropertyLabel(formLayout, QStringLiteral("点数"), QString::number(mesh->GetNumberOfPoints()));
        } break;
        case IG_STRUCTURED_MESH: {
            auto mesh = iGame::DynamicCast<iGame::StructuredMesh>(obj);
            auto size = mesh->GetDimensionSize();
            createPropertyLabel(formLayout, QStringLiteral("类型"), QStringLiteral("结构化网格"));
            createPropertyLabel(formLayout, QStringLiteral("X 维度"), QString::number(size[0]));
            createPropertyLabel(formLayout, QStringLiteral("Y 维度"), QString::number(size[1]));
            createPropertyLabel(formLayout, QStringLiteral("Z 维度"), QString::number(size[2]));
        } break;
        case IG_UNSTRUCTURED_MESH: {
            auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);
            createPropertyLabel(formLayout, QStringLiteral("类型"), QStringLiteral("非结构化网格"));
            createPropertyLabel(formLayout, QStringLiteral("单元数"), QString::number(mesh->GetNumberOfCells()));
            createPropertyLabel(formLayout, QStringLiteral("点数"), QString::number(mesh->GetNumberOfPoints()));
        } break;
        default:
            break;
    }
}
void igQtModelInformationWidget::updateInformationFrame() {
    // 禁用界面更新
    this->setUpdatesEnabled(false);
    frameLayout->blockSignals(true);

    // 清空现有控件
    while (QLayoutItem* item = frameLayout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->hide();        // 隐藏控件
            widget->deleteLater(); // 标记控件待删除
        }
        delete item; // 删除布局项
    }

    // 获取当前场景和模型
    auto sceneManeger = iGame::SceneManager::Instance();
    auto scene = sceneManeger->GetCurrentScene();
    if (!scene) {
        frameLayout->blockSignals(false);
        this->setUpdatesEnabled(true);
        return;
    }

    auto currentModel = scene->GetCurrentModel();
    if (!currentModel) {
        this->hide();
        this->setUpdatesEnabled(true);
        return; // 直接返回，避免继续执行
    }
    this->show();

    auto obj = currentModel->GetDataObject();

    // 处理文件属性
    iGame::Property::Pointer p;
    std::string filePath("");
    if ((p = obj->GetProperties()->GetProperty("FilePath")) != nullptr) { filePath = p->Get<std::string>(); }

    size_t lastSlashPos = filePath.find_last_of("/\\");
    QString directory, fileName;
    if (lastSlashPos == std::string::npos) {
        directory = "(n/a)";
        if (filePath.length() == 0) {
            fileName = QString::fromStdString(obj->GetName());
            if (fileName.length() == 0) { fileName = "(n/a)"; }
        } else {
            fileName = QString::fromStdString(filePath);
        }
    } else {
        directory = QString::fromStdString(filePath.substr(0, lastSlashPos));
        fileName = QString::fromStdString(filePath.substr(lastSlashPos + 1));
    }


    frameLayout->addWidget(createLabel(QStringLiteral("文件属性")));



    frameLayout->addWidget(createSeparator());
    QWidget* filePropWidget = new QWidget(this->informationFrame);
    QFormLayout* filePropForm = new QFormLayout(filePropWidget);
    filePropForm->setContentsMargins(0, 0, 0, 0);
    filePropForm->setHorizontalSpacing(10);
    filePropForm->setVerticalSpacing(6);
    filePropForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    createPropertyLabel(filePropForm, QStringLiteral("名称"), pathForLabelWrap(fileName));
    createPropertyLabel(filePropForm, QStringLiteral("路径"), pathForLabelWrap(directory));
    frameLayout->addWidget(filePropWidget);

    // 数据统计
    frameLayout->addWidget(createLabel(QStringLiteral("数据统计")));
    frameLayout->addWidget(createSeparator());
    QWidget* statWidget = new QWidget(this->informationFrame);
    QFormLayout* statForm = new QFormLayout(statWidget);
    statForm->setContentsMargins(0, 0, 0, 0);
    statForm->setHorizontalSpacing(10);
    statForm->setVerticalSpacing(6);
    statForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    if (obj->HasSubDataObject()) {
        createPropertyLabel(statForm, QStringLiteral("类型"), QStringLiteral("多块网格"));
        createPropertyLabel(statForm, QStringLiteral("块数"), QString::number(obj->GetNumberOfSubDataObjects()));
    } else {
        CreateDataObjectLayoutInfo(obj, statForm);

        // 属性数组：名称 / 类型 / 范围（与 ParaView 的属性表格式一致）
        auto attrSet = obj->GetAttributeSet();
        if (attrSet != nullptr && attrSet->GetNumberOfAttributes() > 0) {
            // ParaView 的信息面板在统计数组范围时会跳过被 blank（幽灵）的元组，这里采用同一口径：
            // 按 vtkGhostType 中的 HIDDENPOINT / HIDDENCELL 位跳过被隐藏的点或单元。
            auto AppendAttributeRows = [&](auto attrs, unsigned char hiddenBit) {
                if (attrs == nullptr || attrs->GetNumberOfElements() == 0) return;
                iGame::ArrayObject::Pointer ghostArray = nullptr;
                for (int g = 0; g < attrs->GetNumberOfElements(); g++) {
                    auto& ghostAttr = attrs->GetElement(g);
                    if (ghostAttr.IsNone() || ghostAttr.pointer == nullptr) continue;
                    const std::string ghostName = ghostAttr.pointer->GetName();
                    if (ghostName == "vtkGhostType" || ghostName == "GhostType") {
                        ghostArray = ghostAttr.pointer;
                        break;
                    }
                }
                for (int i = 0; i < attrs->GetNumberOfElements(); i++) {
                    auto& attr = attrs->GetElement(i);
                    auto arr = attr.pointer;
                    if (arr == nullptr || attr.IsNone()) continue;
                    // 数组类型名（与 ParaView 的属性表一致）：按 GetArrayType() 枚举完整映射，
                    // 避免某些类型落进 unknown（例如八叉树过滤器输出的 UnsignedCharArray）
                    QString typeName = QStringLiteral("unknown");
                    switch (arr->GetArrayType()) {
                        case IG_FloatArray:
                            typeName = QStringLiteral("float");
                            break;
                        case IG_DoubleArray:
                            typeName = QStringLiteral("double");
                            break;
                        case IG_IntArray:
                            typeName = QStringLiteral("int");
                            break;
                        case IG_UnsignedIntArray:
                            typeName = QStringLiteral("unsigned int");
                            break;
                        case IG_ShortArray:
                            typeName = QStringLiteral("short");
                            break;
                        case IG_UnsignedShortArray:
                            typeName = QStringLiteral("unsigned short");
                            break;
                        case IG_CharArray:
                            typeName = QStringLiteral("char");
                            break;
                        case IG_UnsignedCharArray:
                            typeName = QStringLiteral("unsigned char");
                            break;
                        case IG_LongLongArray:
                            typeName = QStringLiteral("long long");
                            break;
                        case IG_UnsignedLongLongArray:
                            typeName = QStringLiteral("unsigned long long");
                            break;
                        default:
                            break;
                    }
                    QString rangeText = QStringLiteral("n/a");
                    if (arr->GetNumberOfElements() > 0) {
                        const int dim = arr->GetDimension() > 0 ? arr->GetDimension() : 1;
                        double rmin = 0.0;
                        double rmax = 0.0;
                        bool hasValue = false;
                        // 第一遍跳过被 blank 的元组；若全被 blank，第二遍回退为统计全部，避免显示 n/a
                        for (int pass = 0; pass < 2 && !hasValue; ++pass) {
                            const bool skipHidden = (pass == 0);
                            for (IGsize t = 0; t < arr->GetNumberOfElements(); ++t) {
                                if (skipHidden && ghostArray != nullptr &&
                                    t < ghostArray->GetNumberOfElements() &&
                                    (static_cast<unsigned char>(ghostArray->GetElementValue(t, 0)) & hiddenBit) !=
                                            0) {
                                    continue;
                                }
                                for (int c = 0; c < dim; ++c) {
                                    const double v = arr->GetElementValue(t, c);
                                    if (!hasValue) {
                                        rmin = rmax = v;
                                        hasValue = true;
                                    } else if (v < rmin) {
                                        rmin = v;
                                    } else if (v > rmax) {
                                        rmax = v;
                                    }
                                }
                            }
                        }
                        if (hasValue) {
                            // 与 ParaView / VTK 一致：double 按 17 位有效数字输出
                            // （float 数组经提升后也能给出与 ParaView 相同的十进制形式）
                            rangeText = QStringLiteral("[%1, %2]")
                                            .arg(QString::number(rmin, 'g', 17),
                                                 QString::number(rmax, 'g', 17));
                        }
                    }
                    createPropertyLabel(statForm, QString::fromStdString(arr->GetName()),
                                        typeName + QStringLiteral(" | ") + rangeText);
                }
            };
            // 2 = vtkDataSetAttributes::HIDDENPOINT，32 = vtkDataSetAttributes::HIDDENCELL
            AppendAttributeRows(attrSet->GetAllPointAttributes(), 2);
            AppendAttributeRows(attrSet->GetAllCellAttributes(), 32);
        }
    }

    IGsize memorySize = obj->GetRealMemorySize();
    QString dw[5] = {"B", "KB", "MB", "GB", "TB"};
    igIndex index = 0;
    while (memorySize > 1024 && index < 4) {
        memorySize /= 1024;
        index++;
    }
    createPropertyLabel(statForm, QStringLiteral("内存占用"), QString::number(memorySize) + dw[index]);
    frameLayout->addWidget(statWidget);

    // 处理边界框（并入表单布局，保持与其它项一致对齐）
    iGame::BoundingBox bound = obj->GetBoundingBox();
    const QString axisNames[3] = {QStringLiteral("X 范围"), QStringLiteral("Y 范围"), QStringLiteral("Z 范围")};
    for (int i = 0; i < 3; i++) {
        const float min = bound.min[i];
        const float max = bound.max[i];
        const float delta = max - min;
        const QString boundsValue = QStringLiteral("%1 至 %2  (差值: %3)")
                                            .arg(min, 0, 'f', 2)
                                            .arg(max, 0, 'f', 2)
                                            .arg(delta, 0, 'f', 2);
        createPropertyLabel(statForm, axisNames[i], boundsValue);
    }


    // 底部弹性区保证内容在较少时贴顶显示
    frameLayout->addStretch();

    // 恢复界面更新
    this->setUpdatesEnabled(true);
    // 修改布局
    frameLayout->blockSignals(false);
}

QLabel* igQtModelInformationWidget::createLabel(const QString& text) {
    QLabel* label = new QLabel(text);
    label->setWordWrap(false);                                            // 禁用换行
    label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);   // 允许水平压缩
    label->setStyleSheet(R"(
        QLabel {
            font-size: 14px !important;
            color: #FFFFFF !important; /* 纯纯白，匹配iOS的#FFFFFF */
        }
    )");
    return label;
}

void igQtModelInformationWidget::createPropertyLabel(QFormLayout* formLayout, const QString& name, const QString& value) {
    QLabel* nameLabel = new QLabel(name + ":");
    nameLabel->setAlignment(Qt::AlignCenter);
    nameLabel->setMinimumWidth(120);
    nameLabel->setStyleSheet("QLabel { font-size: 14px; color: #C8C8C8; }");

    QLabel* valueLabel = new QLabel(value);
    valueLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    valueLabel->setWordWrap(true);
    valueLabel->setMinimumWidth(0);
    valueLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    valueLabel->setStyleSheet("QLabel { font-size: 14px; color: #FFFFFF; }");
    formLayout->addRow(nameLabel, valueLabel);
}
QFrame* igQtModelInformationWidget::createSeparator() {
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    //添加修改样式
    line->setStyleSheet(R"(
        QWidget {
            background-color: #3A3A3A !important; /* 深灰分割线，适配深色主题 */
            height: 1px !important;
            margin: 4px 0 !important;
        }
    )");
    return line;
}