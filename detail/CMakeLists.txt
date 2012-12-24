include_directories ( ${PROJECT_SOURCE_DIR} )

set ( AGENT_SRC 
  log.cpp
  parser.cpp generator.cpp entity.cpp
  connection.cpp
  agent.cpp)


if(MSVC)
  set ( LIBS libeay32mt ssleay32mt Crypt32)
  add_library ( agent STATIC ${AGENT_SRC} )
  target_link_libraries( agent LINK_INTERFACE_LIBRARIES ${Boost_LIBRARIES} ${LIBS} ) 
else()
  set ( LIBS pthread crypto ssl)
  add_library ( agent SHARED ${AGENT_SRC} )
  target_link_libraries( agent ${Boost_LIBRARIES} ${LIBS} ) 
endif()



