include(FindPackageHandleStandardArgs)

find_path(
    RAPIDJSON_INCLUDE_DIR
    rapidjson/rapidjson.h
    PATH ${RAPIDJSON_ROOT}/include ${CMAKE_SOURCE_DIR}/thirdparty/rapidjson/include NO_DEFAULT_PATH
)

find_package_handle_standard_args(
	RapidJSON
	DEFAULT_MSG
    RAPIDJSON_INCLUDE_DIR
)
