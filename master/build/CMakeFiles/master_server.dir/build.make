# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.20

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/test/tencent_mini_proj/LittlePenguin/master

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/test/tencent_mini_proj/LittlePenguin/master/build

# Include any dependencies generated for this target.
include CMakeFiles/master_server.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/master_server.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/master_server.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/master_server.dir/flags.make

CMakeFiles/master_server.dir/src/HashSlot.cc.o: CMakeFiles/master_server.dir/flags.make
CMakeFiles/master_server.dir/src/HashSlot.cc.o: ../src/HashSlot.cc
CMakeFiles/master_server.dir/src/HashSlot.cc.o: CMakeFiles/master_server.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/test/tencent_mini_proj/LittlePenguin/master/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/master_server.dir/src/HashSlot.cc.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/master_server.dir/src/HashSlot.cc.o -MF CMakeFiles/master_server.dir/src/HashSlot.cc.o.d -o CMakeFiles/master_server.dir/src/HashSlot.cc.o -c /home/test/tencent_mini_proj/LittlePenguin/master/src/HashSlot.cc

CMakeFiles/master_server.dir/src/HashSlot.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/master_server.dir/src/HashSlot.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/test/tencent_mini_proj/LittlePenguin/master/src/HashSlot.cc > CMakeFiles/master_server.dir/src/HashSlot.cc.i

CMakeFiles/master_server.dir/src/HashSlot.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/master_server.dir/src/HashSlot.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/test/tencent_mini_proj/LittlePenguin/master/src/HashSlot.cc -o CMakeFiles/master_server.dir/src/HashSlot.cc.s

CMakeFiles/master_server.dir/src/MasterServer.cpp.o: CMakeFiles/master_server.dir/flags.make
CMakeFiles/master_server.dir/src/MasterServer.cpp.o: ../src/MasterServer.cpp
CMakeFiles/master_server.dir/src/MasterServer.cpp.o: CMakeFiles/master_server.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/test/tencent_mini_proj/LittlePenguin/master/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/master_server.dir/src/MasterServer.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/master_server.dir/src/MasterServer.cpp.o -MF CMakeFiles/master_server.dir/src/MasterServer.cpp.o.d -o CMakeFiles/master_server.dir/src/MasterServer.cpp.o -c /home/test/tencent_mini_proj/LittlePenguin/master/src/MasterServer.cpp

CMakeFiles/master_server.dir/src/MasterServer.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/master_server.dir/src/MasterServer.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/test/tencent_mini_proj/LittlePenguin/master/src/MasterServer.cpp > CMakeFiles/master_server.dir/src/MasterServer.cpp.i

CMakeFiles/master_server.dir/src/MasterServer.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/master_server.dir/src/MasterServer.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/test/tencent_mini_proj/LittlePenguin/master/src/MasterServer.cpp -o CMakeFiles/master_server.dir/src/MasterServer.cpp.s

CMakeFiles/master_server.dir/src/TcpServer.cpp.o: CMakeFiles/master_server.dir/flags.make
CMakeFiles/master_server.dir/src/TcpServer.cpp.o: ../src/TcpServer.cpp
CMakeFiles/master_server.dir/src/TcpServer.cpp.o: CMakeFiles/master_server.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/test/tencent_mini_proj/LittlePenguin/master/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/master_server.dir/src/TcpServer.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/master_server.dir/src/TcpServer.cpp.o -MF CMakeFiles/master_server.dir/src/TcpServer.cpp.o.d -o CMakeFiles/master_server.dir/src/TcpServer.cpp.o -c /home/test/tencent_mini_proj/LittlePenguin/master/src/TcpServer.cpp

CMakeFiles/master_server.dir/src/TcpServer.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/master_server.dir/src/TcpServer.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/test/tencent_mini_proj/LittlePenguin/master/src/TcpServer.cpp > CMakeFiles/master_server.dir/src/TcpServer.cpp.i

