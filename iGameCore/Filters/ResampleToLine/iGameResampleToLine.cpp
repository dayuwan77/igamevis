#include "iGameResampleToLine.h"

IGAME_NAMESPACE_BEGIN



bool ResampleToLine::Execute() {
    auto input = GetInput(0);
    if (input == nullptr) {
        m_Message = "未选择";

        return false;
    }
    if (n <= 1) { 
        m_Message = "n must be greater than 1";
        return false;
    }

    /*auto CheckType = [&]() -> bool {
        attributeSet = input->GetAttributeSet();
        if (attributeSet == nullptr) {
            m_Message = "please choose a  attribute";
            return false;
        }
        if (curIndex == -1 && name == "") {
            m_Message = "please choose a attribute";
            return false;
        }
        if (curIndex == -1) curIndex = attributeSet->GetAttributeIndex(name);
        if (curIndex < 0 || curIndex >= attributeSet->GetNumberOfAttributes()) {
            m_Message = "please choose a attribute";
            return false;
        }

        dim = input->GetAttributeSet()->GetAttribute(curIndex).pointer->GetDimension();
        m_currentAttributeDimension = input->GetCurrentAttributeDimension();
        if (dim != 1 && m_currentAttributeDimension == -1) {
            m_Message = "please choose a component";
            return false;
        }
        if (dim == 1) m_currentAttributeDimension = 0;
        return true;
    };*/

    // SetOutput(input);
    if (input->GetDataObjectType() == IG_UNSTRUCTURED_MESH){
        auto mesh = DynamicCast<UnstructuredMesh>(input);
        auto output = resample_to_line_UnstructuredMesh(mesh, orig, target, n);
        SetOutput(output);
        return true;
    
    } else {
        m_Message = "please input UnstructuredMesh";
        return false;
    }
    
    //switch (input->GetDataObjectType()) {
    //    std::cout << input->GetDataObjectType()<<std::endl;
    //    /*case IG_SURFACE_MESH: {
    //        surface_Mesh = DynamicCast<SurfaceMesh>(input);*/
    //        //if (!CheckType()) return false;
    //        //return ComputeSurfaceCurvatureCotangent(surface_Mesh, attributeSet, curIndex);
    //        // attributeSet = surface_Mesh->GetAttributeSet();
    //        // if (attributeSet == nullptr) return false;
    //        //
    //        // auto attachmentType = attributeSet->GetAttribute(curIndex).attachmentType;
    //        //
    //        // int FaceNum = surface_Mesh->GetNumberOfFaces();
    //        // int PointNum = surface_Mesh->GetNumberOfPoints();
    //        // Points::Pointer Points = surface_Mesh->GetPoints();
    //        // surface_Mesh->RequestEditStatus();
    //        // if (PointNum != 0 && attachmentType == 0) {
    //        //     surface_Mesh = TriangulateSurfaceMesh(surface_Mesh);
    //        //     return ComputeSurfaceCurvatureCotangent(surface_Mesh, attributeSet, curIndex);
    //        // }
    //        // // 附着在cell
    //        // else if (FaceNum != 0 && attachmentType == 1)
    //        //     return GetOtherCurvature(0, FaceNum);
    //    //} break;
    //    case IG_VOLUME_MESH: {
    //        return false;
    //    } break;
    //    case IG_UNSTRUCTURED_MESH: {
    //        auto mesh = DynamicCast<UnstructuredMesh>(input);
    //        auto output = resample_to_line_UnstructuredMesh(mesh, orig, target, n);
    //        SetOutput(output);
    //        
    //    } break;
    //    default:
    //        return false;
    //}
    return false;
}

