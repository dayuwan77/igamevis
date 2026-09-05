#pragma once

#include "iGameFilter.h"
#include "iGameDataObject.h"

IGAME_NAMESPACE_BEGIN

/**
 * @class   iGameForceStaticMeshFilter
 * @brief  
 *          Caches the input mesh the first time it executes and reuses it as
 *          a static mesh: only the attribute data (point/cell/field) is
 *          updated as long as the geometry dimensions (points/cells) stay the
 *          same. Pure cache management.
 */
class ForceStaticMeshFilter : public Filter {
public:
    I_OBJECT(ForceStaticMeshFilter)
    static Pointer New() { return new ForceStaticMeshFilter; }

    bool Execute() override;

    /**
     * @brief 强制重建缓存（不再复用已有缓存）。
     */
    void SetForceCacheComputation(bool on);
    bool GetForceCacheComputation() const;

protected:
    ForceStaticMeshFilter();
    ~ForceStaticMeshFilter() override = default;

    // 缓存是否仍有效：几何（点数/单元数）未变化
    bool IsValidCache(const DataObject::Pointer& input);
    // 仅用输入的属性数据更新缓存（几何保持缓存不动）
    void InputToCache(const DataObject::Pointer& input);

protected:
    DataObject::Pointer m_Cache;   // 缓存的静态网格（几何固定）
    bool m_CacheInitialized{false};
    bool m_ForceCacheComputation{false};
};

IGAME_NAMESPACE_END
