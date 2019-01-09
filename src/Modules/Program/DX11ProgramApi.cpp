#include "Program/DX11ProgramApi.h"
#include <Shader/DXCompiler.h>

DX11ProgramApi::DX11ProgramApi(DX11Context& context)
    : CommonProgramApi(context)
    , m_context(context)
    , m_view_creater(context, *this)
{
}

void DX11ProgramApi::LinkProgram()
{
}

void DX11ProgramApi::UseProgram()
{
    m_context.UseProgram(*this);
    m_context.device_context->VSSetShader(vshader.Get(), nullptr, 0);
    m_context.device_context->GSSetShader(gshader.Get(), nullptr, 0);
    m_context.device_context->DSSetShader(nullptr, nullptr, 0);
    m_context.device_context->HSSetShader(nullptr, nullptr, 0);
    m_context.device_context->PSSetShader(pshader.Get(), nullptr, 0);
    m_context.device_context->CSSetShader(cshader.Get(), nullptr, 0);

    ID3D11ShaderResourceView* empty_srv[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};
    m_context.device_context->VSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, empty_srv);
    m_context.device_context->GSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, empty_srv);
    m_context.device_context->DSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, empty_srv);
    m_context.device_context->HSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, empty_srv);
    m_context.device_context->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, empty_srv);
    m_context.device_context->CSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, empty_srv);

    ID3D11UnorderedAccessView* empty_uav[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    m_context.device_context->CSSetUnorderedAccessViews(0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, empty_uav, nullptr);

    ID3D11RenderTargetView* empty_rtv[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    ID3D11DepthStencilView* empty_dsv = nullptr;
    m_context.device_context->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, empty_rtv, empty_dsv);

    m_context.device_context->IASetInputLayout(input_layout.Get());
    m_context.device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void DX11ProgramApi::ApplyBindings()
{
    UpdateCBuffers();
    
    std::vector<ID3D11RenderTargetView*> rtv_ptr;
    for (auto & rtv : m_rtvs)
    {
        rtv_ptr.emplace_back(rtv.Get());
    }
    ComPtr<ID3D11DepthStencilView> dsv = m_dsv.Get();
    m_context.device_context->OMSetRenderTargetsAndUnorderedAccessViews(static_cast<uint32_t>(rtv_ptr.size()), rtv_ptr.data(), dsv.Get(), 0, D3D11_KEEP_UNORDERED_ACCESS_VIEWS, nullptr, nullptr);
}

void DX11ProgramApi::CreateInputLayout()
{
    ComPtr<ID3D11ShaderReflection> reflector;
    D3DReflect(m_blob_map[ShaderType::kVertex]->GetBufferPointer(), m_blob_map[ShaderType::kVertex]->GetBufferSize(), IID_PPV_ARGS(&reflector));

    D3D11_SHADER_DESC shader_desc = {};
    reflector->GetDesc(&shader_desc);

    std::vector<D3D11_INPUT_ELEMENT_DESC> input_layout_desc;
    for (UINT i = 0; i < shader_desc.InputParameters; ++i)
    {
        D3D11_SIGNATURE_PARAMETER_DESC param_desc = {};
        reflector->GetInputParameterDesc(i, &param_desc);

        D3D11_INPUT_ELEMENT_DESC layout = {};
        layout.SemanticName = param_desc.SemanticName;
        layout.SemanticIndex = param_desc.SemanticIndex;
        layout.InputSlot = i;
        layout.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        layout.InstanceDataStepRate = 0;

        if (param_desc.Mask == 1)
        {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                layout.Format = DXGI_FORMAT_R32_UINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                layout.Format = DXGI_FORMAT_R32_SINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                layout.Format = DXGI_FORMAT_R32_FLOAT;
        }
        else if (param_desc.Mask <= 3)
        {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                layout.Format = DXGI_FORMAT_R32G32_UINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                layout.Format = DXGI_FORMAT_R32G32_SINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                layout.Format = DXGI_FORMAT_R32G32_FLOAT;
        }
        else if (param_desc.Mask <= 7)
        {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                layout.Format = DXGI_FORMAT_R32G32B32_UINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                layout.Format = DXGI_FORMAT_R32G32B32_SINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                layout.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        }
        else if (param_desc.Mask <= 15)
        {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                layout.Format = DXGI_FORMAT_R32G32B32A32_UINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                layout.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                layout.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }
        input_layout_desc.push_back(layout);
    }

    ASSERT_SUCCEEDED(m_context.device->CreateInputLayout(
        input_layout_desc.data(),
        static_cast<uint32_t>(input_layout_desc.size()),
        m_blob_map[ShaderType::kVertex]->GetBufferPointer(),
        m_blob_map[ShaderType::kVertex]->GetBufferSize(), &input_layout));
}

void DX11ProgramApi::CompileShader(const ShaderBase& shader)
{
    auto blob = DXCompile(shader);
    m_blob_map[shader.type] = blob;
    switch (shader.type)
    {
    case ShaderType::kVertex:
        ASSERT_SUCCEEDED(m_context.device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &vshader));
        CreateInputLayout();
        break;
    case ShaderType::kPixel:
        ASSERT_SUCCEEDED(m_context.device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &pshader));
        break;
    case ShaderType::kCompute:
        ASSERT_SUCCEEDED(m_context.device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &cshader));
        break;
    case ShaderType::kGeometry:
        ASSERT_SUCCEEDED(m_context.device->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &gshader));
        break;
    }
}

