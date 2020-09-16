cmake_minimum_required(VERSION 3.16)

# set the project name
project(KVM_Fuzzer VERSION 0.1)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# add the executable
add_executable(KVM_Fuzzer src/main.cpp)

target_include_directories(KVM_Fuzzer PUBLIC include)






