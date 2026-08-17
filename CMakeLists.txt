#
# Inter-process Queue
#
cmake_minimum_required(VERSION 3.21)

# --- macOS is not supported ---
if(APPLE OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  message(FATAL_ERROR "macOS is not supported.")
endif()

# --- version ---
project(IQueue VERSION 0.0.1 LANGUAGES C CXX)

option(IQUEUE_BUILD_EXAMPLES "Build inter-queue example programs" ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# --- source files ---
file(GLOB IQ_SOURCES   "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")
file(GLOB IQ_WIN_SOURCES    "${CMAKE_CURRENT_SOURCE_DIR}/source/win/*.cpp")
file(GLOB IQ_LINUX_SOURCES  "${CMAKE_CURRENT_SOURCE_DIR}/source/linux/*.cpp")

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/source/win/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/source/linux/*.cpp"
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/source/version.hpp.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/source/version.hpp"
)

# --- inter-queue library (libiqueue) ---
add_library(iqueue SHARED ${IQ_SOURCES})

target_compile_features(iqueue PUBLIC cxx_std_20)

target_include_directories(iqueue
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/source
    $<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/source/win>
    $<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/source/linux>
)

target_sources(iqueue PRIVATE
  $<$<PLATFORM_ID:Windows>:${IQ_WIN_SOURCES}>
  $<$<PLATFORM_ID:Linux>:${IQ_LINUX_SOURCES}>
)

target_compile_definitions(iqueue
  PUBLIC
    $<$<PLATFORM_ID:Windows>:IQ_PLATFORM_WINDOWS>
    $<$<PLATFORM_ID:Linux>:IQ_PLATFORM_LINUX>
  PRIVATE
    $<$<CONFIG:Debug>:IQ_DEBUG_BUILD>
    $<$<CONFIG:Release>:IQ_RELEASE_BUILD>
    IQ_BUILD
)

if(MSVC)
  target_compile_options(iqueue PRIVATE /W4 /permissive-)
else()
  target_compile_options(iqueue PRIVATE -Wall -Wextra -Wpedantic
    $<$<CONFIG:Debug>:-Og -g>
    $<$<CONFIG:Release>:-O3>
  )
endif()

if(WIN32)
  # TODO
elseif(UNIX AND NOT APPLE)
  # TODO
endif()

set_target_properties(iqueue PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN YES
)

set_target_properties(iqueue PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY  "${CMAKE_BINARY_DIR}/bin"
  LIBRARY_OUTPUT_DIRECTORY  "${CMAKE_BINARY_DIR}/bin"
  ARCHIVE_OUTPUT_DIRECTORY  "${CMAKE_BINARY_DIR}/lib"
)

set_target_properties(iqueue PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
)

# --- install / package ---
install(TARGETS iqueue
  EXPORT IQueueTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT IQueueTargets
  FILE IQueueTargets.cmake
  NAMESPACE IQueue::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/IQueue
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/IQueueConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/IQueueConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/IQueueConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/IQueue
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/IQueueConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/IQueueConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/IQueue
)

# --- examples ---
if(IQUEUE_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# --- clang-format ---
file(GLOB_RECURSE format_files
  "${CMAKE_CURRENT_SOURCE_DIR}/include/*.[ch]pp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/*.[ch]"
  "${CMAKE_CURRENT_SOURCE_DIR}/source/*.[ch]pp"
  "${CMAKE_CURRENT_SOURCE_DIR}/source/*.[ch]"
  "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.[ch]pp"
  "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.[ch]"
)

# Safety net: even if directories change later, never format generated/build outputs.
list(FILTER format_files EXCLUDE REGEX "/build[^/]*/")
list(FILTER format_files EXCLUDE REGEX "/CMakeFiles/")

list(REMOVE_DUPLICATES format_files)
add_custom_target(
  format
  COMMAND clang-format -i ${format_files}
  COMMENT "Formatting source files with clang-format"
)