void DX11ProgramApi::Attach(const BindKey& bind_key, const ViewDesc& view_desc, const Resource::Ptr& res)
{
    auto name = GetBindingName(bind_key);
    switch (bind_key.res_type)
    {
    case ResourceType::kSrv:
        AttachSRV(bind_key.shader_type, name, bind_key.slot, res);
        break;
    case ResourceType::kUav:
        AttachUAV(bind_key.shader_type, name, bind_key.slot, res);
        break;
    case ResourceType::kCbv:
        AttachCBV(bind_key.shader_type, bind_key.slot, res);
        break;
    case ResourceType::kSampler:
        AttachSampler(bind_key.shader_type, name, bind_key.slot, res);
        break;
    case ResourceType::kRtv:
        AttachRTV(bind_key.slot, res);
        break;
    case ResourceType::kDsv:
        AttachDSV(res);
        break;
    }
}

void DX11ProgramApi::AttachSRV(ShaderType type, const std::string & name, uint32_t slot, const Resource::Ptr & res)
{
    AttachView(type, slot, m_view_creater.CreateSrv(type, name, slot, res));
}

void DX11ProgramApi::AttachUAV(ShaderType type, const std::string & name, uint32_t slot, const Resource::Ptr & res)
{
    AttachView(type, slot, m_view_creater.CreateUAV(type, name, slot, res));
}

void DX11ProgramApi::AttachView(ShaderType type, uint32_t slot, ComPtr<ID3D11SamplerState>& sampler)
{
    switch (type)
    {
    case ShaderType::kVertex:
        m_context.device_context->VSSetSamplers(slot, 1, sampler.GetAddressOf());
        break;
    case ShaderType::kPixel:
        m_context.device_context->PSSetSamplers(slot, 1, sampler.GetAddressOf());
        break;
    case ShaderType::kCompute:
        m_context.device_context->CSSetSamplers(slot, 1, sampler.GetAddressOf());
        break;
    case ShaderType::kGeometry:
        m_context.device_context->GSSetSamplers(slot, 1, sampler.GetAddressOf());
        break;
    }
}

void DX11ProgramApi::AttachSampler(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires)
{
    ComPtr<ID3D11SamplerState> sampler;
    if (ires)
    {
        DX11Resource& res = static_cast<DX11Resource&>(*ires);
        sampler = res.sampler;
    }
    AttachView(type, slot, sampler);
}

void DX11ProgramApi::AttachRTV(uint32_t slot, const Resource::Ptr& ires)
{
    if (m_rtvs.size() >= slot)
        m_rtvs.resize(slot + 1);
    m_rtvs[slot] = m_view_creater.CreateRtv(slot, ires);
}

void DX11ProgramApi::AttachDSV(const Resource::Ptr& ires)
{
    m_dsv = m_view_creater.CreateDsv(ires);
}

void DX11ProgramApi::ClearRenderTarget(uint32_t slot, const FLOAT ColorRGBA[4])
{
    m_context.device_context->ClearRenderTargetView(m_rtvs[slot].Get(), ColorRGBA);
}

void DX11ProgramApi::ClearDepthStencil(UINT ClearFlags, FLOAT Depth, UINT8 Stencil)
{
    m_context.device_context->ClearDepthStencilView(m_dsv.Get(), ClearFlags, Depth, Stencil);
}

void DX11ProgramApi::SetRasterizeState(const RasterizerDesc & desc)
{
    D3D11_RASTERIZER_DESC rs_desc = {};
    switch (desc.fill_mode)
    {
    case FillMode::kWireframe:
        rs_desc.FillMode = D3D11_FILL_WIREFRAME;
        break;
    case FillMode::kSolid:
        rs_desc.FillMode = D3D11_FILL_SOLID;
        break;
    }

    switch (desc.cull_mode)
    {
    case CullMode::kNone:
        rs_desc.CullMode = D3D11_CULL_NONE;
        break;
    case CullMode::kFront:
        rs_desc.CullMode = D3D11_CULL_FRONT;
        break;
    case CullMode::kBack:
        rs_desc.CullMode = D3D11_CULL_BACK;
        break;
    }

    rs_desc.DepthBias = desc.DepthBias;

    ComPtr<ID3D11RasterizerState> rasterizer_state;
    m_context.device->CreateRasterizerState(&rs_desc, &rasterizer_state);
    m_context.device_context->RSSetState(rasterizer_state.Get());
}

