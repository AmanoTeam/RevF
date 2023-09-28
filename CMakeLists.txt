cmake_minimum_required(VERSION 3.13)

project(
	revf
	VERSION 0.1
	DESCRIPTION "Reverse the content of files"
	HOMEPAGE_URL "https://github.com/AmanoTeam/revf"
	LANGUAGES C
)

option(REVF_ENABLE_LTO "Turn on compiler Link Time Optimizations" OFF)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)

include_directories(
	"${CMAKE_SOURCE_DIR}/src"
)

add_executable(
	revf
	src/argparser.c
	src/errors.c
	src/fileinfo.c
	src/filesystem.c
	src/fstream.c
	src/main.c
	src/os.c
	src/reverse_memcpy.c
	src/stringu.c
	src/terminal.c
	src/walkdir.c
)

if (REVF_ENABLE_LTO)
	set(REVF_HAS_LTO OFF)
	
	include(CheckIPOSupported)
	check_ipo_supported(RESULT REVF_HAS_LTO LANGUAGES C)
	
	if (REVF_HAS_LTO)
		set_target_properties(
			revf
			PROPERTIES
			INTERPROCEDURAL_OPTIMIZATION TRUE
		)
	endif()
endif()

install(
	TARGETS revf
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
)
