cmake_minimum_required(VERSION 3.0)

project(CapPrint)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake/Modules/)

find_package(RapidJSON REQUIRED)
find_package(OpenCV 2 REQUIRED)

include_directories(${RAPIDJSON_INCLUDE_DIR} ${OpenCV_INCLUDE_DIRS})
add_compile_options(-std=c++14 -fPIC)
add_executable(CapPrint capprint.cpp)
target_link_libraries(CapPrint ${OpenCV_LIBS})

configure_file(
    ${CMAKE_SOURCE_DIR}/capprint.settings.json
    capprint.settings.json
    COPYONLY
)