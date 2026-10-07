#include "iGameMeshleter.h"

#include "iGameDrawObject.h"
//#include <format>

IGAME_NAMESPACE_BEGIN

Meshleter::Meshleter() {
    m_DataObject = nullptr;

#ifdef GL_SUPPORTS_MESH_SHADER
    m_MeshletBuffer = GLBuffer::New();
    m_MeshletVertexBuffer = GLBuffer::New();
    m_MeshletTriangleBuffer = GLBuffer::New();

    m_MeshletDescriptorBuffer = GLBuffer::New();
    m_InvisibleMeshletBuffer = GLBuffer::New();

    m_PositionBuffer = GLBuffer::New();
    m_ColorBuffer = GLBuffer::New();
    m_NormalBuffer = GLBuffer::New();
    m_UVBuffer = GLBuffer::New();
#else
    m_TriangleVAO = GLVertexArray::New();
    m_TriangleEBO = GLBuffer::New();

    m_PositionVBO = GLBuffer::New();
    m_ColorVBO = GLBuffer::New();
    m_NormalVBO = GLBuffer::New();
    m_UVVBO = GLBuffer::New();

    m_MeshletDescriptorBuffer = GLBuffer::New();
    m_DrawCommandBuffer = GLBuffer::New();
    m_VisibleMeshletBuffer = GLBuffer::New();
    m_FinalDrawCommandBuffer = GLBuffer::New();

    m_CellTriangleVAO = GLVertexArray::New();
    m_CellPositionVBO = GLBuffer::New();
    m_CellColorVBO = GLBuffer::New();
    m_CellDrawCommandBuffer = GLBuffer::New();
    m_CellFinalDrawCommandBuffer = GLBuffer::New();
#endif
}

Meshleter::~Meshleter() {}

void Meshleter::SetInput(SmartPointer<DataObject> obj) {
    m_DataObject = obj;
    // this->SetName(std::format("{}'s Meshleter", m_DataObject->GetName()));
    this->SetName(m_DataObject->GetName());
}

SmartPointer<DataObject> Meshleter::GetInput() const { return m_DataObject; }

