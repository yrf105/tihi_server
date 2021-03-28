cmake_minimum_required(VERSION 3.19.1)
project(tihi)

include(cmake/utils.cmake)

# 显示详细的 make 命令
set(CMAKE_VERBOSE_MAKEFILE ON)

#判断编译器类型,如果是gcc编译器,则在编译选项中加入c++11支持
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -std=c++11 ${CMAKE_CXX_FLAGS}")
    # message(STATUS "optional:-std=c++11")   
endif(CMAKE_COMPILER_IS_GNUCXX)

# 定义工程根目录; CMAKE_SOURCE_DIR为内建变量，表示工程根目录的CMakeLists.txt文件路径
SET(ROOT_DIR ${CMAKE_SOURCE_DIR})

# 指定头文件搜索路径
include_directories(tihi/)
include_directories(third_party/yaml-cpp/include)

# 指定引用的外部库的路径
link_directories(third_party/yaml-cpp/bulid)

# find_library(TEST third_party/yaml-cpp/build)


set(LIB_SRC
    tihi/log/log.cc
    tihi/fiber/fiber.cc
    tihi/utils/utils.cc
    tihi/utils/noncopyable.cc
    tihi/utils/macro.cc
    tihi/utils/mutex.cc
    tihi/config/config.cc
    tihi/scheduler/scheduler.cc
    tihi/iomanager/iomanager.cc
    tihi/thread/thread.cc
    tihi/timer/timer.cc
    tihi/hook/hook.cc
    tihi/hook/fd_manager.cc
    tihi/socket/address/address.cc
    tihi/socket/socket/socket.cc
    tihi/bytearray/bytearray.cc
    tihi/http/http.cc
    tihi/http/http11_parser.rl.cc
    tihi/http/httpclient_parser.rl.cc
    tihi/http/http_parser.cc
    tihi/tcp_server/tcp_server.cc)

set(LIBS
    tihi
    yaml-cpp
    pthread
    dl)

# 生成动态库
add_library(tihi SHARED ${LIB_SRC})
redefine_file_macro(tihi)

tihi_add_executable(test "tests/test.cc" tihi "${LIBS}")
tihi_add_executable(test_config "tests/test_config.cc" tihi "${LIBS}")
tihi_add_executable(test_thread "tests/test_thread.cc" tihi "${LIBS}")
tihi_add_executable(test_macro "tests/test_macro.cc" tihi "${LIBS}")
tihi_add_executable(test_fiber "tests/test_fiber.cc" tihi "${LIBS}")
tihi_add_executable(test_scheduler "tests/test_scheduler.cc" tihi "${LIBS}")
tihi_add_executable(test_iomanager "tests/test_iomanager.cc" tihi "${LIBS}")
tihi_add_executable(test_hook "tests/test_hook.cc" tihi "${LIBS}")
tihi_add_executable(test_address "tests/test_address.cc" tihi "${LIBS}")
tihi_add_executable(test_socket "tests/test_socket.cc" tihi "${LIBS}")
tihi_add_executable(test_bytearray "tests/test_bytearray.cc" tihi "${LIBS}")
tihi_add_executable(test_http "tests/test_http.cc" tihi "${LIBS}")
tihi_add_executable(test_http_parser "tests/test_http_parser.cc" tihi "${LIBS}")
tihi_add_executable(test_tcp_server "tests/test_tcp_server.cc" tihi "${LIBS}")
tihi_add_executable(echo_server "example/echo_server.cc" tihi "${LIBS}")
# add_executable(test_config tests/test_config.cc)
# add_dependencies(test_config tihi)
# target_link_libraries(test_config tihi -L/home/wddxrw/myproject/tihi_server/third_party/yaml-cpp/bulid -lyaml-cpp)


# 指定可执行文件的存放路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 指定输出库的存放路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)