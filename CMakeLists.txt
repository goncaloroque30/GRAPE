# CMakeList.txt : Top-level CMake project file, do global configuration and include sub-projects.
cmake_minimum_required (VERSION 3.24)

#---------------------------
# Read Variables from src/Core/Core.h
#---------------------------
set(GRAPE_CORE_FILE_PATH "src/Core/Core.h")

set(GRAPE_DESCRIPTION_REGEX "#define GRAPE_DESCRIPTION[ \t]+\"(.+)\"")
file(STRINGS "${GRAPE_CORE_FILE_PATH}" GRAPE_DESCRIPTION REGEX ${GRAPE_DESCRIPTION_REGEX})
string(REGEX REPLACE ${GRAPE_DESCRIPTION_REGEX} "\\1" GRAPE_DESCRIPTION ${GRAPE_DESCRIPTION})

set(GRAPE_URL_REGEX "#define GRAPE_URL[ \t]+\"(.+)\"")
file(STRINGS "${GRAPE_CORE_FILE_PATH}" GRAPE_URL REGEX ${GRAPE_URL_REGEX})
string(REGEX REPLACE ${GRAPE_URL_REGEX} "\\1" GRAPE_URL ${GRAPE_URL})

set(GRAPE_VERSION_MAJOR_REGEX "#define GRAPE_VERSION_MAJOR[ \t]+(.+)")
file(STRINGS "${GRAPE_CORE_FILE_PATH}" GRAPE_VERSION_MAJOR REGEX ${GRAPE_VERSION_MAJOR_REGEX})
string(REGEX REPLACE ${GRAPE_VERSION_MAJOR_REGEX} "\\1" GRAPE_VERSION_MAJOR ${GRAPE_VERSION_MAJOR})

set(GRAPE_VERSION_MINOR_REGEX "#define GRAPE_VERSION_MINOR[ \t]+(.+)")
file(STRINGS "${GRAPE_CORE_FILE_PATH}" GRAPE_VERSION_MINOR REGEX ${GRAPE_VERSION_MINOR_REGEX})
string(REGEX REPLACE ${GRAPE_VERSION_MINOR_REGEX} "\\1" GRAPE_VERSION_MINOR ${GRAPE_VERSION_MINOR})

#---------------------------
# Set GRAPE paths
#---------------------------
cmake_path(SET GRAPE_DIR ${CMAKE_SOURCE_DIR})
cmake_path(SET GRAPE_DIR_MISC "${GRAPE_DIR}/misc")
cmake_path(SET GRAPE_DIR_EXAMPLES "${GRAPE_DIR}/examples")
cmake_path(SET GRAPE_DIR_CMAKE "${GRAPE_DIR}/cmake")
cmake_path(SET GRAPE_DIR_RES "${GRAPE_DIR}/res")
cmake_path(SET GRAPE_DIR_SRC "${GRAPE_DIR}/src")
cmake_path(SET GRAPE_DIR_SCRIPTS "${GRAPE_DIR}/scripts")
cmake_path(SET GRAPE_DIR_VENDOR "${GRAPE_DIR}/vendor")

#---------------------------
# Configure GRAPE project
#---------------------------
project ("GRAPE"
	VERSION "${GRAPE_VERSION_MAJOR}.${GRAPE_VERSION_MINOR}"
	DESCRIPTION "${GRAPE_DESCRIPTION}"
	HOMEPAGE_URL "${GRAPE_URL}"
	LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
include(InstallRequiredSystemLibraries)

option(GRAPE_BUILD_TESTS "Build the tests executable" OFF)
option(GRAPE_BUILD_SCRIPTS "Run the build scripts" OFF)

find_package(Vulkan REQUIRED)

include(GNUInstallDirs)
set(CMAKE_INSTALL_BINDIR "./")

#---------------------------
# Add 3rd Party Targets
#---------------------------
message(STATUS "Configuring vendor projects")
add_subdirectory(vendor)
message(STATUS "Configuring vendor projects - done")

#---------------------------
# Run Scripts
#---------------------------
if (GRAPE_BUILD_SCRIPTS)
	find_package(Python3 COMPONENTS Interpreter)
	if(Python3_FOUND)
        message(STATUS "Running python scripts")
       
        # Schemas
		execute_process(COMMAND ${Python3_EXECUTABLE} "${GRAPE_DIR_SCRIPTS}/GenerateSchemaSourceFiles.py" "${GRAPE_DIR_RES}/files/empty.grp" "${GRAPE_DIR_SRC}/Schema" "Schema" "GRAPE::Schema")
		execute_process(COMMAND ${Python3_EXECUTABLE} "${GRAPE_DIR_SCRIPTS}/GenerateSchemaSourceFiles.py" "${GRAPE_DIR_RES}/files/empty.gpkg" "${GRAPE_DIR_SRC}/Schema" "SchemaGpkg" "GRAPE::Schema::GPKG")

        # Embedded SQLite Schemas
        execute_process(COMMAND ${Python3_EXECUTABLE} "${GRAPE_DIR_SCRIPTS}/Bin2Header.py" "${GRAPE_DIR_RES}/files/empty.grp" "${GRAPE_DIR_SRC}/Study/GrapeSchema.embed")
        execute_process(COMMAND ${Python3_EXECUTABLE} "${GRAPE_DIR_SCRIPTS}/Bin2Header.py" "${GRAPE_DIR_RES}/files/empty.gpkg" "${GRAPE_DIR_SRC}/App/Embed/GrapeGeopackageSchema.embed")

        # Embedded Fonts
        execute_process(COMMAND ${Python3_EXECUTABLE} "${GRAPE_DIR_SCRIPTS}/Bin2Header.py" "${GRAPE_DIR_RES}/Fonts/Roboto-Medium.ttf" "${GRAPE_DIR_SRC}/App/Embed/RobotoMedium.embed")
        execute_process(COMMAND ${Python3_EXECUTABLE} "${GRAPE_DIR_SCRIPTS}/Bin2Header.py" "${GRAPE_DIR_RES}/Fonts/fa-solid-900.ttf" "${GRAPE_DIR_SRC}/App/Embed/FaSolid.embed")

        # Embedded Images
        execute_process(COMMAND ${Python3_EXECUTABLE} "${GRAPE_DIR_SCRIPTS}/Bin2Header.py" "${GRAPE_DIR_RES}/Icon/Default.png" "${GRAPE_DIR_SRC}/App/Embed/DefaultImage.embed")
        execute_process(COMMAND ${Python3_EXECUTABLE} "${GRAPE_DIR_SCRIPTS}/Bin2Header.py" "${GRAPE_DIR_RES}/Icon/GrapeIcon256.png" "${GRAPE_DIR_SRC}/App/Embed/GrapeIcon256.embed")
        execute_process(COMMAND ${Python3_EXECUTABLE} "${GRAPE_DIR_SCRIPTS}/Bin2Header.py" "${GRAPE_DIR_RES}/Icon/GrapeIcon16.png" "${GRAPE_DIR_SRC}/App/Embed/GrapeIcon16.embed")

        message(STATUS "Running python scripts - done")
	endif()
endif()

#---------------------------
# Add GRAPE Targets
#---------------------------
message(STATUS "Configuring ${PROJECT_NAME}")
add_subdirectory(src)
message(STATUS "Configuring ${PROJECT_NAME} - done")
