cmake_minimum_required(VERSION 3.16)


# set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -stdlib=libc++")

# Set debug mode
set(CMAKE_BUILD_TYPE Debug)

if (MSVC)
    message("Using MSVC")
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
else()

    message("Using clang")
    find_program(Clang_P clang++ REQUIRED True)
    set(CMAKE_CXX_COMPILER ${Clang_P})
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -pedantic -Werror
        -Wno-unused-variable
    )

    if(CMAKE_BUILD_TYPE MATCHES Debug)
        message("Adding ubsan!")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined")
    endif()
endif()

# set the project name
project(KVM_Fuzzer VERSION 0.1)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Get num cores & set num threads for build
include(ProcessorCount)
ProcessorCount(N)
if(NOT N EQUAL 0)
    set(CMAKE_BUILD_PARALLEL_LEVEL ${N})
endif()

# add FetchContent
include(FetchContent)

# add the executable
add_executable(${PROJECT_NAME} src/main.cpp)

# add include directory include/
target_include_directories(${PROJECT_NAME} PUBLIC include)

FetchContent_Declare(
fmt
GIT_REPOSITORY https://github.com/fmtlib/fmt
GIT_TAG 7.0.3
GIT_PROGRESS True
)
FetchContent_MakeAvailable(fmt)

target_link_libraries(${PROJECT_NAME} fmt)
