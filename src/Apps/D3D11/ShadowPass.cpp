#include "ShadowPass.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

ShadowPass::ShadowPass(Context& context, const Input& input, int width, int height)
    : m_context(context)
    , m_input(input)
    , m_width(width)
    , m_height(height)
    , m_program(context.device)
{
    CreateTexture();
    CreateViewPort();
    CreateTextureId();
}

void ShadowPass::OnUpdate()
{
    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 Down = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 Left = glm::vec3(-1.0f, 0.0f, 0.0f);
    glm::vec3 Right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 ForwardRH = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 ForwardLH = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 BackwardRH = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 BackwardLH = glm::vec3(0.0f, 0.0f, -1.0f);

    glm::vec3 position = m_input.camera.GetCameraPos();

    m_program.gs.cbuffer.Params.Projection = glm::transpose(glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 1024.0f));

    std::array<glm::mat4, 6>& view = m_program.gs.cbuffer.Params.View;
    view[0] = glm::transpose(glm::lookAt(position, position + Right, Up));
    view[1] = glm::transpose(glm::lookAt(position, position + Left, Up));
    view[2] = glm::transpose(glm::lookAt(position, position + Up, BackwardRH));
    view[3] = glm::transpose(glm::lookAt(position, position + Down, ForwardRH));
    view[4] = glm::transpose(glm::lookAt(position, position + BackwardLH, Up));
    view[5] = glm::transpose(glm::lookAt(position, position + ForwardLH, Up));

    glm::mat4 model = m_input.camera.GetModelMatrix();
    float model_scale = 0.01f;
    model = glm::scale(glm::vec3(model_scale)) * model;

    m_program.vs.cbuffer.Params.World = glm::transpose(model);
}

void ShadowPass::OnRender()
{
    m_context.device_context->RSSetViewports(1, &m_viewport);

    m_program.UseProgram(m_context.device_context);
    m_context.device_context->IASetInputLayout(m_program.vs.input_layout.Get());

    m_context.device_context->ClearDepthStencilView(m_depth_stencil_view.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    m_context.device_context->OMSetRenderTargets(0, nullptr, m_depth_stencil_view.Get());

    for (DX11Mesh& cur_mesh : m_input.model.meshes)
    {
        cur_mesh.SetIndexBuffer();
        cur_mesh.SetVertexBuffer(m_program.vs.geometry.SV_POSITION, VertexType::kPosition);
        m_context.device_context->DrawIndexed(cur_mesh.indices.size(), 0, 0);
    }
    m_context.device_context->GSSetShader(nullptr, nullptr, 0);
}

void ShadowPass::OnResize(int width, int height)
{
}

void ShadowPass::CreateTexture()
{
    D3D11_TEXTURE2D_DESC texture_desc = {};
    texture_desc.Width = m_size;
    texture_desc.Height = m_size;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 6;
    texture_desc.Format = DXGI_FORMAT_R32_TYPELESS;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    texture_desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
    dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
    dsv_desc.Texture2DArray.ArraySize = texture_desc.ArraySize;

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srv_desc.Texture2DArray.ArraySize = texture_desc.ArraySize;
    srv_desc.Texture2DArray.MipLevels = 1;

    ComPtr<ID3D11Texture2D> cube_texture;
    ASSERT_SUCCEEDED(m_context.device->CreateTexture2D(&texture_desc, nullptr, &cube_texture));
    ASSERT_SUCCEEDED(m_context.device->CreateDepthStencilView(cube_texture.Get(), &dsv_desc, &m_depth_stencil_view));
    ASSERT_SUCCEEDED(m_context.device->CreateShaderResourceView(cube_texture.Get(), &srv_desc, &output.srv));
}

void ShadowPass::CreateTextureId()
{
    ComPtr<ID3D11Texture2D> cubeTexture;

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = m_size;
    texDesc.Height = m_size;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 6;
    texDesc.Format = DXGI_FORMAT_R32_FLOAT;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

    D3D11_SHADER_RESOURCE_VIEW_DESC SMViewDesc = {};
    SMViewDesc.Format = texDesc.Format;
    SMViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    SMViewDesc.TextureCube.MipLevels = texDesc.MipLevels;

    D3D11_SUBRESOURCE_DATA pData[6];
    std::vector<float> data[6];
    for (int cubeMapFaceIndex = 0; cubeMapFaceIndex < 6; cubeMapFaceIndex++)
    {
        data[cubeMapFaceIndex].resize(m_size * m_size, cubeMapFaceIndex);
        pData[cubeMapFaceIndex].pSysMem = data[cubeMapFaceIndex].data();
        pData[cubeMapFaceIndex].SysMemPitch = m_size * sizeof(uint32_t);
        pData[cubeMapFaceIndex].SysMemSlicePitch = 0;
    }

    ASSERT_SUCCEEDED(m_context.device->CreateTexture2D(&texDesc, &pData[0], &cubeTexture));
    ASSERT_SUCCEEDED(m_context.device->CreateShaderResourceView(cubeTexture.Get(), &SMViewDesc, &output.cubemap_id));
}

void ShadowPass::CreateViewPort()
{
    ZeroMemory(&m_viewport, sizeof(D3D11_VIEWPORT));
    m_viewport.TopLeftX = 0;
    m_viewport.TopLeftY = 0;
    m_viewport.Width = m_size;
    m_viewport.Height = m_size;
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;
}
