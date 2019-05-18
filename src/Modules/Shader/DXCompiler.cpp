#include "Shader/DXCompiler.h"
#include "Shader/DXCLoader.h"

class FileWrap
{
public:
    FileWrap(const std::string& path)
        : m_file_path(GetExecutableDir() + "/" + path)
    {
    }

    bool IsExist() const
    {
        return std::ifstream(m_file_path, std::ios::binary).good();
    }

    std::vector<uint8_t> ReadFile() const
    {
        std::ifstream file(m_file_path, std::ios::binary);

        file.seekg(0, std::ios::end);
        size_t file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> res(file_size);
        file.read((char*)res.data(), file_size);
        return res;
    }

private:
    std::string m_file_path;
};

bool GetBlobFromCache(const std::string& shader_path, ComPtr<ID3DBlob>& shader_buffer)
{
    std::string shader_name = shader_path.substr(shader_path.find_last_of("\\/") + 1);
    shader_name = shader_name.substr(0, shader_name.find(".hlsl")) + ".cso";
    FileWrap precompiled_blob(shader_name);
    if (!precompiled_blob.IsExist())
        return false;
    auto data = precompiled_blob.ReadFile();
    D3DCreateBlob(data.size(), &shader_buffer);
    shader_buffer->GetBufferPointer();
    memcpy(shader_buffer->GetBufferPointer(), data.data(), data.size());
    return true;
}

ComPtr<ID3DBlob> DXBCCompile(const ShaderDesc& shader)
{
    ComPtr<ID3DBlob> shader_buffer;
    std::vector<D3D_SHADER_MACRO> macro;
    for (const auto & x : shader.define)
    {
        macro.push_back({ x.first.c_str(), x.second.c_str() });
    }
    macro.push_back({ nullptr, nullptr });

    ComPtr<ID3DBlob> errors;
    ASSERT_SUCCEEDED(D3DCompileFromFile(
        GetAssetFullPathW(shader.shader_path).c_str(),
        macro.data(),
        nullptr,
        shader.entrypoint.c_str(),
        shader.target.c_str(),
        D3DCOMPILE_DEBUG,
        0,
        &shader_buffer,
        &errors));
    if (errors)
        OutputDebugStringA(reinterpret_cast<char*>(errors->GetBufferPointer()));
    return shader_buffer;
}

bool IsDXCTarget(const std::string& target)
{
    size_t major_index = 3;
    if (target.compare(0, 4, "lib_") == 0)
        major_index = 4;
    return major_index < target.size() && isdigit(target[major_index]) && target[major_index] >= '6';
}

ComPtr<ID3DBlob> DXCCompile(const ShaderDesc& shader, const DXOption& option)
{
    DXCLoader loader;

    CComPtr<IDxcBlobEncoding> source;
    ASSERT_SUCCEEDED(loader.library->CreateBlobFromFile(
        GetAssetFullPathW(shader.shader_path).c_str(),
        nullptr,
        &source));

    std::wstring target = utf8_to_wstring(shader.target);
    if (!IsDXCTarget(shader.target))
        target = target[0] + std::wstring(L"s_6_0");

    std::wstring entrypoint = utf8_to_wstring(shader.entrypoint);
    std::vector<std::pair<std::wstring, std::wstring>> defines_store;
    std::vector<DxcDefine> defines;
    for (const auto& define : shader.define)
    {
        defines_store.emplace_back(utf8_to_wstring(define.first), utf8_to_wstring(define.second));
        defines.push_back({ defines_store.back().first.c_str(), defines_store.back().second.c_str() });
    }

    std::vector<LPCWSTR> arguments;
    if (option.spirv)
    {
        arguments.push_back(L"-spirv");
        bool vs_or_gs = target.find(L"vs") != -1 || target.find(L"gs") != -1;
        if (option.spirv_invert_y && vs_or_gs)
            arguments.push_back(L"-fvk-invert-y");
    }

    CComPtr<IDxcOperationResult> result;
    ASSERT_SUCCEEDED(loader.compiler->Compile(
        source,
        L"main.hlsl",
        entrypoint.c_str(),
        target.c_str(),
        arguments.data(), static_cast<UINT32>(arguments.size()),
        defines.data(), static_cast<UINT32>(defines.size()),
        nullptr,
        &result
    ));

    HRESULT hr = {};
    result->GetStatus(&hr);
    ComPtr<ID3DBlob> shader_buffer;
    if (SUCCEEDED(hr))
    {
        ComPtr<IDxcBlob> tmp_blob;
        ASSERT_SUCCEEDED(result->GetResult(&tmp_blob));
        tmp_blob.As(&shader_buffer);
    }
    else
    {
        ComPtr<IDxcBlobEncoding> errors;
        result->GetErrorBuffer(&errors);
        OutputDebugStringA(reinterpret_cast<char*>(errors->GetBufferPointer()));
    }
    return shader_buffer;
}

ComPtr<ID3DBlob> DXCompile(const ShaderDesc& shader, const DXOption& option)
{
    bool dxc_target = IsDXCTarget(shader.target);
    bool different_options = !shader.define.empty();
    different_options |= CurState::Instance().force_dxil != dxc_target;

    ComPtr<ID3DBlob> shader_buffer;
    if (!different_options && GetBlobFromCache(shader.shader_path, shader_buffer))
        return shader_buffer;

    if (dxc_target || option.spirv || CurState::Instance().force_dxil)
        return DXCCompile(shader, option);
    else
        return DXBCCompile(shader);
}
