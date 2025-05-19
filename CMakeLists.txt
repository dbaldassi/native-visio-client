cmake_minimum_required( VERSION 3.16 FATAL_ERROR )

project( vm-visio-client )

find_package( libwebrtc ${WEBRTC_API_VERSION}  )
message( STATUS "Found libwebrtc at ${libwebrtc_DIR}." )

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option( FAKE_RENDERER OFF )

find_package( X11 REQUIRED )

if( NOT FAKE_RENDERER )
  find_package( Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets )
  qt_standard_project_setup()
endif()

# --- ASIO
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/external/asio/asio/include )

# --- websocketpp
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/external/websocketpp )

# --- JSON
set( JSON_BuildTests      OFF CACHE INTERNAL "" )
set( JSON_Install         OFF CACHE INTERNAL "" )
set( JSON_MultipleHeaders ON  CACHE INTERNAL "" )
add_subdirectory( external/nlohmann_json )

set( exe visioclient )
if ( NOT FAKE_RENDERER )
  qt_add_executable( ${exe} )
else()
  add_executable( ${exe} )
endif()

# --- src
add_subdirectory( src )
