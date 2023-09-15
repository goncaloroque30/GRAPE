// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "UI.h"

#include "Jobs/Job.h"

#pragma warning ( push )
#pragma warning ( disable : GRAPE_VENDOR_WARNINGS )
#include <imgui_internal.h>
#include <nfd.h>
#pragma warning ( pop )

namespace GRAPE::UI {
    namespace {
        struct MinMaxCallback {
            double Min = Constants::NaN;
            double Max = Constants::NaN;
            bool InvalidTextStyle = false;
        };

        // ReSharper disable once CppParameterMayBeConstPtrOrRef (ImGui API requirement)
        int inputDoubleCallback(ImGuiInputTextCallbackData* Data) {
            const auto minMax = static_cast<MinMaxCallback*>(Data->UserData);

            try
            {
                const double val = std::stod(Data->Buf);
                if (!std::isnan(minMax->Min) && val < minMax->Min || !std::isnan(minMax->Max) && val > minMax->Max)
                {
                    pushInvalidTextStyle();
                    minMax->InvalidTextStyle = true;
                }
            }
            catch (const std::logic_error&)
            {
                pushInvalidTextStyle();
                minMax->InvalidTextStyle = true;
            }

            return 0;
        }
    }

    void textCentered(const char* Text) {
        alignForWidth(ImGui::CalcTextSize(Text).x);
        ImGui::TextUnformatted(Text);
    }

    // Unformatted text with info color
    void textInfo(const std::string& Text) {
        ImGui::PushStyleColor(ImGuiCol_Text, g_ExtraColors[GrapeColInfoText]);
        ImGui::TextUnformatted(Text.c_str());
        ImGui::PopStyleColor();
    }

    // Unformatted text with invalid text style
    void textInvalid(const std::string& Text) {
        pushInvalidTextStyle();
        ImGui::TextUnformatted(Text.c_str());
        popInvalidTextStyle();
    }

    // Sets tooltip with text color as GrapeColDelete
    void setTooltipInvalid(const std::string& Text) {
        ImGui::PushStyleColor(ImGuiCol_Text, g_ExtraColors[GrapeColDelete]);
        ImGui::SetTooltip("%s", Text.c_str());
        ImGui::PopStyleColor();
    }