//判断点是否在单元内
bool ResampleToLine::IsPointInCell(const Point& p, const std::vector<Point>& cellPoints) {
    if (cellPoints.size() < 3) return false; // 至少需要三个点才能形成一个单元
    // 计算单元的法向量
    Vector3f normal = CrossProduct(cellPoints[1] - cellPoints[0], cellPoints[2] - cellPoints[0]);
    normal.normalize();
    // 检查点是否在单元的同一侧
    for (size_t i = 0; i < cellPoints.size(); ++i) {
        const Point& a = cellPoints[i];
        const Point& b = cellPoints[(i + 1) % cellPoints.size()];
        Vector3f edge = b - a;
        Vector3f toPoint = p - a;
        Vector3f cross = CrossProduct(edge, toPoint);
        if (DotProduct(cross, normal) < 0) {
            return false; // 点在单元的外侧
        }
    }
    return true; // 点在单元内
}


//不插值，取离目标点最近点数值
float ResampleToLine::NearestVal(const Point& target, const std::vector<Point>& points,
                                 const std::vector<float>& values) {
    float minDistSq = std::numeric_limits<float>::max();
    float result = values[0];
    for (int i = 0; i < points.size(); i++) {
        Point closest = points[i];
        float dist = target.distance2(closest);
        if (dist < minDistSq) { 
            minDistSq = dist;
            result = values[i];
        }
    }
    return result;
}


//计算加权插值函数输入目标点target，已知点集points和对应的值values，返回插值结果
float ResampleToLine::Interpolate(const Point& target, const std::vector<Point>& points, const std::vector<float>& values, float power) {
    float weightSum = 0.0f;
    float weightedSum = 0.0f;
    bool exactMatch = false;
    float exactValue = 0.0f;

    for (int i = 0; i < points.size(); i++) { 
        float d2 = target.distance2(points[i]);
        if (d2 < FLT_EPSILON) { 
            exactMatch = true;
            exactValue = values[i];
            break;
        }
        float w = 1.0 / (d2+FLT_EPSILON);
        weightedSum += w * values[i];
        weightSum += w;
    }

    if (exactMatch) { return exactValue;}
    if (weightSum < FLT_EPSILON) { return 0.0f;}
    return weightedSum / weightSum;
    
}





//构建均匀桶
std::vector<std::vector<int>> ResampleToLine::builduniformGrid(const BoundingBox& bbox, const UnstructuredMesh::Pointer& mesh, int nx, int ny, int nz) {

    auto g_voxelSizeX = (bbox.max[0] - bbox.min[0]) / nx;
    auto g_voxelSizeY = (bbox.max[1] - bbox.min[1]) / ny;
    auto g_voxelSizeZ = (bbox.max[2] - bbox.min[2]) / nz;
    if (g_voxelSizeX < 1e-12) g_voxelSizeX = 1.0;
    if (g_voxelSizeY < 1e-12) g_voxelSizeY = 1.0;
    if (g_voxelSizeZ < 1e-12) g_voxelSizeZ = 1.0;
    std::vector<std::vector<int>> g_voxelCells;
    g_voxelCells.assign(nx * ny * nz, {});
    
    for (int cellid = 0; cellid < mesh->GetNumberOfCells(); cellid++) { 
        struct AABB aabb; 
        auto cell = mesh->GetCell(cellid);
        for (int nid = 0; nid < mesh->GetCell(cellid)->GetNumberOfPoints(); nid++) { 
            aabb.expand(mesh->GetPoint(cell->GetPointId(nid)));
        }

        int ix_min = std::max(0, (int)((aabb.min[0] - bbox.min[0]) / g_voxelSizeX));
        int iy_min = std::max(0, (int) ((aabb.min[1] - bbox.min[1]) / g_voxelSizeY));
        int iz_min = std::max(0, (int) ((aabb.min[2] - bbox.min[2]) / g_voxelSizeZ));
        int ix_max = std::min(nx - 1, (int) ((aabb.max[0] - bbox.min[0]) / g_voxelSizeX));
        int iy_max = std::min(ny - 1, (int) ((aabb.max[1] - bbox.min[1]) / g_voxelSizeY));
        int iz_max = std::min(nz - 1, (int) ((aabb.max[2] - bbox.min[2]) / g_voxelSizeZ));

        for (int ix = ix_min; ix <= ix_max; ix++) {
            for (int iy = iy_min; iy <= iy_max; iy++) {
                for (int iz = iz_min; iz <= iz_max; iz++) {
                    int idx = ix + iy * nx + iz * nx * ny;
                        g_voxelCells[idx].push_back(cellid);

            }
            }
        }
    }



    return g_voxelCells;
}
 
