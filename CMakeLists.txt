cmake_minimum_required(VERSION 3.16)

# Set debug mode
set(CMAKE_BUILD_TYPE Debug)

if (MSVC)
    message("Using MSVC")
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
else()
    message("Using clang")

    # Find clang executable and set it as compiler
    find_program(Clang_X clang REQUIRED True)
    set(CMAKE_C_COMPILER ${Clang_X})
    # Find clang executable and set it as compiler
    find_program(Clang_P clang++ REQUIRED True)
    set(CMAKE_CXX_COMPILER ${Clang_P})

    # Use clang lld instead of gnu ld
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")

    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -pedantic -Werror
        -Wno-unused-variable
    )

    # Enable clang ubsan when in debug mode
    if(CMAKE_BUILD_TYPE MATCHES Debug)
        message("Adding ubsan!")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=integer")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=nullability")
        set(CMAKE_EXE_LINKER_FLAGS
            "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=implicit-conversion")
        set(CMAKE_EXE_LINKER_FLAGS
            "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=array-bounds")
        add_compile_options(-fno-omit-frame-pointer)
    endif()
endif()

# set the project name
project(KVM_Fuzzer VERSION 0.1)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
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
