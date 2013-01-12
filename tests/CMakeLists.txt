set( test_targets basic tube log )

foreach( test IN LISTS test_targets )
  add_executable ( ${test} ${test}.cpp )
  target_link_libraries ( ${test} agent )
endforeach()

