cmake_minimum_required(VERSION 3.10)

if(CMAKE_VERSION VERSION_LESS 3.21)
  get_property(PARENT_SOURCE_DIRECTORY DIRECTORY PROPERTY PARENT_DIRECTORY)

  if(NOT PARENT_SOURCE_DIRECTORY)
    set(PROJECT_IS_TOP_LEVEL true)
  endif()
endif()

project("Subdivided Pill")

add_library(subdivided_pill INTERFACE)

target_include_directories(subdivided_pill INTERFACE .)

if(PROJECT_IS_TOP_LEVEL)
  add_subdirectory(tools)
endif()
