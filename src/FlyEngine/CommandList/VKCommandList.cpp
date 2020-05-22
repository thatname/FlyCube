#include "CommandList/VKCommandList.h"
#include <Device/VKDevice.h>
#include <Resource/VKResource.h>
#include <View/VKView.h>
#include <Pipeline/VKGraphicsPipeline.h>
#include <Pipeline/VKComputePipeline.h>
#include <Framebuffer/VKFramebuffer.h>
#include <BindingSet/VKBindingSet.h>

VKCommandList::VKCommandList(VKDevice& device)
    : m_device(device)
{
    vk::CommandBufferAllocateInfo cmd_buf_alloc_info = {};
    cmd_buf_alloc_info.commandPool = device.GetCmdPool();
    cmd_buf_alloc_info.commandBufferCount = 1;
    cmd_buf_alloc_info.level = vk::CommandBufferLevel::ePrimary;
    std::vector<vk::UniqueCommandBuffer> cmd_bufs = device.GetDevice().allocateCommandBuffersUnique(cmd_buf_alloc_info);
    m_command_list = std::move(cmd_bufs.front());
}

void VKCommandList::Open()
{
    vk::CommandBufferBeginInfo begin_info = {};
    begin_info.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;
    m_command_list->begin(begin_info);
}

void VKCommandList::Close()
{
    m_command_list->end();
}

void VKCommandList::BindPipeline(const std::shared_ptr<Pipeline>& state)
{
    auto type = state->GetPipelineType();
    if (type == PipelineType::kGraphics)
    {
        decltype(auto) vk_state = state->As<VKGraphicsPipeline>();
        m_command_list->bindPipeline(vk::PipelineBindPoint::eGraphics, vk_state.GetPipeline());
    }
    else if (type == PipelineType::kCompute)
    {
        decltype(auto) vk_state = state->As<VKComputePipeline>();
        m_command_list->bindPipeline(vk::PipelineBindPoint::eCompute, vk_state.GetPipeline());
    }
}

void VKCommandList::BindBindingSet(const std::shared_ptr<BindingSet>& binding_set)
{
    decltype(auto) vk_binding_set = binding_set->As<VKBindingSet>();
    decltype(auto) descriptor_sets = vk_binding_set.GetDescriptorSets();
    if (vk_binding_set.IsCompute())
    {
        m_command_list->bindDescriptorSets(vk::PipelineBindPoint::eCompute, vk_binding_set.GetPipelineLayout(), 0, descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);
    }
    else
    {
        m_command_list->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, vk_binding_set.GetPipelineLayout(), 0, descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);
    }
}

void VKCommandList::BeginRenderPass(const std::shared_ptr<Framebuffer>& framebuffer)
{
    decltype(auto) vk_framebuffer = framebuffer->As<VKFramebuffer>();
    vk::RenderPassBeginInfo render_pass_info = {};
    render_pass_info.renderPass = vk_framebuffer.GetRenderPass();
    render_pass_info.framebuffer = vk_framebuffer.GetFramebuffer();
    render_pass_info.renderArea.extent = vk_framebuffer.GetExtent();
    m_command_list->beginRenderPass(render_pass_info, vk::SubpassContents::eInline);
}

void VKCommandList::EndRenderPass()
{
    m_command_list->endRenderPass();
}

void VKCommandList::BeginEvent(const std::string& name)
{
    vk::DebugUtilsLabelEXT label = {};
    label.pLabelName = name.c_str();
    m_command_list->beginDebugUtilsLabelEXT(&label);
}

void VKCommandList::EndEvent()
{
    m_command_list->endDebugUtilsLabelEXT();
}

void VKCommandList::ClearColor(const std::shared_ptr<View>& view, const std::array<float, 4>& color)
{
    if (!view)
        return;
    decltype(auto) vk_view = view->As<VKView>();

    std::shared_ptr<Resource> resource = vk_view.GetResource();
    if (!resource)
        return;
    decltype(auto) vk_resource = resource->As<VKResource>();

    vk::ClearColorValue clear_color = {};
    clear_color.float32[0] = color[0];
    clear_color.float32[1] = color[1];
    clear_color.float32[2] = color[2];
    clear_color.float32[3] = color[3];

    const vk::ImageSubresourceRange& ImageSubresourceRange = vk_view.GeViewInfo().subresourceRange;
    m_command_list->clearColorImage(vk_resource.image.res.get(), vk_resource.image.layout[ImageSubresourceRange], clear_color, ImageSubresourceRange);
}

