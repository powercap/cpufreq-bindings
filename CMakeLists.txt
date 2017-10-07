cmake_minimum_required(VERSION 2.8)

project(cpufreq-bindings)
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)
set(PROJECT_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99")

include_directories(${PROJECT_SOURCE_DIR}/inc)

# Libraries

add_library(${PROJECT_NAME} src/cpufreq-bindings.c)

# Binaries

add_executable(cpufreq-bindings-read-cpu src/cpufreq-bindings-read-cpu.c)
target_link_libraries(cpufreq-bindings-read-cpu ${PROJECT_NAME})
if(BUILD_SHARED_LIBS)
  set_target_properties(cpufreq-bindings PROPERTIES VERSION ${PROJECT_VERSION}
                                                    SOVERSION ${VERSION_MAJOR})
endif()

# pkg-config

set(PKG_CONFIG_EXEC_PREFIX "\${prefix}")
set(PKG_CONFIG_LIBDIR "\${prefix}/lib")
set(PKG_CONFIG_INCLUDEDIR "\${prefix}/include/${PROJECT_NAME}")
set(PKG_CONFIG_CFLAGS "-I\${includedir}")

set(PKG_CONFIG_NAME "${PROJECT_NAME}")
set(PKG_CONFIG_DESCRIPTION "C bindings to cpufreq in Linux sysfs")
set(PKG_CONFIG_LIBS "-L\${libdir} -l${PROJECT_NAME}")
set(PKG_CONFIG_LIBS_PRIVATE "")
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig.in
  ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/${PROJECT_NAME}.pc
)

# Install

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
install(TARGETS cpufreq-bindings-read-cpu DESTINATION bin)
install(DIRECTORY inc/ DESTINATION "include/${PROJECT_NAME}")
install(DIRECTORY ${CMAKE_BINARY_DIR}/pkgconfig/ DESTINATION lib/pkgconfig)

# Uninstall

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
  @ONLY
)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
