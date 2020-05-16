#include "Framebuffer/VKFramebuffer.h"
#include <Device/VKDevice.h>
#include <View/VKView.h>
#include <Pipeline/VKPipeline.h>

VKFramebuffer::VKFramebuffer(VKDevice& device, const std::shared_ptr<VKPipeline>& pipeline, const std::vector<std::shared_ptr<View>>& rtvs, const std::shared_ptr<View>& dsv)
    : FramebufferBase(rtvs, dsv)
{
    std::vector<vk::ImageView> attachment_views;
    std::shared_ptr<View> first_view;
    auto add_view = [&](const std::shared_ptr<View>& view)
    {
        if (!view)
            return;
        decltype(auto) vk_view = view->As<VKView>();
        attachment_views.emplace_back(vk_view.GetRtv());
        if (!first_view)
            first_view = view;
    };
    for (auto& rtv : rtvs)
    {
        add_view(rtv);
    }
    add_view(dsv);

    m_render_pass = pipeline->GetRenderPass();

    vk::FramebufferCreateInfo framebuffer_info = {};
    framebuffer_info.renderPass = m_render_pass;
    framebuffer_info.attachmentCount = attachment_views.size();
    framebuffer_info.pAttachments = attachment_views.data();

    if (first_view)
    {
        decltype(auto) vk_view = first_view->As<VKView>();
        decltype(auto) vk_resource = vk_view.GetResource();
        m_extent = { vk_resource->image.size.width, vk_resource->image.size.height };
        framebuffer_info.width = m_extent.width;
        framebuffer_info.height = m_extent.height;
        framebuffer_info.layers = vk_resource->image.array_layers;
    }

    m_framebuffer = device.GetDevice().createFramebufferUnique(framebuffer_info);
}

vk::Framebuffer VKFramebuffer::GetFramebuffer() const
{
    return m_framebuffer.get();
}

vk::RenderPass VKFramebuffer::GetRenderPass() const
{
    return m_render_pass;
}

vk::Extent2D VKFramebuffer::GetExtent() const
{
    return m_extent;
}