void VKCommandList::ClearDepth(const std::shared_ptr<View>& view, float depth)
{
    if (!view)
        return;
    decltype(auto) vk_view = view->As<VKView>();

    std::shared_ptr<Resource> resource = vk_view.GetResource();
    if (!resource)
        return;
    decltype(auto) vk_resource = resource->As<VKResource>();

    vk::ClearDepthStencilValue clear_color = {};
    clear_color.depth = depth;
    const vk::ImageSubresourceRange& ImageSubresourceRange = vk_view.GeViewInfo().subresourceRange;
    m_command_list->clearDepthStencilImage(vk_resource.image.res.get(), vk_resource.image.layout[ImageSubresourceRange], clear_color, ImageSubresourceRange);
}

void VKCommandList::DrawIndexed(uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location)
{
    m_command_list->drawIndexed(index_count, 1, start_index_location, base_vertex_location, 0);
}

void VKCommandList::Dispatch(uint32_t thread_group_count_x, uint32_t thread_group_count_y, uint32_t thread_group_count_z)
{
    m_command_list->dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z);
}

void VKCommandList::ResourceBarrier(const std::shared_ptr<Resource>& resource, ResourceState state)
{
    if (resource)
        return ResourceBarrier(resource, {}, state);
}

void VKCommandList::SetViewport(float width, float height)
{
    vk::Viewport viewport = {};
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = 0;
    viewport.maxDepth = 1.0;
    m_command_list->setViewport(0, 1, &viewport);

    vk::Rect2D rect = {};
    rect.extent.width = static_cast<int32_t>(width);
    rect.extent.height = static_cast<int32_t>(height);
    m_command_list->setScissor(0, 1, &rect);
}

vk::IndexType GetVkIndexType(gli::format format)
{
    vk::Format vk_format = static_cast<vk::Format>(format);
    switch (vk_format)
    {
    case vk::Format::eR16Uint:
        return vk::IndexType::eUint16;
    case vk::Format::eR32Uint:
        return vk::IndexType::eUint32;
    default:
        assert(false);
        return {};
    }
}

void VKCommandList::IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format)
{
    decltype(auto) vk_resource = resource->As<VKResource>();
    vk::IndexType index_type = GetVkIndexType(format);
    m_command_list->bindIndexBuffer(vk_resource.buffer.res.get(), 0, index_type);
}

void VKCommandList::IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource)
{
    decltype(auto) vk_resource = resource->As<VKResource>();
    vk::Buffer vertex_buffers[] = { vk_resource.buffer.res.get() };
    vk::DeviceSize offsets[] = { 0 };
    m_command_list->bindVertexBuffers(slot, 1, vertex_buffers, offsets);
}

