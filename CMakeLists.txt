cmake_minimum_required(VERSION 3.10)
project(AdvancedOS)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找必需的包
find_package(Qt5 COMPONENTS Widgets Charts REQUIRED)
find_package(OpenSSL REQUIRED)

# 添加源文件
set(SOURCES
    src/main.cpp
    src/kernel.cpp
    src/advanced_kernel.cpp
    src/network.cpp
    src/user_auth.cpp
    src/disk_manager.cpp
    src/logger.cpp
    src/mainwindow.cpp
    src/filesystem.cpp
    src/cli.cpp
    src/ipc.cpp
    src/screen.cpp
    src/keyboard.cpp
    src/process.cpp
    src/memory.cpp
)

# 添加头文件
set(HEADERS
    include/kernel.h
    include/advanced_kernel.h
    include/network.h
    include/user_auth.h
    include/disk_manager.h
    include/logger.h
    include/mainwindow.h
    include/filesystem.h
    include/cli.h
    include/ipc.h
    include/screen.h
    include/keyboard.h
    include/process.h
    include/memory.h
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# 包含头文件目录
target_include_directories(${PROJECT_NAME} PRIVATE include)

# 链接库
target_link_libraries(${PROJECT_NAME}
    Qt5::Widgets
    Qt5::Charts
    OpenSSL::SSL
    OpenSSL::Crypto
    pthread
)

# 启用自动生成的Qt元对象代码
set_target_properties(${PROJECT_NAME} PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

# 安装目标
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)