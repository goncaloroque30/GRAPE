#---------------------------
# Core
#---------------------------
set(CORE_TARGET "GRAPE_CORE")
add_library(${CORE_TARGET} INTERFACE)
target_link_libraries(${CORE_TARGET} INTERFACE spdlog::spdlog_header_only magic_enum::magic_enum doctest::doctest)
target_include_directories(${CORE_TARGET} INTERFACE ${GRAPE_DIR_SRC})
target_compile_definitions(${CORE_TARGET} INTERFACE
	"$<$<CONFIG:Debug>:GRAPE_DEBUG>"
	"$<$<CONFIG:Distribution>:GRAPE_DISTRIBUTION>"
    "$<$<NOT:$<BOOL:${GRAPE_BUILD_TESTS}>>:DOCTEST_CONFIG_DISABLE>"
)
target_compile_options(${CORE_TARGET} INTERFACE
   "$<$<AND:$<CONFIG:Distribution>,${IS_MSVC}>:/O2;/DNDEBUG>"
)
target_link_options(${CORE_TARGET} INTERFACE
   "$<$<AND:$<CONFIG:Distribution>,${IS_MSVC}>:/INCREMENTAL:NO>"
)

#---------------------------
# PCH
#---------------------------
set(PCH_TARGET "GRAPE_PCH")
set(PCH_FILE "GRAPE_pch")

# Empty pch.cpp needed so that target knows the language
add_library(${PCH_TARGET} OBJECT "${PCH_FILE}.cpp")
target_link_libraries(${PCH_TARGET} PRIVATE ${CORE_TARGET})
target_precompile_headers(${PCH_TARGET} PRIVATE "${PCH_FILE}.h")
set(CMAKE_PCH_INSTANTIATE_TEMPLATES ON)

#---------------------------
# Models
#---------------------------
set(MODELS_TARGET "GRAPE_MODELS")
set(MODELS_TARGET_OBJ "GRAPE_MODELS_OBJ")
add_library(${MODELS_TARGET_OBJ} OBJECT
    "Models/Base/Atmosphere.cpp"
    "Models/Base/AtmosphereSeries.cpp"
	"Models/Base/CoordinateSystem.cpp"
	"Models/Aircraft/Aircraft.cpp"
    "Models/Aircraft/Doc29/Doc29Aircraft.cpp"
	"Models/Aircraft/Doc29/Doc29Profile.cpp"
	"Models/Aircraft/Doc29/Doc29ProfileCalculator.cpp"
	"Models/Aircraft/Doc29/Doc29Thrust.cpp"
	"Models/Aircraft/Doc29/Doc29Noise.cpp"
	"Models/Aircraft/Doc29/Doc29NoiseGenerator.cpp"
	"Models/Aircraft/FuelEmissions/LTO.cpp"
    "Models/Aircraft/FuelEmissions/SFI.cpp"
	"Models/Aircraft/FuelEmissions/LTOFuelFlowGenerator.cpp"
	"Models/Aircraft/FuelEmissions/LTODoc9889FuelFlowGenerator.cpp"
	"Models/Aircraft/FuelEmissions/BFFM2EmissionsGenerator.cpp"
	"Models/Airport/Airport.cpp"
	"Models/Airport/Runway.cpp"
	"Models/Airport/Route.cpp"
	"Models/Airport/RouteCalculator.cpp"
	"Models/Airport/RouteOutput.cpp"
    "Models/Operation/Operation.cpp"
	"Models/Operation/Flight.cpp"
	"Models/Operation/Track4d.cpp"
	"Models/Performance/PerformanceSpecification.cpp"
    "Models/Performance/PerformanceCalculator.cpp"
    "Models/Performance/PerformanceCalculatorTrack4d.cpp"
	"Models/Performance/PerformanceCalculatorDoc29.cpp"
	"Models/Performance/PerformanceOutput.cpp"
	"Models/Performance/ProfileOutput.cpp"
	"Models/Performance/FuelFlow/FuelFlowCalculator.cpp"
	"Models/Performance/FuelFlow/FuelFlowCalculatorLTO.cpp"
	"Models/Performance/FuelFlow/FuelFlowCalculatorLTODoc9889.cpp"
	"Models/Performance/FuelFlow/FuelFlowCalculatorSFI.cpp"
	"Models/Noise/AtmosphericAbsorption.cpp"
	"Models/Noise/ReceptorSets.cpp"
	"Models/Noise/ReceptorOutput.cpp"
	"Models/Noise/NoiseCalculator.cpp"
	"Models/Noise/NoiseSingleEventOutput.cpp"
	"Models/Noise/NoiseCumulativeOutput.cpp"
	"Models/Noise/NoiseCalculatorDoc29.cpp"
	"Models/Emissions/EmissionsSpecification.cpp"
	"Models/Emissions/EmissionsCalculator.cpp"
    "Models/Emissions/EmissionsCalculatorLTOCycle.cpp"
    "Models/Emissions/EmissionsCalculatorLTO.cpp"
	"Models/Emissions/EmissionsCalculatorBFFM2.cpp"
)
target_link_libraries(${MODELS_TARGET_OBJ} PUBLIC GeographicLib::GeographicLib PRIVATE ${CORE_TARGET})
target_include_directories(${MODELS_TARGET_OBJ} PUBLIC "${GRAPE_DIR_SRC}/Models")
target_precompile_headers(${MODELS_TARGET_OBJ} REUSE_FROM ${PCH_TARGET})
add_library(${MODELS_TARGET} INTERFACE)
target_link_libraries(${MODELS_TARGET} INTERFACE ${MODELS_TARGET_OBJ} $<TARGET_OBJECTS:${MODELS_TARGET_OBJ}>)

