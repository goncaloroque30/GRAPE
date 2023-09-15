#---------------------------
# Global
#---------------------------
if (NOT GRAPE_BUILD_TESTS)
	add_compile_definitions(DOCTEST_CONFIG_DISABLE)
endif()

#---------------------------
# Core
#---------------------------
set(CORE_TARGET "GRAPE_CORE")
add_library(${CORE_TARGET} INTERFACE)
target_link_libraries(${CORE_TARGET} INTERFACE spdlog::spdlog_header_only magic_enum::magic_enum doctest::doctest)
target_include_directories(${CORE_TARGET} INTERFACE ${GRAPE_DIR_SRC})
target_compile_definitions(${CORE_TARGET} INTERFACE
	$<$<CONFIG:Debug>:GRAPE_DEBUG>
	$<$<CONFIG:Distribution>:GRAPE_DISTRIBUTION>
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
    "Models/Aircraft/Doc29/Doc29Performance.cpp"
	"Models/Aircraft/Doc29/Doc29Profile.cpp"
	"Models/Aircraft/Doc29/Doc29ProfileCalculator.cpp"
	"Models/Aircraft/Doc29/Doc29Thrust.cpp"
	"Models/Aircraft/Doc29/Doc29Noise.cpp"
	"Models/Aircraft/Doc29/Doc29NoiseGenerator.cpp"
	"Models/Aircraft/FuelEmissions/LTO.cpp"
	"Models/Aircraft/FuelEmissions/LTOFuelFlowGenerator.cpp"
	"Models/Aircraft/FuelEmissions/LTOEmissionsGenerator.cpp"
    "Models/Aircraft/FuelEmissions/SFI.cpp"
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
	"Models/Performance/PerformanceCalculatorDoc29.cpp"
	"Models/Performance/PerformanceOutput.cpp"
	"Models/Performance/ProfileOutput.cpp"
	"Models/Performance/FuelFlow/FuelFlowCalculator.cpp"
	"Models/Performance/FuelFlow/FuelFlowCalculatorLTO.cpp"
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
	"Models/Emissions/EmissionsCalculatorLTO.cpp"
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
set(SCHEMA_TARGET "GRAPE_SCHEMA")
add_library(${SCHEMA_TARGET} OBJECT 
	"Schema/Schema.cpp"
	"Schema/SchemaGpkg.cpp"
)
target_link_libraries(${SCHEMA_TARGET} PRIVATE ${CORE_TARGET} ${DATABASE_TARGET} )
target_precompile_headers(${SCHEMA_TARGET} REUSE_FROM ${PCH_TARGET})

#---------------------------
# Study
#---------------------------
set(STUDY_TARGET "GRAPE_STUDY")
add_library(${STUDY_TARGET} STATIC
    "Study/Constraints.cpp"
    "Study/Study.cpp"
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
)
target_link_libraries(${STUDY_TARGET} PUBLIC ${CORE_TARGET} ${MODELS_TARGET} PRIVATE ${DATABASE_TARGET} ${SCHEMA_TARGET})
target_include_directories(${STUDY_TARGET} PUBLIC "${GRAPE_DIR_SRC}/Study")
target_precompile_headers(${STUDY_TARGET} REUSE_FROM ${PCH_TARGET})


#---------------------------
# Application
#---------------------------
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
)

if (WIN32)
	list(APPEND GRAPE_APP_SOURCES "${GRAPE_DIR_RES}/Icon/Grape.rc")
endif()

if(GRAPE_BUILD_TESTS)
	list(APPEND GRAPE_APP_SOURCES "App/MainTest.cpp")
else()
	list(APPEND GRAPE_APP_SOURCES "App/Main.cpp")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Distribution" AND WIN32)
	add_executable(${APP_TARGET} WIN32 ${GRAPE_APP_SOURCES})
else()
	add_executable(${APP_TARGET} ${GRAPE_APP_SOURCES})
endif()

target_link_libraries(${APP_TARGET} PUBLIC Vulkan::Vulkan glfw::glfw imgui::imgui nfd::nfd stb::stb rapidcsv::rapidcsv ${STUDY_TARGET})
target_include_directories(${APP_TARGET} PUBLIC "${GRAPE_DIR_SRC}/App")
target_precompile_headers(${APP_TARGET} REUSE_FROM ${PCH_TARGET})

#---------------------------
# Install
#---------------------------
install(TARGETS ${APP_TARGET})
install(FILES "${GRAPE_DIR}/LICENSE" "${GRAPE_DIR}/README.md" "${GRAPE_DIR_MISC}/grape.ini"
        DESTINATION "."
)
install(DIRECTORY ${GRAPE_DIR_RES}
		DESTINATION "."
		FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
		DIRECTORY_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
)

file(GLOB GRAPE_EXAMPLES "${GRAPE_DIR_EXAMPLES}/*.grp")
install(FILES ${GRAPE_EXAMPLES}
        DESTINATION "examples"
)
