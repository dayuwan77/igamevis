"""Run with pvpython from the project root after BlockExtractionExamples."""
import json
from pathlib import Path

import vtk
from paraview import servermanager
from paraview.simple import ExtractBlock, XMLMultiBlockDataReader
from CompareBlockVTK import compare, first_leaf, read_mesh


def main():
    path = Path("Examples/Models/ExtractBlockThreeTypes.vtm").resolve()
    source = XMLMultiBlockDataReader(FileName=[str(path)])
    source.UpdatePipeline()
    reader = vtk.vtkXMLMultiBlockDataReader()
    reader.SetFileName(str(path))
    reader.Update()
    reports = []
    for name, flat_index, expected_type in (("Triangles", 1, 5), ("Tetrahedra", 3, 10), ("Hexahedron", 4, 12)):
        native_vtk = vtk.vtkExtractBlock()
        native_vtk.SetInputData(reader.GetOutput())
        native_vtk.AddIndex(flat_index)
        native_vtk.Update()
        native_pv = ExtractBlock(Input=source)
        native_pv.Selectors = ["//" + name]
        native_pv.UpdatePipeline()
        for engine, data in (("VTK", native_vtk.GetOutput()), ("ParaView", servermanager.Fetch(native_pv))):
            expected = first_leaf(data)
            assert expected.GetCellType(0) == expected_type
            for mode in ("path", "name"):
                actual = read_mesh(Path("ExtractionResults") / (name + "-" + mode + ".vtk"))
                compare(actual, expected)
                reports.append(dict(engine=engine, block=name, mode=mode, passed=True,
                                    points=actual.GetNumberOfPoints(), cells=actual.GetNumberOfCells(),
                                    cell_type=expected_type))
                print(f"PASS {engine}: {name}, {mode}")
    result = dict(vtk_version=vtk.vtkVersion.GetVTKVersion(),
                  scope="Leaf geometry, connectivity and numeric values; not identical container layout or writer storage types",
                  cases=reports)
    Path("ExtractionResults/block-three-types-report.json").write_text(json.dumps(result, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
