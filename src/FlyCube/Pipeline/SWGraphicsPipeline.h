#pragma once
#include "Instance/BaseTypes.h"
#include "Pipeline/Pipeline.h"

class SWDevice;

class SWGraphicsPipeline : public Pipeline {
public:
    SWGraphicsPipeline(SWDevice& device, const GraphicsPipelineDesc& desc);
    PipelineType GetPipelineType() const override;
    std::vector<uint8_t> GetRayTracingShaderGroupHandles(uint32_t first_group, uint32_t group_count) const override;

private:
    GraphicsPipelineDesc m_desc;
};
