cmake_minimum_required(VERSION 3.8)
project(aml_dtv_testserver VERSION 1.0)
message(STATUS "aml_dtv_testserver: aml-comp/vendor/amlogic/aml_dtv_testserver")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -fpermissive")
set(YOCTO_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../../)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB cpp_source ./*.cpp ./*.c)

set(aml_dtv_testserver_VERSION "1.0.0")
message(STATUS "Version: ${aml_dtv_testserver_VERSION}")

set(TARGET aml_dtv_testserver)
add_executable(${TARGET} ${cpp_source})
target_link_libraries(${TARGET} PUBLIC log sqlite3 am_adp -ldl am_mw binder mediahal_tsplayer mediahal_resman mediahal_mediasync)
install(TARGETS ${TARGET})
