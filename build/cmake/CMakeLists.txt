cmake_minimum_required(VERSION 3.10)

project(audout)

# !!! find_package must go after project() declaration !!!
# Otherwise VCPKG does not set the CMAKE_PREFIX_PATH to find packages.
find_package(myci CONFIG REQUIRED)

set(srcs)
myci_add_source_files(srcs
    DIRECTORY
        ../../src/${PROJECT_NAME}
    RECURSIVE
)

myci_declare_library(${PROJECT_NAME}
    SOURCES
        ${srcs}
    PUBLIC_INCLUDE_DIRECTORIES
        ../../src
    INSTALL_INCLUDE_DIRECTORIES
        ../../src/${PROJECT_NAME}
    DEPENDENCIES
        utki
    WINDOWS_ONLY_DEPENDENCIES
        nitki
    LINUX_ONLY_DEPENDENCIES
        nitki
        PkgConfig::libpulse-simple
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE dsound)
elseif(IOS)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework AudioToolbox")
elseif(APPLE) # macos and ios
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework AudioUnit")
endif()
