cmake_minimum_required(VERSION 2.8)
project(Agent)

list (APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake_mod)

include (use_c++11)
include (disable_useless_vc_warning)

if(MSVC)
  include (force_static_msvc_runtime)
  set (Boost_USE_STATIC_LIBS ON)
  set (Boost_USE_STATIC_RUNTIME ON)
  set (Boost_USE_MULTITHREADED ON)
endif()

include_directories (${PROJECT_SOURCE_DIR})

set (Boost_ADDITIONAL_VERSIONS "1.52" "1.52.0" "1.53" "1.53.0")
find_package ( Boost REQUIRED COMPONENTS 
               system
               thread)

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  link_directories(${Boost_LIBRARY_DIRS})
endif()

add_subdirectory ( detail )
add_subdirectory ( test )
