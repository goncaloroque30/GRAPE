// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

/************************
* Main GRAPE Definitions
************************/
#define GRAPE_MACRO_CONCAT_IMPLEMENTATION(x, y) x##y
#define GRAPE_MACRO_CONCAT(x, y) GRAPE_MACRO_CONCAT_IMPLEMENTATION(x, y)

#define GRAPE_MACRO_EXPAND(x) x
#define GRAPE_MACRO_STRINGIFY_IMPLEMENTATION(x) #x
#define GRAPE_MACRO_STRINGIFY(x) GRAPE_MACRO_STRINGIFY_IMPLEMENTATION(x)

#define GRAPE_DESCRIPTION "GRAPE: A desktop application to calculate airport environmental impacts"
#define GRAPE_URL "https://github.com/goncaloroque30/GRAPE"
#define GRAPE_DOCS_URL "https://goncaloroque30.github.io/GRAPE-Docs/"
#define GRAPE_ID 367
#define GRAPE_VERSION_MAJOR 1
#define GRAPE_VERSION_MINOR 1

#define GRAPE_VERSION_NUMBER GRAPE_MACRO_CONCAT(GRAPE_VERSION_MAJOR, GRAPE_VERSION_MINOR)
#define GRAPE_VERSION_STRING GRAPE_MACRO_STRINGIFY(GRAPE_VERSION_MAJOR) "." GRAPE_MACRO_STRINGIFY(GRAPE_VERSION_MINOR)

#define ALL_CPPCORECHECK_WARNINGS 26400 26401 26402 26403 26404 26405 26406 26407 26408 26409 26410 26411 26414 26415 26416 26417 26418 26426 26427 26429 26430 26431 26432 26433 26434 26435 26436 26437 26438 26439 26440 26441 26443 26444 26445 26446 26447 26448 26449 26450 26451 26452 26453 26454 26455 26456 26459 26460 26461 26462 26463 26464 26465 26466 26467 26471 26472 26473 26474 26475 26476 26477 26478 26481 26482 26483 26485 26486 26487 26488 26489 26490 26491 26492 26493 26494 26495 26496 26497 26498 26800 26810 26811 26812 26814 26815 26816 26817 26818 26819 26820 26821
#define ALL_CODE_ANALYSIS_WARNINGS 6001 6011 6014 6029 6031 6053 6054 6059 6063 6064 6065 6066 6067 6101 6200 6201 6211 6214 6215 6216 6217 6219 6220 6221 6225 6226 6230 6235 6236 6237 6239 6240 6242 6244 6246 6248 6250 6255 6258 6259 6260 6262 6263 6268 6269 6270 6271 6272 6273 6274 6276 6277 6278 6279 6280 6281 6282 6283 6284 6285 6286 6287 6288 6289 6290 6291 6292 6293 6294 6295 6296 6297 6298 6299 6302 6303 6305 6306 6308 6310 6312 6313 6314 6315 6316 6317 6318 6319 6320 6322 6323 6324 6326 6328 6329 6330 6331 6332 6333 6334 6335 6336 6340 6381 6383 6384 6385 6386 6387 6388 6400 6401 6411 6412 6500 6501 6503 6504 6505 6506 6508 6509 6510 6511 6513 6514 6515 6516 6517 6518 6522 6525 6530 6540 6551 6552 6701 6702 6703 6704 6705 6706 6707 6800 6801 6993 6997 26100 26101 26102 26105 26106 26110 26111 26112 26115 26116 26117 26130 26135 26136 26137 26138 26140 26160 26165 26166 26167 26831 28001 28002 28003 28004 28011 28012 28013 28014 28020 28021 28022 28023 28024 28039 28051 28052 28053 28103 28104 28105 28106 28107 28108 28109 28112 28113 28125 28137 28138 28159 28160 28163 28164 28182 28183 28191 28192 28193 28194 28195 28196 28197 28198 28199 28200 28201 28202 28203 28204 28205 28206 28207 28208 28209 28211 28212 28213 28214 28215 28216 28217 28218 28219 28220 28221 28222 28223 28224 28225 28226 28227 28228 28229 28230 28231 28232 28233 28234 28235 28236 28237 28238 28239 28240 28241 28242 28243 28244 28245 28246 28247 28248 28250 28251 28252 28253 28254 28262 28263 28267 28272 28273 28275 28276 28278 28279 28280 28281 28282 28283 28284 28285 28286 28287 28288 28289 28290 28291 28292 28293 28294 28295 28296 28297 28298 28299 28300 28301 28302 28303 28304 28305 28306 28307 28308 28309 28310 28311 28312 28313 28350 28351 28500 28504 33001 33004 33005 33010 33011 33020 33022 36503 36504 36505 36506 36509 36510 36515 36522 36526 36530 ALL_CPPCORECHECK_WARNINGS
#define GRAPE_VENDOR_WARNINGS 4005 4244 4251 4267 4996 ALL_CODE_ANALYSIS_WARNINGS

