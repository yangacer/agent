cmake_minimum_required(VERSION 2.8)
project(Agent CXX)

list (APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake_mod)

include (GetGitRevisionDescription)
include (use_c++11)
include (disable_useless_vc_warning)

git_describe ( VERSION --tags --long)

#parse the version information into pieces.
string(REGEX REPLACE "^v([0-9]+)\\..*" "\\1" VERSION_MAJOR "${VERSION}" )
string(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" VERSION_MINOR "${VERSION}" )
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" VERSION_PATCH "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+(.*)" "\\1" VERSION_SHA1 "${VERSION}")
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set(VERSION_FULL "${VERSION}${VERSION_SHA1}")

message ("Agent version: ${VERSION_FULL}")

configure_file( detail/version.cpp.in ${PROJECT_SOURCE_DIR}/detail/version.cpp )


# TODO Enable dynamic linking in windows
if(MSVC)
  include (force_static_msvc_runtime)
  set (Boost_USE_STATIC_LIBS ON)
  set (Boost_USE_STATIC_RUNTIME ON)
  set (Boost_USE_MULTITHREADED ON)
endif()

include_directories ( ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/contrib )

set (Boost_ADDITIONAL_VERSIONS "1.52" "1.52.0" "1.53" "1.53.0")
find_package ( Boost REQUIRED COMPONENTS 
               system
               date_time
               thread)

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  link_directories(${Boost_LIBRARY_DIRS})
endif()

find_package ( OpenSSL REQUIRED )
if(OPENSSL_FOUND)
  include_directories(${OPENSSL_INCLUDE_DIR})
endif()

ENABLE_TESTING()

find_package ( ZLIB )

if ( NOT ZLIB_FOUND )
  add_subdirectory ( contrib/zlib-1.2.8 )
  get_target_property ( ZLIB_PATH zlib LOCATION)
  if( MSVC AND  ${ZLIB_PATH} MATCHES ".dll" )
    if( "${CMAKE_BUILD_TYPE}" MATCHES "Debug")
      string ( REGEX REPLACE ".dll" "d.lib" ZLIB_PATH "${ZLIB_PATH}")
    else()
      string ( REGEX REPLACE ".dll" ".lib" ZLIB_PATH "${ZLIB_PATH}")
    endif()
    message( "zlib path: ${ZLIB_PATH}" )
  endif()

#set ( CMAKE_PREFIX_PATH ${PROJECT_BINARY_DIR}/contrib/zlib-1.2.8 )
  set ( ZLIB_INCLUDE_DIR # BINARY_DIR for zconf.h
        ${PROJECT_SOURCE_DIR}/contrib/zlib-1.2.8;${PROJECT_BINARY_DIR}/contrib/zlib-1.2.8 )
  set ( ZLIB_LIBRARY "${ZLIB_PATH}" )
endif( NOT ZLIB_FOUND)

add_subdirectory ( contrib/libarchive-3.1.2 )
add_subdirectory ( detail )
add_subdirectory ( tests )

