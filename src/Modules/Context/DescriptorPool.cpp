#include "Context/DescriptorPool.h"

#include <assert.h>

#include "Context/DX12Context.h"

DescriptorHeapRange::DescriptorHeapRange(
    DX12Context& context,
    ComPtr<ID3D12DescriptorHeap>& heap,
    D3D12_CPU_DESCRIPTOR_HANDLE& cpu_handle,
    D3D12_GPU_DESCRIPTOR_HANDLE& gpu_handle,
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& copied_handle,
    size_t offset,
    size_t size,
    uint32_t increment_size,
    D3D12_DESCRIPTOR_HEAP_TYPE type)
    : m_context(context)
    , m_heap(heap)
    , m_cpu_handle(cpu_handle)
    , m_gpu_handle(gpu_handle)
    , m_copied_handle(copied_handle)
    , m_offset(offset)
    , m_size(size)
    , m_increment_size(increment_size)
    , m_type(type)
{
}

void DescriptorHeapRange::CopyCpuHandle(size_t dst_offset, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    if (m_copied_handle.get()[m_offset + dst_offset].ptr == handle.ptr)
        return;
    D3D12_CPU_DESCRIPTOR_HANDLE self = GetCpuHandle(dst_offset);
    m_context.get().device->CopyDescriptors(
        1, &self, nullptr,
        1, &handle, nullptr,
        m_type);
    m_copied_handle.get()[m_offset + dst_offset].ptr = handle.ptr;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapRange::GetCpuHandle(size_t offset) const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        m_cpu_handle,
        static_cast<INT>(m_offset + offset),
        m_increment_size);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapRange::GetGpuHandle(size_t offset) const
{
    return CD3DX12_GPU_DESCRIPTOR_HANDLE(
        m_gpu_handle,
        static_cast<INT>(m_offset + offset),
        m_increment_size);
}

const ComPtr<ID3D12DescriptorHeap>& DescriptorHeapRange::GetHeap() const
{
    return m_heap;
}

size_t DescriptorHeapRange::GetSize() const
{
    return m_size;
}

DescriptorHeapAllocator::DescriptorHeapAllocator(DX12Context& context, D3D12_DESCRIPTOR_HEAP_TYPE type)
    : m_context(context)
    , m_type(type)
    , m_offset(0)
    , m_size(0)
{
}

DescriptorHeapRange DescriptorHeapAllocator::Allocate(size_t count)
{
    if (m_offset + count > m_size)
        ResizeHeap(std::max(m_offset + count, 2 * (m_size + 1)));
    m_offset += count;
    return DescriptorHeapRange(m_context, m_heap, m_cpu_handle, m_gpu_handle, m_copied_handle, m_offset - count, count, m_context.device->GetDescriptorHandleIncrementSize(m_type), m_type);
}

void DescriptorHeapAllocator::ResizeHeap(size_t req_size)
{
    if (m_size >= req_size)
        return;

    ComPtr<ID3D12DescriptorHeap> heap;
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.NumDescriptors = static_cast<UINT>(req_size);
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heap_desc.Type = m_type;
    ASSERT_SUCCEEDED(m_context.device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap)));

    m_context.QueryOnDelete(m_heap);
    
    m_size = heap_desc.NumDescriptors;
    m_heap = heap;
    m_cpu_handle = m_heap->GetCPUDescriptorHandleForHeapStart();
    m_gpu_handle = m_heap->GetGPUDescriptorHandleForHeapStart();
    m_copied_handle.assign(m_size, {});
}

void DescriptorHeapAllocator::ResetHeap()
{
    m_offset = 0;
}

DescriptorPool::DescriptorPool(DX12Context& context)
    : m_context(context)
    , m_shader_resource(m_context, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    , m_shader_sampler(m_context, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
{
}

DescriptorHeapRange DescriptorPool::Allocate(ResourceType res_type, size_t count)
{
    switch (res_type)
    {
    case ResourceType::kSampler:
        return m_shader_sampler.Allocate(count);
    default:
        return m_shader_resource.Allocate(count);
    }
}
