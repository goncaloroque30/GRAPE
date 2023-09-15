// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Units.h"

#pragma warning ( push )
#pragma warning ( disable : GRAPE_VENDOR_WARNINGS )
#include <imgui.h>
#include <imgui_stdlib.h>
#pragma warning ( pop )

#include <IconsFontAwesome6.h>

namespace GRAPE {
    class Job;
    namespace UI {

        // Constants
        constexpr float g_StandardItemWidth = 150.0f;
        constexpr float g_StandardLeftAlignFraction = 5.0f / 6.0f;

        // Extra colors
        enum ExtraColors : int {
            GrapeColInfoText = 0,
            GrapeColDelete,
            GrapeColNew,
            GrapeColEdit,
            GrapeColInvalidInputTextSelectedBg,
        };
        inline std::array<ImVec4, magic_enum::enum_count<ExtraColors>()> g_ExtraColors{}; // Set in application

        // Text & Tooltips
        void textCentered(const char* Text);
        void textInfo(const std::string& Text);
        void textInvalid(const std::string& Text);
        void setTooltipInvalid(const std::string& Text);
        void pushInvalidTextStyle();
        void popInvalidTextStyle();

        // Buttons
        bool buttonNew(const std::string& Text = "New", const ImVec2& Size = ImVec2(0, 0), bool Icon = true);
        bool buttonDelete(const std::string& Text = "Delete", const ImVec2& Size = ImVec2(0, 0), bool Icon = true);
        bool buttonEdit(const std::string& Text = ICON_FA_WAND_MAGIC);
        bool buttonEditRight(const std::string& Text = ICON_FA_WAND_MAGIC);

        // Selectables
        bool selectableRowInfo(const std::string& Text, bool Selected = false);
        bool selectableRowEmpty(bool Selected = false);
        bool selectableNew(const std::string& Text = "New", bool Icon = true, ImGuiSelectableFlags Flags = 0);
        bool selectableDelete(const std::string& Text = "Delete", bool Icon = true, ImGuiSelectableFlags Flags = 0);
        bool selectableWithIcon(const std::string& Text, const char* Icon, bool Enabled = true, ImGuiSelectableFlags Flags = 0);

        // Tree Nodes
        bool treeNode(const std::string& Label, bool Empty);
        bool treeNodeEmpty(bool Empty);

        // Progress Bar
        bool progressBar(const Job& Jb);

        // Tables
        float getTableHeight(std::size_t RowCount, bool Header = true, float MaximumHeight = 0.0f);
        bool beginTable(const std::string& Label, int ColumnCount, ImGuiTableFlags Flags = ImGuiTableFlags_None, const ImVec2& OuterSize = ImVec2(0.0f, 0.0f), float InnerWidth = g_StandardItemWidth);
        void endTable();
        void tableNextColumn(bool NextItemFill = true);

        // Inputs
        bool inputText(const std::string& Label, std::string& InputText, bool Invalid, const std::string& Hint = "", const std::string& InvalidHelp = "");

        bool inputDouble(std::string_view TooltipText, double& Value, double Minimum, double Maximum, bool NotNull, std::size_t Decimals, std::string_view Suffix);
        bool inputDouble(std::string_view TooltipText, double& Value);
        bool inputDouble(std::string_view TooltipText, double& Value, std::size_t Decimals);
        bool inputDouble(std::string_view TooltipText, double& Value, std::string_view Suffix);
        bool inputDouble(std::string_view TooltipText, double& Value, std::size_t Decimals, std::string_view Suffix);

        bool inputDouble(std::string_view TooltipText, double& Value, double Minimum, double Maximum);
        bool inputDouble(std::string_view TooltipText, double& Value, double Minimum, double Maximum, std::size_t Decimals);
        bool inputDouble(std::string_view TooltipText, double& Value, double Minimum, double Maximum, std::string_view Suffix);
        bool inputDouble(std::string_view TooltipText, double& Value, double Minimum, double Maximum, std::size_t Decimals, std::string_view Suffix);

