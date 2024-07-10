cmake_minimum_required(VERSION 3.16)
project(knob_profiler)

find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
set(CMAKE_CXX_COMPILER "clang")

add_library(HelloPass2 MODULE hello_llvm.cc)
target_compile_features(HelloPass2 PRIVATE cxx_range_for cxx_auto_type)
set_target_properties(HelloPass2 PROPERTIES COMPILE_FLAGS "-fno-rtti")
set_target_properties(HelloPass2 PROPERTIES LINK_FLAGS "-Wl,-znodelete -lSegFault")

add_library(KnobDependencyPass MODULE knob_deps_pass.cc)
target_compile_features(KnobDependencyPass PRIVATE cxx_range_for cxx_auto_type)
set_target_properties(KnobDependencyPass PROPERTIES COMPILE_FLAGS "-fno-rtti")
set_target_properties(KnobDependencyPass PROPERTIES LINK_FLAGS "-Wl,-znodelete -lSegFault")


