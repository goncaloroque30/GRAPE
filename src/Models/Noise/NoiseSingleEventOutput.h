// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

namespace GRAPE {
    /**
    * @brief Stores the values of a single event output as a vector of pairs (maximum, equivalent). Each entry corresponds to a receptor.
    */
    class NoiseSingleEventOutput {
    public:
        // Constructors & Destructor
        explicit NoiseSingleEventOutput(std::size_t Size = 0);
        NoiseSingleEventOutput(const NoiseSingleEventOutput&) = delete;
        NoiseSingleEventOutput(NoiseSingleEventOutput&&) = default;
        NoiseSingleEventOutput& operator=(const NoiseSingleEventOutput&) = delete;
        NoiseSingleEventOutput& operator=(NoiseSingleEventOutput&&) = default;
        ~NoiseSingleEventOutput() = default;

        /**
        * @return Pair of maximum sound, equivalent sound at Index.
        *
        * ASSERT Index < size().
        */
        [[nodiscard]] const auto& values(std::size_t Index) const { GRAPE_ASSERT(Index < size()); return m_Values.at(Index); }

        [[nodiscard]] auto lamax() const { return m_Values | std::views::elements<0>; }
        [[nodiscard]] auto sel() const { return m_Values | std::views::elements<1>; }
        [[nodiscard]] auto begin() const { return m_Values.begin(); }
        [[nodiscard]] auto end() const { return m_Values.end(); }

        /**
        * Allocates capcaity of Size to the vector and initializes all valus to Value
        */
        void fill(std::size_t Size, double Value = 0);

        /**
        * @brief Sets  the values at the specified index
        *  ASSERT Index < size()
        */
        void setValues(std::size_t Index, double Lamax, double Sel);

        /**
        * @brief Add values to the vector.
        */
        void addValues(double Lamax, double Sel);

        /**
        * @brief Clear the vector.
        */
        void clear();

        /**
        * @brief The number of pairs in the vector.
        */
        [[nodiscard]] std::size_t size() const { return m_Values.size(); }
    private:
        std::vector<std::pair<double, double>> m_Values;
    };
}
