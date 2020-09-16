cmake_minimum_required(VERSION 3.16)

# set the project name
project(KVM_Fuzzer VERSION 0.1)

# add FetchContent
include(FetchContent)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# add the executable
add_executable(${PROJECT_NAME} src/main.cpp)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# add include directory include/
target_include_directories(${PROJECT_NAME} PUBLIC include)

# Get num cores & set num threads for build
include(ProcessorCount)
ProcessorCount(N)
if(NOT N EQUAL 0)
    set(CMAKE_BUILD_PARALLEL_LEVEL ${N})
endif()

FetchContent_Declare(
fmt
GIT_REPOSITORY https://github.com/fmtlib/fmt
GIT_TAG 7.0.3
GIT_PROGRESS True
)
FetchContent_MakeAvailable(fmt)

target_link_libraries(${PROJECT_NAME} fmt)
