cmake_minimum_required(VERSION 3.20)
project(AgenticCompiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ZLIB REQUIRED)
find_package(zstd QUIET)
find_package(nlohmann_json REQUIRED)
find_package(CURL REQUIRED)

set(LLVM_DIR "/usr/lib/llvm-18/cmake")
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs 
    Core 
    Support 
    IRReader 
    Analysis
    Passes
)

add_executable(agentic-cc main.cpp)

target_link_libraries(agentic-cc PRIVATE ${llvm_libs} ZLIB::ZLIB nlohmann_json::nlohmann_json CURL::libcurl)