void VKCommandList::UpdateSubresource(const std::shared_ptr<Resource>& resource, uint32_t subresource, const void* data, uint32_t row_pitch, uint32_t depth_pitch)
{
    decltype(auto) vk_resource = resource->As<VKResource>();

    if (vk_resource.res_type == VKResource::Type::kBuffer)
    {
        void* dst_data = nullptr;
        m_device.GetDevice().mapMemory(vk_resource.buffer.memory.get(), 0, vk_resource.buffer.size, {}, &dst_data);
        memcpy(dst_data, data, static_cast<size_t>(vk_resource.buffer.size));
        m_device.GetDevice().unmapMemory(vk_resource.buffer.memory.get());
    }
    else if (vk_resource.res_type == VKResource::Type::kImage)
    {
        auto& staging = vk_resource.GetUploadResource(subresource);
        if (!staging || staging->res_type == VKResource::Type::kUnknown)
            staging = std::static_pointer_cast<VKResource>(m_device.CreateBuffer(0, depth_pitch, 0));
        UpdateSubresource(staging, 0, data, row_pitch, depth_pitch);

        // Setup buffer copy regions for each mip level
        std::vector<vk::BufferImageCopy> bufferCopyRegions;
        uint32_t offset = 0;

        vk::BufferImageCopy bufferCopyRegion = {};
        bufferCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        bufferCopyRegion.imageSubresource.mipLevel = subresource;
        bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
        bufferCopyRegion.imageSubresource.layerCount = 1;
        bufferCopyRegion.imageExtent.width = std::max(1u, static_cast<uint32_t>(vk_resource.image.size.width >> subresource));
        bufferCopyRegion.imageExtent.height = std::max(1u, static_cast<uint32_t>(vk_resource.image.size.height >> subresource));
        bufferCopyRegion.imageExtent.depth = 1;

        bufferCopyRegions.push_back(bufferCopyRegion);

        // The sub resource range describes the regions of the image that will be transitioned using the memory barriers below
        vk::ImageSubresourceRange subresourceRange = {};
        // Image only contains color data
        subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        // Start at first mip level
        subresourceRange.baseMipLevel = subresource;
        // We will transition on all mip levels
        subresourceRange.levelCount = 1;
        // The 2D texture only has one layer
        subresourceRange.layerCount = 1;

        // Transition the texture image layout to transfer target, so we can safely copy our buffer data to it.
        vk::ImageMemoryBarrier imageMemoryBarrier{};
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.image = vk_resource.image.res.get();
        imageMemoryBarrier.subresourceRange = subresourceRange;
        imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        imageMemoryBarrier.oldLayout = vk::ImageLayout::eUndefined;
        imageMemoryBarrier.newLayout = vk::ImageLayout::eTransferDstOptimal;

        // Insert a memory dependency at the proper pipeline stages that will execute the image layout transition 
        // Source pipeline stage is host write/read exection (vk::PipelineStageFlagBits::eHost)
        // Destination pipeline stage is copy command exection (vk::PipelineStageFlagBits::eTransfer)
        m_command_list->pipelineBarrier(
            vk::PipelineStageFlagBits::eHost,
            vk::PipelineStageFlagBits::eTransfer,
            {},
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier);

        // Copy mip levels from staging buffer
        m_command_list->copyBufferToImage(
            staging->buffer.res.get(),
            vk_resource.image.res.get(),
            vk::ImageLayout::eTransferDstOptimal,
            static_cast<uint32_t>(bufferCopyRegions.size()),
            bufferCopyRegions.data());

        // Once the data has been uploaded we transfer to the texture image to the shader read layout, so it can be sampled from
        imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        imageMemoryBarrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        imageMemoryBarrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

        // Insert a memory dependency at the proper pipeline stages that will execute the image layout transition 
        // Source pipeline stage stage is copy command exection (vk::PipelineStageFlagBits::eTransfer)
        // Destination pipeline stage fragment shader access (vk::PipelineStageFlagBits::eFragmentShader)
        m_command_list->pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader,
            {},
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier);

        // Store current layout for later reuse
        vk_resource.image.layout[subresourceRange] = vk::ImageLayout::eShaderReadOnlyOptimal;
    }
}

