// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#pragma warning ( push )
#pragma warning ( disable : GRAPE_VENDOR_WARNINGS )
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/ringbuffer_sink.h"
#pragma warning ( pop )

namespace GRAPE {
    // Typedefs for sinks
    typedef spdlog::sinks::stdout_color_sink_mt ConsoleSink;
    typedef spdlog::sinks::ringbuffer_sink_mt RingBufferSink;

    /**
    * @brief Data struct with level and formatted message
    */
    struct LogMessage {
        spdlog::level::level_enum Level;
        std::string FormattedString;
    };

    /**
    * @brief Log class with console and ring sinks and multiple loggers.
    */
    class Log {
    public:
        /**
        * @brief Initialize sinks and loggers, call before any call to other members.
        */
        static void init();

        /**
        * @param Limit maximum size of the vector to be returned.
        * @return A vector with log messages
        */
        static std::vector<LogMessage> last(std::size_t Limit = 1000);

        static const std::shared_ptr<spdlog::logger>& core() { return s_CoreLogger; }
        static const std::shared_ptr<spdlog::logger>& dataLogic() { return s_DataLogicLogger; }
        static const std::shared_ptr<spdlog::logger>& models() { return s_ModelsLogger; }
        static const std::shared_ptr<spdlog::logger>& database() { return s_DatabaseLogger; }
        static const std::shared_ptr<spdlog::logger>& study() { return s_StudyLogger; }
        static const std::shared_ptr<spdlog::logger>& io() { return s_IOLogger; }

    private:
        inline static std::shared_ptr<RingBufferSink> s_RingBufferSink;
        inline static std::shared_ptr<spdlog::logger> s_CoreLogger;
        inline static std::shared_ptr<spdlog::logger> s_DataLogicLogger;
        inline static std::shared_ptr<spdlog::logger> s_ModelsLogger;
        inline static std::shared_ptr<spdlog::logger> s_DatabaseLogger;
        inline static std::shared_ptr<spdlog::logger> s_StudyLogger;
        inline static std::shared_ptr<spdlog::logger> s_IOLogger;
    };

    inline void Log::init() {
        auto consoleSink = std::make_shared<ConsoleSink>();
        consoleSink->set_pattern("%n [%l]: %v");

        s_RingBufferSink = std::make_shared<RingBufferSink>(1000);
        s_RingBufferSink->set_pattern("%n: %v");

        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(consoleSink);
        sinks.push_back(s_RingBufferSink);

        s_CoreLogger = std::make_shared<spdlog::logger>("GRAPE", consoleSink);
        s_ModelsLogger = std::make_shared<spdlog::logger>("Models", sinks.begin(), sinks.end());
        s_DataLogicLogger = std::make_shared<spdlog::logger>("Data Logic", sinks.begin(), sinks.end());

        s_DatabaseLogger = std::make_shared<spdlog::logger>("Database", sinks.begin(), sinks.end());

        s_StudyLogger = std::make_shared<spdlog::logger>("Study", sinks.begin(), sinks.end());
        s_IOLogger = std::make_shared<spdlog::logger>("IO", sinks.begin(), sinks.end());
    }

    inline std::vector<LogMessage> Log::last(const std::size_t Limit) {
        const auto rawVec = s_RingBufferSink->last_raw(Limit);
        const auto formattedVec = s_RingBufferSink->last_formatted(rawVec.size());
        std::vector<LogMessage> retVec;
        retVec.reserve(rawVec.size());

        for (std::size_t i = 0; i < rawVec.size(); ++i)
            retVec.emplace_back(rawVec.at(i).level, formattedVec.at(i));

        return retVec;
    }
}
