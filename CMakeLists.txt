cmake_minimum_required(VERSION 2.8)
project(agent)

include(use_c++11.cmake)

set (Boost_ADDITIONAL_VERSIONS "1.52" "1.52.0" )
find_package ( Boost 1.51.0 REQUIRED COMPONENTS 
               system
               thread)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  link_directories(${Boost_LIBRARY_DIRS})
endif()

add_library ( 
  agent
  parser.cpp generator.cpp entity.cpp
  connection.cpp
  agent.cpp)
target_link_libraries( agent ${Boost_LIBRARIES} pthread)

add_executable ( test_agent test_agent.cpp)
target_link_libraries ( test_agent agent )