    // Pushes text color to GrapeColDelete and text selection background to GrapeColInvalidInputTextSelectedBg
    void pushInvalidTextStyle() {
        ImGui::PushStyleColor(ImGuiCol_Text, g_ExtraColors[GrapeColDelete]);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, g_ExtraColors[GrapeColInvalidInputTextSelectedBg]);
    }

    // Pops two style colors
    void popInvalidTextStyle() { ImGui::PopStyleColor(2); }

    // Button with text color as GrapeColNew and a preceding + if Icon is true
    bool buttonNew(const std::string& Text, const ImVec2& Size, bool Icon) {
        std::string bText = Icon ? "+ " : "";
        bText.append(Text);

        ImGui::PushStyleColor(ImGuiCol_Text, g_ExtraColors[GrapeColNew]);
        ImGui::PushStyleColor(ImGuiCol_Border, g_ExtraColors[GrapeColNew]);
        const bool clicked = ImGui::Button(bText.c_str(), Size);
        ImGui::PopStyleColor(2);
        return clicked;
    }

    // Button with text color as GrapeColDelete and a preceding X if Icon is true
    bool buttonDelete(const std::string& Text, const ImVec2& Size, bool Icon) {
        std::string bText = Icon ? "X " : "";
        bText.append(Text);

        ImGui::PushStyleColor(ImGuiCol_Text, g_ExtraColors[GrapeColDelete]);
        ImGui::PushStyleColor(ImGuiCol_Border, g_ExtraColors[GrapeColDelete]);
        const bool clicked = ImGui::Button(bText.c_str(), Size);
        ImGui::PopStyleColor(2);

        return clicked;
    }

    // Button with width fit to text size and default text as a Wand
    bool buttonEdit(const std::string& Text) {
        const float width = ImGui::CalcTextSize(Text.c_str()).x + ImGui::GetStyle().ItemInnerSpacing.x * 2.0f;

        ImGui::PushStyleColor(ImGuiCol_Text, g_ExtraColors[GrapeColEdit]);
        ImGui::PushStyleColor(ImGuiCol_Border, g_ExtraColors[GrapeColEdit]);
        const bool clicked = ImGui::Button(Text.c_str(), ImVec2(width, 0));
        ImGui::PopStyleColor(2);

        return clicked;
    }

    // Same as buttonEdit but right aligned
    bool buttonEditRight(const std::string& Text) {
        const float width = ImGui::CalcTextSize(Text.c_str()).x + ImGui::GetStyle().ItemInnerSpacing.x * 2.0f;
        ImGui::SameLine(ImGui::GetContentRegionMax().x - width);

        ImGui::PushStyleColor(ImGuiCol_Text, g_ExtraColors[GrapeColEdit]);
        ImGui::PushStyleColor(ImGuiCol_Border, g_ExtraColors[GrapeColEdit]);
        const bool clicked = ImGui::Button(Text.c_str(), ImVec2(-FLT_MIN, 0));
        ImGui::PopStyleColor(2);

        return clicked;
    }

    // Text is displayed with GrapeColInfo and selectable is set to span all columns
    // Call PopupContextItem after this to create a popup menu for the row
    bool selectableRowInfo(const std::string& Text, bool Selected) {
        ImGui::PushStyleColor(ImGuiCol_Text, g_ExtraColors[GrapeColInfoText]);
        const bool clicked = ImGui::Selectable(Text.c_str(), Selected, ImGuiSelectableFlags_DontClosePopups | ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap);
        ImGui::PopStyleColor();
        return clicked;
    }

    // Creates a selectable row without moving the cursor
    // Call PopupContextItem after this to create a popup menu for the row
    bool selectableRowEmpty(bool Selected) {
        const float cursorPosX = ImGui::GetCursorPosX();
        const bool clicked = ImGui::Selectable("##Empty", Selected, ImGuiSelectableFlags_DontClosePopups | ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap);
        ImGui::SameLine(cursorPosX);
        return clicked;
    }

    // Selectable with text color as GrapeColNew and a preceding + if Icon is true
    bool selectableNew(const std::string& Text, bool Icon, ImGuiSelectableFlags Flags) {
        std::string bText = Icon ? "+ " : "";
        bText.append(Text);

        ImGui::PushStyleColor(ImGuiCol_Text, g_ExtraColors[GrapeColNew]);
        const bool clicked = ImGui::Selectable(bText.c_str(), false, Flags);
        ImGui::PopStyleColor();

        return clicked;
    }

    // Selectable with text color as GrapeColDelete and a preceding X if Icon is true
    bool selectableDelete(const std::string& Text, bool Icon, ImGuiSelectableFlags Flags) {
        std::string bText = Icon ? "X " : "";
        bText.append(Text);

        ImGui::PushStyleColor(ImGuiCol_Text, g_ExtraColors[GrapeColDelete]);
        const bool clicked = ImGui::Selectable(bText.c_str(), false, Flags);
        ImGui::PopStyleColor();

        return clicked;
    }

    // Selectable with Icon preceding Text
    bool selectableWithIcon(const std::string& Text, const char* Icon, bool Enabled, ImGuiSelectableFlags Flags) {
        if (!Enabled)
            ImGui::BeginDisabled();

        Flags |= ImGuiSelectableFlags_AllowItemOverlap;
        const float cursorX = ImGui::GetCursorPosX();
        const bool clicked = ImGui::Selectable(Icon, false, Flags);
        ImGui::SameLine(cursorX + 20.0f, ImGui::GetStyle().ItemInnerSpacing.x);
        ImGui::TextUnformatted(Text.c_str());

        if (!Enabled)
            ImGui::EndDisabled();

        return clicked;
    }

    // Tree node with default flags
    bool treeNode(const std::string& Label, bool Empty) {
        ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
        if (Empty)
            treeNodeFlags |= ImGuiTreeNodeFlags_Leaf;

        return ImGui::TreeNodeEx(Label.c_str(), treeNodeFlags);
    }

    // Tree node with no text
    // If Empty tree node is not shown
    bool treeNodeEmpty(bool Empty) {
        ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
        if (Empty)
            treeNodeFlags |= ImGuiTreeNodeFlags_Leaf;

        const float cursorPosX = ImGui::GetCursorPosX();
        const bool open = ImGui::TreeNodeEx("##EditableNode", treeNodeFlags);
        ImGui::SameLine(cursorPosX + ImGui::GetTreeNodeToLabelSpacing());

        return open;
    }

    bool progressBar(const Job& Jb) {
        bool start = false;
        if (Jb.ready())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, g_ExtraColors[GrapeColNew]);
            if (selectableWithIcon("Start", ICON_FA_PLAY))
                start = true;
        }
        else if (Jb.waiting())
        {
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, g_ExtraColors[GrapeColEdit]);
            ImGui::ProgressBar(0.0f, ImVec2(-FLT_MIN, 0), "Waiting...");
        }
        else if (Jb.running())
        {
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, g_ExtraColors[GrapeColEdit]);
            ImGui::ProgressBar(Jb.progress());
        }
        else if (Jb.stopped())
        {
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, g_ExtraColors[GrapeColDelete]);
            ImGui::ProgressBar(1.0f, ImVec2(-FLT_MIN, 0), "Failed");
        }
        else if (Jb.finished())
        {
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, g_ExtraColors[GrapeColNew]);
            ImGui::ProgressBar(1.0f, ImVec2(-FLT_MIN, 0), "Done");
        }
        else { GRAPE_ASSERT(false); }
        ImGui::PopStyleColor();

        return start;
    }

    // Table height based on frame height and row count
    // Row count incremented if Header
    float getTableHeight(std::size_t RowCount, bool Header, float MaximumHeight) {
        if (Header)
            ++RowCount;

        float height = ImGui::GetFrameHeight() * static_cast<float>(RowCount);

        if (MaximumHeight > 0.0f)
            height = std::min(MaximumHeight, height);

        return height;
    }

    // BeginTable with default flags and style
    bool beginTable(const std::string& Label, int ColumnCount, ImGuiTableFlags Flags, const ImVec2& OuterSize, float InnerWidth) {
        ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY;

        if (Flags != ImGuiTableFlags_None)
            flags = Flags;

        const bool open = ImGui::BeginTable(Label.c_str(), ColumnCount, flags, OuterSize, InnerWidth);
        if (open)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        }
        return open;
    }

    // EndTable and pop default style
    void endTable() {
        ImGui::PopStyleVar(2);
        ImGui::EndTable();
    }

    // If NextItemFill sets the width of the next item to the ContentRegionAvail
    void tableNextColumn(bool NextItemFill) {
        ImGui::TableNextColumn();
        if (NextItemFill)
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    }

    /*
     * Input Text
     */
    bool inputText(const std::string& Label, std::string& InputText, bool Invalid, const std::string& Hint, const std::string& InvalidHelp) {
        bool edited = false;

        if (Invalid)
            pushInvalidTextStyle();

        ImGui::InputTextWithHint(std::format("##{}", Label).c_str(), Hint.c_str(), &InputText, ImGuiInputTextFlags_EnterReturnsTrue);
        if (ImGui::IsItemDeactivatedAfterEdit())
            edited = true;
        if (ImGui::IsItemHovered() && Invalid)
            ImGui::SetTooltip("%s", InvalidHelp.c_str());

        if (Invalid)
            popInvalidTextStyle();

        return edited;
    }

    /*
     * Input Doubles
     * Provides an editable number with optional limits and units
     */
    bool inputDouble(std::string_view TooltipText, double& Value, double Minimum, double Maximum, bool NotNull, std::size_t Decimals, std::string_view Suffix) {
        std::string valStr = std::format("{0:.{1}f}", Value, Decimals);
        valStr.append(" ").append(Suffix);
        bool edited = false;
        MinMaxCallback minMax = { Minimum, Maximum };
        ImGui::InputText(std::format("##{}", TooltipText).c_str(), &valStr, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CallbackAlways, inputDoubleCallback, &minMax);
        if (ImGui::IsItemDeactivatedAfterEdit())
            try
        {
            if (!NotNull && valStr.empty())
            {
                Value = Constants::NaN;
                edited = true;
            }
            else
            {
                const double newVal = std::stod(valStr);
                if (!std::isnan(Minimum) && newVal < Minimum)
                    throw std::out_of_range("");

                if (!std::isnan(Maximum) && newVal > Maximum)
                    throw std::out_of_range("");

                Value = newVal;
                edited = true;
            }
        }
        catch (const std::logic_error&) {}

        if (minMax.InvalidTextStyle)
            popInvalidTextStyle();

        if (minMax.InvalidTextStyle && ImGui::IsItemHovered())
        {
            if (!std::isnan(Minimum) && !std::isnan(Maximum))
                ImGui::SetTooltip("%s", std::format("{0} must be between {1:.{3}f} and {2:.{3}f}", TooltipText, Minimum, Maximum, Decimals).c_str());
            else if (!std::isnan(Minimum))
                ImGui::SetTooltip("%s", std::format("{0} must be higher than {1:.{2}f}", TooltipText, Minimum, Decimals).c_str());
            else if (!std::isnan(Maximum))
                ImGui::SetTooltip("%s", std::format("{0} must be lower than {1:.{2}f}", TooltipText, Maximum, Decimals).c_str());
        }

        return edited;
    }

    bool inputDouble(std::string_view TooltipText, double& Value) { return inputDouble(TooltipText, Value, Constants::NaN, Constants::NaN, true, 6, ""); }

    bool inputDouble(std::string_view TooltipText, double& Value, std::size_t Decimals) { return inputDouble(TooltipText, Value, Constants::NaN, Constants::NaN, true, Decimals, ""); }

    bool inputDouble(std::string_view TooltipText, double& Value, std::string_view Suffix) { return inputDouble(TooltipText, Value, Constants::NaN, Constants::NaN, true, 6, Suffix); }

    bool inputDouble(std::string_view TooltipText, double& Value, std::size_t Decimals, std::string_view Suffix) { return inputDouble(TooltipText, Value, Constants::NaN, Constants::NaN, true, Decimals, Suffix); }

    bool inputDouble(std::string_view TooltipText, double& Value, double Minimum, double Maximum) { return inputDouble(TooltipText, Value, Minimum, Maximum, true, 6, ""); }

    bool inputDouble(std::string_view TooltipText, double& Value, double Minimum, double Maximum, std::size_t Decimals) { return inputDouble(TooltipText, Value, Minimum, Maximum, true, Decimals, ""); }

    bool inputDouble(std::string_view TooltipText, double& Value, double Minimum, double Maximum, std::string_view Suffix) { return inputDouble(TooltipText, Value, Minimum, Maximum, true, 6, Suffix); }

    bool inputDouble(std::string_view TooltipText, double& Value, double Minimum, double Maximum, std::size_t Decimals, std::string_view Suffix) { return inputDouble(TooltipText, Value, Minimum, Maximum, true, Decimals, Suffix); }

    bool inputInt(const std::string& TooltipText, int& Value, int Minimum, int Maximum, const std::string& Suffix) {
        std::string valStr = std::format("{}", Value);
        valStr.append(" ").append(Suffix);
        bool edited = false;
        MinMaxCallback minMax = { static_cast<double>(Minimum), static_cast<double>(Maximum) };
        ImGui::InputText(std::format("##{}", TooltipText).c_str(), &valStr, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CallbackAlways, inputDoubleCallback, &minMax);
        if (ImGui::IsItemDeactivatedAfterEdit())
            try
        {
            const int newVal = std::stoi(valStr);
            if (newVal < Minimum)
                throw std::out_of_range("");

            if (newVal > Maximum)
                throw std::out_of_range("");

            Value = newVal;
            edited = true;
        }
        catch (const std::logic_error&) {}

        if (minMax.InvalidTextStyle)
            popInvalidTextStyle();

        if (minMax.InvalidTextStyle && ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", std::format("{0} must be between {1} and {2}", TooltipText, Minimum, Maximum).c_str());

        return edited;
    }

    bool inputDateTime(const std::string& TooltipText, TimePoint& Time) {
        std::string utcTimeString = timeToUtcString(Time);

        if (inputText(TooltipText, utcTimeString, false, TooltipText))
        {
            const std::optional<TimePoint> timeOpt = utcStringToTime(utcTimeString);
            if (timeOpt)
            {
                Time = timeOpt.value();
                return true;
            }
        }

        return false;
    }

    bool inputTime(const std::string& TooltipText, Duration& Dur) {
        std::string timeString = durationToString(Dur);

        if (inputText(TooltipText, timeString, false, TooltipText))
        {
            const auto durationOpt = stringToDuration(timeString);
            if (durationOpt)
            {
                Dur = durationOpt.value();
                return true;
            }
        }

        return false;
    }

    // Converts Value from the range 0.0 to 1.0 to be displayed
    bool inputPercentage(const std::string& TooltipText, double& Value, double Minimum, double Maximum, std::size_t Decimals, bool Suffix) {
        std::string valStr = std::format("{0:.{1}f}", Value * 100.0, Decimals);
        if (Suffix)
            valStr.append(" %");
        bool edited = false;
        MinMaxCallback minMax = { Minimum * 100.0, Maximum * 100.0 };
        ImGui::InputText(std::format("##{}", TooltipText).c_str(), &valStr, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CallbackAlways, inputDoubleCallback, &minMax);
        if (ImGui::IsItemDeactivatedAfterEdit())
            try
        {
            const double newVal = std::stod(valStr) / 100.0;
            if (newVal < Minimum)
                throw std::out_of_range("");

            if (newVal > Maximum)
                throw std::out_of_range("");

            Value = newVal;
            edited = true;
        }
        catch (const std::logic_error&) {}

        if (minMax.InvalidTextStyle)
            popInvalidTextStyle();

        if (minMax.InvalidTextStyle && ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", std::format("{0} must be between {1:.{3}f} and {2:.{3}f}", TooltipText, Minimum * 100.0, Maximum * 100.0, Decimals).c_str());

        return edited;
    }

    // Extends ImGui::IsItemClicked to custom hovered flags (ex. allow when disabled)
    // Default allows click when disabled
    bool isItemClicked(ImGuiHoveredFlags HoveredFlags, ImGuiMouseButton MouseButton) { return ImGui::IsItemHovered(HoveredFlags) && ImGui::IsMouseClicked(MouseButton); }

    // Spinner
    bool spinner(const std::string& Label, float Radius, float Thickness) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        const ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(Label.c_str());

        const ImVec2 pos = window->DC.CursorPos;
        const ImVec2 size(Radius * 2, (Radius + style.FramePadding.y) * 2);

        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImGui::ItemSize(bb, style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, id))
            return false;

        // Render
        window->DrawList->PathClear();

        constexpr int numSegments = 30;
        const int start = static_cast<int>(abs(ImSin(static_cast<float>(g.Time) * 1.8f) * (numSegments - 5)));

        const float aMin = IM_PI * 2.0f * static_cast<float>(start) / static_cast<float>(numSegments);
        constexpr float aMax = IM_PI * 2.0f * (static_cast<float>(numSegments) - 3.0f) / static_cast<float>(numSegments);

        const auto centre = ImVec2(pos.x + Radius, pos.y + Radius + style.FramePadding.y);

        for (int i = 0; i < numSegments; i++)
        {
            const float a = aMin + static_cast<float>(i) / static_cast<float>(numSegments) * (aMax - aMin);
            window->DrawList->PathLineTo(ImVec2(centre.x + ImCos(a + static_cast<float>(g.Time) * 8.0f) * Radius,
                centre.y + ImSin(a + static_cast<float>(g.Time) * 8.0f) * Radius));
        }

        window->DrawList->PathStroke(ImGui::GetColorU32(ImGuiCol_Header), false, Thickness);
        return true;
    }

    void alignForWidth(float Width, float Alignment) {
        const float avail = ImGui::GetContentRegionAvail().x;
        const float off = (avail - Width) * Alignment;
        if (off > 0.0f)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
    }

    // Select folder dialog
    std::pair<std::string, bool> pickFolder() {
        nfdchar_t* outPath;
        const nfdresult_t res = NFD_PickFolder(&outPath, nullptr);
        if (res == NFD_OKAY)
        {
            std::string path(outPath);
            NFD_FreePath(outPath);
            return { path, true };
        }
        return { "", false };
    }

    std::pair<std::string, bool> openFile(const char* FileTypeName, const char* FileType) {
        nfdchar_t* outPath;
        const nfdfilteritem_t filter[1] = { { FileTypeName, FileType } };
        const nfdresult_t res = NFD_OpenDialog(&outPath, filter, 1, nullptr);
        if (res == NFD_OKAY)
        {
            std::string path(outPath);
            NFD_FreePath(outPath);
            return { path, true };
        }

        return { "", false };
    }

    std::pair<std::string, bool> saveFile(const char* FileTypeName, const char* FileType, const char* DefaultName) {
        nfdchar_t* outPath;
        const nfdfilteritem_t filter[1] = { { FileTypeName, FileType } };
        const nfdresult_t res = NFD_SaveDialog(&outPath, filter, 1, nullptr, DefaultName);
        if (res == NFD_OKAY)
        {
            std::string path(outPath);
            NFD_FreePath(outPath);
            return { path, true };
        }

        return { "", false };
    }

    std::pair<std::string, bool> openGrapeFile() {
        return openFile("GRAPE file", "grp");
    }

    std::pair<std::string, bool> openCsvFile() {
        return openFile("CSV file", "csv");
    }

    std::pair<std::string, bool> saveGrapeFile(const char* DefaultName) {
        return saveFile("GRAPE file", "grp", DefaultName);
    }

    std::pair<std::string, bool> saveCsvFile(const char* DefaultName) {
        return saveFile("CSV file", "csv", DefaultName);
    }

    std::pair<std::string, bool> saveGpkgFile(const char* DefaultName) {
        return saveFile("GeoPackage file", "gpkg", DefaultName);
    }

    bool TextFilter::draw(const std::string& Hint, float Width) {
        if (Width != 0.0f)
            ImGui::SetNextItemWidth(Width);
        const bool changed = ImGui::InputTextWithHint("##Search", Hint.c_str(), m_Filter.InputBuf, IM_ARRAYSIZE(m_Filter.InputBuf));
        if (changed)
            m_Filter.Build();
        return changed;
    }
}
