cmake_minimum_required(VERSION 3.16)
project(imx6pull_wifi_bluetooth VERSION 1.0.0 LANGUAGES C CXX)

# 设置C标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 设置C++标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置交叉编译
if(DEFINED ENV{CROSS_COMPILE})
    set(CMAKE_SYSTEM_NAME Linux)
    set(CMAKE_SYSTEM_PROCESSOR arm)
    
    # 设置工具链
    set(CMAKE_C_COMPILER $ENV{CROSS_COMPILE}gcc)
    set(CMAKE_CXX_COMPILER $ENV{CROSS_COMPILE}g++)
    set(CMAKE_STRIP $ENV{CROSS_COMPILE}strip)
    
    # 设置查找路径
    set(CMAKE_FIND_ROOT_PATH $ENV{SDKTARGETSYSROOT})
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif()

# 编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2 -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2 -g")

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/drivers
    ${CMAKE_SOURCE_DIR}/src/network
    ${CMAKE_SOURCE_DIR}/src/application
    ${CMAKE_SOURCE_DIR}/src/utils
)

# 查找依赖包
find_package(PkgConfig REQUIRED)

# 查找WiFi相关库
pkg_check_modules(WIFI REQUIRED libnl-3.0 libnl-genl-3.0)

# 查找蓝牙相关库
pkg_check_modules(BLUETOOTH REQUIRED bluez)

# 查找网络相关库
pkg_check_modules(NETWORK REQUIRED libssl libcrypto)

# 查找数据库库
find_package(SQLite3 REQUIRED)

# 查找JSON库
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSON REQUIRED json-c)

# 源文件
file(GLOB_RECURSE SOURCES 
    "src/*.c"
    "src/*.cpp"
    "src/drivers/*.c"
    "src/network/*.c"
    "src/application/*.c"
    "src/utils/*.c"
)

# 头文件
file(GLOB_RECURSE HEADERS 
    "src/*.h"
    "src/drivers/*.h"
    "src/network/*.h"
    "src/application/*.h"
    "src/utils/*.h"
)

# 创建库文件
add_library(imx6pull_wifi_bluetooth SHARED ${SOURCES} ${HEADERS})

# 设置库属性
set_target_properties(imx6pull_wifi_bluetooth PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "imx6pull_wifi_bluetooth"
)

# 链接库
target_link_libraries(imx6pull_wifi_bluetooth
    ${WIFI_LIBRARIES}
    ${BLUETOOTH_LIBRARIES}
    ${NETWORK_LIBRARIES}
    SQLite3::SQLite3
    ${JSON_LIBRARIES}
    pthread
    dl
    m
    rt
)

# 编译定义
target_compile_definitions(imx6pull_wifi_bluetooth PRIVATE
    _GNU_SOURCE
    DEBUG
    VERSION="${PROJECT_VERSION}"
)

# 安装规则
install(TARGETS imx6pull_wifi_bluetooth
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${HEADERS}
    DESTINATION include/imx6pull
)

# 测试
enable_testing()
add_subdirectory(tests)

# 工具
add_subdirectory(tools)

# 文档
add_subdirectory(docs)
