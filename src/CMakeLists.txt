
target_compile_options( ${exe} PRIVATE
  -Wall -Wextra -Wno-unused-parameter -Wno-gnu-anonymous-struct -Wno-nested-anon-types -Wno-null-pointer-subtraction -fexperimental-library
  )

set_target_properties( ${exe} PROPERTIES CXX_STANDARD 23 )


if ( NOT FAKE_RENDERER )
  message( STATUS "Using Qt..." )

  target_link_libraries( ${exe} PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
    Qt::OpenGL
    Qt::OpenGLWidgets
    )

    target_sources( ${exe} PRIVATE main_window.cpp widget.cpp widget.h )
else()
  target_compile_definitions( ${exe} PRIVATE
    FAKE_RENDERER
    )
endif()

message( STATUS "Building ${exe}..." )

target_sources( ${exe} PRIVATE
  main.cpp
  main_window.h
  peerconnection.cpp
  peerconnection.h
  peerconnection_manager.cpp
  peerconnection_manager.h
  session_description_obserser.h
  websocket.cpp
  websocket.h
  medooze_mgr.cpp
  medooze_mgr.h
  monitor_manager.cpp
  monitor_manager.h
  tunnel_loggin.h
  tunnel_loggin.cpp
  y4msource.cpp
  y4msource.h
  )

target_link_libraries( ${exe} PRIVATE
  ${WEBRTC_LIBRARIES}
  nlohmann_json
  )



