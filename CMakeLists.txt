cmake_minimum_required(VERSION 3.21)
project(taiko_wonder VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Works with Visual Studio's CMake Open Folder workflow and with vcpkg toolchains.
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)

add_executable(taiko-wonder WIN32 src/main.cpp)
target_link_libraries(taiko-wonder PRIVATE SDL2::SDL2 SDL2_ttf::SDL2_ttf)
if(TARGET SDL2::SDL2main)
  target_link_libraries(taiko-wonder PRIVATE SDL2::SDL2main)
endif()

if(WIN32)
  add_custom_command(TARGET taiko-wonder POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:SDL2::SDL2> $<TARGET_FILE_DIR:taiko-wonder>)
endif()

if(UNIX)
  find_package(Threads REQUIRED)
  add_executable(taiko-wonder-server server/room_server.cpp)
  target_link_libraries(taiko-wonder-server PRIVATE Threads::Threads)
endif()
