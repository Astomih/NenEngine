cmake_minimum_required(VERSION 3.7)
project(editor)
if(NOT MSVC)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG -march=native")
endif()

if(WIN32)
	if(ALL_BUILD)
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -static -mwindows")
	endif()
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

file(GLOB_RECURSE SOURCE_FILES
	./native/*.cpp
	../libs/ImGuiColorTextEdit/*.cpp
	../libs/ImGuizmo/*.cpp
	./data/icon/icon.rc
)

file(GLOB_RECURSE HEADER_FILES
	./native/*.hpp
	../libs/ImGuiColorTextEdit/*.h
	../libs/ImGuiColorTextEdit/*.hpp
	../libs/ImGuizmo/*.h
	../libs/ImGuizmo/*.hpp
)

include_directories(
	../include
	../include/sinen
	../libs/imgui_markdown
	../libs/ImGuiColorTextEdit
	../libs/ImGuizmo
	../libs/rapidjson/include
)
link_directories(
	..
)
find_package(Vulkan REQUIRED)

set(COMMON_LIBRARIES
	sinen
	script_engine
	${Vulkan_LIBRARIES}
)

if(WIN32)
	set(LIBRARIES ${COMMON_LIBRARIES})
else()
	set(LIBRARIES ${COMMON_LIBRARIES} ${CMAKE_DL_LIBS})
endif()

add_executable(editor
	${SOURCE_FILES}
	${HEADER_FILES}
)
target_link_libraries(editor
	${LIBRARIES}
)

target_compile_features(editor PUBLIC cxx_std_20)
