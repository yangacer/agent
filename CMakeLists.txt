cmake_minimum_required(VERSION 2.8)
project(agent)

include(use_c++11.cmake)

include_directories (${PROJECT_SOURCE_DIR})

set (Boost_ADDITIONAL_VERSIONS "1.52" "1.52.0" )
find_package ( Boost 1.51.0 REQUIRED COMPONENTS 
               system
               thread)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  link_directories(${Boost_LIBRARY_DIRS})
endif()

add_subdirectory ( detail )
add_subdirectory ( test )
