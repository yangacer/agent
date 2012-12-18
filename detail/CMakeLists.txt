include_directories ( ${PROJECT_SOURCE_DIR} )

add_library ( 
  agent SHARED
  parser.cpp generator.cpp entity.cpp
  connection.cpp
  agent.cpp)
target_link_libraries( agent ${Boost_LIBRARIES} pthread)


