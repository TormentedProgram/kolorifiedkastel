cmake_minimum_required(VERSION 3.14)
project(kastel VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

set(KF6_MIN_VERSION 6)
find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set (CMAKE_MODULE_PATH
     ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_MODULE_PATH}
)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

find_package (Qt6 ${QT_MIN_VERSION} REQUIRED CONFIG COMPONENTS Core Widgets)
find_package (KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS Config Runner)

set(KASTEL_SOURCES src/kastel.cpp src/kastel.h src/commands.cpp)

include_directories (src ${CMAKE_BUILD_DIR})

kcoreaddons_add_plugin(kastel SOURCES ${KASTEL_SOURCES} INSTALL_NAMESPACE "kf6/krunner")

target_link_libraries(kastel KF6::Runner Qt6::Core Qt6::Widgets)