CMakeFiles/master_server.dir/src/TcpServer.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/master_server.dir/src/TcpServer.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/test/tencent_mini_proj/LittlePenguin/master/src/TcpServer.cpp -o CMakeFiles/master_server.dir/src/TcpServer.cpp.s

CMakeFiles/master_server.dir/src/TcpSocket.cpp.o: CMakeFiles/master_server.dir/flags.make
CMakeFiles/master_server.dir/src/TcpSocket.cpp.o: ../src/TcpSocket.cpp
CMakeFiles/master_server.dir/src/TcpSocket.cpp.o: CMakeFiles/master_server.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/test/tencent_mini_proj/LittlePenguin/master/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object CMakeFiles/master_server.dir/src/TcpSocket.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/master_server.dir/src/TcpSocket.cpp.o -MF CMakeFiles/master_server.dir/src/TcpSocket.cpp.o.d -o CMakeFiles/master_server.dir/src/TcpSocket.cpp.o -c /home/test/tencent_mini_proj/LittlePenguin/master/src/TcpSocket.cpp

CMakeFiles/master_server.dir/src/TcpSocket.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/master_server.dir/src/TcpSocket.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/test/tencent_mini_proj/LittlePenguin/master/src/TcpSocket.cpp > CMakeFiles/master_server.dir/src/TcpSocket.cpp.i

CMakeFiles/master_server.dir/src/TcpSocket.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/master_server.dir/src/TcpSocket.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/test/tencent_mini_proj/LittlePenguin/master/src/TcpSocket.cpp -o CMakeFiles/master_server.dir/src/TcpSocket.cpp.s

CMakeFiles/master_server.dir/src/cmcdata.pb.cc.o: CMakeFiles/master_server.dir/flags.make
CMakeFiles/master_server.dir/src/cmcdata.pb.cc.o: ../src/cmcdata.pb.cc
CMakeFiles/master_server.dir/src/cmcdata.pb.cc.o: CMakeFiles/master_server.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/test/tencent_mini_proj/LittlePenguin/master/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object CMakeFiles/master_server.dir/src/cmcdata.pb.cc.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/master_server.dir/src/cmcdata.pb.cc.o -MF CMakeFiles/master_server.dir/src/cmcdata.pb.cc.o.d -o CMakeFiles/master_server.dir/src/cmcdata.pb.cc.o -c /home/test/tencent_mini_proj/LittlePenguin/master/src/cmcdata.pb.cc

CMakeFiles/master_server.dir/src/cmcdata.pb.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/master_server.dir/src/cmcdata.pb.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/test/tencent_mini_proj/LittlePenguin/master/src/cmcdata.pb.cc > CMakeFiles/master_server.dir/src/cmcdata.pb.cc.i

CMakeFiles/master_server.dir/src/cmcdata.pb.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/master_server.dir/src/cmcdata.pb.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/test/tencent_mini_proj/LittlePenguin/master/src/cmcdata.pb.cc -o CMakeFiles/master_server.dir/src/cmcdata.pb.cc.s

CMakeFiles/master_server.dir/src/crc16.cpp.o: CMakeFiles/master_server.dir/flags.make
CMakeFiles/master_server.dir/src/crc16.cpp.o: ../src/crc16.cpp
CMakeFiles/master_server.dir/src/crc16.cpp.o: CMakeFiles/master_server.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/test/tencent_mini_proj/LittlePenguin/master/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building CXX object CMakeFiles/master_server.dir/src/crc16.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/master_server.dir/src/crc16.cpp.o -MF CMakeFiles/master_server.dir/src/crc16.cpp.o.d -o CMakeFiles/master_server.dir/src/crc16.cpp.o -c /home/test/tencent_mini_proj/LittlePenguin/master/src/crc16.cpp

CMakeFiles/master_server.dir/src/crc16.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/master_server.dir/src/crc16.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/test/tencent_mini_proj/LittlePenguin/master/src/crc16.cpp > CMakeFiles/master_server.dir/src/crc16.cpp.i

