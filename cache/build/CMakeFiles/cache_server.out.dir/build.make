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
CMAKE_SOURCE_DIR = /home/test/dc/LittlePenguin/cache

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/test/dc/LittlePenguin/cache/build

# Include any dependencies generated for this target.
include CMakeFiles/cache_server.out.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/cache_server.out.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/cache_server.out.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/cache_server.out.dir/flags.make

CMakeFiles/cache_server.out.dir/src/CacheServer.cpp.o: CMakeFiles/cache_server.out.dir/flags.make
CMakeFiles/cache_server.out.dir/src/CacheServer.cpp.o: ../src/CacheServer.cpp
CMakeFiles/cache_server.out.dir/src/CacheServer.cpp.o: CMakeFiles/cache_server.out.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/test/dc/LittlePenguin/cache/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/cache_server.out.dir/src/CacheServer.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/cache_server.out.dir/src/CacheServer.cpp.o -MF CMakeFiles/cache_server.out.dir/src/CacheServer.cpp.o.d -o CMakeFiles/cache_server.out.dir/src/CacheServer.cpp.o -c /home/test/dc/LittlePenguin/cache/src/CacheServer.cpp

CMakeFiles/cache_server.out.dir/src/CacheServer.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/cache_server.out.dir/src/CacheServer.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/test/dc/LittlePenguin/cache/src/CacheServer.cpp > CMakeFiles/cache_server.out.dir/src/CacheServer.cpp.i

CMakeFiles/cache_server.out.dir/src/CacheServer.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/cache_server.out.dir/src/CacheServer.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/test/dc/LittlePenguin/cache/src/CacheServer.cpp -o CMakeFiles/cache_server.out.dir/src/CacheServer.cpp.s

CMakeFiles/cache_server.out.dir/src/HashSlot.cc.o: CMakeFiles/cache_server.out.dir/flags.make
CMakeFiles/cache_server.out.dir/src/HashSlot.cc.o: ../src/HashSlot.cc
CMakeFiles/cache_server.out.dir/src/HashSlot.cc.o: CMakeFiles/cache_server.out.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/test/dc/LittlePenguin/cache/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/cache_server.out.dir/src/HashSlot.cc.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/cache_server.out.dir/src/HashSlot.cc.o -MF CMakeFiles/cache_server.out.dir/src/HashSlot.cc.o.d -o CMakeFiles/cache_server.out.dir/src/HashSlot.cc.o -c /home/test/dc/LittlePenguin/cache/src/HashSlot.cc

CMakeFiles/cache_server.out.dir/src/HashSlot.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/cache_server.out.dir/src/HashSlot.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/test/dc/LittlePenguin/cache/src/HashSlot.cc > CMakeFiles/cache_server.out.dir/src/HashSlot.cc.i

CMakeFiles/cache_server.out.dir/src/HashSlot.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/cache_server.out.dir/src/HashSlot.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/test/dc/LittlePenguin/cache/src/HashSlot.cc -o CMakeFiles/cache_server.out.dir/src/HashSlot.cc.s

CMakeFiles/cache_server.out.dir/src/LRUCache.cpp.o: CMakeFiles/cache_server.out.dir/flags.make
CMakeFiles/cache_server.out.dir/src/LRUCache.cpp.o: ../src/LRUCache.cpp
CMakeFiles/cache_server.out.dir/src/LRUCache.cpp.o: CMakeFiles/cache_server.out.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/test/dc/LittlePenguin/cache/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/cache_server.out.dir/src/LRUCache.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/cache_server.out.dir/src/LRUCache.cpp.o -MF CMakeFiles/cache_server.out.dir/src/LRUCache.cpp.o.d -o CMakeFiles/cache_server.out.dir/src/LRUCache.cpp.o -c /home/test/dc/LittlePenguin/cache/src/LRUCache.cpp

CMakeFiles/cache_server.out.dir/src/LRUCache.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/cache_server.out.dir/src/LRUCache.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/test/dc/LittlePenguin/cache/src/LRUCache.cpp > CMakeFiles/cache_server.out.dir/src/LRUCache.cpp.i

CMakeFiles/cache_server.out.dir/src/LRUCache.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/cache_server.out.dir/src/LRUCache.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/test/dc/LittlePenguin/cache/src/LRUCache.cpp -o CMakeFiles/cache_server.out.dir/src/LRUCache.cpp.s

