set(BUILD_TESTING_SAVED "${BUILD_TESTING}")
set(BUILD_TESTING OFF)

add_subdirectory(${project_root}/3rdparty/nowide nowide EXCLUDE_FROM_ALL)
set_target_properties(nowide PROPERTIES FOLDER "3rdparty/nowide")

set(BUILD_TESTING "${BUILD_TESTING_SAVED}")