CMakeFiles/master_server.dir/src/crc16.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/master_server.dir/src/crc16.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/test/tencent_mini_proj/LittlePenguin/master/src/crc16.cpp -o CMakeFiles/master_server.dir/src/crc16.cpp.s

CMakeFiles/master_server.dir/src/master_main.cpp.o: CMakeFiles/master_server.dir/flags.make
CMakeFiles/master_server.dir/src/master_main.cpp.o: ../src/master_main.cpp
CMakeFiles/master_server.dir/src/master_main.cpp.o: CMakeFiles/master_server.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/test/tencent_mini_proj/LittlePenguin/master/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building CXX object CMakeFiles/master_server.dir/src/master_main.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/master_server.dir/src/master_main.cpp.o -MF CMakeFiles/master_server.dir/src/master_main.cpp.o.d -o CMakeFiles/master_server.dir/src/master_main.cpp.o -c /home/test/tencent_mini_proj/LittlePenguin/master/src/master_main.cpp

CMakeFiles/master_server.dir/src/master_main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/master_server.dir/src/master_main.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/test/tencent_mini_proj/LittlePenguin/master/src/master_main.cpp > CMakeFiles/master_server.dir/src/master_main.cpp.i

CMakeFiles/master_server.dir/src/master_main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/master_server.dir/src/master_main.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/test/tencent_mini_proj/LittlePenguin/master/src/master_main.cpp -o CMakeFiles/master_server.dir/src/master_main.cpp.s

# Object files for target master_server
master_server_OBJECTS = \
"CMakeFiles/master_server.dir/src/HashSlot.cc.o" \
"CMakeFiles/master_server.dir/src/MasterServer.cpp.o" \
"CMakeFiles/master_server.dir/src/TcpServer.cpp.o" \
"CMakeFiles/master_server.dir/src/TcpSocket.cpp.o" \
"CMakeFiles/master_server.dir/src/cmcdata.pb.cc.o" \
"CMakeFiles/master_server.dir/src/crc16.cpp.o" \
"CMakeFiles/master_server.dir/src/master_main.cpp.o"

# External object files for target master_server
master_server_EXTERNAL_OBJECTS =

../bin/master_server: CMakeFiles/master_server.dir/src/HashSlot.cc.o
../bin/master_server: CMakeFiles/master_server.dir/src/MasterServer.cpp.o
../bin/master_server: CMakeFiles/master_server.dir/src/TcpServer.cpp.o
../bin/master_server: CMakeFiles/master_server.dir/src/TcpSocket.cpp.o
../bin/master_server: CMakeFiles/master_server.dir/src/cmcdata.pb.cc.o
../bin/master_server: CMakeFiles/master_server.dir/src/crc16.cpp.o
../bin/master_server: CMakeFiles/master_server.dir/src/master_main.cpp.o
../bin/master_server: CMakeFiles/master_server.dir/build.make
../bin/master_server: CMakeFiles/master_server.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/test/tencent_mini_proj/LittlePenguin/master/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Linking CXX executable ../bin/master_server"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/master_server.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/master_server.dir/build: ../bin/master_server
.PHONY : CMakeFiles/master_server.dir/build

CMakeFiles/master_server.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/master_server.dir/cmake_clean.cmake
.PHONY : CMakeFiles/master_server.dir/clean

CMakeFiles/master_server.dir/depend:
	cd /home/test/tencent_mini_proj/LittlePenguin/master/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/test/tencent_mini_proj/LittlePenguin/master /home/test/tencent_mini_proj/LittlePenguin/master /home/test/tencent_mini_proj/LittlePenguin/master/build /home/test/tencent_mini_proj/LittlePenguin/master/build /home/test/tencent_mini_proj/LittlePenguin/master/build/CMakeFiles/master_server.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/master_server.dir/depend

