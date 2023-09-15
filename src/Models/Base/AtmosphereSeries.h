// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Atmosphere.h"

namespace GRAPE {
    /**
    * @brief Container for atmospheres for different time points.
    */
    class AtmosphereSeries {
    public:
        /**
        * @brief On construction no atmospheres are added.
        */
        AtmosphereSeries() = default;

        [[nodiscard]] auto begin() const noexcept { return m_Atmospheres.begin(); }
        [[nodiscard]] auto end() const noexcept { return m_Atmospheres.end(); }
        [[nodiscard]] auto begin() noexcept { return m_Atmospheres.begin(); }
        [[nodiscard]] auto end() noexcept { return m_Atmospheres.end(); }

        /**
        * @brief Get the atmosphere closest to Time
        *
        * ASSERT !empty()
        *
        * @return An Atmosphere object.
        */
        [[nodiscard]] const Atmosphere& atmosphere(TimePoint Time) const noexcept;

        /**
        * @brief Adds a default atmosphere to the end of the container.
        *
        * If empty adds an atmosphere at the current time point.
        * Else adds an atmosphere at the last time point + 30 minutes
        */
        void addAtmosphere() noexcept;

        /**
        * @brief Adds an atmosphere at the specified time point.
        */
        void addAtmosphere(TimePoint Time, const Atmosphere& Atm = Atmosphere()) noexcept;

        /**
        * @brief Adds an atmosphere at the specified time point.
        *
        * Throws if Time is in the container.
        */
        void addAtmosphereE(TimePoint Time, const Atmosphere& Atm);

        /**
        * @brief Update an existing Atmosphere time point in the container.
        *
        * ASSERT NewTime not in the container.
        */
        bool updateTime(const TimePoint& OldTime, const TimePoint NewTime) noexcept;

        /**
        * @brief Delete an Atmosphere from the container.
        *
        * ASSERT Time in the container.
        */
        void deleteAtmosphere(const TimePoint Time) noexcept;

        /**
        * @brief Clear all atmospheres from the container.
        */
        void clear() noexcept;

        /**
        * @return The number of atmospheres in the container.
        */
        [[nodiscard]] std::size_t size() const noexcept { return m_Atmospheres.size(); }

        /**
        * @return True if there are no atmospheres in the container.
        */
        [[nodiscard]] bool empty() const noexcept { return m_Atmospheres.empty(); }
    private:
        std::map<TimePoint, Atmosphere> m_Atmospheres;
    };
}
