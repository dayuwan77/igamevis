# CGNS Resample To Image Regression

## Scope

The DrivAer NGON/NFACE case originally produced no output. Its desktop build
had CGNS disabled, and the resampler did not support native polyhedron cells.
The repair enables the local CGNS build and probes native volume topology
without expanding all shared faces into a second mesh.

CGNS double-precision coordinates are retained in metadata for probing while
the existing float coordinates remain the rendering representation. The saved
coordinates are used only while the Points modification time is unchanged.
Numbered CGNS scalar fields remain independent; X/Y/Z fields still form vectors.

Points representation draws the extracted visible-cell surface, as ParaView's
geometry representation does, NOT all valid image samples. The output point
size defaults to two pixels, without changing other objects' defaults.
An entirely blank image draws no points without changing its dataset coordinates.
Pure surface Points mode tolerates an absent edge array; wireframe still creates
edges lazily. An earlier repair incorrectly drew all valid samples, which filled
the interior and made the image appear much denser. That repair was removed
following the user's screenshot comparison on 2026-10-14.

## Compatibility And Limits

Native and face-stream polyhedra currently support **cell fields only**.
Point-field mean-value interpolation is not implemented and fails explicitly.

`SetUseVTKPolyhedronCompatibility(true)` is the default. It reproduces the
observed ParaView 6.2.0-RC1 / VTK 9.7.0 polyhedron probing behavior: acceptance
within a cell's bounding box. This is NOT strict geometric containment. A point
outside a slanted or concave cell can still be accepted. The installed VTK
confirms this on a tetrahedron represented as a polyhedron: `(0.75,0.75,0.75)`
returns `EvaluatePosition == 1` but `IsInside(point, 0) == 0`.

`SetUseVTKPolyhedronCompatibility(false)` uses triangulated-boundary ray tests
instead. It does not reproduce the legacy infinite-tolerance VTK behavior;
general nonplanar/concave polygon triangulation is not an exact VTK port.

Both modes process source cells deterministically in input order. Parallel VTK
may choose a different cell where accepted regions overlap. Do not interpret
this ordering difference as floating-point roundoff or claim bitwise parity
with every parallel run. Multi-zone composite resampling remains unsupported.

## Reproduction

Initialize the CGNS submodule and configure the project's existing build with
`ENABLE_CGNS_MODULE=ON`, a valid `HDF5_DIR`, and
`IGAME_BUILD_FILTER_CHECKS=ON`. Shared HDF5 builds need their runtime DLLs beside
the executable or on PATH. The local tested HDF5 build uses `hdf5.dll` and
`zlib.dll`. No third-party source was modified.

Build `iGameVis`, `testCGNSResample`, `testResamplePolyhedronChecks`, and
`testResampleOctreeChecks` in Release. Run the latter from `Examples` so its
`Models` fixtures resolve. The CGNS exporter accepts:

```text
testCGNSResample MODEL.cgns OUTPUT_DIRECTORY [--render-check]
```

The optional rendering check creates a hidden OpenGL 4.6 context and exercises
the production Scene/Painter pipeline. It checks framebuffer differences for
solid points and Pressure coloring; it does not take desktop screenshots.

## Verified Case

Date: 2026-10-14. Fix commit subject:
`fix: correct CGNS resampling and align Resample To Image UI`.

Input: `DrivAer_fastback_base_0.4_remesh_coarse_kw_CPU_test_P_V.cgns`:
7,730,570 points, 7,230,142 polyhedra, one zone, 43 cell-centered fields.
Parameters: 100 x 100 x 100, UseInputBounds=true, default tolerance.

Output: 1,000,000 points, 970,299 cells, 999,514 valid points,
486 hidden points, 776 hidden cells. Pressure, Velocity, VelocityMagnitude,
Velocity_0/1/2, valid mask, point ghosts, and cell ghosts match the sequential
ParaView reference exactly. The captured parallel reference differs at six
overlapping-cell ownership points; all masks match exactly. Other fields are
exported but were not independently compared.

Display: both programs use exactly 59,358 original grid point ids, including
points on boundaries of blank regions. The one-million-point image remains
unchanged. With the same 640 x 480 viewport, perspective camera, white color,
point size 2, and four-sample MSAA, foreground pixel IoU is 99.998% (two pixel
classification differences). This is not a claim of identical default settings:
the tested ParaView default is no MSAA, whereas iGameVis uses four samples.
The mismatched-AA capture has 87.55% foreground IoU, despite identical geometry.
No global camera or antialiasing defaults were changed. Desktop screenshots
remain a user check.

The detailed scripts, references, logs, and comparison JSON are outside the
repository at `D:/iGame/resample-debug/20261013-cgns`.
