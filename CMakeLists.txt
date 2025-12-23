cmake_minimum_required(VERSION 3.15)
project(rlxr C)

option(BUILD_RLXR_EXAMPLES "Builds the examples." OFF)
option(SUPPORT_TRACELOG "Enables Raylib TraceLog." ON)

# Force building examples if building in the root as standalone.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(BUILD_RLXR_EXAMPLES TRUE)
endif()

# rlxr
add_library(rlxr INTERFACE)

install(FILES ${CMAKE_CURRENT_LIST_DIR}/src/rlxr.h DESTINATION include)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR}/src)

if(${SUPPORT_TRACELOG})
    target_compile_definitions(rlxr INTERFACE RLXR_SUPPORT_TRACELOG)
endif()

# link dependencies for user
find_package(OpenXR REQUIRED)
target_link_libraries(rlxr INTERFACE OpenXR::openxr_loader)

# link libs for XR graphics bindings
if(WIN32)
    find_package(OpenGL REQUIRED)
    target_link_libraries(rlxr INTERFACE OpenGL::GL)
elseif(ANDROID)
    # find_package seems to always fail under android
    # find_package(OpenGL COMPONENTS GLES3 REQUIRED)

    target_link_libraries(rlxr INTERFACE EGL INTERFACE GLESv3)
else()
    find_package(OpenGL REQUIRED)
    find_package(X11 REQUIRED)
    # find_package(Wayland)

    target_link_libraries(rlxr INTERFACE X11::X11 INTERFACE OpenGL::GL INTERFACE m)
endif()

# examples
if(${BUILD_RLXR_EXAMPLES})
    find_package(raylib REQUIRED)
    set(example_sources)

    # list all examples
    file(GLOB sources ${CMAKE_CURRENT_LIST_DIR}/examples/*.c)
    list(APPEND example_sources ${sources})

    foreach(example_source ${example_sources})
        # Create the basename for the example
        get_filename_component(example_name ${example_source} NAME)
        string(REPLACE ".c" "${OUTPUT_EXT}" example_name ${example_name})

        # Setup the example
        add_executable(${example_name} ${example_source})
        target_link_libraries(${example_name} PUBLIC raylib rlxr)
        target_compile_options(${example_name} PRIVATE -Wall)
    endforeach()
endif()
