include_directories ( ${PROJECT_SOURCE_DIR} )

add_library ( 
  agent SHARED
  log.cpp
  parser.cpp generator.cpp entity.cpp
  connection.cpp
  agent.cpp)

if(${CMAKE_CXX_COMPILER} MATCHES "Visual Studio")
  set ( LIBS libeay32mt ssleay32mt Crypt32)
else()
  set ( LIBS pthread crypto ssl)
endif()

target_link_libraries( agent ${Boost_LIBRARIES} ${LIBS} ) 


