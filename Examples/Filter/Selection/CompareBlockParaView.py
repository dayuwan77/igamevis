"""Run with pvpython from the project root after BlockExtractionExamples."""
import json
from pathlib import Path

from paraview import servermanager
from paraview.simple import (
    ColorBy, CreateLayout, CreateView, ExtractBlock, LegacyVTKReader,
    RenderAllViews, ResetCamera, SaveScreenshot, SaveState, Show, Text,
    XMLMultiBlockDataReader,
)
from CompareBlockVTK import compare, first_leaf, read_mesh


def main():
    models = Path("Examples/Models").resolve()
    results = Path("ExtractionResults").resolve()
    evidence = results / "paraview-block"
    evidence.mkdir(parents=True, exist_ok=True)
    reports = []
    for filename in ("ExtractBlockFlat.vtm", "ExtractBlockNested.vtm"):
        source = XMLMultiBlockDataReader(registrationName=filename,
                                        FileName=[str(models / filename)])
        native = ExtractBlock(registrationName="Native Extract Block " + filename, Input=source)
        native.Selectors = ["//ExtractSelectionTetrahedra"]
        native.UpdatePipeline()
        fetched = servermanager.Fetch(native)
        iterator = fetched.NewIterator()
        iterator.InitTraversal()
        count = 0
        while not iterator.IsDoneWithTraversal():
            if iterator.GetCurrentDataObject() is not None:
                count += 1
            iterator.GoToNextItem()
        if count != 1:
            raise RuntimeError("Expected exactly one extracted leaf block")
        expected = first_leaf(fetched)
        actual_path = results / (filename + ".vtk")
        actual = read_mesh(actual_path)
        compare(actual, expected)
        imported = LegacyVTKReader(registrationName="iGame result " + filename,
                                   FileNames=[str(actual_path)])
        layout = CreateLayout(name="Block comparison " + filename)
        layout.SplitHorizontal(0, 0.5)
        for index, (data, title, mesh) in enumerate((
            (imported, "iGame Extract Block output", actual),
            (native, "ParaView native Extract Block output", expected),
        )):
            view = CreateView("RenderView")
            layout.AssignView(index + 1, view)
            display = Show(data, view)
            display.Representation = "Surface With Edges"
            ColorBy(display, None)
            display.DiffuseColor = [0.25, 0.73, 0.70]
            display.EdgeColor = [0.9, 0.9, 0.9]
            view.Background = [0.14, 0.15, 0.16]
            label = Text(registrationName=title)
            label.Text = f"{title}\n{mesh.GetNumberOfPoints()} points, {mesh.GetNumberOfCells()} cells"
            representation = Show(label, view)
            representation.WindowLocation = "Upper Left Corner"
            representation.FontSize = 18
            view.CameraPosition = [3, 2, 4]
            view.CameraFocalPoint = [0.3, 0.3, 0]
            view.CameraViewUp = [0, 0, 1]
            ResetCamera(view)
        layout.SetSize(1600, 800)
        RenderAllViews()
        SaveScreenshot(str(evidence / (filename + ".png")), layout, ImageResolution=[1600, 800])
        reports.append({"file": filename, "passed": True,
                        "points": actual.GetNumberOfPoints(), "cells": actual.GetNumberOfCells()})
        print("PASS ParaView " + filename + ": geometry, connectivity and attributes")
    SaveState(str(evidence / "block-comparison.pvsm"))
    (evidence / "report.json").write_text(json.dumps(reports, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
