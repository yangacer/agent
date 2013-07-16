include_directories ( ${PROJECT_SOURCE_DIR} )

set ( AGENT_SRC
  version.cpp
  io_service_pool.cpp
  log.cpp log_impl.cpp
  parser.cpp generator.cpp entity.cpp
  multipart.cpp
  timeout_config.cpp
  connection.cpp
  session.cpp
  agent_base.cpp
  agent_base_v2.cpp
  agent_v2.cpp
  agent.cpp)
#agency_base.cpp
# agency.cpp)

if(MSVC)
  set ( LIBS ${OPENSSL_LIBRARIES} ) #libeay32mt ssleay32mt Crypt32)
  add_library ( agent STATIC ${AGENT_SRC} )
  target_link_libraries( agent ${Boost_LIBRARIES} ${LIBS} ) 
else()
  set ( LIBS pthread ${OPENSSL_LIBRARIES} )#crypto ssl)
  add_library ( agent SHARED ${AGENT_SRC} )
  target_link_libraries( agent ${Boost_LIBRARIES} ${LIBS} ) 
endif()

set_target_properties ( 
  agent PROPERTIES 
  COMPILE_DEFINITIONS "_WIN32_WINNT=0x0501" )

