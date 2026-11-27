cmake_minimum_required(VERSION 3.16)
project(LO02Project LANGUAGES CXX)

add_library(lo02_game src/game.cpp)
target_include_directories(lo02_game PUBLIC include)
target_compile_features(lo02_game PUBLIC cxx_std_17)
set_target_properties(lo02_game PROPERTIES CXX_EXTENSIONS OFF)

add_executable(lo02_flow_demo examples/flow_demo.cpp)
target_link_libraries(lo02_flow_demo PRIVATE lo02_game)
set_target_properties(lo02_flow_demo PROPERTIES CXX_EXTENSIONS OFF)

include(CTest)
if(BUILD_TESTING)
    add_test(NAME flow_solo COMMAND lo02_flow_demo Alice)
    add_test(NAME flow_two_players COMMAND lo02_flow_demo)
    add_test(NAME flow_three_players COMMAND lo02_flow_demo Alice Bob Carol)
    add_test(NAME flow_four_players COMMAND lo02_flow_demo Alice Bob Carol Dave)
endif()
