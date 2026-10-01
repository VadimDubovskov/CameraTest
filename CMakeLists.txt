cmake_minimum_required(VERSION 4.0)
project(CameraTest LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# configure version
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 3)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(LINUX)
    # Install path
    set(INSTALL_BIN_DIR "/opt/${PROJECT_NAME}")
    set(INSTALL_INCLUDE_DIR "/opt/${PROJECT_NAME}/include")
    set(INSTALL_LIB_DIR "/opt/${PROJECT_NAME}/lib")
    set(INSTALL_DOC_DIR "/opt/${PROJECT_NAME}/doc")
    # RPATH
    set(CMAKE_INSTALL_RPATH "${INSTALL_LIB_DIR};/usr/local/lib;")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    set(BUILD_WITH_INSTALL_RPATH TRUE)
endif()

include(FetchContent)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Widgets)
# OpenCV
find_package(OpenCV REQUIRED)
message(STATUS " version: ${OpenCV_VERSION}")
message(STATUS " libraries: ${OpenCV_LIBS}")
message(STATUS " include path: ${OpenCV_INCLUDE_DIRS}")
message(STATUS " dir path: ${OpenCV_DIR}")

# spdlog
FetchContent_Declare(
    spdlog
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external_lib/spdlog"
    #GIT_REPOSITORY https://github.com/gabime/spdlog.git
    #GIT_TAG        v1.17.0 # Replace with preferred version
)
FetchContent_MakeAvailable(spdlog)

qt_standard_project_setup()

qt_add_executable(CameraTest
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    # MVS
    include/MVS/CameraParams.h
    include/MVS/MvCameraControl.h
    include/MVS/MvErrorDefine.h
    include/MVS/MvISPErrorDefine.h
    include/MVS/MvObsoleteInterfaces.h
    include/MVS/ObsoleteCamParams.h
    include/MVS/PixelType.h
    #
    include/satecameracontrol.h
    include/mvcstate.h
    include/satecvdatatypes.h
    include/cvutils.h
    #
    src/satecameracontrol.cpp
    src/mvcstate.cpp
    src/cvutils.cpp
)

include_directories(
    ${OpenCV_INCLUDE_DIRS}
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# Logger settings
if(LINUX)
    set(FILE_LOG "/var/CameraTest/log.txt")
else()
    set(FILE_LOG "logs/log.txt")
endif()
set(FORMAT_LOG "[%Y-%m-%d  %H:%M:%S.%e] [%l] [thread %t] %v")
# Logger level
# spdlog::level::trace
# spdlog::level::debug
# spdlog::level::info
# spdlog::level::warn
# spdlog::level::err
# spdlog::level::critical
set(LEVEL_LOG spdlog::level::trace)
# Specify the configuration file from which the header file will be generated
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/cmake/config_satecv.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/config_satecv.h @ONLY)

if(WIN32)
    target_link_libraries(CameraTest PRIVATE
        Qt::Core
        Qt::Widgets
        spdlog::spdlog
        ${OpenCV_LIBRARIES}
        ${CMAKE_CURRENT_SOURCE_DIR}/MVS/win64/MvCameraControl.lib
    )
else()
    add_library(libMVS SHARED IMPORTED)
    set_property(TARGET libMVS PROPERTY IMPORTED_LOCATION "/opt/MVS/lib/64/libMvCameraControl.so")
    set_property(TARGET libMVS PROPERTY IMPORTED_IMPLIB "/opt/MVS/lib/64/libMvCameraControl.so")
    target_link_libraries(CameraTest PRIVATE
        Qt::Core
        Qt::Widgets
        spdlog::spdlog
        ${OpenCV_LIBRARIES}
        libMVS
    )
endif()

## --------------------- Install pipeline ---------------------
if(LINUX)
    include(GNUInstallDirs)
    # Specify where header and built files should be copied
    install(TARGETS CameraTest
        LIBRARY DESTINATION ${INSTALL_LIB_DIR}
        NAMELINK_SKIP
        RUNTIME DESTINATION ${INSTALL_BIN_DIR}
        COMPONENT Runtime
        PERMISSIONS
                    OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ GROUP_WRITE GROUP_EXECUTE
                    WORLD_READ WORLD_WRITE WORLD_EXECUTE
    )
    # OpenCV
    install(FILES
        /usr/local/lib/libopencv_imgproc.so.4.13.0
        DESTINATION ${INSTALL_LIB_DIR} RENAME libopencv_imgproc.so.413)
    install(FILES
        /usr/local/lib/libopencv_core.so.4.13.0
        DESTINATION ${INSTALL_LIB_DIR} RENAME libopencv_core.so.413)

elseif (WIN32)
    include(DeployQt)
    windeployqt(CameraTest ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} ${PROJECT_SOURCES}/resources)
endif()

if(LINUX)
    # Set additional project information
    set(COMPANY "ООО Совтест АТЕ")
    add_definitions("-DCOMPANY=\"${COMPANY}\"")
    add_definitions("-DAPPLICATION_NAME=\"${PROJECT_NAME}\"")
    set(APPLICATION_MAINTAINER "${COMPANY} <vdubovskov@sovtest-ate.ru>")
    set(COPYRIGHT "Copyright (c) 2023 SOVTEST-ATE. All rights reserved.")
    add_definitions("-DCOPYRIGHT=\"${COPYRIGHT}\"")

    add_definitions("-DSATELoader_LIB_VERSION=\"${LIBRARY_VERSION}\"")
    add_definitions("-DSATELoader_VERSION=\"${LIBRARY_VERSION}\"")

    # CPACK
    SET(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_DEBIAN_PACKAGE_NAME ${PROJECT_NAME})
    set(CPACK_DEBIAN_PACKAGE_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
    set(CPACK_DEBIAN_PACKAGE_RELEASE "")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Library for working with the semiconductor wafer loading/unloading device.")
    set(CPACK_PACKAGE_DESCRIPTION "Library for working SATELoader device.")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${APPLICATION_MAINTAINER})
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://sovtest-ate.com/")

    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.24)")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "unzip")

    INCLUDE(CPack)
endif()