StructuredMesh::Pointer ResampleToLine::resample_to_line_UnstructuredMesh(const UnstructuredMesh::Pointer mesh,
                                                                     const Point& p0, const Point& p1, int n, double maxDistance) {
    StructuredMesh::Pointer output = StructuredMesh::New();
    output->SetName("resample_to_line");
    Points::Pointer samples = Points::New();
    AttributeSet::Pointer attrSet = mesh->GetAttributeSet();
    AttributeSet::Pointer outputattr = output->GetAttributeSet();
    
    std::vector<Point> candidatePoint(n);
    std::vector<igIndex> candidateCell(n, -1);
    std::vector<igIndex> candidateFace(n, -1);
    auto bbox = mesh->GetBoundingBox();
    std::vector<std::vector<int>> ungrid;
    ungrid = builduniformGrid(bbox, mesh, g_nx, g_ny, g_nz);

    for (int i = 0; i < n; i++) {
        float t = (float) i / (n - 1);
        float x = (1 - t) * p0[0] + t * p1[0];
        float y = (1 - t) * p0[1] + t * p1[1];
        float z = (1 - t) * p0[2] + t * p1[2];
        Point p = {x, y, z};
        samples->AddPoint(p);
        Point closest;
        int cellid;
        int faceid;
        //最近面距离截断数值
        float distSq = maxdistSq;
        if (findNearest(mesh, p, closest, cellid, faceid, distSq, ungrid, bbox)) {
            //std::cout << cellid << std::endl;
            candidatePoint[i] = closest;
            candidateCell[i] = cellid;
            candidateFace[i] = faceid;
        } else {
            candidatePoint[i] = p;
        }
    }
    //存储单元索引
    auto Cells = mesh->GetCellArray();
    
    auto attrnum = attrSet->GetNumberOfAttributes();
    for (int i = 0; i< attrnum; i++){
        auto attr = attrSet->GetAttribute(i).GetPointer();
        auto type = attrSet->GetAttribute(i).GetType();
        
        if (type == IG_CELL)  continue; 
            ;
        auto attrtype = attr->GetArrayType();
        auto name = attr->GetName();
        auto dimension = attr->GetDimension();
        auto attrsize = attr->GetNumberOfElements();
        
        FloatArray::Pointer data = FloatArray::New();
        data->SetName(name);
        data->SetDimension(dimension);
        data->Resize(n);

        for (int j = 0; j < candidateCell.size(); j++) {
            int cellid = candidateCell[j];
            int faceid = candidateFace[j];
            Point p = candidatePoint[j];
            std::vector<float> newvalue;
            for (int dim = 0; dim < dimension; dim++) {
                if (cellid != -1 && faceid != -1) {
                    
                    auto Cell = mesh->GetCell(cellid);
                    auto face = Cell->GetFace(faceid);
                    int size = face->GetNumberOfPoints();
                    std::vector<Point> cellPoints(size);
                    std::vector<float> values(size);
                    for (int k = 0; k < size; k++) {
                        auto pointid = face->GetPointId(k);
                        cellPoints[k] = mesh->GetPoint(pointid);
                        values[k] = attr->GetElementValue(pointid, dim);
                    }

                    if (IsPointInCell(p, cellPoints)) { 
                        newvalue.push_back(Interpolate(p, cellPoints, values));
                    } else {
                        newvalue.push_back(NearestVal(p, cellPoints, values));
                    }
                } else {
                    newvalue.push_back(0.f);
                }
            }
            data->SetElement(j, newvalue);
        } 
        outputattr->AddAttribute(type, IG_POINT, data);
        
    }
    output->SetPoints(samples);
    output->GenStructuredCellConnectivities();
    //buildLine(output);
    return output;
}
//bool ResampleToLine::buildLine(StructuredMesh::Pointer& mesh){
//    CellArray::Pointer cells = CellArray::New();
//    for (int i = 0; i< mesh->GetNumberOfPoints()-1; i++){ 
//        cells->AddCellId2(i, i + 1);
//    }
//    
//    return true;
//}

