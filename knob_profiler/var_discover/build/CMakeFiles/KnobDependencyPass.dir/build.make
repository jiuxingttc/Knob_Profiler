# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/LLVM_MYSQL/knob_profiler/knob_profiler/var_discover

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/LLVM_MYSQL/knob_profiler/knob_profiler/var_discover/build

# Include any dependencies generated for this target.
include CMakeFiles/KnobDependencyPass.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/KnobDependencyPass.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/KnobDependencyPass.dir/flags.make

CMakeFiles/KnobDependencyPass.dir/knob_deps_pass.cc.o: CMakeFiles/KnobDependencyPass.dir/flags.make
CMakeFiles/KnobDependencyPass.dir/knob_deps_pass.cc.o: ../knob_deps_pass.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/LLVM_MYSQL/knob_profiler/knob_profiler/var_discover/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/KnobDependencyPass.dir/knob_deps_pass.cc.o"
	clang  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/KnobDependencyPass.dir/knob_deps_pass.cc.o -c /home/LLVM_MYSQL/knob_profiler/knob_profiler/var_discover/knob_deps_pass.cc

CMakeFiles/KnobDependencyPass.dir/knob_deps_pass.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/KnobDependencyPass.dir/knob_deps_pass.cc.i"
	clang $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/LLVM_MYSQL/knob_profiler/knob_profiler/var_discover/knob_deps_pass.cc > CMakeFiles/KnobDependencyPass.dir/knob_deps_pass.cc.i

CMakeFiles/KnobDependencyPass.dir/knob_deps_pass.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/KnobDependencyPass.dir/knob_deps_pass.cc.s"
	clang $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/LLVM_MYSQL/knob_profiler/knob_profiler/var_discover/knob_deps_pass.cc -o CMakeFiles/KnobDependencyPass.dir/knob_deps_pass.cc.s

# Object files for target KnobDependencyPass
KnobDependencyPass_OBJECTS = \
"CMakeFiles/KnobDependencyPass.dir/knob_deps_pass.cc.o"

# External object files for target KnobDependencyPass
KnobDependencyPass_EXTERNAL_OBJECTS =

libKnobDependencyPass.so: CMakeFiles/KnobDependencyPass.dir/knob_deps_pass.cc.o
libKnobDependencyPass.so: CMakeFiles/KnobDependencyPass.dir/build.make
libKnobDependencyPass.so: CMakeFiles/KnobDependencyPass.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/LLVM_MYSQL/knob_profiler/knob_profiler/var_discover/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX shared module libKnobDependencyPass.so"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/KnobDependencyPass.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/KnobDependencyPass.dir/build: libKnobDependencyPass.so

.PHONY : CMakeFiles/KnobDependencyPass.dir/build

CMakeFiles/KnobDependencyPass.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/KnobDependencyPass.dir/cmake_clean.cmake
.PHONY : CMakeFiles/KnobDependencyPass.dir/clean

CMakeFiles/KnobDependencyPass.dir/depend:
	cd /home/LLVM_MYSQL/knob_profiler/knob_profiler/var_discover/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/LLVM_MYSQL/knob_profiler/knob_profiler/var_discover /home/LLVM_MYSQL/knob_profiler/knob_profiler/var_discover /home/LLVM_MYSQL/knob_profiler/knob_profiler/var_discover/build /home/LLVM_MYSQL/knob_profiler/knob_profiler/var_discover/build /home/LLVM_MYSQL/knob_profiler/knob_profiler/var_discover/build/CMakeFiles/KnobDependencyPass.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/KnobDependencyPass.dir/depend

