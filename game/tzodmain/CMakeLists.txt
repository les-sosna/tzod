cmake_minimum_required (VERSION 3.3)

link_directories(${Boost_LIBRARY_DIR})

if(WIN32)
	add_definitions(-D_WIN32_WINNT=0x0501) # Windows XP
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-DNOMINMAX)
	add_executable(tzodmain WIN32 Main.cpp win/tank.rc)
	set_target_properties(tzodmain PROPERTIES
		VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
	)
	set(FSLIB fswin)
elseif(APPLE)
	file(GLOB_RECURSE DATA_FILES "${CMAKE_SOURCE_DIR}/data/*")
	add_executable(tzodmain MACOSX_BUNDLE Main.cpp mac/tank.icns ${DATA_FILES})
	set_target_properties(tzodmain PROPERTIES
		RESOURCE "mac/tank.icns"
		MACOSX_BUNDLE_ICON_FILE "tank.icns"
		MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/mac/MacOSXBundleInfo.plist.in
	)
	source_group(TREE "${CMAKE_SOURCE_DIR}/" FILES ${DATA_FILES})
	foreach(data_file ${DATA_FILES})
		get_filename_component(data_file_directory ${data_file} DIRECTORY)
		file(RELATIVE_PATH data_file_rel_directory "${CMAKE_SOURCE_DIR}/" ${data_file_directory})
		set_source_files_properties(${data_file} PROPERTIES
			MACOSX_PACKAGE_LOCATION "Resources/${data_file_rel_directory}")
	endforeach()
	set(FSLIB fsposix)
elseif(UNIX)
	add_executable(tzodmain Main.cpp)
	set(FSLIB fsposix)
else()
	message(FATAL_ERROR "Unknown platform")
endif()

target_link_libraries(tzodmain PRIVATE
	app
	platglfw
	${FSLIB}
)
set_target_properties(tzodmain PROPERTIES FOLDER game)