std::array<float, 3> ResampleToLine::GetPosition_face(Face* f, int num) {
    std::array<float, 3> position = {0.0f, 0.0f, 0.0f};
    for (igIndex idx = 0; idx < num; idx++) {
        position[0] += f->GetPoint(idx)[0];
        position[1] += f->GetPoint(idx)[1];
        position[2] += f->GetPoint(idx)[2];
    }

    for (igIndex i = 0; i < 3; i++) {
        if (position[i] != 0) position[i] /= num;
    }
    return position;
}

double ResampleToLine::GetArea(Vector3d a, Vector3d b, Vector3d c) { return CrossProduct(a - b, a - c).length() / 2; }

SurfaceMesh::Pointer ResampleToLine::TriangulateSurfaceMesh(SurfaceMesh::Pointer mesh) {
    {
        bool f = true;
        igIndex face[16]{};
        for (int i = 0; i < mesh->GetNumberOfFaces(); i++) {
            int size = mesh->GetFacePointIds(i, face);
            if (size != 3) {
                f = false;
                break;
            }
        }

        if (f) { return mesh; }
    }
    auto attrbs = mesh->GetAttributeSet();

    CellArray::Pointer Faces = CellArray::New();
    Points::Pointer Points = mesh->GetPoints();

    igIndex face[16]{}, tri[3]{};
    for (int i = 0; i < mesh->GetNumberOfFaces(); i++) {
        int size = mesh->GetFacePointIds(i, face);

        if (size == 3) {
            Faces->AddCellId3(face[0], face[1], face[2]);
        } else if (size == 4) {
            Point p0 = mesh->GetPoint(face[0]);
            Point p1 = mesh->GetPoint(face[1]);
            Point p2 = mesh->GetPoint(face[2]);
            Point p3 = mesh->GetPoint(face[3]);
            double area01 = GetArea(p0, p1, p3);
            double area02 = GetArea(p1, p2, p3);

            double area11 = GetArea(p0, p1, p2);
            double area12 = GetArea(p2, p3, p0);

            double r0 = area01 / area02;
            double r1 = area11 / area12;

            if (r0 < 1) r0 = 1 / r0;
            if (r1 < 1) r1 = 1 / r1;
            if (r0 < r1) {
                Faces->AddCellId3(face[0], face[1], face[3]);
                Faces->AddCellId3(face[1], face[2], face[3]);
            } else {
                Faces->AddCellId3(face[0], face[1], face[2]);
                Faces->AddCellId3(face[2], face[3], face[0]);
            }

        } else {
            Point center(0, 0, 0);
            for (int j = 0; j < size; j++) { center += Points->GetPoint(face[j]); }
            center /= size;
            igIndex newPtId = Points->AddPoint(center);

            for (int j = 0; j < attrbs->GetNumberOfAttributes(); j++) {
                auto& attrb = attrbs->GetAttribute(j);
                if (attrb.isDeleted) continue;
                if (attrb.attachmentType == IG_POINT) {
                    double val[8]{0}, sum[8]{0};
                    int dim = attrb.pointer->GetDimension();
                    for (int k = 0; k < size; k++) {
                        attrb.pointer->GetElement(face[k], val);
                        for (int d = 0; d < dim; d++) { sum[d] += val[d]; }
                    }
                    for (int d = 0; d < dim; d++) { sum[d] /= size; }
                    attrb.pointer->AddElement(sum);
                }
            }

            for (int j = 0; j < size; j++) { Faces->AddCellId3(newPtId, face[j], face[(j + 1) % size]); }
        }
    }

    SurfaceMesh::Pointer Mesh = SurfaceMesh::New();
    Mesh->SetName(mesh->GetName());
    Mesh->SetPoints(Points);
    Mesh->SetFaces(Faces);
    Mesh->SetAttributeSet(mesh->GetAttributeSet());

    return Mesh;
}

