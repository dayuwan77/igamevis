#ifndef iGameResampleToImageFilter_h
#define iGameResampleToImageFilter_h

#include "iGameFilter.h"

#include <string>

IGAME_NAMESPACE_BEGIN

/**
 * @brief 将输入网格重采样到规则图像网格 (Resample To Image)。
 *
 * 本过滤器是 VTK 中 vtkResampleToImage 的移植实现，功能保持一致：
 * 对输入的网格（PointSet 及其子类）在指定的采样区域上建立一个规则的
 * 图像网格（StructuredMesh），并在每一个网格格点处对输入的点属性进行
 * 探针插值（probe），从而把网格上的场重采样为图像上的点场。
 *
 * 输出是一个 StructuredMesh（等价 vtkImageData）：
 *   - 原点 origin   = samplingBounds 的 (xmin, ymin, zmin)；
 *   - 间距 spacing  = (SamplingDimensions[i] == 1) ? 0
 *                     : (bounds[2i+1] - bounds[2i]) / (SamplingDimensions[i]-1)；
 *   - 维度 dimensions = SamplingDimensions（默认 10 x 10 x 10）；
 *   - 点数据中带有一个 char 数组 "vtkValidPointMask"，格点落在输入网格内为 1，
 *     否则为 0（同时对应插值点场数组默认值 0）。
 *
 * 参数语义与 VTK 一致：
 *   - UseInputBounds = true 时使用输入包围盒（并向内收缩 epsilon 以避免
 *     浮点误差导致的越界采样）；否则使用显式 SamplingBounds。
 *   - SamplingBounds / SamplingDimensions 默认为 {0,1,0,1,0,1} / {10,10,10}。
 */
class ResampleToImageFilter : public Filter {
public:
    I_OBJECT(ResampleToImageFilter);
    static Pointer New() { return new ResampleToImageFilter; }

    bool Execute() override;

    //@{
    /// 输出图像的采样维度（格点数）。默认 {10, 10, 10}，等价 VTK 的 SamplingDimensions。
    void SetSamplingDimensions(int i, int j, int k);
    void SetSamplingDimensions(int dims[3]);
    int* GetSamplingDimensions() { return this->SamplingDimensions; }
    void GetSamplingDimensions(int dims[3]) const;
    //@}

    //@{
    /// 显式采样区域（UseInputBounds 为 false 时生效）。默认 {0,1,0,1,0,1}。
    void SetSamplingBounds(const double bounds[6]);
    void SetSamplingBounds(double x0, double x1, double y0, double y1, double z0, double z1);
    void GetSamplingBounds(double bounds[6]) const;
    double* GetSamplingBounds() { return this->SamplingBounds; }
    //@}

    //@{
    /// 是否使用输入数据的包围盒作为采样区域。默认 true，等价 VTK 的 UseInputBounds。
    void SetUseInputBounds(bool b) { this->UseInputBounds = b; }
    bool GetUseInputBounds() const { return this->UseInputBounds; }
    //@}

    /// 有效点掩膜数组名。固定为 "vtkValidPointMask"，等价 VTK 的 GetMaskArrayName()。
    static const char* GetMaskArrayName() { return "vtkValidPointMask"; }

protected:
    ResampleToImageFilter();
    ~ResampleToImageFilter() override = default;

    int SamplingDimensions[3] = {10, 10, 10};
    double SamplingBounds[6] = {0.0, 1.0, 0.0, 1.0, 0.0, 1.0};
    bool UseInputBounds = true;
};

IGAME_NAMESPACE_END
#endif