# ============================================================================
#  CMakeLists.txt  ——  告诉 CMake：这个项目有哪些文件、要用 OpenCV
# ----------------------------------------------------------------------------
#  在 armor/ 目录下执行：
#      mkdir -p build
#      cd build
#      cmake ..
#      make
#      ./armor ../data/armor.avi
# ============================================================================
cmake_minimum_required(VERSION 3.10)
project(armor LANGUAGES CXX)
# 用 C++14。
set(CMAKE_CXX_STANDARD 14)
# Release 会让程序跑得快一些。调试的时候可以改成 Debug。
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
# ----------------------------------------------------------------------------
#  找 OpenCV
# ----------------------------------------------------------------------------
#  任务一把 OpenCV 装在了 /usr/local/opencv4.5.4，
#  这个位置不是系统的默认搜索路径，所以要手动告诉 CMake 去哪找。
set(OPENCV_PATH "/usr/local/opencv4.5.4")
if (EXISTS "${OPENCV_PATH}/lib/cmake/opencv4/OpenCVConfig.cmake")
    set(OpenCV_DIR "${OPENCV_PATH}/lib/cmake/opencv4")
endif()
find_package(OpenCV REQUIRED)
# ----------------------------------------------------------------------------
#  要编译的文件
# ----------------------------------------------------------------------------
#  只写 .cpp，头文件（.h）不用写，它们是被 include 进去的。
#  少写一个 .cpp，链接时会报 undefined reference（找不到函数）。
add_executable(armor
    src/main.cpp
    src/detect.cpp
)
# 告诉编译器：
#   include/ 是我自己的头文件目录（这样能写 #include "detect.h"）
#   ${OpenCV_INCLUDE_DIRS} 是 OpenCV 的头文件目录
target_include_directories(armor PRIVATE
    include
    ${OpenCV_INCLUDE_DIRS}
)
# 告诉链接器把 OpenCV 的库连进来，不然会报一堆 undefined reference to 'cv::...'
target_link_libraries(armor PRIVATE
    ${OpenCV_LIBS}
)