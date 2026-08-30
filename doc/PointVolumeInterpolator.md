# Point Volume Interpolation

## Overview

`Point Volume Interpolation` interpolates a point attribute of a volume mesh
at a set of query points and outputs a `PointSet` that carries the
interpolated values together with a hit mask attribute.

## Algorithm

- `input[0]` is the volume mesh (a `VolumeMesh`, or an `UnstructuredMesh`
  that is automatically converted to a `VolumeMesh`), and `input[1]` is a
  `PointSet` containing the query points.
- Only **Tetrahedron** and **Hexahedron** cells are supported:
  - Tetrahedra use barycentric (volume) coordinates.
  - Hexahedra use a Newton iteration on the trilinear shape functions in the
    reference unit cube `[0,1]^3`.
- An axis-aligned bounding box is pre-computed for every volume cell and used
  as a fast rejection pass, so a query point only tests cells whose bounding
  box contains it.
- Output is a `PointSet` with:
  - the interpolated point attribute (same dimension as the input attribute),
  - a `HitMask` byte attribute (`1` = the query point is inside the volume
    mesh, `0` = outside; outside points keep a value of `0`).

## Supported meshes

| Mesh type | Support |
| --- | --- |
| `VolumeMesh` | Tetrahedron and Hexahedron cells |
| `UnstructuredMesh` | Converted to `VolumeMesh` first; other 3D cell types are not interpolated |
| Query input | Any `PointSet` (e.g. points, a point cloud, another mesh's points) |

## Example

Source:

```text
Examples/Filter/Interpolation/TestPointVolumeInterpolator.cpp
```

Usage:

```text
testPointVolumeInterpolator
```

The example reads `Models/Tet_Plane.vtk`, verifies that querying all mesh
vertices exactly reproduces the stored point attribute and that tetrahedron
centroids give the average of their four corner values, then samples a
`50 x 50 x 10` grid inside the mesh and shows the hit points colored by the
interpolated value.

## Validation

The same model was processed in iGameVis and ParaView. The resulting
interpolated values are consistent.

### iGameVis result

![Point Volume Interpolation result in iGameVis](images/PointVolumeInterpolator_iGameVis.png)

### ParaView reference

![Point Volume Interpolation reference in ParaView](images/PointVolumeInterpolator_ParaView.png)

## Logging

The filter logs missing inputs, unsupported mesh or cell types, attribute
selection failures and successful completion. Runtime logs are written to:

```text
logs/iGame-core-log.txt
```
