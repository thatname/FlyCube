list(APPEND headers
    Fence.h
)

if (VULKAN_SUPPORT)
    list(APPEND headers
        VKTimelineSemaphore.h
    )
    list(APPEND sources
        VKTimelineSemaphore.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXFence.h
    )
    list(APPEND sources
        DXFence.cpp
    )
endif()

if (METAL_SUPPORT)
    list(APPEND headers
        MTFence.h
    )
    list(APPEND sources
        MTFence.mm
    )
endif()

if (SOFTWARE_SUPPORT)
    list(APPEND headers
        SWFence.h
    )
    list(APPEND sources
        SWFence.cpp
    )
endif()

add_library(Fence ${headers} ${sources})

target_include_directories(Fence
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(Fence
    ApiType
    Device
    Utilities
)

if (VULKAN_SUPPORT)
    target_link_libraries(Fence
        vulkan
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(Fence
        d3d12
        dxgi
        dxguid
    )
endif()

set_target_properties(Fence PROPERTIES FOLDER "FlyCube")
