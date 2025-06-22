nap_command_app(ndoc "nlib;html5;encode" NRC_EMBEDDED)
target_include_directories(ndoc PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>)
set_target_properties(ndoc PROPERTIES FOLDER "utils")