#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ExtractCellsByTypeFilter 测试模型生成脚本（AI 生成，标准 VTK 3.0 ASCII UNSTRUCTURED_GRID）
产出：
  Examples/Models/ExtractCellsByType_mixed.vtk    混合单元(三角+四边形+四面体+六面体) + 属性
  Examples/Models/ExtractCellsByType_surface.vtk  纯表面(三角+四边形) + 属性
格式与 iGameVTKWriter / iGameVTKAbstractReader 完全一致，可直接被 FileIO::ReadFile 读取。
"""
import os

# VTK 单元类型号（与 iGameVTKAbstractReader.h 对齐）
VTK_TRIANGLE, VTK_QUAD = 5, 9
VTK_TETRA, VTK_HEXAHEDRON = 10, 12

HERE = os.path.dirname(os.path.abspath(__file__))
MODELS = os.path.join(HERE, "Examples", "Models")


def fmt_val(v):
    return repr(v) if isinstance(v, float) else str(v)


def write_vtk(path, title, points, cells, cell_types,
              point_scalars=None, cell_scalars=None):
    lines = []
    lines.append("# vtk DataFile Version 3.0")
    lines.append(title)
    lines.append("ASCII")
    lines.append("DATASET UNSTRUCTURED_GRID")
    lines.append(f"POINTS {len(points)} float")
    for p in points:
        lines.append(f"{fmt_val(p[0])} {fmt_val(p[1])} {fmt_val(p[2])}")
    nid = sum(len(c) + 1 for c in cells)
    lines.append(f"CELLS {len(cells)} {nid}")
    for c in cells:
        lines.append(str(len(c)) + " " + " ".join(str(i) for i in c))
    lines.append(f"CELL_TYPES {len(cell_types)}")
    for t in cell_types:
        lines.append(str(t))
    if point_scalars:
        lines.append(f"POINT_DATA {len(points)}")
        for name, dtype, vals in point_scalars:
            lines.append(f"SCALARS {name} {dtype} 1")
            lines.append("LOOKUP_TABLE default")
            for v in vals:
                lines.append(fmt_val(v))
    if cell_scalars:
        lines.append(f"CELL_DATA {len(cell_types)}")
        for name, dtype, vals in cell_scalars:
            lines.append(f"SCALARS {name} {dtype} 1")
            lines.append("LOOKUP_TABLE default")
            for v in vals:
                lines.append(fmt_val(v))
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    print(f"wrote {path} ({os.path.getsize(path)} bytes)")


# =====================================================================
# 模型 1：混合单元 —— 三角 + 四边形 + 四面体 + 六面体（18 点，两层 3x3 格点）
# =====================================================================
pts = []
for z in (0.0, 1.0):
    for y in range(3):
        for x in range(3):
            pts.append((float(x), float(y), z))   # 0..17

cells = [
    (0, 1, 3),                       # 三角形
    (0, 1, 4, 3),                    # 四边形
    (0, 1, 3, 9),                    # 四面体
    (0, 1, 3, 4, 9, 10, 12, 13),     # 六面体
]
cell_types = [VTK_TRIANGLE, VTK_QUAD, VTK_TETRA, VTK_HEXAHEDRON]

pid = [i * 10.0 for i in range(len(pts))]
pid_double = [i * 10.0 + 0.5 for i in range(len(pts))]
cid = [100.0, 200.0, 300.0, 400.0]

write_vtk(
    os.path.join(MODELS, "ExtractCellsByType_mixed.vtk"),
    "ExtractCellsByType mixed test model (triangle+quad+tetra+hexahedron)",
    pts, cells, cell_types,
    point_scalars=[("pid", "float", pid), ("pid_double", "double", pid_double)],
    cell_scalars=[("cid", "double", cid)],
)


# =====================================================================
# 模型 2：纯表面 —— 三角 + 四边形（9 点，一层 3x3 格点，z=0）
# =====================================================================
pts2 = []
for y in range(3):
    for x in range(3):
        pts2.append((float(x), float(y), 0.0))    # 0..8

cells2 = [
    (0, 1, 3),          # 三角形
    (1, 2, 5, 4),       # 四边形
    (3, 4, 7, 6),       # 四边形
    (6, 7, 8),          # 三角形
]
cell_types2 = [VTK_TRIANGLE, VTK_QUAD, VTK_QUAD, VTK_TRIANGLE]

pid2 = [float(i) for i in range(len(pts2))]
pid2_double = [i * 2.0 + 0.25 for i in range(len(pts2))]
cid2 = [10.0, 20.0, 30.0, 40.0]

write_vtk(
    os.path.join(MODELS, "ExtractCellsByType_surface.vtk"),
    "ExtractCellsByType surface test model (triangle+quad)",
    pts2, cells2, cell_types2,
    point_scalars=[("pid", "float", pid2), ("pid_double", "double", pid2_double)],
    cell_scalars=[("cid", "double", cid2)],
)
