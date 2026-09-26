#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
CellCenterFilter 测试模型生成脚本（AI 生成，标准 VTK 3.0 ASCII UNSTRUCTURED_GRID）
产出：
  Examples/Models/CellCenter_hexa_grid.vtk     2 层 3x3 格点 → 4 个六面体 + float/double 属性
  Examples/Models/CellCenter_surface_mixed.vtk 1 层平面三角/四边形 + double 属性
每个单元的中心坐标都简单可算，便于测试断言；属性同时含 float/double，
可覆盖 CellCenterFilter 的几何中心计算、点属性插值、单元属性拷贝（统一 IG_POINT）
以及 Double 数组类型/精度保留（createLikeArray）。
"""
import os

VTK_QUAD = 9
VTK_HEXAHEDRON = 12

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


# 模型 1：2 层 3x3 格点 → 4 个六面体（18 点）
# 格点：z in {0,1}，y,x in {0,1,2}；点 id = x + y*3 + z*9
# 中心分别为 (0.5,0.5,0.5) (1.5,0.5,0.5) (0.5,1.5,0.5) (1.5,1.5,0.5)
pts = []
for z in (0.0, 1.0):
    for y in range(3):
        for x in range(3):
            pts.append((float(x), float(y), z))      # 0..17

cells = []
cell_types = []
for k in range(1):
    for j in range(2):
        for i in range(2):
            p0 = i + j * 3 + k * 9
            hexa = [
                p0, p0 + 1,
                p0 + 1 + 3, p0 + 3,
                p0 + 9, p0 + 1 + 9,
                p0 + 1 + 3 + 9, p0 + 3 + 9,
            ]
            cells.append(tuple(hexa))
            cell_types.append(VTK_HEXAHEDRON)

pid = [float(i) * 10.0 for i in range(len(pts))]
pid_double = [float(i) * 10.0 + 0.125 for i in range(len(pts))]
cid = [100.0, 200.0, 300.0, 400.0]

write_vtk(
    os.path.join(MODELS, "CellCenter_hexa_grid.vtk"),
    "CellCenter hexa grid test model (4 hexahedra, 18 points)",
    pts, cells, cell_types,
    point_scalars=[("pid", "float", pid), ("pid_double", "double", pid_double)],
    cell_scalars=[("cid", "double", cid)],
)


# 模型 2：一层 3x3 平面 → 三角 + 四边形（9 点，z=0）
VTK_TRIANGLE = 5

pts2 = []
for y in range(3):
    for x in range(3):
        pts2.append((float(x), float(y), 0.0))        # 0..8

cells2 = [
    (0, 1, 3),          # 三角形（左下）
    (1, 2, 5, 4),       # 四边形（右下）
    (3, 4, 7, 6),       # 四边形（左上）
    (6, 7, 8),          # 三角形（右上）
]
cell_types2 = [VTK_TRIANGLE, VTK_QUAD, VTK_QUAD, VTK_TRIANGLE]

pid2 = [float(i) for i in range(len(pts2))]
pid2_double = [float(i) * 2.0 + 0.5 for i in range(len(pts2))]
cid2 = [10.0, 20.0, 30.0, 40.0]

write_vtk(
    os.path.join(MODELS, "CellCenter_surface_mixed.vtk"),
    "CellCenter surface mixed test model (triangles + quads, 9 points)",
    pts2, cells2, cell_types2,
    point_scalars=[("pid", "float", pid2), ("pid_double", "double", pid2_double)],
    cell_scalars=[("cid", "double", cid2)],
)