void DX11ProgramApi::SetBlendState(const BlendDesc& desc)
{
    D3D11_BLEND_DESC blend_desc = {};

    D3D11_RENDER_TARGET_BLEND_DESC& rt_desc = blend_desc.RenderTarget[0];

    auto convert = [](Blend type)
    {
        switch (type)
        {
        case Blend::kZero:
            return D3D11_BLEND_ZERO;
        case Blend::kSrcAlpha:
            return D3D11_BLEND_SRC_ALPHA;
        case Blend::kInvSrcAlpha:
            return D3D11_BLEND_INV_SRC_ALPHA;
        }
        return static_cast<D3D11_BLEND>(0);
    };

    auto convert_op = [](BlendOp type)
    {
        switch (type)
        {
        case BlendOp::kAdd:
            return D3D11_BLEND_OP_ADD;
        }
        return static_cast<D3D11_BLEND_OP>(0);
    };

    rt_desc.BlendEnable = desc.blend_enable;
    rt_desc.BlendOp = convert_op(desc.blend_op);
    rt_desc.SrcBlend = convert(desc.blend_src);
    rt_desc.DestBlend = convert(desc.blend_dest);
    rt_desc.BlendOpAlpha = convert_op(desc.blend_op_alpha);
    rt_desc.SrcBlendAlpha = convert(desc.blend_src_alpha);
    rt_desc.DestBlendAlpha = convert(desc.blend_dest_apha);
    rt_desc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    ComPtr<ID3D11BlendState> blend_state;
    m_context.device->CreateBlendState(&blend_desc, &blend_state);

    const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
    m_context.device_context->OMSetBlendState(blend_state.Get(), blend_factor, 0xffffffff);
}

void DX11ProgramApi::SetDepthStencilState(const DepthStencilDesc& desc)
{
    D3D11_DEPTH_STENCIL_DESC depth_stencil_desc = {};
    depth_stencil_desc.DepthEnable = desc.depth_enable;
    ComPtr<ID3D11DepthStencilState> depth_stencil_state;
    m_context.device->CreateDepthStencilState(&depth_stencil_desc, &depth_stencil_state);
    m_context.device_context->OMSetDepthStencilState(depth_stencil_state.Get(), 0);
}

void DX11ProgramApi::AttachCBV(ShaderType type, uint32_t slot, const Resource::Ptr & ires)
{
    auto res = std::static_pointer_cast<DX11Resource>(ires);

    ComPtr<ID3D11Buffer> buf;
    res->resource.As(&buf);

    switch (type)
    {
    case ShaderType::kVertex:
        m_context.device_context->VSSetConstantBuffers(slot, 1, buf.GetAddressOf());
        break;
    case ShaderType::kPixel:
        m_context.device_context->PSSetConstantBuffers(slot, 1, buf.GetAddressOf());
        break;
    case ShaderType::kCompute:
        m_context.device_context->CSSetConstantBuffers(slot, 1, buf.GetAddressOf());
        break;
    case ShaderType::kGeometry:
        m_context.device_context->GSSetConstantBuffers(slot, 1, buf.GetAddressOf());
        break;
    }
}

void DX11ProgramApi::AttachView(ShaderType type, uint32_t slot, ComPtr<ID3D11UnorderedAccessView>& uav)
{
    switch (type)
    {
    case ShaderType::kCompute:
        m_context.device_context->CSSetUnorderedAccessViews(slot, 1, uav.GetAddressOf(), nullptr);
        break;
    case ShaderType::kPixel:
        m_context.device_context->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr, slot, 1, uav.GetAddressOf(), nullptr);
        break;
    }
}

void DX11ProgramApi::AttachView(ShaderType type, uint32_t slot, ComPtr<ID3D11ShaderResourceView>& srv)
{
    switch (type)
    {
    case ShaderType::kVertex:
        m_context.device_context->VSSetShaderResources(slot, 1, srv.GetAddressOf());
        break;
    case ShaderType::kPixel:
        m_context.device_context->PSSetShaderResources(slot, 1, srv.GetAddressOf());
        break;
    case ShaderType::kCompute:
        m_context.device_context->CSSetShaderResources(slot, 1, srv.GetAddressOf());
        break;
    case ShaderType::kGeometry:
        m_context.device_context->GSSetShaderResources(slot, 1, srv.GetAddressOf());
        break;
    }
}
