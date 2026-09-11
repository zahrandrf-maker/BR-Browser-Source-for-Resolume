cmake_minimum_required(VERSION 3.20)
project(BR_Browser_Source VERSION 0.1.0 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "BR Browser Source targets Windows only.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(FFGL_BUILD_EXAMPLE_PLUGINS OFF CACHE BOOL "" FORCE)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/ffgl/CMakeLists.txt")
    message(FATAL_ERROR "Resolume FFGL SDK is missing. Run the GitHub Actions workflow or clone it into external/ffgl.")
endif()

add_subdirectory(external/ffgl ffgl-build EXCLUDE_FROM_ALL)

add_library(BR_Browser_Source MODULE
    src/BRBrowserSource.cpp
    src/BRBrowserSource.h
)

target_compile_features(BR_Browser_Source PRIVATE cxx_std_17)
target_compile_definitions(BR_Browser_Source PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _WIN32_WINNT=0x0A00
    WINVER=0x0A00
)
target_compile_options(BR_Browser_Source PRIVATE /W4 /permissive- /EHsc)
target_link_libraries(BR_Browser_Source PRIVATE ffgl::sdk opengl32)

set_target_properties(BR_Browser_Source PROPERTIES
    PREFIX ""
    OUTPUT_NAME "BR_Browser_Source_FFGL21"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
