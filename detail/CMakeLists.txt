include_directories ( ${PROJECT_SOURCE_DIR} )

set ( AGENT_SRC 
  io_service_pool.cpp
  log.cpp log_impl.cpp
  parser.cpp generator.cpp entity.cpp
  timeout_config.cpp
  connection.cpp
  agent.cpp)


if(MSVC)
  set ( LIBS libeay32mt ssleay32mt Crypt32)
  add_library ( agent STATIC ${AGENT_SRC} )
  target_link_libraries( agent ${Boost_LIBRARIES} ${LIBS} ) 
else()
  set ( LIBS pthread crypto ssl)
  add_library ( agent SHARED ${AGENT_SRC} )
  target_link_libraries( agent ${Boost_LIBRARIES} ${LIBS} ) 
endif()



