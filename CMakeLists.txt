cmake_minimum_required(VERSION 3.15)
project(vr_driver)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directory for the built driver
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# OpenVR SDK paths
set(OPENVR_DIR "${CMAKE_SOURCE_DIR}/third_party/openvr")
set(OPENVR_INCLUDE_DIR "${OPENVR_DIR}/headers")
set(OPENVR_LIB_DIR "${OPENVR_DIR}/lib/win64")

# Rust library paths
set(RUST_CORE_DIR "${CMAKE_SOURCE_DIR}/rust_core")
set(RUST_TARGET_DIR "${RUST_CORE_DIR}/target/release")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/cpp_driver/include
    ${OPENVR_INCLUDE_DIR}
    ${RUST_CORE_DIR}/src
)

# C++ source files
set(DRIVER_SOURCES
    cpp_driver/src/driver_factory.cpp
    cpp_driver/src/driver_provider.cpp
    cpp_driver/src/hmd_device.cpp
    cpp_driver/src/display_component.cpp
    cpp_driver/src/controller_device.cpp 
)

# Create the driver DLL
add_library(driver_custom_vr_driver SHARED ${DRIVER_SOURCES})

# Link against OpenVR
target_link_libraries(driver_custom_vr_driver
    ${OPENVR_LIB_DIR}/openvr_api.lib
)

# Link against Rust library (will be built separately)
target_link_libraries(driver_custom_vr_driver
    ${RUST_TARGET_DIR}/vr_driver.dll.lib
)

# Copy OpenVR DLL to output directory
add_custom_command(TARGET driver_custom_vr_driver POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OPENVR_LIB_DIR}/openvr_api.dll"
        $<TARGET_FILE_DIR:driver_custom_vr_driver>
)

# Copy Rust DLL to output directory
add_custom_command(TARGET driver_custom_vr_driver POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${RUST_TARGET_DIR}/vr_driver.dll"
        $<TARGET_FILE_DIR:driver_custom_vr_driver>
)