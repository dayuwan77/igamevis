# Angular Periodic

## Overview

`Angular Periodic` rotates a mesh around a user-defined axis and merges the
original mesh together with `N - 1` rotated copies into a single
`UnstructuredMesh`. It implements the classic angle-periodic replication
pattern (for example turbine blades, radial arrays and other symmetric
geometries).

The default setup replicates the input 3 times (3 copies in total, including
the original) around the Z-axis with a total angle of 360 degrees, so the
copies are evenly spaced.

## Algorithm

- The rotation axis is defined by a point `origin` and a direction `axis`
  (the direction is automatically normalized).
- For each copy `i` (0-based), the total angle is divided evenly and every
  point of the mesh is rotated by `angle * i / copies` degrees around the axis
  using the Rodrigues rotation formula.
- All points of the original mesh are kept; for each rotated copy the
  faces/cells are re-created with the corresponding point-ID offset, so the
  output is a valid merged `UnstructuredMesh`.

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

The example reads `Models/mazewheel.obj`, rotates it 3 times around the
Z-axis (axis origin `(1, 0, 0)`) with a total angle of 360 degrees, draws the
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
