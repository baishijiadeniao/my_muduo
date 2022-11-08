cmake_minimum_required(VERSION 3.0)
project(main)

#配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

#表示将.路径下所有源文件都放在SRC_LIST变量中
aux_source_directory(. SRC_LIST)

#设置可执行文件存放的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#配置头文件搜索路径
#include_directories()

#配置库文件搜索路径
#link_libraries()
link_libraries(/home/lighthouse/my_muduo/lib)
#表示生成可执行文件server，由SRC_LIST中的源文件编译而来
add_executable(server ${SRC_LIST})

#表示生成server这个目标程序需要链接muduo_net、muduo_base、pthread
target_link_libraries(server muduo_net muduo_base pthread)

#查找子目录中的CMakeLists.txt文件
#add_subdirectory(目录)