//求射线与三角形的交点
bool ResampleToLine::rayTriangleIntersect(const Point& orign, const Point& dir, const Point& v0, const Point& v1, const Point& v2, double& t, double& u, double& v) {
    const double EPSILON = 1e-8;
    Point edge1 = v1 - v0;
    Point edge2 = v2 - v0;
    Point h = CrossProduct(dir, edge2);
    double a = DotProduct(edge1, h);

    if (a > -EPSILON && a < EPSILON) return false; // 平行
    double f = 1.0 / a;

    Point s = orign - v0;
    u = f * DotProduct(s, h);
    if (u < 0.0 || u > 1.0) return false;

    Point q = CrossProduct(s, edge1);
    v = f * DotProduct(dir, q);
    if (v < 0.0 || u + v > 1.0) return false;

    t = f * DotProduct(edge2, q);
    return t >= 0; // 交点在射线方向上
 
}
//返回最近点、距离平方
void ResampleToLine::closestPointOnTriangle(const Point& p, const Point& a, const Point& b, const Point& c, Point& closest, float& distSq) {
    Point ab = b - a, ac = c - a, ap = p - a;
    float d1 = DotProduct(ab, ap), d2 = DotProduct(ac, ap);
    if (d1 <= 0 && d2 <= 0) { 
        closest = a;
        
        distSq = DotProduct(ap, ap);
        return;  
    } 
    Point bp = p - b;
    float d3 = DotProduct(ab, bp), d4 = DotProduct(ac, bp);
    if (d3 >= 0 && d4 <= d3) { 
        closest = b;
        
        distSq = DotProduct(bp, bp);
        return;  
    }
    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0 && d1 >= 0 && d3 <= 0) { 
        float v = d1 / (d1 - d3);
        Point pv = {v, v, v};
        closest = a + pv * ab;
        
        distSq = DotProduct(closest - p, closest - p);
        return;  
    }
    Point cp = p - c;
    float d5 = DotProduct(ab, cp), d6 = DotProduct(ac, cp);
    if (d6 >= 0 && d5 <= d6) { 
        closest = c;
        
        distSq = DotProduct(cp, cp);
        return;  
    }
    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0 && d2 >= 0 && d6 <= 0) { 
        float w = d2 / (d2 - d6);
        Point pw = {w, w, w};
        closest = a + pw * ac;
        
        distSq = DotProduct(closest - p, closest - p);
        return;  
    }
    float va = d3 * d6 - d5 * d4;
    if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) { 
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        Point pw = {w, w, w};
        closest = b + pw * (c - b);
        
        distSq = DotProduct(closest - p, closest - p);
        return;  
    }
    //若点在三角形内部，投影在平面上
    Point n = CrossProduct(ab, ac);
    float denom = DotProduct(n, n);
    if (denom > FLT_EPSILON) { 
        Point ap = p - a;
        float t = DotProduct(n, ap)/denom;
        closest = p - n * t;
        distSq = DotProduct(closest - p, closest - p);
        return;
    }
    
    closest = a; // 三角形退化为点
    float minDistSq = DotProduct(ap, ap);
    float distSqB = DotProduct(bp, bp);
    if (distSqB < minDistSq) {
        closest = b;
        minDistSq = distSqB;
    }
    float distSqC = DotProduct(cp, cp);
    if (distSqC < minDistSq) {
        closest = c;
        minDistSq = distSqC;
    }
    distSq = minDistSq;
    return;
    

}

