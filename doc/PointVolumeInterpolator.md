# Point Volume Interpolator

## Overview

`Point Volume Interpolator` interpolates the point arrays of a point cloud (or
any dataset, using its points) onto a **regular volume grid**, using a
**kernel-based scattered interpolation** — the same model as ParaView's filter
of the same name (`vtkPointInterpolator`).

Unlike a probe filter (which locates the cell containing a query point and uses
cell shape functions), this filter only uses the **points** of the input; no
cell connectivity is required.

## Algorithm

- Input: a single dataset whose **points** form the interpolation samples `Pc`
  (any `PointSet` subclass). All `POINT` arrays of the input are interpolated.
- Output: a `StructuredMesh` (iGame's equivalent of `vtkImageData`) covering the
  input bounding box (or explicit bounds), with the interpolated point arrays and
  a `CharArray` mask named `vtkValidPointMask` (`1` = valid, `0` = null point).
- Neighbour search uses a KD-tree (`nanoflann`), with two footprints:
  - `Radius`: all points within `Radius` of the target point;
  - `N Closest`: the `NumberOfPoints` nearest samples.
- Kernels (weights are normalised by their sum):

| Kernel | Weight | Notes |
| --- | --- | --- |
| Voronoi | `1` (closest point) | default; exact at coincident points |
| Gaussian | `exp(-(Sharpness * r / Radius)^2)` | `Sharpness` default 2.0 |
| Shepard | `1 / r^PowerParameter` | inverse-distance, power default 2.0 |
| Linear | `1` (uniform average) | "averages the contributions of all points in the basis" |

### Kernels not implemented

- `EllipsoidalGaussian` is intentionally **not** implemented: it requires a
  per-point normal (and optional scalar) to form an anisotropic ellipsoidal
  metric, i.e. normals must exist or be estimated first, and the kernel cannot be
  expressed with the isotropic distance used here.
- The SPH family (`vtkSPHKernel` and subclasses) is **not provided** either: it is
  specialised for smoothed-particle-hydrodynamics data (mass/density arrays,
  smoothing length `h`) and VTK itself notes it is not a general-purpose
  interpolation kernel. This filter targets general mesh/point-cloud
  interpolation.
- `ProbabilisticVoronoi` is **not provided**: without a probability array it is
  identical to Voronoi (VTK: "provides the same results as vtkVoronoiKernel,
  except a less efficiently"), and ParaView does not expose the probability array
  in the UI, so it would only add a redundant option.

The supported ParaView kernels are: Voronoi, Gaussian, Shepard and Linear.

- Null points (empty neighbourhood) are handled by `NullPointsStrategy`:
  - `MaskPoints`: output set to `NullValue`, mask `0`;
  - `NullValue`: output set to `NullValue` (default);
  - `ClosestPoint`: fall back to the nearest sample.
- Coincident target points: Voronoi returns the nearest point
  (exact); Shepard uses its `d=0` limit (single point); Gaussian / Linear
  follow their formulas, matching VTK.
- A hard cap rejects extremely large grids (more than 1e8 grid points); the GUI
  additionally warns before running beyond 2e6 grid points.

## Parameters

| Parameter | Default | Meaning |
| --- | --- | --- |
| `KernelType` | `Voronoi` | `Voronoi` / `Gaussian` / `Shepard` |
| `KernelFootprint` | `Radius` | `Radius` / `NClosest` |
| `Radius` | `1.0` | neighbourhood radius (Radius footprint) |
| `NumberOfPoints` | `8` | neighbour count (N Closest footprint) |
| `Sharpness` | `2.0` | Gaussian falloff |
| `PowerParameter` | `2.0` | Shepard exponent |
| `InterpolateArrayNames` | empty (all) | only interpolate the listed POINT arrays |
| `NullPointsStrategy` | `NullValue` | null-point handling |
| `NullValue` | `0.0` | value written for null points |
| `UseInputBounds` | `true` | sample the input bounding box (else `SamplingBounds`) |
| `SamplingBounds` | `{0,1,0,1,0,1}` | explicit sampling box |
| `Resolution` | `64 x 64 x 64` | output grid dimensions |

> ParaView's `BoundedVolumeSource` defaults to `100 x 100 x 100`; iGame's default
> is `64 x 64 x 64` because `StructuredMesh` stores explicit cells.

## Supported input

| Input | Support |
| --- | --- |
| `PointSet` (point cloud, `POLYDATA`/vertex cells) | points + `POINT` arrays |
| `UnstructuredMesh` / `SurfaceMesh` / `VolumeMesh` / `StructuredMesh` | only their points/point arrays are used; cells are ignored |
| `CELL` arrays | not interpolated |

## Example

Sources:

```text
Examples/Filter/Interpolation/TestPointVolumeInterpolator.cpp           (GUI)
Examples/Filter/Interpolation/TestPointVolumeInterpolatorSelfCheck.cpp  (headless)
```

Models (procedurally generated point clouds with `field` scalar and `momentum`
vector):

```text
Examples/Models/AIGen_Points_ScatterCloud.vtk      (POLYDATA)
Examples/Models/AIGen_Points_VertexCloud.vtk  (UNSTRUCTURED_GRID + VTK_VERTEX)
```

Run (working directory is `Examples`, paths are fixed — no manual input):

```text
testPointVolumeInterpolator            # reads PolyCloud, Gaussian/Radius, 64^3, shows the volume
testPointVolumeInterpolatorSelfCheck   # headless assertions, prints PASS/FAIL, returns 0/1
```

## Validation

The headless self check covers 36 assertions:

1. Voronoi reproduces scalar and vector exactly at coincident grid points, and
   `vtkValidPointMask` is 1;
2. a constant field is preserved for Voronoi / Gaussian / Shepard / Linear
   (weight normalisation);
3. `N Closest` produces finite values and marks all points valid;
4. the three null-point strategies behave as specified (mask 0 / `NullValue` /
   nearest fallback);
5. array selection interpolates only the chosen arrays, and an over-large
   resolution is rejected;
6. null points do not corrupt valid multi-component (vector) values — regression
   for using `SetValue` (scalar index) instead of `SetElement` (tuple index).

The GUI example reports the output grid, cell count and mask hits, e.g.

```text
input cloud: 2000 points
output volume: 64 x 64 x 64, points 262144, cells 250047, valid 262144 / 262144
```

## Notes

- Only `POINT` arrays are interpolated; output arrays are `FloatArray` with the
  same dimension as the source.
- The input's cells are ignored — a point cloud (even without cells) is enough.
- At high resolutions the explicit `StructuredMesh` cells dominate memory and
  build time (KD-tree only speeds up neighbour search, not cell storage).
