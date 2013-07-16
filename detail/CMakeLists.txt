include_directories ( ${PROJECT_SOURCE_DIR} 
                      ${ZLIB_INCLUDE_DIR} 
                      ${PROJECT_SOURCE_DIR}/contrib/libarchive-3.1.2 )

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

if(MSVC)
  set ( LIBS ${OPENSSL_LIBRARIES} ) 
  add_library ( agent STATIC ${AGENT_SRC} )
else()
  set ( LIBS pthread ${OPENSSL_LIBRARIES} )
  add_library ( agent SHARED ${AGENT_SRC} )
endif()

target_link_libraries( agent ${Boost_LIBRARIES} ${LIBS} archive zlib) 
set_target_properties ( 
  agent PROPERTIES 
  COMPILE_DEFINITIONS "_WIN32_WINNT=0x0501" )

install ( TARGETS agent DESTINATION "." )
