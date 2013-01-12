cmake_minimum_required(VERSION 2.8)
project(Agent CXX)

list (APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake_mod)

include (use_c++11)
include (disable_useless_vc_warning)

# TODO Enable dynamic linking in windows
if(MSVC)
  include (force_static_msvc_runtime)
  set (Boost_USE_STATIC_LIBS ON)
  set (Boost_USE_STATIC_RUNTIME ON)
  set (Boost_USE_MULTITHREADED ON)
  add_definitions ( -D_WIN32_WINNT=0x0501 )
endif()

include_directories (${PROJECT_SOURCE_DIR})

set (Boost_ADDITIONAL_VERSIONS "1.52" "1.52.0" "1.53" "1.53.0")
find_package ( Boost REQUIRED COMPONENTS 
               system
               date_time
               thread
               exception)

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  link_directories(${Boost_LIBRARY_DIRS})
endif()

ENABLE_TESTING()

add_subdirectory ( detail )
add_subdirectory ( test )
