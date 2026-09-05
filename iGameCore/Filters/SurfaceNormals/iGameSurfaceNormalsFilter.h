#pragma once

#include <iGameFilter.h>
#include <iGameSurfaceMesh.h>

IGAME_NAMESPACE_BEGIN

// SurfaceNormalsFilter
// 计算表面网格（Poly Data：三角形 / 四边形 / 多边形面片）的面法向量与点法向量。
//
// 输出属性：
//   面数据 (IG_CELL):
//     - "Normals"           : 3 分量法向量 (IG_NORMAL)，单位化
//     - "Normals_Magnitude" : 1 分量标量 (IG_SCALAR)，法向量模长
//   点数据 (IG_POINT):
//     - "Normals"           : 3 分量法向量 (IG_NORMAL)，单位化
//     - "Normals_Magnitude" : 1 分量标量 (IG_SCALAR)，法向量模长
//
// 仅支持 SurfaceMesh（多边形表面网格）；其他类型返回 false。
class SurfaceNormalsFilter : public Filter {
public:
    I_OBJECT(SurfaceNormalsFilter);
    static Pointer New() { return new SurfaceNormalsFilter; }

    bool Execute() override;

protected:
    SurfaceNormalsFilter();
    ~SurfaceNormalsFilter() override = default;
};

IGAME_NAMESPACE_END
