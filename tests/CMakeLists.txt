
set( test_targets 
     basic log io_service_pool redirect 
     range_request
     agent_v2
     multipart
     httparser
     wget)

set( example_targets tube )
  #agency)

foreach( test IN LISTS test_targets )
  add_executable ( ${test} ${test}.cpp )
  set_target_properties ( 
    ${test} PROPERTIES 
    COMPILE_DEFINITIONS "_WIN32_WINNT=0x0501" )
  target_link_libraries ( ${test} agent )
  add_test ( ${test}_test ${test} ) 
endforeach()

#add_executable ( fb_agent fb_agent.cpp )
#if(MSVC)
#	target_link_libraries ( fb_agent agent json_spiritmt )
#else()
#	target_link_libraries ( fb_agent agent json_spirit )
#endif()

foreach( example IN LISTS example_targets )
  add_executable ( ${example} ${example}.cpp )
  set_target_properties ( 
    ${example} PROPERTIES 
    COMPILE_DEFINITIONS "_WIN32_WINNT=0x0501" )
  target_link_libraries ( ${example} agent )
endforeach()

find_package ( Boost REQUIRED COMPONENTS
               filesystem
               system )

include_directories ( 
  ${PROJECT_BINARY_DIR}/contrib/zlib-1.2.8
  ${PROJECT_SOURCE_DIR}/contrib/zlib-1.2.8/include
  ${PROJECT_SOURCE_DIR}/contrib/libarchive-3.1.2/libarchive
  ${Boost_INCLUDE_DIRS} )

link_directories ( 
  ${PROJECT_BINARY_DIR}/contrib/zlib-1.2.8
  ${PROJECT_BINARY_DIR}/contrib/libarchive-3.1.2)

add_executable ( arv archive.cpp )
target_link_libraries ( arv archive ${Boost_LIBRARIES} zlib)

