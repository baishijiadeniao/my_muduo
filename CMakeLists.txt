#cmake最低版本
cmake_minimum_required(VERSION 2.8.9)
#项目名称
project(m_muduo)

include_directories(${PROJECT_SOURCE_DIR}/include)
#将输出库都放在/lib中
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
#编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -fPIC")
#定义参与编译的源代码文件
aux_source_directory(./src SRC_LIST)
#翻译生成动态库
add_library(m_mduo SHARED ${SRC_LIST})