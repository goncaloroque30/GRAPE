# magic_enum
add_subdirectory(magic_enum EXCLUDE_FROM_ALL)

# spdlog
set(SPDLOG_USE_STD_FORMAT)
add_subdirectory(spdlog EXCLUDE_FROM_ALL)

# nfd
add_subdirectory(nfd EXCLUDE_FROM_ALL)
add_library(nfd::nfd ALIAS nfd)

# rapidcsv
set(RAPIDCSV_DIR "${GRAPE_DIR_VENDOR}/rapidcsv")
add_library(rapidcsv INTERFACE "${RAPIDCSV_DIR}/src/rapidcsv.h")
target_include_directories(rapidcsv INTERFACE "${RAPIDCSV_DIR}/src")
add_library(rapidcsv::rapidcsv ALIAS rapidcsv)

# font awesome 6
set(ICONS_FONT_AWESOME_HELPERS_DIR "${GRAPE_DIR_VENDOR}/IconsFontCppHeaders")
add_library(iconsfontawesome INTERFACE)
target_include_directories(iconsfontawesome INTERFACE "${ICONS_FONT_AWESOME_HELPERS_DIR}")
add_library(iconsfontawesome::iconsfontawesome ALIAS iconsfontawesome)

# GeographicLib
add_subdirectory(geographiclib EXCLUDE_FROM_ALL)

# sqlite
set(SQLITE_DIR "${GRAPE_DIR_VENDOR}/sqlite")
add_library(sqlite STATIC
	"${SQLITE_DIR}/sqlite3.h"
	"${SQLITE_DIR}/sqlite3.c"
)
target_include_directories(sqlite PUBLIC "${SQLITE_DIR}")
target_compile_definitions(sqlite PRIVATE "-DSQLITE_THREADSAFE=2")
add_library(sqlite::sqlite ALIAS sqlite)

# glfw
add_subdirectory(glfw EXCLUDE_FROM_ALL)
add_library(glfw::glfw ALIAS glfw)

# imgui
set(IMGUI_DIR "${GRAPE_DIR_VENDOR}/imgui" CACHE PATH "The path to imgui.")
set(ICONS_FONT_DIR "${GRAPE_DIR_VENDOR}/IconsFontCppHeaders")
add_library(imgui INTERFACE)
target_link_libraries(imgui INTERFACE iconsfontawesome::iconsfontawesome)
target_include_directories(imgui INTERFACE "${IMGUI_DIR}" "${IMGUI_DIR}/misc/cpp")
target_sources(imgui INTERFACE
					"${IMGUI_DIR}/imgui.cpp"
                    "${IMGUI_DIR}/imgui_demo.cpp"
                    "${IMGUI_DIR}/imgui_draw.cpp"
                    "${IMGUI_DIR}/imgui_tables.cpp"
                    "${IMGUI_DIR}/imgui_widgets.cpp"
                    "${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp"
)
add_library(imgui::imgui ALIAS imgui)

# stb
set(STB_DIR "${GRAPE_DIR_VENDOR}/stb")
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${STB_DIR})
add_library(stb::stb ALIAS stb)

#doctest
add_subdirectory(doctest EXCLUDE_FROM_ALL)