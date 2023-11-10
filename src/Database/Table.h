// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

namespace GRAPE {
    /**
     * @brief Constant representation of a database table via its name and variables. Can be used in a constexpr context.
     */
    template <std::size_t Size>
    class Table {
    public:
        constexpr Table(std::string_view Name, const std::string_view(&Vars)[Size]) : m_Name(Name), m_Variables(std::to_array(Vars)) {}

        [[nodiscard]] std::string_view name() const { return m_Name; }

        /**
         * @param Index The 0 based index of the variable to be accessed.
         */
        [[nodiscard]] std::string_view variableName(std::size_t Index) const { return m_Variables.at(Index); }

        /**
        * @return Number of variables in the table.
        */
        [[nodiscard]] static std::size_t size() { return Size; }

        /**
        * @return Iterator to the begin of the variables array.
        */
        [[nodiscard]] auto begin() const { return m_Variables.begin(); }

        /**
        * @return Iterator to the end of the variables array.
        */
        [[nodiscard]] auto end() const { return m_Variables.end(); }

        /**
        * @param InsertVars The 0 based indexes of the insert variables. All if empty.
        * @return The string of the INSERT INTO query with "?" as placeholders for the values. If InsertVars is empty all variables in the table should be set.
        *
        */
        [[nodiscard]] std::string queryInsert(std::initializer_list<std::size_t> InsertVars = {}) const;

        /**
        * @param SetVars The 0 based indexes of the variables to be updated. All if empty.
        * @param FilterVars The 0 based indexes of the filter variables. None if empty.
        * @return The string of the UPDATE query with "?" as placeholders for the set and filter values.
        */
        [[nodiscard]] std::string queryUpdate(std::initializer_list<std::size_t> SetVars = {}, std::initializer_list<std::size_t> FilterVars = {}) const;

        /**
        * @param FilterVars The 0 based indexes of the filter variables.
        * @return The string of the DELETE FROM query with "?" as placeholders for the filter values.
        */
        [[nodiscard]] std::string queryDelete(std::initializer_list<std::size_t> FilterVars = {}) const;

        /**
        * @param SelectVars The 0 based indexes of the variables to be selected.
        * @param FilterVars The 0 based indexes of the filter variables.
        * @param SortVars The 0 based indexes of the sort variables.
        * @param Distinct SELECT DISTINCT if true.
        * @return The string of the SELECT query with "?" as placeholders for the filter and sort values.
        */
        [[nodiscard]] std::string querySelect(const std::vector<std::size_t>& SelectVars = {}, const std::vector<std::size_t>& FilterVars = {}, const std::vector<std::size_t>& SortVars = {}, bool Distinct = false) const;

    private:
        std::string_view m_Name;
        std::array<std::string_view, Size> m_Variables;
    };

    template <std::size_t Size>
    std::string Table<Size>::queryInsert(std::initializer_list<std::size_t> InsertVars) const {
        const auto insertSize = InsertVars.size() != 0 ? InsertVars.size() : Size;

        std::string q{ std::string("INSERT INTO ").append(name()) };

        if (InsertVars.size() != 0)
        {
            q.append(" (");
            for (const auto i : InsertVars)
                q.append(variableName(i)).append(", ");
            q.erase(q.end() - 2, q.end());
            q.append(")");
        }

        q.append(" VALUES(");
        for (std::size_t i = 0; i < insertSize; i++)
            q.append("?, ");
        q.erase(q.end() - 2, q.end());
        q.append(")");

        return q;
    }

    template <std::size_t Size>
    std::string Table<Size>::queryUpdate(std::initializer_list<std::size_t> SetVars, std::initializer_list<std::size_t> FilterVars) const {
        std::string q{ std::string("UPDATE ").append(name()).append(" SET ") };

        if (SetVars.size() == 0)
            for (std::size_t i = 0; i < size(); i++)
                q.append(variableName(i)).append(" = ?, ");
        else
            for (const auto i : SetVars)
                q.append(variableName(i)).append(" = ?, ");

        q.erase(q.end() - 2, q.end());

        if (FilterVars.size() != 0)
        {
            q.append(" WHERE ");

            for (const auto i : FilterVars)
                q.append(variableName(i)).append(" = ? AND ");

            q.erase(q.end() - 5, q.end());
        }
        return q;
    }

    template <std::size_t Size>
    std::string Table<Size>::queryDelete(std::initializer_list<std::size_t> FilterVars) const {
        std::string q{ std::string("DELETE FROM ").append(name()) };

        if (FilterVars.size() != 0)
        {
            q.append(" WHERE ");
            for (const auto i : FilterVars)
                q.append(variableName(i)).append(" = ? AND ");
            q.erase(q.end() - 5, q.end());
        }

        return q;
    }

    template <std::size_t Size>
    std::string Table<Size>::querySelect(const std::vector<std::size_t>& SelectVars, const std::vector<std::size_t>& FilterVars, const std::vector<std::size_t>& SortVars, bool Distinct) const {
        std::string q = "SELECT ";

        if (Distinct)
            q.append("DISTINCT ");

        if (SelectVars.empty()) { q.append("*"); }
        else
        {
            for (const auto i : SelectVars)
                q.append(variableName(i)).append(", ");
            q.erase(q.end() - 2, q.end());
        }

        q.append(" FROM ").append(name());

        if (!FilterVars.empty())
        {
            q.append(" WHERE ");
            for (const auto i : FilterVars)
                q.append(variableName(i)).append(" = ?").append(" AND ");
            q.erase(q.end() - 5, q.end());
        }

        if (!SortVars.empty())
        {
            q.append(" ORDER BY ");
            for (const auto i : SortVars)
                q.append(variableName(i)).append(", ");
            q.erase(q.end() - 2, q.end());
        }
        return q;
    }
}
