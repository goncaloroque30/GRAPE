// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include <map>
#include <string>
#include <utility>

namespace GRAPE {
    /**
    * @brief GRAPE standard map class.
    *
    * Class can be moved, but not copied.
    */
    template <class Key, class Value>
    class GrapeMap {
    public:
        GrapeMap() = default;
        GrapeMap(const GrapeMap&) = delete;
        GrapeMap(GrapeMap&&) = default;
        GrapeMap& operator=(const GrapeMap&) = delete;
        GrapeMap& operator=(GrapeMap&&) = default;
        ~GrapeMap() = default;

        /**
        * @brief Const access a value in the container.
        * ASSERT contains(K).
        */
        [[nodiscard]] const Value& at(const Key& K) const noexcept {
            GRAPE_ASSERT(contains(K));
            return m_Values.at(K);
        }

        /**
        * @brief Const access a value in the container.
        * ASSERT contains(K).
        */
        [[nodiscard]] const Value& operator()(const Key& K) const noexcept { return at(K); }

        /**
        * @return Const iterator to the beginning of the underlying map.
        */
        [[nodiscard]] auto begin() const noexcept { return m_Values.begin(); }

        /**
        * @return Const iterator to the beginning of the underlying map.
        */
        [[nodiscard]] auto end() const noexcept { return m_Values.end(); }

        /**
        * @brief Access a value in the container
        * ASSERT contains(K).
        */
        Value& at(const Key& K) noexcept {
            GRAPE_ASSERT(contains(K));
            return m_Values.at(K);
        }

        /**
        * @brief Access a value in the container.
        * ASSERT contains(K).
        */
        Value& operator()(const Key& K) noexcept { return at(K); }

        /**
        * @return Iterator to the beginning of the underlying map.
        */
        auto begin() noexcept { return m_Values.begin(); }

        /**
        * @return Iterator to the end of the underlying map.
        */
        auto end() noexcept { return m_Values.end(); }

        /**
        * @return True if K is a key in the underlying map.
        */
        [[nodiscard]] bool contains(const Key& K) const noexcept { return m_Values.contains(K); }

        /**
        * @return True if underlying map is empty.
        */
        [[nodiscard]] bool empty() const noexcept { return m_Values.empty(); }

        /**
        * @return Size of the underlying map.
        */
        [[nodiscard]] std::size_t size() const noexcept { return m_Values.size(); }

        /**
        * @brief Adds K to the container and forwards Args to the constructor of Value.
        * @return A pair with the new value and true or the already existing value and false.
        */
        template <class... ValueArgs>
        std::pair<Value&, bool> add(const Key& K, ValueArgs&&... Args) noexcept;

        /**
        * @brief Erases K from the container.
        * @return True if value was erased, false otherwise (K was not int the container).
        */
        bool erase(const Key& K) noexcept { return m_Values.erase(K); } // Implicit conversion from size_type to bool | erase returns the number of elements removed

        /**
        * @brief Erases all values from the container for which the predicate is true.
        * @return True if at least one value was erased, false otherwise.
        */
        template <typename UnaryPredicate>
        bool eraseIf(UnaryPredicate Predicate) noexcept { return std::erase_if(m_Values, Predicate); } // Implicit conversion from size_type to bool | erase_if returns the number of elements removed

        /**
        * @brief Update OldKey to NewKey.
        * ASSERT contains(OldKey).
        *
        * @return True if key was updated, false if NewKey was already in the container.
        */
        bool update(const Key& OldKey, const Key& NewKey) noexcept;

        void clear() { m_Values.clear(); }

    private:
        std::map<Key, Value> m_Values;
    };

    template <class Key, class Value>
    template <class... ValueArgs>
    std::pair<Value&, bool> GrapeMap<Key, Value>::add(const Key& K, ValueArgs&&... Args) noexcept {
        const auto& [it, Emplaced] = m_Values.try_emplace(K, std::forward<ValueArgs>(Args)...);
        return { it->second, Emplaced };
    }

    template <class Key, class Value>
    bool GrapeMap<Key, Value>::update(const Key& OldKey, const Key& NewKey) noexcept {
        GRAPE_ASSERT(contains(OldKey));
        if (contains(NewKey))
            return false;

        auto node = m_Values.extract(OldKey);
        node.key() = NewKey;
        m_Values.insert(std::move(node));
        return true;
    }

    /**
    * @brief Generates an unused key in Maps by appending " {i}" to Key where i is an incrementing index.
    * @return Key if it was not in the container, otherwise "Key {i}" which is not a key of Map.
    */
    template <class Value>
    std::string uniqueKeyGenerator(const GrapeMap<std::string, Value>& Map, const std::string Key) noexcept {
        std::string unusedKey = Key;
        std::size_t i = 1;
        while (Map.contains(unusedKey))
            unusedKey = std::format("{} {}", Key, i++);
        return unusedKey;
    }
}
