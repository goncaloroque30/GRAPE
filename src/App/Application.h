// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "AsyncTask.h"
#include "Image.h"
#include "Modals/Modal.h"
#include "Panels/LogPanel.h"
#include "Panels/Panel.h"
#include "Settings.h"
#include "Study/Study.h"

#pragma warning ( push )
#pragma warning ( disable : GRAPE_VENDOR_WARNINGS )
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>
#include <vulkan/vulkan.h>
#pragma warning ( pop )

struct GLFWwindow;

namespace GRAPE {
    void checkVkResult(VkResult Err);

    class CommandLineArgs {
    public:
        CommandLineArgs() = default;
        CommandLineArgs(int ArgCount, char** Args) : m_Args(Args, Args + ArgCount) {}

        [[nodiscard]] int argCount() const { return m_Args.size(); }
        [[nodiscard]] int argIndex(const std::string& Arg) const { return static_cast<int>(std::distance(m_Args.begin(), std::ranges::find(m_Args, Arg))); }
        [[nodiscard]] auto arg(int Index) const { return Index < static_cast<int>(m_Args.size()) ? m_Args.at(Index) : ""; }
        [[nodiscard]] bool argPassed(const std::string& Arg) const { return std::ranges::find(m_Args, Arg) != m_Args.end(); }

        void deleteArg(int Index) { GRAPE_ASSERT(Index < m_Args.size()); m_Args.erase(std::next(m_Args.begin(), Index)); }
        void deleteArg(const std::string& Arg) { m_Args.erase(std::ranges::find(m_Args, Arg)); }

        [[nodiscard]] bool isControlArg(int Index) const { const std::string argStr = arg(Index); return argStr.empty() ? false : argStr.at(0) == '-'; }
        [[nodiscard]] bool isValueArg(int Index) const { const std::string argStr = arg(Index); return argStr.empty() ? false : argStr.at(0) != '-'; }
    private:
        std::vector<std::string> m_Args{};
    };

    class Application {
    public:
        explicit Application(CommandLineArgs ClArgs = CommandLineArgs());
        ~Application();

        static Application& get() { return *s_Instance; }
        static Study& study() { return *get().m_Study; }
        static Settings& settings() { return get().m_Settings; }

        void run();

        template<typename Function>
        void queueAsyncTask(Function&& Func, const std::string& Message) {
            m_AsyncTask.pushTask(Func, Message);
        }

        void panelStackReset() const;
        void panelStackOnPerformanceRunStart() const;
        void panelStackOnNoiseRunStart() const;

        [[nodiscard]] bool validStudy() const;

        [[nodiscard]] const Image& icon() const;
        [[nodiscard]] const auto& panelStack() const { return m_PanelStack; }

        // GLFW
        [[nodiscard]] GLFWwindow* glfwWindow() const { return m_Window; }

        // Vulkan
        static VkInstance vkInstance();
        static VkPhysicalDevice vkPhysicalDevice();
        static VkDevice vkDevice();
        static VkQueue vkQueue();
        static ImGui_ImplVulkanH_Window* vkMainWindowData();
    private:
        CommandLineArgs m_CommandLineArgs;
        bool m_RunApplication = true;

        GLFWwindow* m_Window = nullptr;

        // Async Task
        AsyncTask m_AsyncTask;

        // Study
        std::shared_ptr<Study> m_Study;

        // Panels
        std::vector<std::unique_ptr<Panel>> m_PanelStack = {}; // Active for Study
        LogPanel m_LogPanel; // Always visible

        // Modals
        std::vector<std::unique_ptr<Modal>> m_ModalsStack = {};

        // Icon
        std::unique_ptr<Image> m_GrapeIcon = nullptr;

        // Settings
        Settings m_Settings;
        std::string m_SettingsPath;

        // Static instance
        static Application* s_Instance;

#ifdef GRAPE_DEBUG
        bool m_ShowImGuiDemo = false;
#endif
    private:
        // Main Loop
        void updateWindow();

        // Draw
        void mainMenuBar();
        void asyncTaskWindow() const;
        void panelStackDraw() const;

        // Study
        void newStudy();
        void openStudy();
        void setStudy(const std::shared_ptr<Study>& StudyIn);

        // Inits
        void parseCommandLineArgs();
        void initDefineHandler();
        void initStyle() const;
    };
}