/************************
* Platform Detection
************************/
#ifdef _WIN32
// Windows
#ifndef _WIN64
        // Not x64
#error "Only x64 builds are supported!"
#else
// Windows x64
#define GRAPE_PLATFORM_WINDOWS
#endif
#elif defined(__APPLE__) || defined(__MACH__)
// Apple
#include <TargetConditionals.h>
/* TARGET_OS_MAC exists on all Apple platforms,
so all must be sequentially checked in order to ensure
that the current platform is MacOS */
#if TARGET_IPHONE_SIMULATOR == 1
#error "iPhone OS simulator is not supported!"
#elif TARGET_OS_IPHONE == 1
#error "iPhone OS is not supported!"
#elif TARGET_OS_MAC == 1
#define GRAPE_PLATFORM_MACOS
#error "MacOS is not yet supported!"
#else
#error "Unknown Apple platform!"
#endif
/* Check __ANDROID__ before __linux__
since android is based on the linux kernel
and it has __linux__ defined */
#elif defined(__ANDROID__)
#error "Android is not supported!"
#elif defined(__linux__)
#define GRAPE_PLATFORM_LINUX
#error "Linux is not yet supported!"
#else
/* Unknown compiler/platform */
#error "Unknown platform!"
#endif

/************************
* Log
************************/
#include "Log.h"
#include <filesystem>

/************************
* DEBUG and ASSERT
************************/
#ifdef GRAPE_DEBUG
#define GRAPE_DEBUG_INFO(...) ::GRAPE::Log::core()->info(__VA_ARGS__)
#define GRAPE_DEBUG_WARN(...) ::GRAPE::Log::core()->warn(__VA_ARGS__)
#define GRAPE_DEBUG_ERROR(...) ::GRAPE::Log::core()->error(__VA_ARGS__)
#ifdef GRAPE_PLATFORM_WINDOWS
#define GRAPE_DEBUGBREAK() __debugbreak()
#elif defined(GRAPE_PLATFORM_LINUX)
#include <signal.h>
#define GRAPE_DEBUGBREAK() raise(SIGTRAP)
#else
#error "Platform doesn't support debugbreak yet!"
#endif
#define GRAPE_ENABLE_ASSERTS
#else
#define GRAPE_DEBUG_INFO(...)
#define GRAPE_DEBUG_WARN(...)
#define GRAPE_DEBUG_ERROR(...)
#define GRAPE_DEBUGBREAK()
#endif

