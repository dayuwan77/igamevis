#pragma once
#ifndef iGameResampleToLine_h
#define iGameResampleToLine_h
#include "iGameDrawObject.h"
#include "iGameSceneManager.h"


#include "Eigen/Dense"
#include "Eigen/Eigenvalues"
#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameStructuredMesh.h"
#include "iGameUnstructuredMesh.h"
#include <cmath>
#include <unordered_set>
#include <algorithm>
#include <iGameCellCenter.h>
#include <iGameVector.h>
#include <vector>
IGAME_NAMESPACE_BEGIN
class ResampleToLine : public Filter {
public:
    I_OBJECT(ResampleToLine);
    static Pointer New() { return new ResampleToLine; }
    bool Execute() override;
    void setOrigTarget(const Point& p0, const Point& p1, const int& x) {
        orig = p0;
        target = p1;
        n = x;
    }
    

    std::string GetMessage() const { return m_Message; }

private:
    Point orig = {-1.0f, -0.983795f, -0.35714f}, target = {1.0f, 0.983795f, 0.35714f};
    int n = 40;
    int g_nx = 50, g_ny = 50, g_nz = 50;
    float maxdistSq = 1e-6f;
    StructuredMesh::Pointer resample_to_line_UnstructuredMesh(const UnstructuredMesh::Pointer mesh, const Point& p0,
                                                              const Point& p1, int n, double maxDistance = 1e-6);
    bool buildLine(StructuredMesh::Pointer& mesh);
    bool rayTriangleIntersect(const Point& orign, const Point& dir, const Point& v0, const Point& v1, const Point& v2,
                              double& t, double& u, double& v);
    std::array<float, 3> GetPosition_face(Face* f, int num);
    double GetArea(Vector3d a, Vector3d b, Vector3d c);
    SurfaceMesh::Pointer TriangulateSurfaceMesh(SurfaceMesh::Pointer mesh);
    
    struct AABB {
        Point min, max;
        //初始化结构体
        AABB() : min({std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()}),
              max({std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
                   std::numeric_limits<float>::lowest()}) {}

        void expand(const Point& p) { 
            min[0] = std::min(min[0], p[0]);
            min[1] = std::min(min[1], p[1]);
            min[2] = std::min(min[2], p[2]);
            max[0] = std::max(max[0], p[0]);
            max[1] = std::max(max[1], p[1]);
            max[2] = std::max(max[2], p[2]);
        }

        void expand(const AABB& box) {
            expand(box.min);
            expand(box.max);
        }

        Point center() const { return (min + max) * 0.5f; }
        int longestAxis() const {
            Point extents = max - min;
            if (extents[0] >= extents[1] && extents[0] >= extents[2]) return 0;
            if (extents[1] >= extents[0] && extents[1] >= extents[2]) return 1;
            return 2;
        }
        double minDistSq(const Point& p) const {
            auto dx = std::max({min[0] - p[0], 0.0f, p[0] - max[0]});
            auto dy = std::max({min[1] - p[1], 0.0f, p[1] - max[1]});
            auto dz = std::max({min[2] - p[2], 0.0f, p[2] - max[2]});
            return dx * dx + dy * dy + dz * dz;
        }
        bool contains(const Point& p) const {
            return p[0] >= min[0] && p[0] <= max[0] && p[1] >= min[1] && p[1] <= max[1] && p[2] >= min[2] &&
                   p[2] <= max[2];
        }
    };

    struct BVHNode {
        AABB box;
        std::unique_ptr<BVHNode> left;
        std::unique_ptr<BVHNode> right;
        std::vector<int> triangleIndices; // 存储三角形索引的数组
        bool isLeaf() const { return left == nullptr && right == nullptr; }
    };

    struct NearestResult {
        double distanceSq = std::numeric_limits<double>::max();
        Point closestPoint;
        int triangleIndex = -1;
        double w0, w1, w2; // Barycentric coordinates
    };
    //AABB triangleAABB(const SurfaceMesh::Pointer Mesh, int faceida, int faceidb, int faceidc);
    //std::unique_ptr<BVHNode> buildBVH(SurfaceMesh::Pointer& mesh, std::vector<int>& triangleIndices, int depth);
    std::vector<std::vector<int>> builduniformGrid(const BoundingBox& bbox, const UnstructuredMesh::Pointer& mesh, int nx, int ny,
                                                   int nz);
    void closestPointOnTriangle(const Point& p, const Point& a, const Point& b, const Point& c, Point& closest,
                                 float& distSq);
    void closestPointOnTriangle(const Point& p, const Point& a, const Point& b, const Point& c,
                                                const Point& d, Point& closest, float& distSq);
    void closestPointOnTriangle(const Point& p, const Point& a, const Point& b, const Point& c,
                                                const Point& d, const Point& e, Point& closest, float& distSq);
    void closestPointOnTriangle(const Point& p, const Point& a, const Point& b, const Point& c, const Point& d,
                                const Point& e, const Point& f, Point& closest, float& distSq);
    bool findNearest(const UnstructuredMesh::Pointer& mesh, const Point& p, Point& closest, int& cellid, int& faceid,
                     float& distSq, const std::vector<std::vector<int>>& grid, const BoundingBox& bbox);
    float Interpolate(const Point& target, const std::vector<Point>& points, const std::vector<float>& values,
                      float power = 2.f);
    bool IsPointInCell(const Point& p, const std::vector<Point>& cellPoints);
    float NearestVal(const Point& target, const std::vector<Point>& points, const std::vector<float>& values);

protected:
    ResampleToLine()
    {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }
    ~ResampleToLine() override = default;

    SurfaceMesh::Pointer surface_Mesh{};
    VolumeMesh::Pointer volume_Mesh{};
    AttributeSet* attributeSet{nullptr};

    int curIndex{-1};
    std::string name;

    int dim{-1};
    int m_currentAttributeDimension{-1};
    std::string m_Message{"Not Surface Mesh !"};
};




IGAME_NAMESPACE_END
#endif

