
add_executable ( base base.cpp )
target_link_libraries ( base agent )

add_executable ( tube tube.cpp )
target_link_libraries ( tube agent)
#add_executable ( driver driver.cpp )
#target_link_libraries ( driver agent crypto)