void VKCommandList::ResourceBarrier(const std::shared_ptr<Resource>& resource, const ViewDesc& view_desc, ResourceState state)
{
    decltype(auto) vk_resource = resource->As<VKResource>();
    VKResource::Image& image = vk_resource.image;
    if (!image.res)
        return;

    vk::ImageLayout new_layout = vk::ImageLayout::eUndefined;
    switch (state)
    {
    case ResourceState::kCommon:
    case ResourceState::kClearColor:
    case ResourceState::kClearDepth:
    case ResourceState::kUnorderedAccess:
        new_layout = vk::ImageLayout::eGeneral;
        break;
    case ResourceState::kPresent:
        new_layout = vk::ImageLayout::ePresentSrcKHR;
        break;
    case ResourceState::kRenderTarget:
        new_layout = vk::ImageLayout::eColorAttachmentOptimal;
        break;
    case ResourceState::kDepthTarget:
        new_layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        break;
    case ResourceState::kPixelShaderResource:
    case ResourceState::kNonPixelShaderResource:
        new_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
        break;
    }

    vk::ImageSubresourceRange range = {};
    range.aspectMask = m_device.GetAspectFlags(image.format);
    range.baseMipLevel = view_desc.level;
    if (new_layout == vk::ImageLayout::eColorAttachmentOptimal)
        range.levelCount = 1;
    else if (view_desc.count == -1)
        range.levelCount = image.level_count - view_desc.level;
    else
        range.levelCount = view_desc.count;
    range.baseArrayLayer = 0;
    range.layerCount = image.array_layers;

    std::vector<vk::ImageMemoryBarrier> image_memory_barriers;

    for (uint32_t i = 0; i < range.levelCount; ++i)
    {
        for (uint32_t j = 0; j < range.layerCount; ++j)
        {
            vk::ImageSubresourceRange barrier_range = range;
            barrier_range.baseMipLevel = range.baseMipLevel + i;
            barrier_range.levelCount = 1;
            barrier_range.baseArrayLayer = range.baseArrayLayer + j;
            barrier_range.layerCount = 1;

            if (image.layout[barrier_range] == new_layout)
                continue;

            image_memory_barriers.emplace_back();
            vk::ImageMemoryBarrier& imageMemoryBarrier = image_memory_barriers.back();
            imageMemoryBarrier.oldLayout = image.layout[barrier_range];

            imageMemoryBarrier.newLayout = new_layout;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.image = image.res.get();
            imageMemoryBarrier.subresourceRange = barrier_range;

            // Source layouts (old)
            // Source access mask controls actions that have to be finished on the old layout
            // before it will be transitioned to the new layout
            switch (image.layout[barrier_range])
            {
            case vk::ImageLayout::eUndefined:
                // Image layout is undefined (or does not matter)
                // Only valid as initial layout
                // No flags required, listed only for completeness
                imageMemoryBarrier.srcAccessMask = {};
                break;
            case vk::ImageLayout::ePreinitialized:
                // Image is preinitialized
                // Only valid as initial layout for linear images, preserves memory contents
                // Make sure host writes have been finished
                imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
                break;
            case vk::ImageLayout::eColorAttachmentOptimal:
                // Image is a color attachment
                // Make sure any writes to the color buffer have been finished
                imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
                break;
            case vk::ImageLayout::eDepthAttachmentOptimal:
                // Image is a depth/stencil attachment
                // Make sure any writes to the depth/stencil buffer have been finished
                imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
                break;
            case vk::ImageLayout::eTransferSrcOptimal:
                // Image is a transfer source 
                // Make sure any reads from the image have been finished
                imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
                break;
            case vk::ImageLayout::eTransferDstOptimal:
                // Image is a transfer destination
                // Make sure any writes to the image have been finished
                imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
                break;

            case vk::ImageLayout::eShaderReadOnlyOptimal:
                // Image is read by a shader
                // Make sure any shader reads from the image have been finished
                imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
                break;
            default:
                // Other source layouts aren't handled (yet)
                break;
            }

            // Target layouts (new)
            // Destination access mask controls the dependency for the new image layout
            switch (new_layout)
            {
            case vk::ImageLayout::eTransferDstOptimal:
                // Image will be used as a transfer destination
                // Make sure any writes to the image have been finished
                imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
                break;

            case vk::ImageLayout::eTransferSrcOptimal:
                // Image will be used as a transfer source
                // Make sure any reads from the image have been finished
                imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
                break;

            case vk::ImageLayout::eColorAttachmentOptimal:
                // Image will be used as a color attachment
                // Make sure any writes to the color buffer have been finished
                imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
                break;

            case vk::ImageLayout::eDepthAttachmentOptimal:
                // Image layout will be used as a depth/stencil attachment
                // Make sure any writes to depth/stencil buffer have been finished
                imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
                break;

            case vk::ImageLayout::eShaderReadOnlyOptimal:
                // Image will be read in a shader (sampler, input attachment)
                // Make sure any writes to the image have been finished
                if (!imageMemoryBarrier.srcAccessMask)
                {
                    imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eTransferWrite;
                }
                imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
                break;
            default:
                // Other source layouts aren't handled (yet)
                break;
            }

            image.layout[barrier_range] = new_layout;
        }
    }

    m_command_list->pipelineBarrier(
        vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands,
        vk::DependencyFlagBits::eByRegion,
        0, nullptr,
        0, nullptr,
        image_memory_barriers.size(), image_memory_barriers.data());
}

vk::CommandBuffer VKCommandList::GetCommandList()
{
    return m_command_list.get();
}
