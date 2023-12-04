add_subdirectory(${project_root}/3rdparty/Vulkan-Headers Vulkan-Headers EXCLUDE_FROM_ALL)

add_library(vulkan INTERFACE)
target_link_libraries(vulkan INTERFACE Vulkan-Headers)

if (NOT VULKAN_SUPPORT)
    return()
endif()

if (APPLE)
    set_property(TARGET vulkan APPEND PROPERTY
        INTERFACE_COMPILE_DEFINITIONS
            USE_STATIC_MOLTENVK
    )

    if (CUSTOM_MOLTENVK)
        set(moltenvk_xcframework "${CUSTOM_MOLTENVK}/MoltenVK.xcframework")
    elseif(CUSTOM_VULKAN_SDK)
        set(moltenvk_xcframework "${CUSTOM_VULKAN_SDK}/MoltenVK/MoltenVK.xcframework")
    elseif(DEFINED ENV{VULKAN_SDK})
        set(moltenvk_xcframework "$ENV{VULKAN_SDK}/MoltenVK/MoltenVK.xcframework")
    else()
        message(FATAL_ERROR "MoltenVK.xcframework is missing")
    endif()
    message("MoltenVK.xcframework location is ${moltenvk_xcframework}")

    set_property(TARGET vulkan APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES
            "${moltenvk_xcframework}"
    )
    set_property(TARGET vulkan APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES
            "-framework IOSurface"
            "-framework CoreGraphics"
    )
    if (NOT IOS_OR_TVOS)
        set_property(TARGET vulkan APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES
                "-framework IOKit"
        )
    endif()
else()
    set_property(TARGET vulkan APPEND PROPERTY
        INTERFACE_COMPILE_DEFINITIONS
            DVULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
            VK_NO_PROTOTYPES
    )
endif()

if (WIN32)
    set_property(TARGET vulkan APPEND PROPERTY
        INTERFACE_COMPILE_DEFINITIONS
            VK_USE_PLATFORM_WIN32_KHR
    )
elseif(APPLE)
    set_property(TARGET vulkan APPEND PROPERTY
        INTERFACE_COMPILE_DEFINITIONS
            VK_USE_PLATFORM_METAL_EXT
    )
elseif(NOT ANDROID)
    set_property(TARGET vulkan APPEND PROPERTY
        INTERFACE_COMPILE_DEFINITIONS
            VK_USE_PLATFORM_XCB_KHR
    )
endif()