bool pointInTetra(const Point& p, const Cell& cell) { 
    const Point& a = cell.GetPoint(0);
    const Point& b = cell.GetPoint(1);
    const Point& c = cell.GetPoint(2);
    const Point& d = cell.GetPoint(3);
    
    auto det3 = [](const Point& a, const Point& b, const Point& c) {
        return a[0] * (b[1] * c[2] - c[1] * b[2]) - a[1] * (b[0] * c[2] - c[0] * b[2]) +
               a[2] * (b[0] * c[1] - c[0] * b[1]);
    };
    float D0 = det3(b-a, c-a, d-a);
    if (std::abs(D0) < 1e-15) return false;
    float D1 = det3(p - b, c - b, d - b);
    float D2 = det3(a - p, c - p, d - p);
    float D3 = det3(a - b, p - b, d - b);
    float D4 = det3(a - b, c - b, p - b);

    float l1 = D1 / D0;
    float l2 = D2 / D0;
    float l3 = D3 / D0;
    float l4 = D4 / D0;

    return (l1 >= FLT_EPSILON && l2 >= FLT_EPSILON && l3 >= FLT_EPSILON && l4 >= FLT_EPSILON && l1 <= 1 + FLT_EPSILON&&
            l2 <= 1 + FLT_EPSILON && l3 <= 1 + FLT_EPSILON && l4 <= 1 + FLT_EPSILON);


}

//计算p到四边形的最近点
void ResampleToLine::closestPointOnTriangle(const Point& p, const Point& a, const Point& b, const Point& c,
    const Point& d, Point& closest, float& distSq) {
    // 将四边形分为两个三角形
    Point closest1, closest2;
    float distSq1, distSq2;
    closestPointOnTriangle(p, a, b, c, closest1, distSq1);
    closestPointOnTriangle(p, a, c, d, closest2, distSq2);
    if (distSq1 <= distSq2) {
        closest = closest1;
        distSq = distSq1;
    } else {
        closest = closest2;
        distSq = distSq2;
    }
}

//计算p到五边形的最近点
void ResampleToLine::closestPointOnTriangle(const Point& p, const Point& a, const Point& b, const Point& c,
    const Point& d, const Point& e, Point& closest, float& distSq) {
    // 将五边形分为三个三角形
    Point closest1, closest2, closest3;
    float distSq1, distSq2, distSq3;
    closestPointOnTriangle(p, a, b, c, closest1, distSq1);
    closestPointOnTriangle(p, a, c, d, closest2, distSq2);
    closestPointOnTriangle(p, a, d, e, closest3, distSq3);
    if (distSq1 <= distSq2 && distSq1 <= distSq3) {
        closest = closest1;
        distSq = distSq1;
    } else if (distSq2 <= distSq1 && distSq2 <= distSq3) {
        closest = closest2;
        distSq = distSq2;
    } else {
        closest = closest3;
        distSq = distSq3;
    }
}

//计算p到六边形的最近点
void ResampleToLine::closestPointOnTriangle(const Point& p, const Point& a, const Point& b, const Point& c,
    const Point& d, const Point& e, const Point& f, Point& closest, float& distSq) {
    // 将六边形分为四个三角形
    Point closest1, closest2, closest3, closest4;
    float distSq1, distSq2, distSq3, distSq4;
    closestPointOnTriangle(p, a, b, c, closest1, distSq1);
    closestPointOnTriangle(p, a, c, d, closest2, distSq2);
    closestPointOnTriangle(p, a, d, e, closest3, distSq3);
    closestPointOnTriangle(p, a, e, f, closest4, distSq4);
    if (distSq1 <= distSq2 && distSq1 <= distSq3 && distSq1 <= distSq4) {
        closest = closest1;
        distSq = distSq1;
    } else if (distSq2 <= distSq1 && distSq2 <= distSq3 && distSq2 <= distSq4) {
        closest = closest2;
        distSq = distSq2;
    } else if (distSq3 <= distSq1 && distSq3 <= distSq2 && distSq3 <= distSq4) {
        closest = closest3;
        distSq = distSq3;
    } else {
        closest = closest4;
        distSq = distSq4;
    }
}




