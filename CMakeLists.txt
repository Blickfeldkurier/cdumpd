cmake_minimum_required (VERSION 3.10)
project (dumpd)
find_package(CURL REQUIRED)
include_directories(
	${CURL_INCLUDE_DIRS}
)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(dumpd_SRC 
	"src/main.cpp"
	"src/log.cpp"
	"src/http_upload.cc"
	"src/using_std_string.h"
)
add_executable(dumpd ${dumpd_SRC})
target_link_libraries(dumpd 
	${CURL_LIBRARIES}
	dl
)
