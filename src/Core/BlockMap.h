// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include <type_traits>
#include <unordered_map>
#include <vector>

namespace GRAPE {
    /**
    * @brief Utility class to keep track of raw pointers as members of associated classes pointing to valid instances.
    *
    * Blocked: Pointer to the class instance which is pointed to in associated class.
    * Blocking: Blocking Pointer to the class instance containing the raw pointer
    *
    * The map is implemented as an unordered map with:
    *	Key: Const pointer to the blocked class instance,
    *	Value: Vector of pointers to the class instances blocking.
    */
    template <class Blocked, class Blocking> requires std::is_pointer_v<Blocked>&& std::is_const_v<std::remove_pointer_t<Blocked>>&& std::is_pointer_v<Blocking>
    class BlockMap {
    public:
        typedef std::remove_cvref_t<std::remove_pointer_t<Blocked>> BlockedType;
        typedef std::remove_pointer_t<Blocking> BlockingType;

        BlockMap() = default;

        /**
        * @param K Reference to class instance which is pointed to in associated class.
        * @return Vector of all the instances of the associated class containing a pointer to BlockedType.
        */
        auto& blocking(const BlockedType& K) { return m_Blocks.at(&K); }

        /**
        * @param K Reference to class instance which is pointed to in associated class.
        * @return Vector of all the instances of the associated class containing a pointer to K.
        */
        const auto& blocking(const BlockedType& K) const { return m_Blocks.at(&K); }

        /**
        * @param K Reference to class instance which is pointed to in associated class.
        * @return True if there is an instance of associated class which contains a pointer to K.
        */
        [[nodiscard]] bool contains(const BlockedType& K) const { return m_Blocks.contains(&K); }

        /**
        * @param K Reference to class instance which is pointed to in associated class.
        * @return Number of instances of associated class which contain a pointer to K.
        */
        [[nodiscard]] std::size_t blockingCount(const BlockedType& K) const { return m_Blocks.at(&K).size(); }

        /**
        * @return True if no instance of blocked class (pointed to in associated class) is pointed to.
        */
        [[nodiscard]] bool empty() const { return m_Blocks.empty(); }

        /**
        * @return Number of instances of blocked class (pointed to in associated class) being blocked.
        */
        [[nodiscard]] std::size_t size() const { return m_Blocks.size(); }

        /**
        * @brief Adds a block to the map.
        * @param K Reference to class instance which is pointed to in associated class.
        * @param B Reference to class instance containing the raw pointer to K.
        */
        void block(const BlockedType& K, BlockingType& B) {
            auto& blockingObjects = m_Blocks[&K];
            if (std::find(blockingObjects.begin(), blockingObjects.end(), &B) == blockingObjects.end())
                blockingObjects.emplace_back(&B);
        }

        /**
        * @brief Removes a block from the map.
        * @param K Reference to class instance which is pointed to in associated class.
        * @param B Reference to class instance containing the raw pointer to K.
        */
        void unblock(const BlockedType& K, BlockingType& B) {
            if (!contains(K))
                return;

            auto& blockingObjects = m_Blocks.at(&K);
            std::erase(blockingObjects, &B);
            if (blockingObjects.empty())
                m_Blocks.erase(&K);
        }

        /**
        * @brief Removes all blocks from the map.
        */
        void clear() { m_Blocks.clear(); }

    private:
        std::unordered_map<const BlockedType*, std::vector<BlockingType*>> m_Blocks;
    };
}
