cmake_minimum_required(VERSION 3.0.0)

project(HttpServer)

add_definitions(-std=c++11)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include/libevent)
include_directories(${CMAKE_SOURCE_DIR}/include/libevent/event2)

link_directories(${CMAKE_SOURCE_DIR}/lib/zlib)
link_directories(${CMAKE_SOURCE_DIR}/lib/curl-7.64.0)
link_directories(${CMAKE_SOURCE_DIR}/lib/openssl)
link_directories(${CMAKE_SOURCE_DIR}/lib/sqlite3)


file(GLOB headers ${CMAKE_SOURCE_DIR}/*.h)
message(STATUS ${CMAKE_SOURCE_DIR}})

aux_source_directory(${CMAKE_SOURCE_DIR} SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST} ${headers})

add_library(libevent STATIC IMPORTED)
add_library(libevent_core STATIC IMPORTED)
add_library(libevent_extras STATIC IMPORTED)
set_property(TARGET libevent PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/lib/libevent/libevent.lib)
set_property(TARGET libevent_core PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/lib/libevent/libevent_core.lib)
set_property(TARGET libevent_extras PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/lib/libevent/libevent_extras.lib)

if(CMAKE_BUILD_TYPE MATCHES "Release")
target_link_libraries(${PROJECT_NAME} ws2_32 libevent libevent_core libevent_extras sqlite3 libcurl)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(OUT "Release")
else()
target_link_libraries(${PROJECT_NAME} ws2_32 libevent libevent_core libevent_extras sqlite3 libcurl)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(OUT "Debug")
endif()

file(GLOB libfile ${CMAKE_SOURCE_DIR}/config/*)
foreach(lib ${libfile})
  message(STATUS "copy file..." ${lib})
  file(COPY ${lib} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${OUT}/config)
endforeach()

file(GLOB libfile ${CMAKE_SOURCE_DIR}/mt4lib/*)
foreach(lib ${libfile})
  message(STATUS "copy file..." ${lib})
  file(COPY ${lib} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${OUT}/mt4lib)
endforeach()

file(GLOB libfile ${CMAKE_SOURCE_DIR}/bin/*)
foreach(lib ${libfile})
  message(STATUS "copy file..." ${lib})
  file(COPY ${lib} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${OUT})
endforeach()

set(DIR_NAME "logs")
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${OUT}/${DIR_NAME})