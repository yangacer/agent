
set( test_targets 
     basic log io_service_pool redirect)

set( example_targets tube )

foreach( test IN LISTS test_targets )
  add_executable ( ${test} ${test}.cpp )
  target_link_libraries ( ${test} agent )
  add_test ( ${test}_test ${test} ) 
endforeach()

foreach( example IN LISTS example_targets )
  add_executable ( ${example} ${example}.cpp )
  target_link_libraries ( ${example} agent )
endforeach()
