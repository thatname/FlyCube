#include "Pipeline/VKPipeline.h"
#include <Device/VKDevice.h>
#include <Program/VKProgram.h>
#include <Shader/SpirvShader.h>

vk::ShaderStageFlagBits ExecutionModel2Bit(spv::ExecutionModel model)
{
    switch (model)
    {
    case spv::ExecutionModel::ExecutionModelVertex:
        return vk::ShaderStageFlagBits::eVertex;
    case spv::ExecutionModel::ExecutionModelFragment:
        return vk::ShaderStageFlagBits::eFragment;
    case spv::ExecutionModel::ExecutionModelGeometry:
        return vk::ShaderStageFlagBits::eGeometry;
    case spv::ExecutionModel::ExecutionModelGLCompute:
        return vk::ShaderStageFlagBits::eCompute;
    case spv::ExecutionModel::ExecutionModelRayGenerationNV:
        return vk::ShaderStageFlagBits::eRaygenNV;
    case spv::ExecutionModel::ExecutionModelIntersectionNV:
        return vk::ShaderStageFlagBits::eIntersectionNV;
    case spv::ExecutionModel::ExecutionModelAnyHitNV:
        return vk::ShaderStageFlagBits::eAnyHitNV;
    case spv::ExecutionModel::ExecutionModelClosestHitNV:
        return vk::ShaderStageFlagBits::eClosestHitNV;
    case spv::ExecutionModel::ExecutionModelMissNV:
        return vk::ShaderStageFlagBits::eMissNV;
    case spv::ExecutionModel::ExecutionModelCallableNV:
        return vk::ShaderStageFlagBits::eCallableNV;
    }
    assert(false);
    return {};
}

VKPipeline::VKPipeline(VKDevice& device, const GraphicsPipelineDesc& desc)
    : m_device(device)
    , m_desc(desc)
    , m_render_pass(device, desc.rtvs, desc.dsv)
{
    decltype(auto) vk_program = desc.program->As<VKProgram>();
    auto shaders = vk_program.GetShaders();
    for (auto& shader : shaders)
    {
        ShaderType shader_type = shader->GetType();
        auto blob = shader->GetBlob();
        switch (shader_type)
        {
        case ShaderType::kVertex:
            CreateInputLayout(blob, m_binding_desc, m_attribute_desc);
            break;
        }

        vk::ShaderModuleCreateInfo shader_module_info = {};
        shader_module_info.codeSize = sizeof(uint32_t) * blob.size();
        shader_module_info.pCode = blob.data();

        m_shader_modules[shader_type] = m_device.GetDevice().createShaderModuleUnique(shader_module_info);

        spirv_cross::CompilerHLSL compiler(blob);
        m_entries[shader_type] = compiler.get_entry_points_and_stages();

        for (auto& entry_point : m_entries[shader_type])
        {
            m_shader_stage_create_info.emplace_back();
            m_shader_stage_create_info.back().stage = ExecutionModel2Bit(entry_point.execution_model);
            m_shader_stage_create_info.back().module = m_shader_modules[shader_type].get();
            m_shader_stage_create_info.back().pName = entry_point.name.c_str();
            m_shader_stage_create_info.back().pSpecializationInfo = NULL;
        }
    }

    CreateGrPipeLine();
}

vk::Pipeline VKPipeline::GetPipeline() const
{
    return m_pipeline.get();
}

vk::RenderPass VKPipeline::GetRenderPass() const
{
    return m_render_pass.GetRenderPass();
}

void VKPipeline::CreateInputLayout(const std::vector<uint32_t>& spirv_binary,
                                        std::vector<vk::VertexInputBindingDescription>& m_binding_desc,
                                        std::vector<vk::VertexInputAttributeDescription>& m_attribute_desc)
{
    spirv_cross::CompilerHLSL compiler(spirv_binary);
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();
    for (auto& resource : resources.stage_inputs)
    {
        auto& type = compiler.get_type(resource.base_type_id);
        unsigned location = compiler.get_decoration(resource.id, spv::DecorationLocation);

        m_binding_desc.emplace_back();
        auto& binding = m_binding_desc.back();
        m_attribute_desc.emplace_back();
        auto& attribute = m_attribute_desc.back();

        attribute.binding = location;
        attribute.location = location;
        binding.binding = location;
        binding.inputRate = vk::VertexInputRate::eVertex;
        binding.stride = type.vecsize * type.width / 8;

        if (type.basetype == spirv_cross::SPIRType::Float)
        {
            if (type.vecsize == 1)
                attribute.format = vk::Format::eR32Sfloat;
            else if (type.vecsize == 2)
                attribute.format = vk::Format::eR32G32Sfloat;
            else if (type.vecsize == 3)
                attribute.format = vk::Format::eR32G32B32Sfloat;
            else if (type.vecsize == 4)
                attribute.format = vk::Format::eR32G32B32A32Sfloat;
        }
        else if (type.basetype == spirv_cross::SPIRType::UInt)
        {
            if (type.vecsize == 1)
                attribute.format = vk::Format::eR32Uint;
            else if (type.vecsize == 2)
                attribute.format = vk::Format::eR32G32Uint;
            else if (type.vecsize == 3)
                attribute.format = vk::Format::eR32G32B32Uint;
            else if (type.vecsize == 4)
                attribute.format = vk::Format::eR32G32B32A32Uint;
        }
        else if (type.basetype == spirv_cross::SPIRType::Int)
        {
            if (type.vecsize == 1)
                attribute.format = vk::Format::eR32Sint;
            else if (type.vecsize == 2)
                attribute.format = vk::Format::eR32G32Sint;
            else if (type.vecsize == 3)
                attribute.format = vk::Format::eR32G32B32Sint;
            else if (type.vecsize == 4)
                attribute.format = vk::Format::eR32G32B32A32Sint;
        }
    }
}