void Meshleter::SyncGpuBuffers() {
#ifndef IGAME_OPENGL_VERSION_460
    IGAME_RENDERING_ERROR("The OpenGL330 version does not support meshleter "
                          "accelerated rendering function");
#else
    if (!m_DataObject) { return; }

    auto drawObject = DynamicCast<DrawObject>(m_DataObject);
    auto positions = drawObject->m_Positions;
    auto colors = drawObject->m_Colors;
    auto cellColors = drawObject->m_CellColors;

    bool needReConvertGeometry = drawObject->m_ReConvertToDrawableData;
    needReConvertGeometry |= positions->GetMTime() > m_PositionVBO->GetMTime();

    // Rebuild if geometry is changed
    if (needReConvertGeometry) { Build(); }

    // Reconvert if scalar is changed
    if (m_RenderWithMeshlet) {
        if (m_RenderWithMeshletChanged) {
            const float meshletColors[][3] = {
                    {1.0f, 0.0f, 0.0f}, // Red
                    {0.0f, 1.0f, 0.0f}, // Green
                    {0.0f, 0.0f, 1.0f}, // Blue
                    {1.0f, 1.0f, 0.0f}, // Yellow
                    {1.0f, 0.0f, 1.0f}, // Magenta
                    {0.0f, 1.0f, 1.0f}, // Cyan
                    {1.0f, 0.5f, 0.0f}, // Orange
                    {0.5f, 0.0f, 1.0f}, // Purple
                    {0.0f, 0.5f, 1.0f}, // Sky Blue
                    {0.5f, 1.0f, 0.0f}, // Yellow-Green
                    {1.0f, 0.0f, 0.5f}, // Pinkish Red
                    {0.0f, 1.0f, 0.5f}, // Aqua Green
            };
            constexpr int kNumColors =
                    sizeof(meshletColors) / sizeof(meshletColors[0]);

            SmartPointer<FloatArray> cs = FloatArray::New();
            cs->SetDimension(3);
            for (int i = 0; i < m_ArraysDrawCommands.size(); i++) {
                auto cmd = m_ArraysDrawCommands[i];

                int meshletId = i % kNumColors;
                for (auto j = 0; j < cmd.count / 3; j++) {
                    cs->AddElement3(meshletColors[meshletId][0],
                                    meshletColors[meshletId][1],
                                    meshletColors[meshletId][2]);
                    cs->AddElement3(meshletColors[meshletId][0],
                                    meshletColors[meshletId][1],
                                    meshletColors[meshletId][2]);
                    cs->AddElement3(meshletColors[meshletId][0],
                                    meshletColors[meshletId][1],
                                    meshletColors[meshletId][2]);
                }
            }

            m_CellColorVBO->Create();
            m_CellColorVBO->Target(GL_ARRAY_BUFFER);
            GLAllocateGLBuffer(m_CellColorVBO,
                               cs->GetNumberOfValues() * sizeof(float),
                               cs->RawPointer());

            m_CellTriangleVAO->Create();
            m_CellTriangleVAO->VertexBuffer(GL_VBO_IDX_1, m_CellColorVBO, 0,
                                            3 * sizeof(float));
            GLSetVertexAttrib(m_CellTriangleVAO, GL_LOCATION_IDX_1,
                              GL_VBO_IDX_1, 3, GL_FLOAT, GL_FALSE, 0);
        }
    } else {
        if (drawObject->m_AttributeIndex == -1) {
            drawObject->m_UseColor = false;
        } else {
            if (colors->GetMTime() > m_ColorVBO->GetMTime() ||
                m_RenderWithMeshletChanged) {
                m_ColorVBO->Create();
                m_ColorVBO->Target(GL_ARRAY_BUFFER);
                GLAllocateGLBuffer(m_ColorVBO,
                                   colors->GetNumberOfValues() * sizeof(float),
                                   colors->RawPointer());
                m_ColorVBO->Modified();

                m_TriangleVAO->VertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0,
                                            3 * sizeof(float));
                GLSetVertexAttrib(m_TriangleVAO, GL_LOCATION_IDX_1,
                                  GL_VBO_IDX_1, 3, GL_FLOAT, GL_FALSE, 0);
            }

            if (cellColors->GetMTime() > m_CellColorVBO->GetMTime() ||
                m_RenderWithMeshletChanged) {
                auto& attr = drawObject->GetAttributeSet()->GetAttribute(
                        drawObject->m_AttributeIndex);
                if (!attr.isDeleted && attr.attachmentType == IG_CELL) {
                    SmartPointer<FloatArray> cellColorMapper =
                            drawObject->m_ColorMapper->MapScalars(
                                    attr.pointer,
                                    drawObject->m_AttributeDimension);

                    float color[3]{};
                    SmartPointer<FloatArray> ces = FloatArray::New();
                    ces->SetDimension(3);

                    // 优先使用"三角形 -> 单元"映射(与绘制的三角形顺序严格一致),
                    // 该映射由 ConvertToDrawableData 生成;缺失时退回 meshlet 路径
                    // 构建的 m_TriangleToFace。此前普通渲染路径下该映射为空,
                    // 逐三角形颜色缓冲未被填充,单元数据着色会退化成按顶点采样。
                    UnsignedIntArray* triangleToCell = drawObject->GetTriangleToCell();
                    const IGsize triangleCount =
                            triangleToCell != nullptr
                                    ? triangleToCell->GetNumberOfElements()
                                    : static_cast<IGsize>(m_TriangleToFace.size());

                    for (IGsize i = 0; i < triangleCount; i++) {
                        const IGsize cellId =
                                triangleToCell != nullptr
                                        ? static_cast<IGsize>(triangleToCell->GetValue(i))
                                        : static_cast<IGsize>(m_TriangleToFace[i]);
                        cellColorMapper->GetElement(cellId, color);
                        ces->AddElement3(color[0], color[1], color[2]);
                        ces->AddElement3(color[0], color[1], color[2]);
                        ces->AddElement3(color[0], color[1], color[2]);
                    }

                    m_CellColorVBO->Create();
                    m_CellColorVBO->Target(GL_ARRAY_BUFFER);
                    GLAllocateGLBuffer(m_CellColorVBO,
                                       ces->GetNumberOfValues() * sizeof(float),
                                       ces->RawPointer());
                    m_CellColorVBO->Modified();

                    m_CellTriangleVAO->Create();
                    m_CellTriangleVAO->VertexBuffer(
                            GL_VBO_IDX_1, m_CellColorVBO, 0, 3 * sizeof(float));
                    GLSetVertexAttrib(m_CellTriangleVAO, GL_LOCATION_IDX_1,
                                      GL_VBO_IDX_1, 3, GL_FLOAT, GL_FALSE, 0);
                }
            }
        }
    }
    m_RenderWithMeshletChanged = false;
#endif
}

void Meshleter::ReleaseGpuBuffers() {
#ifdef GL_SUPPORTS_MESH_SHADER
    m_MeshletBuffer = GLBuffer::New();
    m_MeshletVertexBuffer = GLBuffer::New();
    m_MeshletTriangleBuffer = GLBuffer::New();

    m_MeshletDescriptorBuffer = GLBuffer::New();
    m_InvisibleMeshletBuffer = GLBuffer::New();

    m_PositionBuffer = GLBuffer::New();
    m_ColorBuffer = GLBuffer::New();
    m_NormalBuffer = GLBuffer::New();
    m_UVBuffer = GLBuffer::New();
#else
    m_TriangleVAO = GLVertexArray::New();
    m_TriangleEBO = GLBuffer::New();

    m_PositionVBO = GLBuffer::New();
    m_ColorVBO = GLBuffer::New();
    m_NormalVBO = GLBuffer::New();
    m_UVVBO = GLBuffer::New();

    m_MeshletDescriptorBuffer = GLBuffer::New();
    m_DrawCommandBuffer = GLBuffer::New();
    m_VisibleMeshletBuffer = GLBuffer::New();
    m_FinalDrawCommandBuffer = GLBuffer::New();
#endif
}

void Meshleter::Build() {}

IGAME_NAMESPACE_END
