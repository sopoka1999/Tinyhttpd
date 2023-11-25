# CMake 最低版本号要求
cmake_minimum_required (VERSION 2.8)

# 项目信息
project (Demo3)

# 设置使用 C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 源文件
set(SOURCE_FILES main.cpp ThreadPool.cpp)
set(SOURCE_FILES2 httpd.cpp ThreadPool.cpp)

add_subdirectory(utils)

# 指定生成目标
add_executable(main ${SOURCE_FILES})
add_executable(httpd ${SOURCE_FILES2})

# 添加链接库
target_link_libraries(main utils)

