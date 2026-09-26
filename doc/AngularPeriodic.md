# Angular Periodic

## Overview

`Angular Periodic` rotates a mesh around a user-defined axis and merges the
original mesh together with `N - 1` rotated copies into a single
`UnstructuredMesh`. It implements the classic angle-periodic replication
pattern (for example turbine blades, radial arrays and other symmetric
geometries).

The default setup replicates the input 4 times (4 periods in total, including
the original) around the Z-axis with a period angle of 90 degrees, so the
copies appear at 0, 90, 180 and 270 degrees. The parameter semantics match
ParaView's `vtkAngularPeriodicFilter`.

## Algorithm

- The rotation axis is defined by a point `origin` and a direction `axis`
  (the direction is automatically normalized).
- For each period `i` (0-based), every point of the mesh is rotated by
  `i * angle` degrees around the axis using the Rodrigues rotation formula
  (`angle` is the per-period angle between two consecutive copies).
- The number of periods follows `vtkPeriodicFilter::IterationMode`:
  `DIRECT_NB` uses the user-provided period count, while `MAX` takes the
  largest count that does not exceed a full revolution (`floor(360 / angle)`).
  The angular coverage (`full period` / `gap` / `overlap`) is reported through
  `GetCoverageInfo()`; a gap or overlap is only a warning unless
  `SetRequireFullPeriod(true)` is set.
- Point and cell data arrays are replicated per period and rotated according
  to their attribute semantic: 3-component vectors/normals (`IG_VECTOR`,
  `IG_NORMAL`) are rotated with the geometry and tensors (`IG_TENSOR`, 9- or
  6-component) are transformed as `R * T * R^T`. Scalars and all other arrays
  are copied unchanged.
- All points of the original mesh are kept; for each rotated copy the
  faces/cells are re-created with the corresponding point-ID offset, so the
  output is a valid merged `UnstructuredMesh`.
- In the GUI the panel previews the rotation axis live while it is open
  (removed on close), and the copied model inherits the input's color mapping,
  view style and currently colored attribute (matched by name), so the scalar
  / heat-map visualization is preserved.

## Supported meshes

| Mesh type | Support |
| --- | --- |
| `UnstructuredMesh` / `SurfaceMesh` (point-based mesh, `PointSet`) | Yes |
| Structured mesh or point cloud without topology | Points are rotated and copied; cells are copied when a cell array is available |

## Example

Source:

```text
Examples/Filter/Periodic/TestAngularPeriodic.cpp
```

Usage:

```text
testAngularPeriodic
```

The example reads `Models/AIGen_Surface_RingSector.obj` (a 120 degree ring
sector), rotates it 3 times around the Z-axis (axis origin `(1, 0, 0)`) with a
period angle of 120 degrees (0/120/240, exactly filling the ring), draws the
rotation axis and the XYZ coordinate axes, and shows the merged mesh in a
rendering window.

## Validation

The same model was processed in iGameVis and ParaView. The resulting merged
meshes are consistent.

## Logging

The filter logs missing input, an invalid rotation axis or copy count and
successful completion. Runtime logs are written to:

```text
logs/iGame-core-log.txt
```
