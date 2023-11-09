// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

namespace GRAPE {
    /**
     * @brief A vector of bytes.
     */
    class Blob {
    public:
        Blob() = default;

        /**
        * @return The number of bytes in the vector.
        */
        [[nodiscard]] std::size_t size() const { return m_Bytes.size(); }

        [[nodiscard]] bool empty() const { return m_Bytes.empty(); }

        /**
        * @return Raw pointer to the beginning of the vector, which might be a nullptr.
        */
        [[nodiscard]] const void* data() const { return m_Bytes.data(); }

        /**
        * @brief Adds exactly 4 bytes to the vector.
        * @param Value The 4 bytes to be added represented as an int with the system endianness.
        */
        void add(std::uint32_t Value) {
            m_Bytes.resize(size() + 4, std::byte{ 0 });
            std::memcpy(endPointer(), &Value, 4);
            m_Pos += 4;
        }

        /**
        * @brief Adds exactly 8 bytes to the vector.
        * @param Value The 8 bytes to be added represented as a double with the system endianness.
        */
        void add(double Value) {
            m_Bytes.resize(size() + 8, std::byte{ 0 });
            std::memcpy(endPointer(), &Value, 8);
            m_Pos += 8;
        }

        /**
        * @brief Adds exactly 1 byte to the vector.
        * @param Value The byte to be added represented as a char.
        */
        void add(char Value) {
            m_Bytes.resize(size() + 1, std::byte{ 0 });
            std::memcpy(endPointer(), &Value, 1);
            m_Pos += 1;
        }

        /**
        * @brief Adds exactly 1 byte to the vector.
        * @param Value The byte to be added represented as an int with the system endianness.
        */
        void add(std::uint8_t Value) {
            m_Bytes.resize(size() + 1, std::byte{ 0 });
            std::memcpy(endPointer(), &Value, 1);
            m_Pos += 1;
        }

    private:
        std::vector<std::byte> m_Bytes;
        std::size_t m_Pos = 0;

    private:
        /**
        * @brief Used by the add routines, keeps track of the first writable address.
        * @return Pointer to the first writable address.
        */
        void* endPointer() { return m_Bytes.data() + m_Pos; }
    };
}
