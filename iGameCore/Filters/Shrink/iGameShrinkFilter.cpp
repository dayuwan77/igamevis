#include "iGameShrinkFilter.h"

#include "iGameCell.h"
#include "iGamePoints.h"

#include <algorithm>

IGAME_NAMESPACE_BEGIN

ShrinkFilter::ShrinkFilter() {
	SetNumberOfInputs(1);
	SetNumberOfOutputs(1);
}

void ShrinkFilter::SetShrinkFactor(double factor) {
	m_ShrinkFactor = std::clamp(factor, 0.0, 1.0);
}

double ShrinkFilter::GetShrinkFactor() const { return m_ShrinkFactor; }

bool ShrinkFilter::CopyPointAttributes(PointSet* pointSet, const std::vector<IGsize>& srcOfNew) {
	auto attrs = pointSet->GetAttributeSet();
	if (attrs == nullptr || srcOfNew.empty()) return true;

	IGsize nAttrs = attrs->GetNumberOfAttributes();
	for (IGsize i = 0; i < nAttrs; i++) {
		auto& a = attrs->GetAttribute(i);
		if (a.isDeleted || a.attachmentType != IG_POINT || a.pointer.IsNull()) continue;
		auto newArr = CloneArray(a.pointer, srcOfNew);
		if (newArr.IsNull()) continue;

		a.pointer = newArr;
		a.UpdateAllDataRange();
	}
	return true;
}

ArrayObject::Pointer ShrinkFilter::CloneArray(ArrayObject::Pointer src,
                                              const std::vector<IGsize>& srcOfNew) {
	ArrayObject::Pointer dst;
	switch (src->GetArrayType()) {
		case IG_CharArray: dst = CharArray::New(); break;
		case IG_UnsignedCharArray: dst = UnsignedCharArray::New(); break;
		case IG_ShortArray: dst = ShortArray::New(); break;
		case IG_UnsignedShortArray: dst = UnsignedShortArray::New(); break;
		case IG_IntArray: dst = IntArray::New(); break;
		case IG_UnsignedIntArray: dst = UnsignedIntArray::New(); break;
		case IG_LongLongArray: dst = LongLongArray::New(); break;
		case IG_UnsignedLongLongArray: dst = UnsignedLongLongArray::New(); break;
		case IG_FloatArray: dst = FloatArray::New(); break;
		case IG_DoubleArray: dst = DoubleArray::New(); break;
		default: return nullptr;
	}

	IGsize dim = src->GetDimension();
	if (dim < 1) dim = 1;
	dst->SetName(src->GetName());
	dst->SetDimension(static_cast<int>(dim));
	IGsize newCount = srcOfNew.size();

	dst->Resize(newCount);
	for (IGsize i = 0; i < newCount; i++) {
		IGsize s = srcOfNew[i];
		for (IGsize c = 0; c < dim; c++) {
			dst->SetValue(i * dim + c, src->GetValue(s * dim + c));
		}
	}
	return dst;
}

bool ShrinkFilter::Execute() {
	auto input = GetInput(0);
	if (input.IsNull()) return false;

	auto shrinkCells = [&](PointSet* pointSet, IGsize count, CellArray* cells,
	                       auto getCellPointIds) -> bool {
		if (pointSet == nullptr || cells == nullptr || count == 0) return false;

		auto oldPoints = pointSet->GetPoints();
		if (oldPoints.IsNull()) return false;

		auto newPoints = Points::New();
		std::vector<IGsize> srcOfNew;

		igIndex ids[IGAME_CELL_MAX_SIZE];
		for (IGsize c = 0; c < count; c++) {
			int n = getCellPointIds(c, ids);
			if (n <= 0 || n > IGAME_CELL_MAX_SIZE) return false;

			double cx = 0.0, cy = 0.0, cz = 0.0;
			for (int k = 0; k < n; k++) {
				const auto& p = oldPoints->GetPoint(ids[k]);
				cx += p[0];
				cy += p[1];
				cz += p[2];
			}
			cx /= n;
			cy /= n;
			cz /= n;

			igIndex newIds[IGAME_CELL_MAX_SIZE];
			for (int k = 0; k < n; k++) {
				const auto& p = oldPoints->GetPoint(ids[k]);
				float nx = static_cast<float>(cx + (p[0] - cx) * m_ShrinkFactor);
				float ny = static_cast<float>(cy + (p[1] - cy) * m_ShrinkFactor);
				float nz = static_cast<float>(cz + (p[2] - cz) * m_ShrinkFactor);
				newIds[k] = static_cast<igIndex>(newPoints->AddPoint(nx, ny, nz));
				srcOfNew.push_back(ids[k]);
			}

			cells->SetCellIds(c, newIds, n);

			if ((c & 0x3FF) == 0) {
				UpdateProgress(static_cast<double>(c) / static_cast<double>(count));
			}
		}

		pointSet->SetPoints(newPoints);
		CopyPointAttributes(pointSet, srcOfNew);
		return true;
	};

	// 体网格
	if (auto mesh = DynamicCast<VolumeMesh>(input)) {
		if (!shrinkCells(mesh, mesh->GetNumberOfVolumes(), mesh->GetVolumes(),
		                 [mesh](IGsize c, igIndex* ids) { return mesh->GetVolumePointIds(c, ids); })) {
			return false;
		}
		UpdateProgress(1.0);
		SetOutput(0, input);
		return true;
	}
	// 表面网格
	if (auto mesh = DynamicCast<SurfaceMesh>(input)) {
		if (!shrinkCells(mesh, mesh->GetNumberOfFaces(), mesh->GetFaces(),
		                 [mesh](IGsize c, igIndex* ids) { return mesh->GetFacePointIds(c, ids); })) {
			return false;
		}
		UpdateProgress(1.0);
		SetOutput(0, input);
		return true;
	}
	// 非结构化网格
	if (auto mesh = DynamicCast<UnstructuredMesh>(input)) {
		if (!shrinkCells(mesh, mesh->GetNumberOfCells(), mesh->GetCellArray(),
		                 [mesh](IGsize c, igIndex* ids) { return mesh->GetCellPointIds(c, ids); })) {
			return false;
		}
		UpdateProgress(1.0);
		SetOutput(0, input);
		return true;
	}
	return false;
}

IGAME_NAMESPACE_END
