cmake_minimum_required(VERSION 2.5)
project(mymuduo)

#mymuduo 最终编译成so动态库，设置动态库的路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
#设置为调试模式 以及 声明C++11语言标准
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++14 -fPIC")
#定义参与编译的源文件 起一个别名
aux_source_directory(. SRC_LIST)
#编译生成动态库mymuduo
add_library(mymuduo SHARED ${SRC_LIST})