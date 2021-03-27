#include "ObjectCache.h"
#include <Utilities/DXGIFormatHelper.h>

ObjectCache::ObjectCache(Device& device)
    : m_device(device)
{
}

std::shared_ptr<Pipeline> ObjectCache::GetPipeline(const GraphicsPipelineDesc& desc)
{
    auto it = m_graphics_object_cache.find(desc);
    if (it == m_graphics_object_cache.end())
    {
        auto pipeline = m_device.CreateGraphicsPipeline(desc);
        it = m_graphics_object_cache.emplace(std::piecewise_construct,
            std::forward_as_tuple(desc),
            std::forward_as_tuple(pipeline)).first;
    }
    return it->second;
}

std::shared_ptr<Pipeline> ObjectCache::GetPipeline(const ComputePipelineDesc& desc)
{
    auto it = m_compute_object_cache.find(desc);
    if (it == m_compute_object_cache.end())
    {
        auto pipeline = m_device.CreateComputePipeline(desc);
        it = m_compute_object_cache.emplace(std::piecewise_construct,
            std::forward_as_tuple(desc),
            std::forward_as_tuple(pipeline)).first;
    }
    return it->second;
}

std::shared_ptr<Pipeline> ObjectCache::GetPipeline(const RayTracingPipelineDesc& desc)
{
    auto it = m_ray_tracing_object_cache.find(desc);
    if (it == m_ray_tracing_object_cache.end())
    {
        auto pipeline = m_device.CreateRayTracingPipeline(desc);
        it = m_ray_tracing_object_cache.emplace(std::piecewise_construct,
            std::forward_as_tuple(desc),
            std::forward_as_tuple(pipeline)).first;
    }
    return it->second;
}

std::shared_ptr<RenderPass> ObjectCache::GetRenderPass(const RenderPassDesc& desc)
{
    auto it = m_render_pass_cache.find(desc);
    if (it == m_render_pass_cache.end())
    {
        auto render_pass = m_device.CreateRenderPass(desc);
        it = m_render_pass_cache.emplace(std::piecewise_construct,
            std::forward_as_tuple(desc),
            std::forward_as_tuple(render_pass)).first;
    }
    return it->second;
}

std::shared_ptr<BindingSetLayout> ObjectCache::GetBindingSetLayout(const std::vector<BindKey>& keys)
{
    auto it = m_layout_cache.find(keys);
    if (it == m_layout_cache.end())
    {
        auto layout = m_device.CreateBindingSetLayout(keys);
        it = m_layout_cache.emplace(std::piecewise_construct,
            std::forward_as_tuple(keys),
            std::forward_as_tuple(layout)).first;
    }
    return it->second;
}

std::shared_ptr<BindingSet> ObjectCache::GetBindingSet(const std::shared_ptr<BindingSetLayout>& layout, const std::vector<BindingDesc>& bindings)
{
    auto it = m_binding_set_cache.find({ layout, bindings });
    if (it == m_binding_set_cache.end())
    {
        auto binding_set = m_device.CreateBindingSet(layout);
        binding_set->WriteBindings(bindings);
        it = m_binding_set_cache.emplace(std::piecewise_construct,
            std::forward_as_tuple(layout, bindings),
            std::forward_as_tuple(binding_set)).first;
    }
    return it->second;
}

std::shared_ptr<Framebuffer> ObjectCache::GetFramebuffer(
    const std::shared_ptr<RenderPass>& render_pass,
    uint32_t width,
    uint32_t height,
    const std::vector<std::shared_ptr<View>>& rtvs,
    const std::shared_ptr<View>& dsv)
{
    auto key = std::make_tuple(render_pass, width, height, rtvs, dsv);
    auto it = m_framebuffers.find(key);
    if (it == m_framebuffers.end())
    {
        auto framebuffer = m_device.CreateFramebuffer(render_pass, width, height, rtvs, dsv);
        it = m_framebuffers.emplace(std::piecewise_construct,
            std::forward_as_tuple(key),
            std::forward_as_tuple(framebuffer)).first;
    }
    return it->second;
}

std::shared_ptr<View> ObjectCache::GetView(const std::shared_ptr<Program>& program, const BindKey& bind_key, const std::shared_ptr<Resource>& resource, const LazyViewDesc& view_desc)
{
    auto it = m_views.find({ program, bind_key, resource, view_desc });
    if (it != m_views.end())
        return it->second;
    ViewDesc desc = {};
    static_cast<LazyViewDesc&>(desc) = view_desc;
    desc.view_type = bind_key.view_type;

    bool shader_binding = true;
    switch (bind_key.view_type)
    {
    case ViewType::kRenderTarget:
    case ViewType::kDepthStencil:
    case ViewType::kShadingRateSource:
        shader_binding = false;
        if (resource->GetSampleCount() == 1)
        {
            if (resource->GetLayerCount() > 1)
                desc.dimension = ViewDimension::kTexture2DArray;
            else
                desc.dimension = ViewDimension::kTexture2D;
        }
        else
        {
            if (resource->GetLayerCount() > 1)
                desc.dimension = ViewDimension::kTexture2DMSArray;
            else
                desc.dimension = ViewDimension::kTexture2DMS;
        }
        break;
    }

    if (shader_binding)
    {
        decltype(auto) shader = program->GetShader(bind_key.shader_type);
        ResourceBindingDesc binding_desc = shader->GetResourceBinding(bind_key);
        desc.dimension = binding_desc.dimension;
        desc.stride = binding_desc.stride;
        if (resource && bind_key.view_type == ViewType::kTexture)
        {
            // TODO
            DXGI_FORMAT dx_format = static_cast<DXGI_FORMAT>(gli::dx().translate(resource->GetFormat()).DXGIFormat.DDS);
            if (IsTypelessDepthStencil(MakeTypelessDepthStencil(dx_format)))
            {
                switch (binding_desc.return_type)
                {
                case ReturnType::kFloat:
                    desc.plane_slice = 0;
                    break;
                case ReturnType::kUint:
                    desc.plane_slice = 1;
                    break;
                default:
                    assert(false);
                }
            }
        }
    }
    auto view = m_device.CreateView(resource, desc);
    m_views.emplace(std::piecewise_construct, std::forward_as_tuple(program, bind_key, resource, view_desc), std::forward_as_tuple(view));
    return view;
}
