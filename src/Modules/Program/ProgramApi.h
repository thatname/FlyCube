#pragma once

#include <Context/BaseTypes.h>
#include <Context/Resource.h>
#include <Utilities/FileUtility.h>
#include <Utilities/DXUtility.h>
#include <map>
#include <memory>
#include <vector>

#include <d3dcompiler.h>
#include <wrl.h>
#include <assert.h>
#include "IShaderBlobProvider.h"

using namespace Microsoft::WRL;

class BufferLayout;
class ShaderBase;

class ProgramApi : public IShaderBlobProvider
{
public:
    ProgramApi();
    virtual size_t GetProgramId() const override;
    void SetBindingName(const BindKey& bind_key, const std::string& name);
    const std::string& GetBindingName(const BindKey& bind_key) const;
    virtual void AddAvailableShaderType(ShaderType type) {}
    virtual void LinkProgram() = 0;
    virtual void UseProgram() = 0;
    virtual void ApplyBindings() = 0;
    virtual void CompileShader(const ShaderBase& shader) = 0;
    virtual void SetCBufferLayout(const BindKey& bind_key, BufferLayout& buffer_layout) = 0;
    virtual void Attach(const BindKey& bind_key, const ViewDesc& view_desc, const Resource::Ptr& res) = 0;
    virtual void ClearRenderTarget(uint32_t slot, const FLOAT ColorRGBA[4]) = 0;
    virtual void ClearDepthStencil(UINT ClearFlags, FLOAT Depth, UINT8 Stencil) = 0;
    virtual void SetRasterizeState(const RasterizerDesc& desc) = 0;
    virtual void SetBlendState(const BlendDesc& desc) = 0;
    virtual void SetDepthStencilState(const DepthStencilDesc& desc) = 0;
protected:
    size_t m_program_id;
    std::map<BindKey, std::string> m_binding_names;
};
