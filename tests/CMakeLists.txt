
set( test_targets 
     basic log io_service_pool redirect 
     range_request)

set( example_targets tube )
  #agency)

foreach( test IN LISTS test_targets )
  add_executable ( ${test} ${test}.cpp )
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
  target_link_libraries ( ${example} agent )
endforeach()
