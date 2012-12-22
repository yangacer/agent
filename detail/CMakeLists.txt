include_directories ( ${PROJECT_SOURCE_DIR} )

add_library ( 
  agent SHARED
  log.cpp
  parser.cpp generator.cpp entity.cpp
  connection.cpp
  agent.cpp)
target_link_libraries( agent ${Boost_LIBRARIES} libeay32mt ssleay32mt Crypt32)