#ifdef GRAPE_ENABLE_ASSERTS
#define GRAPE_INTERNAL_ASSERT_IMPLEMENTATION(Check, Message) { if(!(Check)) { GRAPE_DEBUG_ERROR("Assertion failed at {0}({1}). " Message, std::filesystem::path(__FILE__).filename().string(), __LINE__); GRAPE_DEBUGBREAK(); } }
#define GRAPE_INTERNAL_ASSERT_IMPLEMENTATION_WITH_ARGUMENTS(Check, Message, ...) { if(!(Check)) { GRAPE_DEBUG_ERROR("Assertion failed at {0}({1}). " Message, std::filesystem::path(__FILE__).filename().string(), __LINE__, __VA_ARGS__); GRAPE_DEBUGBREAK(); } }
#define GRAPE_INTERNAL_ASSERT_WITHOUT_MESSAGE(Check) GRAPE_INTERNAL_ASSERT_IMPLEMENTATION(Check, "")
#define GRAPE_INTERNAL_ASSERT_WITH_MESSAGE(Check, Message) GRAPE_INTERNAL_ASSERT_IMPLEMENTATION(Check, Message)
#define GRAPE_INTERNAL_ASSERT_WITH_MESSAGE_AND_ARGUMENTS(Check, Message, ...) GRAPE_INTERNAL_ASSERT_IMPLEMENTATION_WITH_ARGUMENTS(Check, Message, __VA_ARGS__)

#define GRAPE_INTERNAL_ASSERT_GET_MACRO_NAME(Message, Argument1, Argument2, Argument3, Argument4, Argument5, Macro, ...) Macro
#define GRAPE_INTERNAL_ASSERT_GET_MACRO(...) GRAPE_MACRO_EXPAND(GRAPE_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, \
			GRAPE_INTERNAL_ASSERT_WITH_MESSAGE_AND_ARGUMENTS, \
			GRAPE_INTERNAL_ASSERT_WITH_MESSAGE_AND_ARGUMENTS, \
			GRAPE_INTERNAL_ASSERT_WITH_MESSAGE_AND_ARGUMENTS, \
			GRAPE_INTERNAL_ASSERT_WITH_MESSAGE_AND_ARGUMENTS, \
			GRAPE_INTERNAL_ASSERT_WITH_MESSAGE_AND_ARGUMENTS, \
			GRAPE_INTERNAL_ASSERT_WITH_MESSAGE, \
			GRAPE_INTERNAL_ASSERT_WITHOUT_MESSAGE \
			))

// Condition must be given, message and arguments are optional. A leading space in message is given, arguments must start at number 2.
#define GRAPE_ASSERT(Check, ...)  GRAPE_MACRO_EXPAND( GRAPE_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(Check, __VA_ARGS__) )
#else
#define GRAPE_ASSERT(...)
#endif

#include <chrono>
#include <format>
#include <limits>
#include <magic_enum.hpp>
#include <numbers>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include "BlockMap.h"
#include "GrapeMap.h"
#include "Timer.h"

#include "Platform.h"

namespace GRAPE {
    inline void initGRAPE() { Log::init(); }

    namespace Constants {
        constexpr double NaN = std::numeric_limits<double>::quiet_NaN();
        constexpr double Inf = std::numeric_limits<double>::infinity();
        constexpr double Precision = 0.000001;
        constexpr double PrecisionTest = 0.00001;
        constexpr double AngleThreshold = 0.1;
        constexpr double DistanceThreshold = 1.0;
        constexpr double Pi = std::numbers::pi;
        constexpr std::streamsize PrecisionDigits = 6;
    }

    // For single line std::visit calls
    template <class... Ts>
    struct Overload : Ts... {
        using Ts::operator()...;
    };

    template <class... Ts>
    Overload(Ts...) -> Overload<Ts...>;

    // Time Types
    typedef std::chrono::tai_seconds TimePoint;
    typedef std::chrono::utc_seconds TimePointUtc;
    typedef std::chrono::seconds Duration;

