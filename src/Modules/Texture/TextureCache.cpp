#include "TextureCache.h"

TextureCache::TextureCache(Context& context)
    : m_context(context)
{
}

Resource::Ptr TextureCache::Load(const std::string& path)
{
    auto it = m_cache.find(path);
    if (it == m_cache.end())
        it = m_cache.emplace(path, CreateTexture(m_context, path)).first;
    return it->second;
}

Resource::Ptr TextureCache::CreateTextuteStab(glm::vec4& val)
{
    auto it = m_stub_cache.find(val);
    if (it != m_stub_cache.end())
        return it->second;
    Resource::Ptr tex = m_context.CreateTexture(BindFlag::kSrv, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 1, 1, 1);
    size_t num_bytes = 0;
    size_t row_bytes = 0;
    GetSurfaceInfo(1, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, &num_bytes, &row_bytes, nullptr);
    m_context.UpdateSubresource(tex, 0, &val, row_bytes, num_bytes);
    m_stub_cache.emplace(val, tex);
    return tex;
}