void VKPipeline::CreateGrPipeLine()
{
    decltype(auto) vk_program = m_desc.program->As<VKProgram>();

    vk::GraphicsPipelineCreateInfo pipeline_info = {};

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};

    vertexInputInfo.vertexBindingDescriptionCount = m_binding_desc.size();
    vertexInputInfo.pVertexBindingDescriptions = m_binding_desc.data();
    vertexInputInfo.vertexAttributeDescriptionCount = m_attribute_desc.size();
    vertexInputInfo.pVertexAttributeDescriptions = m_attribute_desc.data();

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    vk::PipelineViewportStateCreateInfo viewportState = {};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    vk::PipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.depthBiasEnable = m_rasterizer_desc.DepthBias != 0;
    rasterizer.depthBiasConstantFactor = m_rasterizer_desc.DepthBias;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = m_blend_desc.blend_enable;

    if (colorBlendAttachment.blendEnable)
    {
        auto convert = [](Blend type)
        {
            switch (type)
            {
            case Blend::kZero:
                return vk::BlendFactor::eZero;
            case Blend::kSrcAlpha:
                return vk::BlendFactor::eSrcAlpha;
            case Blend::kInvSrcAlpha:
                return vk::BlendFactor::eOneMinusSrcAlpha;
            }
            throw std::runtime_error("unsupported");
        };

        auto convert_op = [](BlendOp type)
        {
            switch (type)
            {
            case BlendOp::kAdd:
                return vk::BlendOp::eAdd;
            }
            throw std::runtime_error("unsupported");
        };

        colorBlendAttachment.srcColorBlendFactor = convert(m_blend_desc.blend_src);
        colorBlendAttachment.dstColorBlendFactor = convert(m_blend_desc.blend_dest);
        colorBlendAttachment.colorBlendOp = convert_op(m_blend_desc.blend_op);
        colorBlendAttachment.srcAlphaBlendFactor = convert(m_blend_desc.blend_src_alpha);
        colorBlendAttachment.dstAlphaBlendFactor = convert(m_blend_desc.blend_dest_apha);
        colorBlendAttachment.alphaBlendOp = convert_op(m_blend_desc.blend_op_alpha);
    }

    std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments(m_desc.rtvs.size(), colorBlendAttachment);

    vk::PipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = vk::LogicOp::eAnd;
    colorBlending.attachmentCount = colorBlendAttachments.size();
    colorBlending.pAttachments = colorBlendAttachments.data();

    vk::PipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.rasterizationSamples = static_cast<vk::SampleCountFlagBits>(m_msaa_count);
    multisampling.sampleShadingEnable = multisampling.rasterizationSamples != vk::SampleCountFlagBits::e1;

    vk::PipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.depthTestEnable = m_depth_stencil_desc.depth_enable;
    depthStencil.depthWriteEnable = m_depth_stencil_desc.depth_enable;
    depthStencil.depthCompareOp = vk::CompareOp::eLessOrEqual;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    pipeline_info.stageCount = m_shader_stage_create_info.size();
    pipeline_info.pStages = m_shader_stage_create_info.data();

    pipeline_info.pVertexInputState = &vertexInputInfo;
    pipeline_info.pInputAssemblyState = &inputAssembly;
    pipeline_info.pViewportState = &viewportState;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = &depthStencil;
    pipeline_info.pColorBlendState = &colorBlending;

    pipeline_info.layout = vk_program.GetPipelineLayout();

    pipeline_info.renderPass = m_render_pass.GetRenderPass();
    pipeline_info.subpass = 0;

    std::vector<vk::DynamicState> dynamicStateEnables = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };
    vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
    pipelineDynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();
    pipelineDynamicStateCreateInfo.dynamicStateCount = dynamicStateEnables.size();

    pipeline_info.pDynamicState = &pipelineDynamicStateCreateInfo;

    m_pipeline = m_device.GetDevice().createGraphicsPipelineUnique({}, pipeline_info);
}
