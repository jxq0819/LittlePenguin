# 注：此CmakeLists适合protobuf安装在自定义目录下的情况（比如我安装在：/usr/local/protobuf/）
#　　　假如是按照默认位置安装，就是/usr/local/，那么此CMakeLists需要进行调整

cmake_minimum_required (VERSION 3.10)

project (client)

# 设置变量
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD 14)
# 添加编译选项线程库
set(CMAKE_CXX_FLAGS -pthread)
# 设置可执行程序的输出目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/)
# 设置protobuf库安装目录
set(GOOGLE_PROTOBUF_DIR /usr/local/protobuf/)

# include头文件目录
# 由于我的protobuf安装目录为自定义目录，故find_package找不到我安装的protobuf库
# find_package(Protobuf REQUIRED)
# 所以上面set中需要指明protobuf路径
# 包含头文件目录(分别是本程序的incldue位置和protobuf库的include位置)
include_directories (./include ${GOOGLE_PROTOBUF_DIR}/include/)

# 生成可执行文件
aux_source_directory (./src SRC_LIST)
add_executable (client ${SRC_LIST})

# 亲测：使用下面的方法设置链接库目录无效
# set(PROTOBUF_LIBRARY ${GOOGLE_PROTOBUF_DIR}/lib/)
# target_link_libraries(client.out ${PROTOBUF_LIBRARY})
# 有效方法如下：
link_directories(${GOOGLE_PROTOBUF_DIR}/lib/)
# 链接
target_link_libraries(client -lprotobuf)