CMakeFiles/cache_server.out.dir/src/TcpServer.cpp.o: CMakeFiles/cache_server.out.dir/flags.make
CMakeFiles/cache_server.out.dir/src/TcpServer.cpp.o: ../src/TcpServer.cpp
CMakeFiles/cache_server.out.dir/src/TcpServer.cpp.o: CMakeFiles/cache_server.out.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/test/dc/LittlePenguin/cache/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object CMakeFiles/cache_server.out.dir/src/TcpServer.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/cache_server.out.dir/src/TcpServer.cpp.o -MF CMakeFiles/cache_server.out.dir/src/TcpServer.cpp.o.d -o CMakeFiles/cache_server.out.dir/src/TcpServer.cpp.o -c /home/test/dc/LittlePenguin/cache/src/TcpServer.cpp

CMakeFiles/cache_server.out.dir/src/TcpServer.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/cache_server.out.dir/src/TcpServer.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/test/dc/LittlePenguin/cache/src/TcpServer.cpp > CMakeFiles/cache_server.out.dir/src/TcpServer.cpp.i

CMakeFiles/cache_server.out.dir/src/TcpServer.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/cache_server.out.dir/src/TcpServer.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/test/dc/LittlePenguin/cache/src/TcpServer.cpp -o CMakeFiles/cache_server.out.dir/src/TcpServer.cpp.s

CMakeFiles/cache_server.out.dir/src/TcpSocket.cpp.o: CMakeFiles/cache_server.out.dir/flags.make
CMakeFiles/cache_server.out.dir/src/TcpSocket.cpp.o: ../src/TcpSocket.cpp
CMakeFiles/cache_server.out.dir/src/TcpSocket.cpp.o: CMakeFiles/cache_server.out.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/test/dc/LittlePenguin/cache/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object CMakeFiles/cache_server.out.dir/src/TcpSocket.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/cache_server.out.dir/src/TcpSocket.cpp.o -MF CMakeFiles/cache_server.out.dir/src/TcpSocket.cpp.o.d -o CMakeFiles/cache_server.out.dir/src/TcpSocket.cpp.o -c /home/test/dc/LittlePenguin/cache/src/TcpSocket.cpp

CMakeFiles/cache_server.out.dir/src/TcpSocket.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/cache_server.out.dir/src/TcpSocket.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/test/dc/LittlePenguin/cache/src/TcpSocket.cpp > CMakeFiles/cache_server.out.dir/src/TcpSocket.cpp.i

CMakeFiles/cache_server.out.dir/src/TcpSocket.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/cache_server.out.dir/src/TcpSocket.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/test/dc/LittlePenguin/cache/src/TcpSocket.cpp -o CMakeFiles/cache_server.out.dir/src/TcpSocket.cpp.s

CMakeFiles/cache_server.out.dir/src/cache_main.cpp.o: CMakeFiles/cache_server.out.dir/flags.make
CMakeFiles/cache_server.out.dir/src/cache_main.cpp.o: ../src/cache_main.cpp
CMakeFiles/cache_server.out.dir/src/cache_main.cpp.o: CMakeFiles/cache_server.out.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/test/dc/LittlePenguin/cache/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building CXX object CMakeFiles/cache_server.out.dir/src/cache_main.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/cache_server.out.dir/src/cache_main.cpp.o -MF CMakeFiles/cache_server.out.dir/src/cache_main.cpp.o.d -o CMakeFiles/cache_server.out.dir/src/cache_main.cpp.o -c /home/test/dc/LittlePenguin/cache/src/cache_main.cpp

CMakeFiles/cache_server.out.dir/src/cache_main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/cache_server.out.dir/src/cache_main.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/test/dc/LittlePenguin/cache/src/cache_main.cpp > CMakeFiles/cache_server.out.dir/src/cache_main.cpp.i

CMakeFiles/cache_server.out.dir/src/cache_main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/cache_server.out.dir/src/cache_main.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/test/dc/LittlePenguin/cache/src/cache_main.cpp -o CMakeFiles/cache_server.out.dir/src/cache_main.cpp.s

CMakeFiles/cache_server.out.dir/src/cmcdata.pb.cc.o: CMakeFiles/cache_server.out.dir/flags.make
CMakeFiles/cache_server.out.dir/src/cmcdata.pb.cc.o: ../src/cmcdata.pb.cc
CMakeFiles/cache_server.out.dir/src/cmcdata.pb.cc.o: CMakeFiles/cache_server.out.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/test/dc/LittlePenguin/cache/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building CXX object CMakeFiles/cache_server.out.dir/src/cmcdata.pb.cc.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/cache_server.out.dir/src/cmcdata.pb.cc.o -MF CMakeFiles/cache_server.out.dir/src/cmcdata.pb.cc.o.d -o CMakeFiles/cache_server.out.dir/src/cmcdata.pb.cc.o -c /home/test/dc/LittlePenguin/cache/src/cmcdata.pb.cc

