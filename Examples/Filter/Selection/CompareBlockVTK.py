"""Compare 29 block results with native VTK; run from the project root."""
import json
import math
from pathlib import Path
import vtk


def read_mesh(path):
    reader = vtk.vtkUnstructuredGridReader()
    reader.SetFileName(str(path))
    reader.ReadAllScalarsOn()
    reader.ReadAllVectorsOn()
    reader.Update()
    result = vtk.vtkUnstructuredGrid()
    result.DeepCopy(reader.GetOutput())
    if result.GetNumberOfPoints() == 0:
        raise RuntimeError(f"Could not read mesh: {path}")
    return result


def first_leaf(data):
    if isinstance(data, vtk.vtkDataSet):
        return data
    iterator = data.NewIterator()
    iterator.InitTraversal()
    while not iterator.IsDoneWithTraversal():
        item = iterator.GetCurrentDataObject()
        if isinstance(item, vtk.vtkDataSet):
            return item
        iterator.GoToNextItem()
    raise RuntimeError("VTK produced no leaf block")


def extract_block(path, flat_index):
    reader = vtk.vtkXMLMultiBlockDataReader()
    reader.SetFileName(str(path))
    reader.Update()
    algorithm = vtk.vtkExtractBlock()
    algorithm.SetInputData(reader.GetOutput())
    algorithm.AddIndex(flat_index)
    algorithm.Update()
    result = vtk.vtkUnstructuredGrid()
    result.DeepCopy(first_leaf(algorithm.GetOutput()))
    return result


def close(a, b):
    return math.isclose(a, b, abs_tol=1e-6, rel_tol=1e-6)


def compare(actual, expected):
    assert actual.GetNumberOfPoints() == expected.GetNumberOfPoints(), "point count"
    assert actual.GetNumberOfCells() == expected.GetNumberOfCells(), "cell count"
    # Cell extraction may compact points in a different order. Match by geometry.
    mapping = {}
    for i in range(actual.GetNumberOfPoints()):
        matches = [j for j in range(expected.GetNumberOfPoints())
                   if all(close(a, b) for a, b in zip(actual.GetPoint(i), expected.GetPoint(j)))]
        assert len(matches) == 1, "point coordinates must match uniquely in these fixtures"
        mapping[i] = matches[0]
    for i in range(actual.GetNumberOfCells()):
        assert actual.GetCellType(i) == expected.GetCellType(i), "cell type"
        ac, ex = actual.GetCell(i), expected.GetCell(i)
        assert ac.GetNumberOfPoints() == ex.GetNumberOfPoints(), "cell size"
        assert [mapping[ac.GetPointId(j)] for j in range(ac.GetNumberOfPoints())] == [
            ex.GetPointId(j) for j in range(ex.GetNumberOfPoints())], "connectivity"
    for attachment in ("Point", "Cell"):
        a_attrs = getattr(actual, f"Get{attachment}Data")()
        e_attrs = getattr(expected, f"Get{attachment}Data")()
        actual_names = {a_attrs.GetArrayName(i) for i in range(a_attrs.GetNumberOfArrays())}
        expected_names = {e_attrs.GetArrayName(i) for i in range(e_attrs.GetNumberOfArrays())}
        expected_names -= {"vtkOriginalPointIds", "vtkOriginalCellIds"}
        assert actual_names == expected_names, f"attribute names differ: {attachment}"
        for i in range(a_attrs.GetNumberOfArrays()):
            a = a_attrs.GetArray(i)
            e = e_attrs.GetArray(a.GetName())
            assert e is not None, f"missing attribute: {a.GetName()}"
            assert a.GetNumberOfComponents() == e.GetNumberOfComponents(), "attribute dimensions"
            assert a.GetNumberOfTuples() == e.GetNumberOfTuples(), "attribute length"
            for row in range(a.GetNumberOfTuples()):
                target = mapping[row] if attachment == "Point" else row
                assert all(close(x, y) for x, y in zip(a.GetTuple(row), e.GetTuple(target))), a.GetName()


def main():
    results = Path("ExtractionResults")
    models = Path("Examples/Models")
    cases = []
    for filename, flat_index in (("ExtractBlockFlat.vtm", 2), ("ExtractBlockNested.vtm", 4)):
        expected = extract_block(models / filename, flat_index)
        actual = read_mesh(results / (filename + ".vtk"))
        compare(actual, expected)
        cases.append({"file": filename, "passed": True,
                      "points": actual.GetNumberOfPoints(), "cells": actual.GetNumberOfCells()})
        print("PASS native VTK " + filename + ": geometry, connectivity and attributes")
    (results / "block_vtk_report.json").write_text(
        json.dumps({"vtk_version": vtk.vtkVersion.GetVTKVersion(), "cases": cases}, indent=2),
        encoding="utf-8")


if __name__ == "__main__":
    main()
