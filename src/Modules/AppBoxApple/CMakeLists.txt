enable_language(Swift)

if (CMAKE_Swift_COMPILER_VERSION VERSION_LESS 5.9)
   message(FATAL_ERROR "Bidirectional C++ Interop requires Swift 5.9 or greater. Have ${CMAKE_Swift_COMPILER_VERSION}")
endif()

list(APPEND headers
   AppBox.h
   MetalRenderer.h
)

list(APPEND sources
   AppBox.mm
   ContentView.swift
   MetalRenderer.mm
   MetalView.swift
)

if (CMAKE_SYSTEM_NAME STREQUAL "iOS" OR CMAKE_SYSTEM_NAME STREQUAL "tvOS")
   list(APPEND sources
      AppDelegateUI.swift
   )
else()
   list(APPEND sources
      AppDelegateNS.swift
   )
endif()

add_library(AppBoxApple
   ${headers}
   ${sources}
)

set_target_properties(AppBoxApple PROPERTIES
   XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/SwiftBridgingHeader.h"
   XCODE_ATTRIBUTE_SWIFT_OBJC_INTERFACE_HEADER_NAME "AppBoxAppleSwift.h"
)

target_include_directories(AppBoxApple
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(AppBoxApple
   "-framework Metal"
   "-framework QuartzCore"
)

if (CMAKE_SYSTEM_NAME STREQUAL "iOS" OR CMAKE_SYSTEM_NAME STREQUAL "tvOS")
   target_link_libraries(AppBoxApple
      "-framework UIKit"
   )
else()
   target_link_libraries(AppBoxApple
      "-framework AppKit"
   )
endif()

set_target_properties(AppBoxApple PROPERTIES FOLDER "Modules")
