nap_command_app(nbuild "nlib;encode" NRC_NONE)
target_include_directories(nbuild PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>)
set_target_properties(nbuild PROPERTIES FOLDER "utils")