    inline TimePoint now() {
        return std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::tai_clock::now());
    }
    /**
    * @brief Standard conversion to string representation of time variable.
    * @return The Time converted to UTC in the format "yyyy-mm-dd HH:MM:SS".
    */
    inline std::string timeToUtcString(const TimePoint& Time) {
        return std::format("{0:%F} {0:%T}", std::chrono::tai_clock::to_utc(Time));
    }

    /**
    * @brief Standard conversion to time point from a string in the format "yyyy-mm-dd HH:MM:SS".
    * @return The TAI clock time point if the conversions was successful.
    */
    inline std::optional<TimePoint> utcStringToTime(const std::string& TimeStr) {
        TimePointUtc utcTime;
        std::stringstream utcStr;
        utcStr << TimeStr;
        from_stream(utcStr, "%F %T", utcTime);
        if (utcStr)
            return std::chrono::tai_clock::from_utc(utcTime);
        else
            return std::nullopt;
    }

    /**
    * @brief Standard conversion to string representation of duration variable.
    * @return The Time in the format "HH:MM:SS".
    */
    inline std::string durationToString(const Duration& Time) {
        return std::format("{0:%T}", Time);
    }

    /**
    * @brief Standard conversion to duration from a string in the format "HH:MM:SS".
    * @return The duration if the conversions was successful.
    */
    inline std::optional<Duration> stringToDuration(const std::string& TimeStr) {
        std::stringstream timeStr;
        timeStr << TimeStr;
        auto time = std::chrono::seconds(0);
        from_stream(timeStr, "%T", time);

        if (timeStr)
            return time;
        else
            return std::nullopt;
    }

    /**
    * @brief Implements conversion between enum and string representation
    */
    template <typename Enum> requires std::is_enum_v<Enum>
    struct EnumStrings {
        template <typename... Sv>
        constexpr EnumStrings(Sv&&... Args) noexcept : Strings{ std::forward<Sv>(Args)... } {}

        /**
        * @brief String to enum conversion.
        * @param String The string representation.
        * @return The enum corresponding to the String.
        */
        [[nodiscard]] Enum fromString(const std::string& String) const noexcept {
            auto index = std::ranges::find(Strings, String) - Strings.begin();
            GRAPE_ASSERT(index < magic_enum::enum_count<Enum>());
            return magic_enum::enum_value<Enum>(index);
        }

        /**
        * @brief Enum to string conversion.
        * @param Value The enum value.
        * @return The string corresponding to the enum value.
        */
        [[nodiscard]] std::string toString(Enum Value) const noexcept { return Strings.at(magic_enum::enum_index(Value).value()); }

        /**
        * @return Iterator to the beginning of the array containing the string representations.
        */
        auto begin() const noexcept { return Strings.begin(); }

        /**
        * @return Iterator to the end of the array containing the string representations.
        */
        auto end() const noexcept { return Strings.end(); }

        /**
        * @return Size of the array containing the string representations.
        */
        [[nodiscard]] constexpr std::size_t size() const noexcept { return Strings.size(); }

        /**
        * @return True if String is in the array containing the string representations.
        */
        [[nodiscard]] bool contains(const std::string& String) const noexcept { return std::ranges::find(Strings, String) != Strings.end(); }

        std::array<const char*, magic_enum::enum_count<Enum>()> Strings;
    };

    /**
    * @brief Simple Vector3 class with 3 dimensional coordinates.
    */
    struct Vector3 {
        double X, Y, Z;

        /**
        * @brief Constructor needed for use with e.g. emplace_back.
        */
        Vector3(double XIn, double YIn, double ZIn = 1) noexcept : X(XIn), Y(YIn), Z(ZIn) {}

        /**
        * @brief Cross product between this and B.
        *
        * The cross product, a × b, is a vector that is perpendicular to both a and b, and thus normal to the plane containing them.
        */
        [[nodiscard]] Vector3 cross(const Vector3& B) const noexcept { return { Y * B.Z - Z * B.Y, Z * B.X - X * B.Z, X * B.Y - Y * B.X }; }

        /**
        * @brief Norm to the Z coordinate (Z with 1 after calling this function).
        */
        void norm() noexcept {
            X /= Z;
            Y /= Z;
            Z = 1;
        }
    };

    /**
    * @brief Exception class to be used for incorrect user inputs (incorrect boundaries, references, ...)
    */
    class GrapeException : public std::exception {
    public:
        /**
        * @param What A string describing the exception.
        */
        explicit GrapeException(std::string_view What) : m_What(What) {}

        /**
        * @return The description passed to the constructor of this exception.
        */
        [[nodiscard]] const char* what() const override { return m_What.c_str(); }

    private:
        std::string m_What;
    };
}