//寻找最近点
bool ResampleToLine::findNearest(const UnstructuredMesh::Pointer& mesh, const Point& p, Point& closest, int& cellid, int& faceid, float& distSq, const std::vector<std::vector<int>>& grid, const BoundingBox& bbox) {
    
    const IGsize numCells = mesh->GetNumberOfCells();
    const IGsize numPoints = mesh->GetNumberOfPoints();
    Point clo;
    Point resclo;
    float mindist = std::numeric_limits<float>::max();
    float dist;
    if (numCells == 0 || numPoints == 0){ 
        m_Message = "Mesh has no cells or points.";
        return false;
    }
    std::vector<int> candidates;
    
    
    
    auto g_voxelSizeX = (bbox.max[0] - bbox.min[0]) / g_nx;
    auto g_voxelSizeY = (bbox.max[1] - bbox.min[1]) / g_ny;
    auto g_voxelSizeZ = (bbox.max[2] - bbox.min[2]) / g_nz;
    
    int ix = std::max(0, std::min(g_nx - 1, (int) ((p[0] - bbox.min[0]) / g_voxelSizeX)));
    int iy = std::max(0, std::min(g_ny - 1, (int) ((p[1] - bbox.min[1]) / g_voxelSizeY)));
    int iz = std::max(0, std::min(g_nz - 1, (int) ((p[2] - bbox.min[2]) / g_voxelSizeZ)));

    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dz = -1; dz <= 1; dz++) { 
                int nx = ix + dx, ny = iy + dy, nz = iz + dz;
                if (nx < 0 || nx >= g_nx || ny < 0 || ny >= g_ny || nz < 0 || nz >= g_nz) continue;
                int idx = nx + ny * g_nx + nz * g_nx * g_ny;
                candidates.insert(candidates.end(), grid[idx].begin(), grid[idx].end());
            }
        }
    }
    std::sort(candidates.begin(), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());



    auto Cell = mesh->GetCellArray();
    igIndex cell[IGAME_CELL_MAX_SIZE];

    for (int i = 0; i < candidates.size(); i++) { 
        
        auto Cells = mesh->GetCell(candidates[i]);
        for (int j = 0; j < Cells->GetNumberOfFaces(); j++) { 
            auto face = Cells->GetFace(j);
            auto size = face->GetNumberOfPoints();
            std::vector<Point> Points(size);

            for (int k = 0; k < size; k++) { Points[k] = face->GetPoint(k); };
            

            closestPointOnTriangle(p, Points[0], Points[1], Points[2], clo, dist);
            if (dist < mindist) { mindist = dist;
            resclo = clo;
            cellid = candidates[i];
            faceid = j;

            
            
            }
        }

        
        
        
        /*Point clo;
        float dist;*/
        ////求三角形上最近点
        //if (type == IG_TRIANGLE) {
        //    closestPointOnTriangle(p, Points[0], Points[1], Points[2], clo, dist);
        //    if (dist < distSq) {
        //        distSq = dist;
        //        closest = clo;
        //        cellid = i;
        //    }
        //    return true;
        //}
        //if (type == IG_QUAD) { 
        //    closestPointOnTriangle(p, Points[0], Points[1], Points[2], Points[3], clo, dist);
        //    if (dist < distSq) { 
        //        distSq = dist;
        //        closest = clo;
        //        cellid = i;
        //    }
        //    return true;
        //}
        //if (type == IG_TETRA) { 
        //    closestPointOnTriangle(p, Points[0], Points[1], Points[2], Points[3], Points[4], clo, dist);
        //    if (dist < distSq) { 
        //        distSq = dist;
        //        closest = clo;
        //        cellid = i;
        //    }
        //    return true;
        //}
        //if (type == IG_HEXAHEDRON) {
        //    closestPointOnTriangle(p, Points[0], Points[1], Points[2], Points[3], Points[4], Points[5], clo, dist);
        //    if (dist < distSq) {
        //        distSq = dist;
        //        closest = clo;
        //        cellid = i;
        //    }
        //    return true; 
        //}
        //IG_TETRA
        
    }
    if (mindist < distSq){
        closest = resclo;
        return true;
    }
    return false;
}






IGAME_NAMESPACE_END