        template<typename Enum>
        bool inputDouble(std::string_view TooltipText, double& Value, const Unit<Enum>& Un, bool Suffix = true, bool NotNull = true) {
            double convertedVal = Un.fromSi(Value);
            if (inputDouble(TooltipText, convertedVal, Constants::NaN, Constants::NaN, NotNull, Un.decimals(), Suffix ? Un.shortName() : ""))
            {
                Value = Un.toSi(convertedVal);
                return true;
            }

            return false;
        }
        template<typename Enum>
        bool inputDoubleDelta(std::string_view TooltipText, double& Value, const Unit<Enum>& Un, bool Suffix = true, bool NotNull = true) {
            double convertedVal = Un.fromSiDelta(Value);
            if (inputDouble(TooltipText, convertedVal, Constants::NaN, Constants::NaN, NotNull, Un.decimals(), Suffix ? Un.shortName() : ""))
            {
                Value = Un.toSiDelta(convertedVal);
                return true;
            }

            return false;
        }
        template<typename Enum>
        bool inputDouble(std::string_view TooltipText, double& Value, double Minimum, double Maximum, const Unit<Enum>& Un, bool Suffix = true, bool NotNull = true) {
            double convertedVal = Un.fromSi(Value);
            if (inputDouble(TooltipText, convertedVal, Un.fromSi(Minimum), Un.fromSi(Maximum), NotNull, Un.decimals(), Suffix ? Un.shortName() : ""))
            {
                Value = Un.toSi(convertedVal);
                return true;
            }

            return false;
        }
        template<typename Enum>
        bool inputDoubleDelta(std::string_view TooltipText, double& Value, double Minimum, double Maximum, const Unit<Enum>& Un, bool Suffix = true, bool NotNull = true) {
            double convertedVal = Un.fromSiDelta(Value);
            if (inputDouble(TooltipText, convertedVal, Un.fromSiDelta(Minimum), Un.fromSiDelta(Maximum), NotNull, Un.decimals(), Suffix ? Un.shortName() : ""))
            {
                Value = Un.toSiDelta(convertedVal);
                return true;
            }

            return false;
        }

        bool inputPercentage(const std::string& TooltipText, double& Value, double Minimum = 0.0, double Maximum = 1.0, std::size_t Decimals = 0, bool Suffix = true);
        bool inputInt(const std::string& TooltipText, int& Value, int Minimum = std::numeric_limits<int>::min(), int Maximum = std::numeric_limits<int>::max(), const std::string& Suffix = "");

        bool inputDateTime(const std::string& TooltipText, TimePoint& Time);
        bool inputTime(const std::string& TooltipText, Duration& Dur);

        // Status Checks
        bool isItemClicked(ImGuiHoveredFlags HoveredFlags = ImGuiHoveredFlags_AllowWhenDisabled, ImGuiMouseButton MouseButton = ImGuiMouseButton_Left);

        // File Dialogs
        std::pair<std::string, bool> pickFolder();
        std::pair<std::string, bool> openFile(const char* FileTypeName, const char* FileType);
        std::pair<std::string, bool> saveFile(const char* FileTypeName, const char* FileType, const char* DefaultName = "File");
        std::pair<std::string, bool> openGrapeFile();
        std::pair<std::string, bool> openCsvFile();
        std::pair<std::string, bool> saveGrapeFile(const char* DefaultName = "Grape Study");
        std::pair<std::string, bool> saveCsvFile(const char* DefaultName = "CSV File");
        std::pair<std::string, bool> saveGpkgFile(const char* DefaultName = "GeoPackage File");

        // Misc
        bool spinner(const std::string& Label, float Radius = 15.0f, float Thickness = 6.0f);
        void alignForWidth(float Width, float Alignment = 0.5f);

        // Text Filter
        struct TextFilter {
            explicit TextFilter(const std::string& DefaultFilter = "") : m_Filter(DefaultFilter.c_str()) {}
            bool draw(const std::string& Hint = "Search...", float Width = 100.0f);
            [[nodiscard]] bool passesFilter(const std::string& Value) const { return m_Filter.PassFilter(Value.c_str()); }
        private:
            ImGuiTextFilter m_Filter;
        };
    }
}