CMakeFiles/cache_server.out.dir/src/cmcdata.pb.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/cache_server.out.dir/src/cmcdata.pb.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/test/dc/LittlePenguin/cache/src/cmcdata.pb.cc > CMakeFiles/cache_server.out.dir/src/cmcdata.pb.cc.i

CMakeFiles/cache_server.out.dir/src/cmcdata.pb.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/cache_server.out.dir/src/cmcdata.pb.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/test/dc/LittlePenguin/cache/src/cmcdata.pb.cc -o CMakeFiles/cache_server.out.dir/src/cmcdata.pb.cc.s

CMakeFiles/cache_server.out.dir/src/crc16.cpp.o: CMakeFiles/cache_server.out.dir/flags.make
CMakeFiles/cache_server.out.dir/src/crc16.cpp.o: ../src/crc16.cpp
CMakeFiles/cache_server.out.dir/src/crc16.cpp.o: CMakeFiles/cache_server.out.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/test/dc/LittlePenguin/cache/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building CXX object CMakeFiles/cache_server.out.dir/src/crc16.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/cache_server.out.dir/src/crc16.cpp.o -MF CMakeFiles/cache_server.out.dir/src/crc16.cpp.o.d -o CMakeFiles/cache_server.out.dir/src/crc16.cpp.o -c /home/test/dc/LittlePenguin/cache/src/crc16.cpp

CMakeFiles/cache_server.out.dir/src/crc16.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/cache_server.out.dir/src/crc16.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/test/dc/LittlePenguin/cache/src/crc16.cpp > CMakeFiles/cache_server.out.dir/src/crc16.cpp.i

CMakeFiles/cache_server.out.dir/src/crc16.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/cache_server.out.dir/src/crc16.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/test/dc/LittlePenguin/cache/src/crc16.cpp -o CMakeFiles/cache_server.out.dir/src/crc16.cpp.s

# Object files for target cache_server.out
cache_server_out_OBJECTS = \
"CMakeFiles/cache_server.out.dir/src/CacheServer.cpp.o" \
"CMakeFiles/cache_server.out.dir/src/HashSlot.cc.o" \
"CMakeFiles/cache_server.out.dir/src/LRUCache.cpp.o" \
"CMakeFiles/cache_server.out.dir/src/TcpServer.cpp.o" \
"CMakeFiles/cache_server.out.dir/src/TcpSocket.cpp.o" \
"CMakeFiles/cache_server.out.dir/src/cache_main.cpp.o" \
"CMakeFiles/cache_server.out.dir/src/cmcdata.pb.cc.o" \
"CMakeFiles/cache_server.out.dir/src/crc16.cpp.o"

# External object files for target cache_server.out
cache_server_out_EXTERNAL_OBJECTS =

../bin/cache_server.out: CMakeFiles/cache_server.out.dir/src/CacheServer.cpp.o
../bin/cache_server.out: CMakeFiles/cache_server.out.dir/src/HashSlot.cc.o
../bin/cache_server.out: CMakeFiles/cache_server.out.dir/src/LRUCache.cpp.o
../bin/cache_server.out: CMakeFiles/cache_server.out.dir/src/TcpServer.cpp.o
../bin/cache_server.out: CMakeFiles/cache_server.out.dir/src/TcpSocket.cpp.o
../bin/cache_server.out: CMakeFiles/cache_server.out.dir/src/cache_main.cpp.o
../bin/cache_server.out: CMakeFiles/cache_server.out.dir/src/cmcdata.pb.cc.o
../bin/cache_server.out: CMakeFiles/cache_server.out.dir/src/crc16.cpp.o
../bin/cache_server.out: CMakeFiles/cache_server.out.dir/build.make
../bin/cache_server.out: CMakeFiles/cache_server.out.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/test/dc/LittlePenguin/cache/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Linking CXX executable ../bin/cache_server.out"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/cache_server.out.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/cache_server.out.dir/build: ../bin/cache_server.out
.PHONY : CMakeFiles/cache_server.out.dir/build

CMakeFiles/cache_server.out.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/cache_server.out.dir/cmake_clean.cmake
.PHONY : CMakeFiles/cache_server.out.dir/clean

CMakeFiles/cache_server.out.dir/depend:
	cd /home/test/dc/LittlePenguin/cache/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/test/dc/LittlePenguin/cache /home/test/dc/LittlePenguin/cache /home/test/dc/LittlePenguin/cache/build /home/test/dc/LittlePenguin/cache/build /home/test/dc/LittlePenguin/cache/build/CMakeFiles/cache_server.out.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/cache_server.out.dir/depend