#---------------------------
# Database
#---------------------------
set(DATABASE_TARGET "GRAPE_DB")
add_library(${DATABASE_TARGET} STATIC
	"Database/Column.cpp"
	"Database/Database.cpp"
	"Database/Statement.cpp"
)
target_link_libraries(${DATABASE_TARGET} PRIVATE ${CORE_TARGET} sqlite::sqlite)
target_precompile_headers(${DATABASE_TARGET} REUSE_FROM ${PCH_TARGET})

#---------------------------
# Schema
#---------------------------
# Source files are generated in the build tree, only when the resource files empty.grp or empty.gpkg change
add_custom_command(
    OUTPUT "Schema/Schema.cpp"
    COMMAND ${Python3_EXECUTABLE}
        "${GRAPE_DIR_SCRIPTS}/GenerateSchemaSourceFiles.py"
        "${GRAPE_DIR_RES}/Files/empty.grp"
        "${CMAKE_CURRENT_BINARY_DIR}/Schema"
        "Schema"
        "GRAPE::Schema"
    DEPENDS "${GRAPE_DIR_RES}/Files/empty.grp"
    COMMENT "Creating GRAPE Schema..."
)
add_custom_command(
    OUTPUT "Schema/SchemaGpkg.cpp"
    COMMAND ${Python3_EXECUTABLE}
        "${GRAPE_DIR_SCRIPTS}/GenerateSchemaSourceFiles.py"
        "${GRAPE_DIR_RES}/Files/empty.gpkg"
        "${CMAKE_CURRENT_BINARY_DIR}/Schema"
        "SchemaGpkg"
        "GRAPE::Schema::GPKG"
    DEPENDS "${GRAPE_DIR_RES}/Files/empty.gpkg"
    COMMENT "Creating GRAPE Geopackage Schema..."
)

