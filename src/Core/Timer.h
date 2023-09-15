// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include <chrono>

namespace GRAPE {
    /**
    * @brief Simple wrapper around std::chrono, providing time ellapsed since reset() is called.
    */
    class Timer {
    public:
        /**
        * @brief Initializes the timer by calling reset().
        */
        Timer() { reset(); }

        /**
        * @brief Sets the start time point to now.
        */
        void reset() { m_Start = std::chrono::high_resolution_clock::now(); }

        /**
        * @return The number of seconds since reset() was called as a float.
        */
        float ellapsed() { return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_Start).count() * 1e-9f; }

        /**
        * @return The number of milliseconds since reset() was called as a float.
        */
        float ellapsedMillis() { return ellapsed() * 1e3f; }

        /**
        * @return The number of seconds since reset() was called as a std::chrono type.
        */
        std::chrono::seconds ellapsedDuration() { return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - m_Start); }
    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
    };
}
