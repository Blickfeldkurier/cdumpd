cmake_minimum_required (VERSION 3.10)
project (cdumpd)
find_package(CURL REQUIRED)
include_directories(
	${CURL_INCLUDE_DIRS}
)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(cdumpd_SRC 
	"src/main.cpp"
	"src/log.cpp"
	"src/http_upload.cc"
	"src/using_std_string.h"
)
add_executable(cdumpd ${cdumpd_SRC})
target_link_libraries(cdumpd 
	${CURL_LIBRARIES}
	dl
)

INSTALL(TARGETS cdumpd DESTINATION local/bin)
SET(CPACK_GENERATOR "DEB;TGZ")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Blickfeldkurier")

INCLUDE(CPack)