set(SCHEMA_TARGET "GRAPE_SCHEMA")
add_library(${SCHEMA_TARGET} OBJECT 
	"${CMAKE_CURRENT_BINARY_DIR}/Schema/Schema.cpp"
	"${CMAKE_CURRENT_BINARY_DIR}/Schema/SchemaGpkg.cpp"
)
target_include_directories(${SCHEMA_TARGET} INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(${SCHEMA_TARGET} PRIVATE ${CORE_TARGET} ${DATABASE_TARGET} )
target_precompile_headers(${SCHEMA_TARGET} REUSE_FROM ${PCH_TARGET})


#---------------------------
# Study
#---------------------------
# GrapeSchema.embed generated in the build tree, only when the resource file empty.grp changes
add_custom_command(
    OUTPUT "Embed/GrapeSchema.embed"
    COMMAND ${Python3_EXECUTABLE}
        "${GRAPE_DIR_SCRIPTS}/Bin2Header.py"
        "${GRAPE_DIR_RES}/files/empty.grp"
        "${CMAKE_CURRENT_BINARY_DIR}/Embed/GrapeSchema.embed"
    DEPENDS "${GRAPE_DIR_RES}/Files/empty.grp"
    COMMENT "Embedding GRAPE Schema..."
)
set(STUDY_TARGET "GRAPE_STUDY")
add_library(${STUDY_TARGET} STATIC
    "Study/Constraints.cpp"
    "Study/Study.cpp"
    "Study/Elevator/Elevator.cpp"
    "Study/Jobs/EmissionsRunJob.cpp"
    "Study/Jobs/JobManager.cpp"
    "Study/Jobs/NoiseRunJob.cpp"
    "Study/Jobs/PerformanceRunJob.cpp"
	"Study/Managers/AircraftsManager.cpp"
    "Study/Managers/AirportsManager.cpp"
    "Study/Managers/Doc29NoiseManager.cpp"
    "Study/Managers/Doc29PerformanceManager.cpp"
    "Study/Managers/LTOManager.cpp"
    "Study/Managers/OperationsManager.cpp"
    "Study/Managers/ScenariosManager.cpp"
    "Study/Managers/SFIManager.cpp"
    "Study/Scenario/EmissionsRun.cpp"
    "Study/Scenario/EmissionsRunOutput.cpp"
    "Study/Scenario/NoiseRun.cpp"
    "Study/Scenario/NoiseRunOutput.cpp"
	"Study/Scenario/PerformanceRun.cpp"
	"Study/Scenario/PerformanceRunOutput.cpp"
	"Study/Scenario/Scenario.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Embed/GrapeSchema.embed"
)
target_link_libraries(${STUDY_TARGET} PUBLIC ${CORE_TARGET} ${MODELS_TARGET} ${SCHEMA_TARGET} PRIVATE ${DATABASE_TARGET})
target_include_directories(${STUDY_TARGET} PUBLIC "${GRAPE_DIR_SRC}/Study")
target_precompile_headers(${STUDY_TARGET} REUSE_FROM ${PCH_TARGET})

#---------------------------
# Application
#---------------------------
# GrapeGeopackageSchema.embed generated in the build tree, only when the resource file empty.gpkg changes
add_custom_command(
    OUTPUT "Embed/GrapeGeopackageSchema.embed"
    COMMAND ${Python3_EXECUTABLE}
        "${GRAPE_DIR_SCRIPTS}/Bin2Header.py"
        "${GRAPE_DIR_RES}/files/empty.gpkg"
        "${CMAKE_CURRENT_BINARY_DIR}/Embed/GrapeGeopackageSchema.embed"
    DEPENDS "${GRAPE_DIR_RES}/Files/empty.gpkg"
    COMMENT "Embedding GRAPE Geopackage Schema..."
)
# Fonts embedded files generated in the build tree
add_custom_command(
    OUTPUT "Embed/RobotoMedium.embed" "Embed/FaSolid.embed"
    COMMAND ${Python3_EXECUTABLE}
    "${GRAPE_DIR_SCRIPTS}/Bin2Header.py"
    "${GRAPE_DIR_RES}/Fonts/Roboto-Medium.ttf"
    "${CMAKE_CURRENT_BINARY_DIR}/Embed/RobotoMedium.embed"
    COMMAND ${Python3_EXECUTABLE}
    "${GRAPE_DIR_SCRIPTS}/Bin2Header.py"
    "${GRAPE_DIR_RES}/Fonts/fa-solid-900.ttf"
    "${CMAKE_CURRENT_BINARY_DIR}/Embed/FaSolid.embed"
    DEPENDS "${GRAPE_DIR_RES}/Fonts/Roboto-Medium.ttf" "${GRAPE_DIR_RES}/Fonts/fa-solid-900.ttf"
    COMMENT "Embedding fonts..."
)
# Icons embedded files generated in the build tree
add_custom_command(
    OUTPUT "Embed/DefaultImage.embed" "Embed/GrapeIcon16.embed" "Embed/GrapeIcon256.embed"
    COMMAND ${Python3_EXECUTABLE}
    "${GRAPE_DIR_SCRIPTS}/Bin2Header.py"
    "${GRAPE_DIR_RES}/Icon/Default.png"
    "${CMAKE_CURRENT_BINARY_DIR}/Embed/DefaultImage.embed"
    COMMAND ${Python3_EXECUTABLE}
    "${GRAPE_DIR_SCRIPTS}/Bin2Header.py"
    "${GRAPE_DIR_RES}/Icon/GrapeIcon16.png"
    "${CMAKE_CURRENT_BINARY_DIR}/Embed/GrapeIcon16.embed"
    COMMAND ${Python3_EXECUTABLE}
    "${GRAPE_DIR_SCRIPTS}/Bin2Header.py"
    "${GRAPE_DIR_RES}/Icon/GrapeIcon256.png"
    "${CMAKE_CURRENT_BINARY_DIR}/Embed/GrapeIcon256.embed"
    DEPENDS "${GRAPE_DIR_RES}/Icon/Default.png" "${GRAPE_DIR_RES}/Icon/GrapeIcon16.png" "${GRAPE_DIR_RES}/Icon/GrapeIcon256.png"
    COMMENT "Embedding icons..."
)

set(APP_TARGET "GRAPE")
set(GRAPE_APP_SOURCES
	"$CACHE{IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "$CACHE{IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
    "App/Application.cpp"
	"App/AsyncTask.cpp"
	"App/Image.cpp"
    "App/Settings.cpp"
    "App/UI.cpp"
    "App/Units.cpp" 
    "App/IO/Csv.cpp"
    "App/IO/CsvExport.cpp"
    "App/IO/CsvImport.cpp"
    "App/IO/AnpImport.cpp"
    "App/IO/GpkgExport.cpp"
	"App/Modals/AboutModal.cpp"
	"App/Modals/SettingsModal.cpp"
    "App/Panels/AirportsPanel.cpp"
    "App/Panels/Doc29Panel.cpp"
    "App/Panels/FleetPanel.cpp"
    "App/Panels/FlightsPanel.cpp"
    "App/Panels/LogPanel.cpp"
    "App/Panels/LTOPanel.cpp"
    "App/Panels/ScenariosPanel.cpp"
    "App/Panels/SFIPanel.cpp"
    "App/Panels/Tracks4dPanel.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Embed/GrapeGeopackageSchema.embed"
    "${CMAKE_CURRENT_BINARY_DIR}/Embed/RobotoMedium.embed"
    "${CMAKE_CURRENT_BINARY_DIR}/Embed/FaSolid.embed"
    "${CMAKE_CURRENT_BINARY_DIR}/Embed/DefaultImage.embed"
    "${CMAKE_CURRENT_BINARY_DIR}/Embed/GrapeIcon16.embed"
    "${CMAKE_CURRENT_BINARY_DIR}/Embed/GrapeIcon256.embed"
)

if (WIN32)
	list(APPEND GRAPE_APP_SOURCES "${GRAPE_DIR_RES}/Icon/Grape.rc")
endif()

if(GRAPE_BUILD_TESTS)
	list(APPEND GRAPE_APP_SOURCES "App/MainTest.cpp")
else()
	list(APPEND GRAPE_APP_SOURCES "App/Main.cpp")
endif()

add_executable(${APP_TARGET} ${GRAPE_APP_SOURCES})
set_target_properties(${APP_TARGET} PROPERTIES WIN32_EXECUTABLE $<AND:$<CONFIG:Distribution>,${IS_WINDOWS}>)

target_link_libraries(${APP_TARGET} PUBLIC Vulkan::Vulkan glfw::glfw imgui::imgui nfd::nfd stb::stb rapidcsv::rapidcsv ${STUDY_TARGET})
target_include_directories(${APP_TARGET} PUBLIC "${GRAPE_DIR_SRC}/App")
target_precompile_headers(${APP_TARGET} REUSE_FROM ${PCH_TARGET})

#---------------------------
# Install
#---------------------------
include(GNUInstallDirs) 
install(
    TARGETS ${APP_TARGET}
)

install(
    FILES "${GRAPE_DIR}/LICENSE" "${GRAPE_DIR}/README.md"
    DESTINATION "."
)
install(
    FILES "${GRAPE_DIR_MISC}/grape.ini"
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
    DIRECTORY ${GRAPE_DIR_RES}
    DESTINATION "."
    FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
    DIRECTORY_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
)

file(GLOB GRAPE_EXAMPLES "${GRAPE_DIR_EXAMPLES}/*.grp")
install(
    FILES ${GRAPE_EXAMPLES}
    DESTINATION "examples"
)

# Requires admin privileges
#[===[
install(
    CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:${APP_TARGET}> ${CMAKE_INSTALL_PREFIX}/$<TARGET_FILE_NAME:${APP_TARGET}>)"
    CONFIGURATIONS "Distribution"
)
]===]

#---------------------------
# Tests
#---------------------------
if(GRAPE_BUILD_TESTS)
    add_test(NAME "Unit Tests" COMMAND ${APP_TARGET})
endif()

#---------------------------
# Packaging
#---------------------------
set(CPACK_PACKAGE_HOMEPAGE_URL "https://goncaloroque30.github.io/GRAPE-Docs/")
set(CPACK_GENERATOR "TGZ")
include(CPack)
