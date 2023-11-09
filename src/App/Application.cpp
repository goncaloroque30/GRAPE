// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Application.h"

#include "IO/CsvExport.h"
#include "IO/CsvImport.h"
#include "IO/AnpImport.h"
#include "IO/GpkgExport.h"
#include "Modals/AboutModal.h"
#include "Modals/SettingsModal.h"
#include "Panels/AirportsPanel.h"
#include "Panels/Doc29Panel.h"
#include "Panels/FleetPanel.h"
#include "Panels/FlightsPanel.h"
#include "Panels/LTOPanel.h"
#include "Panels/ScenariosPanel.h"
#include "Panels/SFIPanel.h"
#include "Panels/Tracks4dPanel.h"

#pragma warning ( push )
#pragma warning ( disable : GRAPE_VENDOR_WARNINGS )
#include <backends/imgui_impl_glfw.h>
#include "imgui_internal.h"
#include <nfd.h>
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#pragma warning ( pop )

// Embedded font
#include "Embed/FaSolid.embed"
#include "Embed/RobotoMedium.embed"

// Embedded Images
#include "Embed/GrapeIcon16.embed"
#include "Embed/GrapeIcon256.embed"


namespace GRAPE {
    void checkVkResult(const VkResult Err) {
        if (Err == 0)
            return;

        Log::core()->error("Vulkan error. VkResult = {}.", Err);

        if (Err < 0)
            abort();
    }

    namespace {
        /***********
        * Vulkan
        ***********/

        VkAllocationCallbacks* g_Allocator = nullptr;
        VkInstance             g_Instance = VK_NULL_HANDLE;
        VkPhysicalDevice       g_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice               g_Device = VK_NULL_HANDLE;
        uint32_t               g_QueueFamily = static_cast<uint32_t>(-1);
        VkQueue                g_Queue = VK_NULL_HANDLE;
        VkPipelineCache        g_PipelineCache = VK_NULL_HANDLE;
        VkDescriptorPool       g_DescriptorPool = VK_NULL_HANDLE;

        ImGui_ImplVulkanH_Window g_MainWindowData;
        int                      g_MinImageCount = 2;
        bool                     g_SwapChainRebuild = false;

        void setupVulkan(const char** Extensions, uint32_t ExtensionsCount) {
            // Create Vulkan Instance
            {
                VkApplicationInfo appInfo = {};
                appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
                appInfo.pApplicationName = "GRAPE";
                appInfo.applicationVersion = GRAPE_VERSION_NUMBER;
                appInfo.apiVersion = VK_API_VERSION_1_0;

                VkInstanceCreateInfo createInfo = {};
                createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
                createInfo.pApplicationInfo = &appInfo;
                createInfo.enabledExtensionCount = ExtensionsCount;
                createInfo.ppEnabledExtensionNames = Extensions;

#ifdef GRAPE_DEBUG
                const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
                createInfo.enabledLayerCount = static_cast<std::uint32_t>(validationLayers.size());
                createInfo.ppEnabledLayerNames = validationLayers.data();
#endif
                const VkResult err = vkCreateInstance(&createInfo, g_Allocator, &g_Instance);
                checkVkResult(err);
            }

            // Select GPU
            {
                uint32_t gpuCount;
                VkResult err = vkEnumeratePhysicalDevices(g_Instance, &gpuCount, nullptr);
                checkVkResult(err);
                IM_ASSERT(gpuCount > 0);

                const auto gpus = static_cast<VkPhysicalDevice*>(malloc(sizeof(VkPhysicalDevice) * gpuCount));
                err = vkEnumeratePhysicalDevices(g_Instance, &gpuCount, gpus);
                checkVkResult(err);

                // If a number >1 of GPUs got reported, find discrete GPU if present, or use first one available. This covers most common cases (multi-gpu/integrated+dedicated graphics).
                int useGpu = 0;
                for (int i = 0; i < static_cast<int>(gpuCount); i++)
                {
                    VkPhysicalDeviceProperties properties;
                    vkGetPhysicalDeviceProperties(gpus[i], &properties);
                    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                    {
                        useGpu = i;
                        break;
                    }
                }

                g_PhysicalDevice = gpus[useGpu];
                free(gpus);
            }

            // Select graphics queue family
            {
                uint32_t count;
                vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &count, nullptr);
                const auto queues = static_cast<VkQueueFamilyProperties*>(malloc(sizeof(VkQueueFamilyProperties) * count));
                vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &count, queues);
                for (uint32_t i = 0; i < count; i++)
                {
                    if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    {
                        g_QueueFamily = i;
                        break;
                    }
                }
                free(queues);
                IM_ASSERT(g_QueueFamily != static_cast<uint32_t>(-1));
            }

            // Create Logical Device (with 1 queue)
            {
                constexpr int deviceExtensionCount = 1;
                const char* deviceExtensions[] = { "VK_KHR_swapchain" };
                constexpr float queuePriority[] = { 1.0f };
                VkDeviceQueueCreateInfo queueInfo[1] = {};
                queueInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueInfo[0].queueFamilyIndex = g_QueueFamily;
                queueInfo[0].queueCount = 1;
                queueInfo[0].pQueuePriorities = queuePriority;
                VkDeviceCreateInfo createInfo = {};
                createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
                createInfo.queueCreateInfoCount = std::size(queueInfo);
                createInfo.pQueueCreateInfos = queueInfo;
                createInfo.enabledExtensionCount = deviceExtensionCount;
                createInfo.ppEnabledExtensionNames = deviceExtensions;
                const VkResult err = vkCreateDevice(g_PhysicalDevice, &createInfo, g_Allocator, &g_Device);
                checkVkResult(err);
                vkGetDeviceQueue(g_Device, g_QueueFamily, 0, &g_Queue);
            }

            // Create Descriptor Pool
            {
                constexpr VkDescriptorPoolSize poolSizes[] =
                {
                    { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
                };
                VkDescriptorPoolCreateInfo poolInfo = {};
                poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
                poolInfo.maxSets = 1000 * IM_ARRAYSIZE(poolSizes);
                poolInfo.poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(poolSizes));
                poolInfo.pPoolSizes = poolSizes;
                const VkResult err = vkCreateDescriptorPool(g_Device, &poolInfo, g_Allocator, &g_DescriptorPool);
                checkVkResult(err);
            }
        }

        void setupVulkanWindow(VkSurfaceKHR Surface, int Width, int Height) {
            g_MainWindowData.Surface = Surface;

            // Check for WSI support
            VkBool32 res;
            vkGetPhysicalDeviceSurfaceSupportKHR(g_PhysicalDevice, g_QueueFamily, g_MainWindowData.Surface, &res);
            if (res != VK_TRUE)
            {
                Log::core()->error("Vulkan: no WSI support on physical device 0");
                exit(-1);  // NOLINT(concurrency-mt-unsafe)
            }

            // Select Surface Format
            constexpr VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
            constexpr VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
            g_MainWindowData.SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(g_PhysicalDevice, g_MainWindowData.Surface, requestSurfaceImageFormat, static_cast<size_t>(IM_ARRAYSIZE(requestSurfaceImageFormat)), requestSurfaceColorSpace);

            constexpr VkPresentModeKHR presentModes[] = { VK_PRESENT_MODE_FIFO_KHR };
            g_MainWindowData.PresentMode = ImGui_ImplVulkanH_SelectPresentMode(g_PhysicalDevice, g_MainWindowData.Surface, &presentModes[0], IM_ARRAYSIZE(presentModes));

            // Create SwapChain, RenderPass, Framebuffer, etc.
            IM_ASSERT(g_MinImageCount >= 2);
            ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, &g_MainWindowData, g_QueueFamily, g_Allocator, Width, Height, g_MinImageCount);
        }

        void cleanupVulkan() {
            vkDestroyDescriptorPool(g_Device, g_DescriptorPool, g_Allocator);
            vkDestroyDevice(g_Device, g_Allocator);
            vkDestroyInstance(g_Instance, g_Allocator);
        }

        void cleanupVulkanWindow() {
            ImGui_ImplVulkanH_DestroyWindow(g_Instance, g_Device, &g_MainWindowData, g_Allocator);
        }

        void frameRender(ImDrawData* DrawData) {
            const VkSemaphore imageAcquiredSemaphore = g_MainWindowData.FrameSemaphores[g_MainWindowData.SemaphoreIndex].ImageAcquiredSemaphore;
            const VkSemaphore renderCompleteSemaphore = g_MainWindowData.FrameSemaphores[g_MainWindowData.SemaphoreIndex].RenderCompleteSemaphore;

            VkResult err = vkAcquireNextImageKHR(g_Device, g_MainWindowData.Swapchain, UINT64_MAX, imageAcquiredSemaphore, VK_NULL_HANDLE, &g_MainWindowData.FrameIndex);
            if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
            {
                g_SwapChainRebuild = true;
                return;
            }
            checkVkResult(err);

            const ImGui_ImplVulkanH_Frame* frame = &g_MainWindowData.Frames[g_MainWindowData.FrameIndex];
            {
                err = vkWaitForFences(g_Device, 1, &frame->Fence, VK_TRUE, UINT64_MAX);    // wait indefinitely instead of periodically checking
                checkVkResult(err);

                err = vkResetFences(g_Device, 1, &frame->Fence);
                checkVkResult(err);
            }

            {
                err = vkResetCommandPool(g_Device, frame->CommandPool, 0);
                checkVkResult(err);
                VkCommandBufferBeginInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                err = vkBeginCommandBuffer(frame->CommandBuffer, &info);
                checkVkResult(err);
            }
            {
                VkRenderPassBeginInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                info.renderPass = g_MainWindowData.RenderPass;
                info.framebuffer = frame->Framebuffer;
                info.renderArea.extent.width = g_MainWindowData.Width;
                info.renderArea.extent.height = g_MainWindowData.Height;
                info.clearValueCount = 1;
                info.pClearValues = &g_MainWindowData.ClearValue;
                vkCmdBeginRenderPass(frame->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
            }

            // Record dear imgui primitives into command buffer
            ImGui_ImplVulkan_RenderDrawData(DrawData, frame->CommandBuffer);

            // Submit command buffer
            vkCmdEndRenderPass(frame->CommandBuffer);
            {
                constexpr VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                VkSubmitInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                info.waitSemaphoreCount = 1;
                info.pWaitSemaphores = &imageAcquiredSemaphore;
                info.pWaitDstStageMask = &waitStage;
                info.commandBufferCount = 1;
                info.pCommandBuffers = &frame->CommandBuffer;
                info.signalSemaphoreCount = 1;
                info.pSignalSemaphores = &renderCompleteSemaphore;

                err = vkEndCommandBuffer(frame->CommandBuffer);
                checkVkResult(err);
                err = vkQueueSubmit(g_Queue, 1, &info, frame->Fence);
                checkVkResult(err);
            }
        }

        void framePresent() {
            if (g_SwapChainRebuild)
                return;
            const VkSemaphore renderCompleteSemaphore = g_MainWindowData.FrameSemaphores[g_MainWindowData.SemaphoreIndex].RenderCompleteSemaphore;
            VkPresentInfoKHR info = {};
            info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            info.waitSemaphoreCount = 1;
            info.pWaitSemaphores = &renderCompleteSemaphore;
            info.swapchainCount = 1;
            info.pSwapchains = &g_MainWindowData.Swapchain;
            info.pImageIndices = &g_MainWindowData.FrameIndex;
            const VkResult err = vkQueuePresentKHR(g_Queue, &info);
            if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
            {
                g_SwapChainRebuild = true;
                return;
            }
            checkVkResult(err);
            g_MainWindowData.SemaphoreIndex = (g_MainWindowData.SemaphoreIndex + 1) % g_MainWindowData.ImageCount; // Now we can use the next set of semaphores
        }

        /***********
        * GLFW
        ***********/
        void glfwErrorCallback(int Error, const char* Description) {
            Log::core()->error("Glfw Error {0}:{1}\n", Error, Description);
        }

        constexpr std::string_view CommandLineIncorrectUsage = "Incorrect command line argument use. Run 'Grape.exe -h' for help.";
        constexpr ImWchar g_IconsRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    }

    Application* Application::s_Instance = nullptr; // Set in Application constructor

    Application::Application(CommandLineArgs ClArgs) : m_CommandLineArgs(std::move(ClArgs)) {
        Log::init();
        m_Study = std::make_shared<Study>();
        m_SettingsPath = getResolvedPath("grape.ini").string();
        s_Instance = this;

        // Parse Command Line Arguments
        parseCommandLineArgs();

        if (!m_RunApplication)
        {
            m_Study->Jobs.waitForJobs();
            return;
        }

        // Setup GLFW
        glfwSetErrorCallback(glfwErrorCallback);
        if (!glfwInit())
            return;

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_Window = glfwCreateWindow(1280, 720, "GRAPE", nullptr, nullptr);

        GLFWimage icon[1];
        icon[0].pixels = stbi_load_from_memory(g_GrapeIcon16, sizeof g_GrapeIcon16, &icon[0].width, &icon[0].height, nullptr, 4);
        if (icon[0].pixels)
            glfwSetWindowIcon(m_Window, 1, icon);
        stbi_image_free(icon[0].pixels);

        // Setup Vulkan
        if (!glfwVulkanSupported())
        {
            Log::core()->error("GLFW: Vulkan not supported.");
            return;
        }
        uint32_t extensionsCount = 0;
        const char** extensions = glfwGetRequiredInstanceExtensions(&extensionsCount);
        setupVulkan(extensions, extensionsCount);

        VkSurfaceKHR surface;
        const VkResult err = glfwCreateWindowSurface(g_Instance, m_Window, g_Allocator, &surface);
        checkVkResult(err);

        int w, h;
        glfwGetFramebufferSize(m_Window, &w, &h);
        setupVulkanWindow(surface, w, h);

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        // Setup IO
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = m_SettingsPath.c_str();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable multiple viewports

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForVulkan(m_Window, true);
        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance = g_Instance;
        initInfo.PhysicalDevice = g_PhysicalDevice;
        initInfo.Device = g_Device;
        initInfo.QueueFamily = g_QueueFamily;
        initInfo.Queue = g_Queue;
        initInfo.PipelineCache = g_PipelineCache;
        initInfo.DescriptorPool = g_DescriptorPool;
        initInfo.Subpass = 0;
        initInfo.MinImageCount = g_MinImageCount;
        initInfo.ImageCount = g_MainWindowData.ImageCount;
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo.Allocator = g_Allocator;
        initInfo.CheckVkResultFn = checkVkResult;
        ImGui_ImplVulkan_Init(&initInfo, g_MainWindowData.RenderPass);

        // Init
        initStyle();
        NFD_Init();

        m_PanelStack.emplace_back(std::make_unique<Doc29Panel>());
        m_PanelStack.emplace_back(std::make_unique<SFIPanel>());
        m_PanelStack.emplace_back(std::make_unique<LTOPanel>());
        m_PanelStack.emplace_back(std::make_unique<FleetPanel>());
        m_PanelStack.emplace_back(std::make_unique<AirportsPanel>());
        m_PanelStack.emplace_back(std::make_unique<FlightsPanel>());
        m_PanelStack.emplace_back(std::make_unique<Tracks4dPanel>());
        m_PanelStack.emplace_back(std::make_unique<ScenariosPanel>());

        m_ModalsStack.emplace_back(std::make_unique<SettingsModal>());
        m_ModalsStack.emplace_back(std::make_unique<AboutModal>());

        m_GrapeIcon = std::make_unique<Image>((void*)g_GrapeIcon256, sizeof g_GrapeIcon256);

        // Setup and load .ini file
        initDefineHandler();
        m_Settings.initDefineHandler();
        ImGui::LoadIniSettingsFromDisk(io.IniFilename);

        setStudy(m_Study);
    }

    Application::~Application() {
        if (!m_RunApplication)
            return;

        // Cleanup
        panelStackReset();
        NFD_Quit();

        // Cleanup
        const VkResult err = vkDeviceWaitIdle(g_Device);
        checkVkResult(err);

        m_GrapeIcon.reset(); // Calls destructor

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        cleanupVulkanWindow();
        cleanupVulkan();

        glfwDestroyWindow(m_Window);
        glfwTerminate();
    }

    void Application::run() {
        if (!m_RunApplication)
            return;

        // Main loop
        while (!glfwWindowShouldClose(m_Window))
        {
            // Poll and handle events (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            glfwPollEvents();

            // Resize swap chain?
            if (g_SwapChainRebuild)
            {
                int width, height;
                glfwGetFramebufferSize(m_Window, &width, &height);
                if (width > 0 && height > 0)
                {
                    ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
                    ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, &g_MainWindowData, g_QueueFamily, g_Allocator, width, height, g_MinImageCount);
                    g_MainWindowData.FrameIndex = 0;
                    g_SwapChainRebuild = false;
                }
            }

            // Set Window Title
            glfwSetWindowTitle(m_Window, m_Study->valid() ? m_Study->name().c_str() : "GRAPE");

            // Start the Dear ImGui frame
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            updateWindow();

            // Rendering
            ImGui::Render();
            ImDrawData* drawData = ImGui::GetDrawData();
            const bool isMinimized = drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f;
            if (!isMinimized)
                frameRender(drawData);

            // Update and Render additional Platform Windows
            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }

            // Present Main Platform Window
            if (!isMinimized)
                framePresent();
        }
    }

    VkInstance Application::vkInstance() {
        return g_Instance;
    }

    VkPhysicalDevice Application::vkPhysicalDevice() {
        return g_PhysicalDevice;
    }

    VkDevice Application::vkDevice() {
        return g_Device;
    }

    VkQueue Application::vkQueue() {
        return g_Queue;
    }

    ImGui_ImplVulkanH_Window* Application::vkMainWindowData() {
        return &g_MainWindowData;
    }

    void Application::updateWindow() {
        // Update Popup Modals ID
        for (const auto& modal : m_ModalsStack)
            modal->updateImGuiId();

        mainMenuBar();

        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        // Modals
        for (const auto& modal : m_ModalsStack)
            modal->imGuiDraw();

        // Async Task or Study Panels
        if (m_AsyncTask.running())
            asyncTaskWindow();
        else
            panelStackDraw();

        // Always available
        m_LogPanel.imGuiDraw();

#ifdef GRAPE_DEBUG
        if (m_ShowImGuiDemo)
            ImGui::ShowDemoWindow(&m_ShowImGuiDemo);
#endif // GRAPE_DEBUG
    }

    namespace {
        enum class CsvDataset {
            None = 0,
            Doc29Aircraft,
            Doc29AerodynamicCoefficients,
            Doc29ThrustRatings,
            Doc29ThrustRatingsPropeller,
            Doc29ProfilesPoints,
            Doc29ProfilesProceduralArrival,
            Doc29ProfilesProceduralDeparture,
            Doc29Noise,
            Doc29NoiseNpd,
            Doc29NoiseSpectrum,
            LTO,
            SFI,
            Fleet,
        };

        enum class CsvInputData {
            None = 0,
            Airports,
            Runways,
            RoutesSimple,
            RoutesVectors,
            RoutesRnp,
            Flights,
            Tracks4d,
            Tracks4dPoints,
            Scenarios,
            ScenariosOperations,
            PerformanceRuns,
            PerformanceRunsAtmospheres,
            NoiseRuns,
            ReceptorsGrid,
            ReceptorsPoints,
            NoiseRunsCumulativeMetrics,
            NoiseRunsCumulativeMetricsWeights,
            EmissionsRuns,
        };

        CsvDataset drawCsvDatasetTree() {
            auto ret = CsvDataset::None;
            if (ImGui::BeginMenu("Doc29 Performance"))
            {
                if (ImGui::MenuItem("Aerodynamic Coefficients"))
                    ret = CsvDataset::Doc29AerodynamicCoefficients;

                if (ImGui::MenuItem("Thrust Ratings"))
                    ret = CsvDataset::Doc29ThrustRatings;

                if (ImGui::MenuItem("Thrust Ratings Propeller"))
                    ret = CsvDataset::Doc29ThrustRatingsPropeller;

                if (ImGui::BeginMenu("Profiles"))
                {
                    if (ImGui::MenuItem("Points"))
                        ret = CsvDataset::Doc29ProfilesPoints;

                    if (ImGui::MenuItem("Procedural Arrival"))
                        ret = CsvDataset::Doc29ProfilesProceduralArrival;

                    if (ImGui::MenuItem("Procedural Departure"))
                        ret = CsvDataset::Doc29ProfilesProceduralDeparture;

                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if (ImGui::IsItemClicked())
                ret = CsvDataset::Doc29Aircraft;

            if (ImGui::BeginMenu("Doc29 Noise"))
            {
                if (ImGui::MenuItem("NPD Data"))
                    ret = CsvDataset::Doc29NoiseNpd;

                if (ImGui::MenuItem("Spectrum"))
                    ret = CsvDataset::Doc29NoiseSpectrum;

                ImGui::EndMenu();
            }
            if (ImGui::IsItemClicked())
                ret = CsvDataset::Doc29Noise;

            if (ImGui::MenuItem("LTO"))
                ret = CsvDataset::LTO;

            if (ImGui::MenuItem("SFI"))
                ret = CsvDataset::SFI;

            if (ImGui::MenuItem("Fleet"))
                ret = CsvDataset::Fleet;

            return ret;
        }

        CsvInputData drawCsvInputDataTree() {
            auto ret = CsvInputData::None;

            if (ImGui::BeginMenu("Airports"))
            {
                if (ImGui::MenuItem("Runways"))
                    ret = CsvInputData::Runways;

                if (ImGui::BeginMenu("Routes"))
                {
                    if (ImGui::MenuItem("Simple"))
                        ret = CsvInputData::RoutesSimple;

                    if (ImGui::MenuItem("Vectors"))
                        ret = CsvInputData::RoutesVectors;

                    if (ImGui::MenuItem("RNP"))
                        ret = CsvInputData::RoutesRnp;

                    ImGui::EndMenu();
                }

                ImGui::EndMenu();
            }
            if (ImGui::IsItemClicked())
                ret = CsvInputData::Airports;

            ImGui::Separator();

            if (ImGui::MenuItem("Flights"))
                ret = CsvInputData::Flights;

            if (ImGui::BeginMenu("Tracks 4D"))
            {
                if (ImGui::MenuItem("Points"))
                    ret = CsvInputData::Tracks4dPoints;

                ImGui::EndMenu();
            }
            if (ImGui::IsItemClicked())
                ret = CsvInputData::Tracks4d;

            ImGui::Separator();

            if (ImGui::BeginMenu("Scenarios"))
            {
                if (ImGui::MenuItem("Operations"))
                    ret = CsvInputData::ScenariosOperations;

                if (ImGui::BeginMenu("Runs"))
                {
                    if (ImGui::BeginMenu("Performance"))
                    {
                        if (ImGui::MenuItem("Atmospheres"))
                            ret = CsvInputData::PerformanceRunsAtmospheres;

                        ImGui::EndMenu();
                    }
                    if (ImGui::IsItemClicked())
                        ret = CsvInputData::PerformanceRuns;

                    if (ImGui::BeginMenu("Noise"))
                    {
                        if (ImGui::BeginMenu("Receptors"))
                        {
                            if (ImGui::MenuItem("Grid"))
                                ret = CsvInputData::ReceptorsGrid;

                            if (ImGui::MenuItem("Points"))
                                ret = CsvInputData::ReceptorsPoints;

                            ImGui::EndMenu();
                        }

                        if (ImGui::BeginMenu("Cumulative Metric"))
                        {
                            if (ImGui::MenuItem("Weights"))
                                ret = CsvInputData::NoiseRunsCumulativeMetricsWeights;

                            ImGui::EndMenu();
                        }
                        if (ImGui::IsItemClicked())
                            ret = CsvInputData::NoiseRunsCumulativeMetrics;

                        ImGui::EndMenu();
                    }
                    if (ImGui::IsItemClicked())
                        ret = CsvInputData::NoiseRuns;

                    if (ImGui::MenuItem("Fuel & Emissions"))
                        ret = CsvInputData::EmissionsRuns;

                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if (ImGui::IsItemClicked())
                ret = CsvInputData::Scenarios;

            return ret;
        }
    }

    void Application::mainMenuBar() {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (UI::selectableWithIcon("New", ICON_FA_FILE_CIRCLE_PLUS))
                    newStudy();

                if (UI::selectableWithIcon("Open", ICON_FA_FOLDER_OPEN))
                    openStudy();

                ImGui::Separator();

                if (UI::selectableWithIcon("Clean study", ICON_FA_SOAP, m_Study->valid()))
                    queueAsyncTask([&] { m_Study->db().vacuum(); }, "Cleaning study");

                if (UI::selectableWithIcon("Verify Integrity", ICON_FA_TRIANGLE_EXCLAMATION, m_Study->valid()))
                    queueAsyncTask([&] { m_Study->db().verify(); }, "Verifying study integrity");

                ImGui::Separator();

                if (UI::selectableWithIcon("Close", ICON_FA_FILE_CIRCLE_XMARK, m_Study->valid()))
                    setStudy(std::make_shared<Study>());

                if (UI::selectableWithIcon("Exit", ICON_FA_XMARK))
                    glfwSetWindowShouldClose(m_Window, GLFW_TRUE);

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::BeginMenu(ICON_FA_FILE_IMPORT " Import", validStudy()))
                {
                    if (ImGui::BeginMenu(ICON_FA_DATABASE " Database"))
                    {
                        if (ImGui::MenuItem(ICON_FA_FOLDER " ANP"))
                        {
                            auto [path, open] = UI::pickFolder();
                            if (open)
                                queueAsyncTask([&, path] {
                                IO::AnpImport importer(path);
                                    }, std::format("Importing ANP database from '{}'", path));
                        }

                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu(ICON_FA_FILE_CSV " Datasets"))
                    {
                        const CsvDataset clicked = drawCsvDatasetTree();

                        if (clicked != CsvDataset::None)
                        {
                            auto [path, open] = UI::openCsvFile();
                            if (open)
                                switch (clicked)
                                {
                                case CsvDataset::None: GRAPE_ASSERT(false); break;
                                case CsvDataset::Doc29Aircraft: queueAsyncTask([&, path] {
                                    IO::CSV::importDoc29Performance(path);
                                    }, std::format("Importing Doc29 Performance entries from '{}'", path)); break;
                                case CsvDataset::Doc29AerodynamicCoefficients: queueAsyncTask([&, path] {
                                    IO::CSV::importDoc29PerformanceAerodynamicCoefficients(path);
                                    }, std::format("Importing Doc29 aerodynamic coefficients from '{}'", path)); break;
                                case CsvDataset::Doc29ThrustRatings: queueAsyncTask([&, path] {
                                    IO::CSV::importDoc29PerformanceThrustRatings(path);
                                    }, std::format("Importing Doc29 thrust ratings from '{}'", path)); break;
                                case CsvDataset::Doc29ThrustRatingsPropeller: queueAsyncTask([&, path] {
                                    IO::CSV::importDoc29PerformanceThrustRatingsPropeller(path);
                                    }, std::format("Importing Doc29 thrust propeller ratings from '{}'", path)); break;
                                case CsvDataset::Doc29ProfilesPoints: queueAsyncTask([&, path] {
                                    IO::CSV::importDoc29PerformanceProfilesPoints(path);
                                    }, std::format("Importing Doc29 point profiles from '{}'", path)); break;
                                case CsvDataset::Doc29ProfilesProceduralArrival: queueAsyncTask([&, path] {
                                    IO::CSV::importDoc29PerformanceProfilesArrivalSteps(path);
                                    }, std::format("Importing Doc29 arrival procedural profiles from '{}'", path)); break;
                                case CsvDataset::Doc29ProfilesProceduralDeparture: queueAsyncTask([&, path] {
                                    IO::CSV::importDoc29PerformanceProfilesDepartureSteps(path);
                                    }, std::format("Importing Doc29 departure procedural profiles from '{}'", path)); break;
                                case CsvDataset::Doc29Noise: queueAsyncTask([&, path] {
                                    IO::CSV::importDoc29Noise(path);
                                    }, std::format("Importing Doc29 noise entries from '{}'", path)); break;
                                case CsvDataset::Doc29NoiseNpd: queueAsyncTask([&, path] {
                                    IO::CSV::importDoc29NoiseNpd(path);
                                    }, std::format("Importing Doc29 NPD data from '{}'", path)); break;
                                case CsvDataset::Doc29NoiseSpectrum: queueAsyncTask([&, path] {
                                    IO::CSV::importDoc29NoiseSpectrum(path);
                                    }, std::format("Importing Doc29 noise spectrum from '{}'", path)); break;
                                case CsvDataset::LTO: queueAsyncTask([&, path] {
                                    IO::CSV::importLTO(path);
                                    }, std::format("Importing LTO database from '{}'", path)); break;
                                case CsvDataset::SFI: queueAsyncTask([&, path] {
                                    IO::CSV::importSFI(path);
                                    }, std::format("Importing SFI database from '{}'", path)); break;
                                case CsvDataset::Fleet: queueAsyncTask([&, path] {
                                    IO::CSV::importFleet(path);
                                    }, std::format("Importing Fleet from '{}'", path)); break;
                                default:  GRAPE_ASSERT(false); break;
                                }
                        }

                        ImGui::EndMenu();
                    }
                    if (ImGui::IsItemClicked())
                    {
                        auto [path, open] = UI::pickFolder();
                        if (open)
                            IO::CSV::importDatasetFiles(path);
                    }

                    if (ImGui::BeginMenu(ICON_FA_FILE_CSV " Input Data"))
                    {
                        const CsvInputData clicked = drawCsvInputDataTree();

                        if (clicked != CsvInputData::None)
                        {
                            auto [path, open] = UI::openCsvFile();
                            if (open)
                                switch (clicked)
                                {
                                case CsvInputData::None: GRAPE_ASSERT(false); break;
                                case CsvInputData::Airports: queueAsyncTask([&, path] {
                                    IO::CSV::importAirports(path);
                                    }, std::format("Importing airports from '{}'", path)); break;
                                case CsvInputData::Runways: queueAsyncTask([&, path] {
                                    IO::CSV::importRunways(path);
                                    }, std::format("Importing runways from '{}'", path)); break;
                                case CsvInputData::RoutesSimple: queueAsyncTask([&, path] {
                                    IO::CSV::importRoutesSimple(path);
                                    }, std::format("Importing simple route points from '{}'", path)); break;
                                case CsvInputData::RoutesVectors: queueAsyncTask([&, path] {
                                    IO::CSV::importRoutesVectors(path);
                                    }, std::format("Importing vector route vectors from '{}'", path)); break;
                                case CsvInputData::RoutesRnp: queueAsyncTask([&, path] {
                                    IO::CSV::importRoutesRnp(path);
                                    }, std::format("Importing RNP route steps from '{}'", path)); break;
                                case CsvInputData::Flights: queueAsyncTask([&, path] {
                                    IO::CSV::importFlights(path);
                                    }, std::format("Importing flights from '{}'", path)); break;
                                case CsvInputData::Tracks4d: queueAsyncTask([&, path] {
                                    IO::CSV::importTracks4d(path);
                                    }, std::format("Importing tracks 4D from '{}'", path)); break;
                                case CsvInputData::Tracks4dPoints: queueAsyncTask([&, path] {
                                    IO::CSV::importTracks4dPoints(path);
                                    }, std::format("Importing tracks 4D points from '{}'", path)); break;
                                case CsvInputData::Scenarios: queueAsyncTask([&, path] {
                                    IO::CSV::importScenarios(path);
                                    }, std::format("Importing scenarios from '{}'", path)); break;
                                case CsvInputData::ScenariosOperations: queueAsyncTask([&, path] {
                                    IO::CSV::importScenariosOperations(path);
                                    }, std::format("Importing scenarios operations from '{}'", path)); break;
                                case CsvInputData::PerformanceRuns: queueAsyncTask([&, path] {
                                    IO::CSV::importPerformanceRuns(path);
                                    }, std::format("Importing performance runs from '{}'", path)); break;
                                case CsvInputData::PerformanceRunsAtmospheres: queueAsyncTask([&, path] {
                                    IO::CSV::importPerformanceRunsAtmospheres(path);
                                    }, std::format("Importing performance runs atmospheres from '{}'", path)); break;
                                case CsvInputData::NoiseRuns: queueAsyncTask([&, path] {
                                    IO::CSV::importNoiseRuns(path);
                                    }, std::format("Importing noise runs from '{}'", path)); break;
                                case CsvInputData::ReceptorsGrid: queueAsyncTask([&, path] {
                                    IO::CSV::importNoiseRunsReceptorsGrids(path);
                                    }, std::format("Importing grid receptors from '{}'", path)); break;
                                case CsvInputData::ReceptorsPoints: queueAsyncTask([&, path] {
                                    IO::CSV::importNoiseRunsReceptorsPoints(path);
                                    }, std::format("Importing point receptors from '{}'", path)); break;
                                case CsvInputData::NoiseRunsCumulativeMetrics: queueAsyncTask([&, path] {
                                    IO::CSV::importNoiseRunsCumulativeMetrics(path);
                                    }, std::format("Importing cumulative metrics from '{}'", path)); break;
                                case CsvInputData::NoiseRunsCumulativeMetricsWeights: queueAsyncTask([&, path] {
                                    IO::CSV::importNoiseRunsCumulativeMetricsWeights(path);
                                    }, std::format("Importing cumulative metrics weights from '{}'", path)); break;
                                case CsvInputData::EmissionsRuns: queueAsyncTask([&, path] {
                                    IO::CSV::importEmissionsRuns(path);
                                    }, std::format("Importing emissions runs from '{}'", path)); break;
                                default: GRAPE_ASSERT(false); break;
                                }
                        }
                        ImGui::EndMenu();
                    }
                    if (ImGui::IsItemClicked())
                    {
                        auto [path, open] = UI::pickFolder();
                        if (open)
                            IO::CSV::importInputDataFiles(path);
                    }

                    if (ImGui::MenuItem(ICON_FA_FILE_CSV " All Files"))
                    {
                        auto [path, open] = UI::pickFolder();
                        if (open)
                            IO::CSV::importAllFiles(path);
                    }

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu(ICON_FA_FILE_EXPORT " Export", validStudy()))
                {
                    if (ImGui::BeginMenu(ICON_FA_FILE_CSV " Datasets"))
                    {
                        const CsvDataset clicked = drawCsvDatasetTree();

                        if (clicked != CsvDataset::None)
                        {
                            auto [path, open] = UI::saveCsvFile();
                            if (open)
                                switch (clicked)
                                {
                                case CsvDataset::None: GRAPE_ASSERT(false); break;
                                case CsvDataset::Doc29Aircraft: queueAsyncTask([&, path] {
                                    IO::CSV::exportDoc29Performance(path);
                                    }, std::format("Exporting Doc29 Performance entries to '{}'", path)); break;
                                case CsvDataset::Doc29AerodynamicCoefficients: queueAsyncTask([&, path] {
                                    IO::CSV::exportDoc29PerformanceAerodynamicCoefficients(path);
                                    }, std::format("Exporting Doc29 aerodynamic coefficients to '{}'", path)); break;
                                case CsvDataset::Doc29ThrustRatings: queueAsyncTask([&, path] {
                                    IO::CSV::exportDoc29PerformanceThrustRatings(path);
                                    }, std::format("Exporting Doc29 thrust ratings to '{}'", path)); break;
                                case CsvDataset::Doc29ThrustRatingsPropeller: queueAsyncTask([&, path] {
                                    IO::CSV::exportDoc29PerformanceThrustRatingsPropeller(path);
                                    }, std::format("Exporting Doc29 thrust propeller ratings to '{}'", path)); break;
                                case CsvDataset::Doc29ProfilesPoints: queueAsyncTask([&, path] {
                                    IO::CSV::exportDoc29PerformanceProfilesPoints(path);
                                    }, std::format("Exporting Doc29 point profiles to '{}'", path)); break;
                                case CsvDataset::Doc29ProfilesProceduralArrival: queueAsyncTask([&, path] {
                                    IO::CSV::exportDoc29PerformanceProfilesArrivalSteps(path);
                                    }, std::format("Exporting Doc29 arrival procedural profiles to '{}'", path)); break;
                                case CsvDataset::Doc29ProfilesProceduralDeparture: queueAsyncTask([&, path] {
                                    IO::CSV::exportDoc29PerformanceProfilesDepartureSteps(path);
                                    }, std::format("Exporting Doc29 departure procedural profiles to '{}'", path)); break;
                                case CsvDataset::Doc29Noise: queueAsyncTask([&, path] {
                                    IO::CSV::exportDoc29Noise(path);
                                    }, std::format("Exporting Doc29 noise entries to '{}'", path)); break;
                                case CsvDataset::Doc29NoiseNpd: queueAsyncTask([&, path] {
                                    IO::CSV::exportDoc29NoiseNpd(path);
                                    }, std::format("Exporting Doc29 NPD data to '{}'", path)); break;
                                case CsvDataset::Doc29NoiseSpectrum: queueAsyncTask([&, path] {
                                    IO::CSV::exportDoc29NoiseSpectrum(path);
                                    }, std::format("Exporting Doc29 noise spectrum to '{}'", path)); break;
                                case CsvDataset::LTO: queueAsyncTask([&, path] {
                                    IO::CSV::exportLTO(path);
                                    }, std::format("Exporting LTO database to '{}'", path)); break;
                                case CsvDataset::SFI: queueAsyncTask([&, path] {
                                    IO::CSV::exportSFI(path);
                                    }, std::format("Exporting SFI database from '{}'", path)); break;
                                case CsvDataset::Fleet: queueAsyncTask([&, path] {
                                    IO::CSV::exportFleet(path);
                                    }, std::format("Exporting Fleet from '{}'", path)); break;
                                default:  GRAPE_ASSERT(false); break;
                                }
                        }
                        ImGui::EndMenu();
                    }
                    if (ImGui::IsItemClicked())
                    {
                        auto [path, open] = UI::pickFolder();
                        if (open)
                            IO::CSV::exportDatasetFiles(path);
                    }

                    if (ImGui::BeginMenu(ICON_FA_FILE_CSV " Input Data"))
                    {
                        const CsvInputData clicked = drawCsvInputDataTree();

                        if (clicked != CsvInputData::None)
                        {
                            auto [path, open] = UI::saveCsvFile();
                            if (open)
                                switch (clicked)
                                {
                                case CsvInputData::None: GRAPE_ASSERT(false); break;
                                case CsvInputData::Airports: queueAsyncTask([&, path] {
                                    IO::CSV::exportAirports(path);
                                    }, std::format("Exporting airports to '{}'", path)); break;
                                case CsvInputData::Runways: queueAsyncTask([&, path] {
                                    IO::CSV::exportRunways(path);
                                    }, std::format("Exporting runways to '{}'", path)); break;
                                case CsvInputData::RoutesSimple: queueAsyncTask([&, path] {
                                    IO::CSV::exportRoutesSimple(path);
                                    }, std::format("Exporting simple route points to '{}'", path)); break;
                                case CsvInputData::RoutesVectors: queueAsyncTask([&, path] {
                                    IO::CSV::exportRoutesVectors(path);
                                    }, std::format("Exporting vector route vectors to '{}'", path)); break;
                                case CsvInputData::RoutesRnp: queueAsyncTask([&, path] {
                                    IO::CSV::exportRoutesRnp(path);
                                    }, std::format("Exporting RNP route steps to '{}'", path)); break;
                                case CsvInputData::Flights: queueAsyncTask([&, path] {
                                    IO::CSV::exportFlights(path);
                                    }, std::format("Exporting flights to '{}'", path)); break;
                                case CsvInputData::Tracks4d: queueAsyncTask([&, path] {
                                    IO::CSV::exportTracks4d(path);
                                    }, std::format("Exporting tracks 4D to '{}'", path)); break;
                                case CsvInputData::Tracks4dPoints: queueAsyncTask([&, path] {
                                    IO::CSV::exportTracks4dPoints(path);
                                    }, std::format("Exporting tracks 4D points to '{}'", path)); break;
                                case CsvInputData::Scenarios: queueAsyncTask([&, path] {
                                    IO::CSV::exportScenarios(path);
                                    }, std::format("Exporting scenarios to '{}'", path)); break;
                                case CsvInputData::ScenariosOperations: queueAsyncTask([&, path] {
                                    IO::CSV::exportScenariosOperations(path);
                                    }, std::format("Exporting scenarios operations to '{}'", path)); break;
                                case CsvInputData::PerformanceRuns: queueAsyncTask([&, path] {
                                    IO::CSV::exportPerformanceRuns(path);
                                    }, std::format("Exporting performance runs to '{}'", path)); break;
                                case CsvInputData::PerformanceRunsAtmospheres: queueAsyncTask([&, path] {
                                    IO::CSV::exportPerformanceRunsAtmospheres(path);
                                    }, std::format("Exporting performance runs atmospheres to '{}'", path)); break;
                                case CsvInputData::NoiseRuns: queueAsyncTask([&, path] {
                                    IO::CSV::exportNoiseRuns(path);
                                    }, std::format("Exporting noise runs to '{}'", path)); break;
                                case CsvInputData::ReceptorsGrid: queueAsyncTask([&, path] {
                                    IO::CSV::exportNoiseRunsReceptorsGrids(path);
                                    }, std::format("Exporting grid receptors to '{}'", path)); break;
                                case CsvInputData::ReceptorsPoints: queueAsyncTask([&, path] {
                                    IO::CSV::exportNoiseRunsReceptorsPoints(path);
                                    }, std::format("Exporting point receptors to '{}'", path)); break;
                                case CsvInputData::NoiseRunsCumulativeMetrics: queueAsyncTask([&, path] {
                                    IO::CSV::exportNoiseRunsCumulativeMetrics(path);
                                    }, std::format("Exporting cumulative metrics to '{}'", path)); break;
                                case CsvInputData::NoiseRunsCumulativeMetricsWeights: queueAsyncTask([&, path] {
                                    IO::CSV::exportNoiseRunsCumulativeMetricsWeights(path);
                                    }, std::format("Exporting cumulative metrics weights to '{}'", path)); break;
                                case CsvInputData::EmissionsRuns: queueAsyncTask([&, path] {
                                    IO::CSV::exportEmissionsRuns(path);
                                    }, std::format("Exporting emissions runs to '{}'", path)); break;
                                default: GRAPE_ASSERT(false); break;
                                }
                        }
                        ImGui::EndMenu();
                    }
                    if (ImGui::IsItemClicked())
                    {
                        auto [path, open] = UI::pickFolder();
                        if (open)
                            IO::CSV::exportInputDataFiles(path);
                    }

                    if (ImGui::MenuItem(ICON_FA_FILE_CSV " All Files"))
                    {
                        auto [path, open] = UI::pickFolder();
                        if (open)
                            IO::CSV::exportAllFiles(path);
                    }

                    if (ImGui::MenuItem(ICON_FA_GLOBE " Airports"))
                    {
                        auto [path, open] = UI::saveGpkgFile();
                        if (open)
                            IO::GPKG::exportAirports(path);
                    }

                    ImGui::EndMenu();
                }

                if (ImGui::MenuItem(ICON_FA_FILE_CIRCLE_MINUS " Delete all outputs", nullptr, false, validStudy()))
                {
                    panelStackReset();
                    queueAsyncTask([&] { m_Study->Scenarios.eraseOutputs(); m_Study->db().vacuum(); }, "Deleting all outputs");
                }

                ImGui::Separator();

                if (UI::selectableWithIcon("Settings", ICON_FA_SLIDERS))
                    ImGui::OpenPopup(m_ModalsStack.at(0)->imGuiId());

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                if (ImGui::BeginMenu("Datasets"))
                {
                    for (std::size_t i = 0; i < 4; ++i)
                    {
                        auto& pnl = *m_PanelStack.at(i);
                        if (ImGui::MenuItem(pnl.name().c_str(), nullptr, pnl.isOpen(), m_Study->valid()))
                            pnl.toggle();
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Input"))
                {
                    for (std::size_t i = 4; i < 7; ++i)
                    {
                        auto& pnl = *m_PanelStack.at(i);
                        if (ImGui::MenuItem(pnl.name().c_str(), nullptr, pnl.isOpen(), m_Study->valid()))
                            pnl.toggle();
                    }
                    ImGui::EndMenu();
                }

                auto& pnl = *m_PanelStack.at(7);
                if (ImGui::MenuItem(pnl.name().c_str(), nullptr, pnl.isOpen(), m_Study->valid()))
                    pnl.toggle();

                ImGui::Separator();

                // Always available panels
                if (ImGui::MenuItem("Log", nullptr, m_LogPanel.isOpen()))
                    m_LogPanel.toggle();

#ifdef GRAPE_DEBUG
                if (ImGui::BeginMenu("Demos"))
                {
                    ImGui::MenuItem("ImGui", nullptr, &m_ShowImGuiDemo);
                    ImGui::EndMenu();
                }
#endif // GRAPE_DEBUG
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help"))
            {
                if (ImGui::MenuItem("Documentation"))
                    platformOpen(GRAPE_DOCS_URL);

                if (ImGui::MenuItem("About"))
                    ImGui::OpenPopup(m_ModalsStack.at(1)->imGuiId());

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        ImGui::PopStyleVar();
    }

    void Application::asyncTaskWindow() const {
        if (!m_AsyncTask.running())
            return;

        ImGui::OpenPopup("##AsyncTask");

        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(600.0f, 100.0f));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        if (ImGui::BeginPopupModal("##AsyncTask", nullptr, ImGuiWindowFlags_NoDecoration))
        {
            ImGui::Spacing();
            UI::textInfoWrapped(std::format("{} ...", m_AsyncTask.message()));

            ImGui::Spacing();
            constexpr float radius = 15.0f;
            UI::alignForWidth(radius * 2.0f);
            UI::spinner("AsyncSpinner", radius);

            if (!m_AsyncTask.running())
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();
    }

    void Application::panelStackDraw() const {
        if (m_Study->valid())
            for (const auto& panel : m_PanelStack)
                panel->imGuiDraw();
    }

    void Application::newStudy() {
        auto [path, open] = UI::saveGrapeFile();
        if (!open)
            return;

        const auto newStudy = std::make_shared<Study>();
        if (newStudy->create(path)) // Implicit char* to std::filesystem::path
            setStudy(newStudy);
    }

    void Application::openStudy() {
        auto [path, open] = UI::openGrapeFile();
        if (!open)
            return;

        queueAsyncTask([&, path] {
            const auto newStudy = std::make_shared<Study>();
            if (newStudy->open(path)) // Implicit std::string to std::filesystem::path
                setStudy(newStudy);
            }, std::format("Opening study at '{}'", path)
                );
    }

    void Application::setStudy(const std::shared_ptr<Study>& StudyIn) {
        panelStackReset();
        m_Study = StudyIn;
    }

    void Application::panelStackReset() const {
        if (m_Study->valid())
            for (const auto& pnl : m_PanelStack)
                pnl->reset();
    }

    void Application::panelStackOnPerformanceRunStart() const {
        if (m_Study->valid())
            for (const auto& pnl : m_PanelStack)
                pnl->onPerformanceRunStart();
    }

    void Application::panelStackOnNoiseRunStart() const {
        if (m_Study->valid())
            for (const auto& pnl : m_PanelStack)
                pnl->onNoiseRunStart();
    }

    bool Application::validStudy() const { return m_Study->valid(); }

    const Image& Application::icon() const { return *m_GrapeIcon; }

    namespace {
        void applicationWriteAll(ImGuiContext*, ImGuiSettingsHandler* Handler, ImGuiTextBuffer* Buf) {
            const auto app = static_cast<Application*>(Handler->UserData);

            // Application
            Buf->appendf("%s", std::format("[{0}][{0}]\n", Handler->TypeName).c_str());

            // Window Position
            int posX, posY;
            int sizeX, sizeY;
            int maximized = glfwGetWindowAttrib(app->glfwWindow(), GLFW_MAXIMIZED);
            glfwGetWindowPos(app->glfwWindow(), &posX, &posY);
            glfwGetWindowSize(app->glfwWindow(), &sizeX, &sizeY);
            Buf->appendf("%s", std::format("WindowPos={},{}\n", posX, posY).c_str());
            Buf->appendf("%s", std::format("WindowSize={},{}\n", sizeX, sizeY).c_str());
            Buf->appendf("%s", std::format("WindowMaximized={}\n", maximized).c_str());
            Buf->append("\n");

            // Panel Stack status
            for (std::size_t i = 0; i < app->panelStack().size(); ++i)
                Buf->appendf("%s", std::format("{}={:d}\n", i, app->panelStack().at(i)->isOpen()).c_str());

            Buf->append("\n");
        }

        void* applicationReadOpen(ImGuiContext*, ImGuiSettingsHandler* Handler, const char*) { return Handler->UserData; }

        void applicationReadLine(ImGuiContext*, ImGuiSettingsHandler* Handler, void*, const char* Line) {
            const auto app = static_cast<Application*>(Handler->UserData);

            // Find virtual screen dimensions
            int monitorCount;
            GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
            int monMinX = 0, monMaxX = 0, monMinY = 0, monMaxY = 0;
            for (int i = 0; i < monitorCount; ++i)
            {
                GLFWmonitor* mon = monitors[i];
                int monPosX, monPosY, monWidth, monHeight;
                glfwGetMonitorWorkarea(mon, &monPosX, &monPosY, &monWidth, &monHeight);

                monMinX = std::min(monMinX, monPosX);
                monMinY = std::min(monMinX, monPosY);

                monMaxX = std::max(monMaxX, monPosX + monWidth);
                monMaxY = std::max(monMaxY, monPosY + monHeight);
            }

            int winWidth, winHeight;
            glfwGetWindowSize(app->glfwWindow(), &winWidth, &winHeight);

            int i1, i2;
            if (sscanf_s(Line, "WindowPos=%i,%i", &i1, &i2) == 2)
            {
                if (i1 >= monMinX && i1 <= monMaxX - winWidth && i2 >= monMinY && i2 <= monMaxY - winHeight)
                    glfwSetWindowPos(app->glfwWindow(), i1, i2);
            }
            else if (sscanf_s(Line, "WindowSize=%i,%i", &i1, &i2) == 2)
            {
                int winPosX, winPosY;
                glfwGetWindowPos(app->glfwWindow(), &winPosX, &winPosY);
                if (winPosX + i1 <= monMaxX && winPosY + i2 <= monMaxY)
                    glfwSetWindowSize(app->glfwWindow(), i1, i2);
                glfwRestoreWindow(app->glfwWindow());
            }
            else if (sscanf_s(Line, "WindowMaximized=%i", &i1) == 1)
            {
                if (i1 > 0)
                    glfwMaximizeWindow(app->glfwWindow());
            }
            else if (sscanf_s(Line, "%i=%i", &i1, &i2) == 2)
            {
                if (i1 < static_cast<int>(app->panelStack().size()))
                {
                    if (i2)
                        app->panelStack().at(i1)->open();
                    else
                        app->panelStack().at(i1)->close();
                }
            }
        }
    }

    void Application::parseCommandLineArgs() {
        // -h -> display help
        if (m_CommandLineArgs.argPassed("-h"))
        {
            Log::core()->info(
                "\n"
                "    **** Command line options ****\n"
                "\n"
                "    [-h]   - Display this help. Equivalent to [-h -x].\n"
                "    [-x]   - Close after processing the command line options, do not run the application.\n"
                "    [-c]   - Create a GRAPE study located at the path specified by the following argument.\n"
                "    [-o]   - Open a GRAPE study located at the path specified by the following argument.\n"
                "    [-anp] - Import the ANP database at the folder path specified by the following argument. Use only in conjunction with -c or -o.\n"
                "    [-d]   - Delete all outputs from the study. Use only in conjunction with -o.\n"
                "    [-rp]  - Start the performance run specified by the following argument as <scenario name>-<performance run name>. Use only in conjunction with -o.\n"
                "    [-rn]  - Start the noise run specified by the following argument as <scenario name>-<performance run name>-<noise run name>. Use only in conjunction with -o.\n"
                "    [-re] - Start the emissions run specified by the following argument as <scenario name>-<performance run name>-<emissions run name>. Use only in conjunction with -o.\n"
            );
            m_RunApplication = false;
            return;
        }

        // -x -> close after parsing the command line arguments
        if (m_CommandLineArgs.argPassed("-x"))
            m_RunApplication = false;

        // -c -> followed by path to create file
        if (m_CommandLineArgs.argPassed("-c"))
        {
            const int valIndex = m_CommandLineArgs.argIndex("-c") + 1;
            if (m_CommandLineArgs.isValueArg(valIndex))
            {
                auto path = m_CommandLineArgs.arg(valIndex);
                m_Study->close();
                if (!m_Study->create(path))
                    return;
            }
            else
            {
                Log::core()->error(CommandLineIncorrectUsage);
            }
        }

        // -o -> followed by path to open
        if (m_CommandLineArgs.argPassed("-o"))
        {
            const int valIndex = m_CommandLineArgs.argIndex("-o") + 1;
            if (m_CommandLineArgs.isValueArg(valIndex))
            {
                m_Study->close();
                m_Study->open(m_CommandLineArgs.arg(valIndex));
            }
            else
            {
                Log::core()->error(CommandLineIncorrectUsage);
            }
        }

        // -anp -> followed by the path to an ANP folder
        if (m_CommandLineArgs.argPassed("-anp"))
            try
        {
            if (!m_Study->valid())
                throw GrapeException(CommandLineIncorrectUsage);

            const int valIndex = m_CommandLineArgs.argIndex("-anp") + 1;
            if (!m_CommandLineArgs.isValueArg(valIndex))
                throw GrapeException(CommandLineIncorrectUsage);

            const auto& path = m_CommandLineArgs.arg(valIndex);
            Log::io()->info("Importing ANP database from '{}'.", path);
            IO::AnpImport importer(path);
        }
        catch (const std::exception& err)
        {
            Log::core()->error(err.what());
        }

        // -d -> Delete all outputs from study
        if (m_CommandLineArgs.argPassed("-d"))
        {
            if (m_Study->valid())
                m_Study->Scenarios.eraseOutputs();
            else
                Log::core()->error(CommandLineIncorrectUsage);
        }

        // -rp -> followed by name of the performance run <scenario name>-<performance run name>
        while (m_CommandLineArgs.argPassed("-rp"))
        {
            try
            {
                if (!m_Study->valid())
                    throw GrapeException(CommandLineIncorrectUsage);

                const int valIndex = m_CommandLineArgs.argIndex("-rp") + 1;
                if (!m_CommandLineArgs.isValueArg(valIndex))
                    throw GrapeException(CommandLineIncorrectUsage);

                const std::string argStr = m_CommandLineArgs.arg(valIndex);
                const auto splitPos = argStr.find('-');
                if (splitPos == std::string::npos)
                    throw GrapeException(CommandLineIncorrectUsage);

                const std::string scenName = argStr.substr(0, splitPos);
                const std::string perfRunName = argStr.substr(splitPos + 1, std::string::npos);

                if (!m_Study->Scenarios().contains(scenName))
                    throw GrapeException(std::format("Scenario '{}' not found in study '{}'.", scenName, m_Study->name()));
                const auto& scen = m_Study->Scenarios(scenName);

                if (!scen.PerformanceRuns.contains(perfRunName))
                    throw GrapeException(std::format("Performance run '{}' not found in scenario '{}'.", perfRunName, scenName));
                const auto& perfRunJob = scen.PerformanceRuns(perfRunName).job();

                if (!perfRunJob->ready())
                    throw GrapeException(std::format("Performance run '{}' of scenario '{}' has already been run.", perfRunName, scenName));

                m_Study->Jobs.queueJob(perfRunJob);
            }
            catch (const std::exception& err)
            {
                Log::core()->error(err.what());
            }
            m_CommandLineArgs.deleteArg("-rp");
        }

        // -rn -> followed by name of the noise run <scenario name>-<performance run name>-<noise run name>
        while (m_CommandLineArgs.argPassed("-rn"))
        {
            try
            {
                if (!m_Study->valid())
                    throw GrapeException(CommandLineIncorrectUsage);

                const int valIndex = m_CommandLineArgs.argIndex("-rn") + 1;
                if (!m_CommandLineArgs.isValueArg(valIndex))
                    throw GrapeException(CommandLineIncorrectUsage);

                std::string argStr = m_CommandLineArgs.arg(valIndex); // <scenario name>-<performance run name>-<noise run name>
                auto splitPos = argStr.find('-');
                if (splitPos == std::string::npos) // '-' character not found
                    throw GrapeException(CommandLineIncorrectUsage);

                const std::string scenName = argStr.substr(0, splitPos);
                argStr = argStr.substr(splitPos + 1); // <performance run name>-<noise run name>
                splitPos = argStr.find('-');
                if (splitPos == std::string::npos) // '-' character not found
                    throw GrapeException(CommandLineIncorrectUsage);

                const std::string perfRunName = argStr.substr(0, splitPos);
                const std::string nsRunName = argStr.substr(splitPos + 1, std::string::npos);

                if (!m_Study->Scenarios().contains(scenName))
                    throw GrapeException(std::format("Scenario '{}' not found in study '{}'.", scenName, m_Study->name()));
                const auto& scen = m_Study->Scenarios(scenName);

                if (!scen.PerformanceRuns.contains(perfRunName))
                    throw GrapeException(std::format("Performance run '{}' not found in scenario '{}'.", perfRunName, scenName));
                const auto& perfRun = scen.PerformanceRuns(perfRunName);

                if (!perfRun.NoiseRuns.contains(nsRunName))
                    throw GrapeException(std::format("Noise run '{}' not found in performance run '{}' of scenario '{}'.", nsRunName, perfRunName, scenName));
                const auto& nsRunJob = perfRun.NoiseRuns(nsRunName).job();

                if (!nsRunJob->ready())
                    throw GrapeException(std::format("Noise run '{}' of performance run '{}' of scenario '{}' has already been run.", nsRunName, perfRunName, scenName));

                m_Study->Jobs.queueJob(nsRunJob);
            }
            catch (const std::exception& err)
            {
                Log::core()->error(err.what());
            }
            m_CommandLineArgs.deleteArg("-rn");
        }

        // -rfe -> followed by name of the emissions run <scenario name>-<performance run name>-<emissions run name>
        while (m_CommandLineArgs.argPassed("-re"))
        {
            try
            {
                if (!m_Study->valid())
                    throw GrapeException(CommandLineIncorrectUsage);

                const int valIndex = m_CommandLineArgs.argIndex("-rfe") + 1;
                if (!m_CommandLineArgs.isValueArg(valIndex))
                    throw GrapeException(CommandLineIncorrectUsage);

                std::string argStr = m_CommandLineArgs.arg(valIndex); // <scenario name>-<performance run name>-<emissions run name>
                auto splitPos = argStr.find('-');
                if (splitPos == std::string::npos) // '-' character not found
                    throw GrapeException(CommandLineIncorrectUsage);

                const std::string scenName = argStr.substr(0, splitPos);
                argStr = argStr.substr(splitPos + 1); // <performance run name>-<emissions run name>
                splitPos = argStr.find('-');
                if (splitPos == std::string::npos) // '-' character not found
                    throw GrapeException(CommandLineIncorrectUsage);

                const std::string perfRunName = argStr.substr(0, splitPos);
                const std::string emiRunName = argStr.substr(splitPos + 1, std::string::npos);

                if (!m_Study->Scenarios().contains(scenName))
                    throw GrapeException(std::format("Scenario '{}' not found in study '{}'.", scenName, m_Study->name()));
                const auto& scen = m_Study->Scenarios(scenName);

                if (!scen.PerformanceRuns.contains(perfRunName))
                    throw GrapeException(std::format("Performance run '{}' not found in scenario '{}'.", perfRunName, scenName));
                const auto& perfRun = scen.PerformanceRuns(perfRunName);

                if (!perfRun.EmissionsRuns.contains(emiRunName))
                    throw GrapeException(std::format("Emissions run '{}' not found in performance run '{}' of scenario '{}'.", emiRunName, perfRunName, scenName));
                const auto& emiRunJob = perfRun.EmissionsRuns(emiRunName).job();

                if (!emiRunJob->ready())
                    throw GrapeException(std::format("Emissions run '{}' of performance run '{}' of scenario '{}' has already been run.", emiRunName, perfRunName, scenName));

                m_Study->Jobs.queueJob(emiRunJob);
            }
            catch (const std::exception& err)
            {
                Log::core()->error(err.what());
            }
            m_CommandLineArgs.deleteArg("-re");
        }
    }

    void Application::initDefineHandler() {
        ImGuiContext* context = ImGui::GetCurrentContext();

        ImGuiSettingsHandler appHandler;
        appHandler.TypeName = "Grape Application";
        appHandler.TypeHash = ImHashStr("Grape Application");
        appHandler.ReadOpenFn = applicationReadOpen;
        appHandler.ReadLineFn = applicationReadLine;
        appHandler.WriteAllFn = applicationWriteAll;
        appHandler.UserData = this;
        context->SettingsHandlers.push_back(appHandler);
    }

    void Application::initStyle() const {
        /***********
        * Sizes
        ***********/
        ImGuiStyle& style = ImGui::GetStyle();

        // Main
        style.WindowPadding = { 1, 4 };
        style.WindowMinSize = { 400, 100 };
        style.FramePadding = { 5, 4 };
        style.CellPadding = style.FramePadding;
        style.ItemSpacing = { 8, 8 };
        style.ItemInnerSpacing = { 4, 4 };
        style.TouchExtraPadding = { 0,0 };
        style.IndentSpacing = 20;
        style.ScrollbarSize = 14;
        style.GrabMinSize = 10;

        // Borders
        style.WindowBorderSize = 0.0f;
        style.ChildBorderSize = 0.0f;
        style.PopupBorderSize = 0.0f;
        style.FrameBorderSize = 1.0f;
        style.TabBorderSize = 0.0f;

        // Rounding
        style.WindowRounding = 0.0f;
        style.ChildRounding = 8.0f;
        style.FrameRounding = 8.0f;
        style.PopupRounding = 0.0f;
        style.ScrollbarRounding = style.FrameRounding;
        style.GrabRounding = style.FrameRounding;
        style.LogSliderDeadzone = 4.0f;
        style.TabRounding = 0.0f;

        /***********
        * Colors
        ***********/
        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f); // GRAPE
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f); // GRAPE
        colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.09f, 0.08f, 1.00f); // GRAPE
        colors[ImGuiCol_DockingEmptyBg] = colors[ImGuiCol_WindowBg]; // Same as Window
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.01f, 0.00f, 0.5f); // GRAPE
        colors[ImGuiCol_PopupBg] = colors[ImGuiCol_WindowBg];
        colors[ImGuiCol_Border] = ImVec4(0.40f, 0.38f, 0.36f, 1.00f); // GRAPE
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f); // Disabled
        colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f); // Disabled
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.30f, 0.31f, 0.30f, 0.82f); // GRAPE
        colors[ImGuiCol_FrameBgActive] = colors[ImGuiCol_WindowBg]; // GRAPE
        colors[ImGuiCol_TitleBg] = colors[ImGuiCol_WindowBg];
        colors[ImGuiCol_TitleBgActive] = colors[ImGuiCol_TitleBg];
        colors[ImGuiCol_TitleBgCollapsed] = colors[ImGuiCol_TitleBg];
        colors[ImGuiCol_MenuBarBg] = colors[ImGuiCol_WindowBg];
        colors[ImGuiCol_ScrollbarBg] = colors[ImGuiCol_WindowBg];
        colors[ImGuiCol_ScrollbarGrab] = colors[ImGuiCol_Border];
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.87f, 0.42f, 0.00f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = colors[ImGuiCol_ScrollbarGrabHovered];
        colors[ImGuiCol_CheckMark] = ImVec4(0.24f, 0.72f, 0.06f, 0.79f); // GRAPE
        colors[ImGuiCol_SliderGrab] = colors[ImGuiCol_CheckMark]; // Same as check mark
        colors[ImGuiCol_SliderGrabActive] = colors[ImGuiCol_SliderGrab]; // Frame highlights
        colors[ImGuiCol_Button] = colors[ImGuiCol_FrameBg]; // Button same as Frame
        colors[ImGuiCol_ButtonHovered] = colors[ImGuiCol_FrameBgHovered];
        colors[ImGuiCol_ButtonActive] = colors[ImGuiCol_FrameBgActive];
        colors[ImGuiCol_Header] = ImVec4(0.36f, 0.48f, 0.30f, 0.79f); // GRAPE
        colors[ImGuiCol_HeaderHovered] = colors[ImGuiCol_FrameBgHovered];
        colors[ImGuiCol_HeaderActive] = colors[ImGuiCol_FrameBgActive];
        colors[ImGuiCol_Separator] = colors[ImGuiCol_Border]; // Separator same as Slider
        colors[ImGuiCol_SeparatorHovered] = colors[ImGuiCol_ScrollbarGrabHovered];
        colors[ImGuiCol_SeparatorActive] = colors[ImGuiCol_SeparatorHovered];
        colors[ImGuiCol_ResizeGrip] = colors[ImGuiCol_Separator]; // Resize grip same as separator
        colors[ImGuiCol_ResizeGripHovered] = colors[ImGuiCol_SeparatorHovered];
        colors[ImGuiCol_ResizeGripActive] = colors[ImGuiCol_SeparatorActive];
        colors[ImGuiCol_Tab] = colors[ImGuiCol_FrameBg]; // Tab same as Frame
        colors[ImGuiCol_TabHovered] = colors[ImGuiCol_FrameBgHovered];
        colors[ImGuiCol_TabActive] = colors[ImGuiCol_FrameBgHovered];
        colors[ImGuiCol_TabUnfocused] = colors[ImGuiCol_Tab]; // Same as Tab | Focus show through active tab
        colors[ImGuiCol_TabUnfocusedActive] = colors[ImGuiCol_TabActive];
        colors[ImGuiCol_DockingPreview] = ImVec4(0.87f, 0.42f, 0.00f, 0.74f); // Similar to separator but more transparent

        // TODO: Plots!
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);

        colors[ImGuiCol_TableHeaderBg] = colors[ImGuiCol_FrameBgHovered]; // Hovered Frame represents selection
        colors[ImGuiCol_TableBorderStrong] = colors[ImGuiCol_ChildBg]; // Same as Child Window
        colors[ImGuiCol_TableBorderLight] = colors[ImGuiCol_ChildBg]; // Same as Child Window
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f); // Disabled
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f); // Disabled
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.48f, 0.81f, 0.31f, 0.79f); // GRAPE

        // TODO
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

        // Extra colors 
        UI::g_ExtraColors[UI::GrapeColInfoText] = ImVec4(0.07f, 0.57f, 0.68f, 1.00f);
        UI::g_ExtraColors[UI::GrapeColNew] = colors[ImGuiCol_CheckMark];
        UI::g_ExtraColors[UI::GrapeColEdit] = colors[ImGuiCol_ScrollbarGrabHovered];
        UI::g_ExtraColors[UI::GrapeColDelete] = ImVec4(0.86f, 0.26f, 0.26f, 1.00f);
        UI::g_ExtraColors[UI::GrapeColInvalidInputTextSelectedBg] = ImVec4(0.82f, 0.83f, 0.85f, 0.44f);

        /***********
        * Fonts
        ***********/
        ImGuiIO& io = ImGui::GetIO();

        ImFontConfig fontConfig;

        // Default GRAPE font
        fontConfig.FontDataOwnedByAtlas = false;
        io.FontDefault = io.Fonts->AddFontFromMemoryTTF((void*)g_RobotoMedium, sizeof g_RobotoMedium, 16.0f, &fontConfig);

        // Icons
        fontConfig.MergeMode = true;
        io.Fonts->AddFontFromMemoryTTF((void*)g_FaSolid, sizeof g_FaSolid, 16.0f, &fontConfig, g_IconsRanges);

        io.Fonts->Build();

        // Upload to Vulkan
        {
            // Use any command queue
            const VkCommandPool commandPool = g_MainWindowData.Frames[g_MainWindowData.FrameIndex].CommandPool;
            const VkCommandBuffer commandBuffer = g_MainWindowData.Frames[g_MainWindowData.FrameIndex].CommandBuffer;

            VkResult err = vkResetCommandPool(g_Device, commandPool, 0);
            checkVkResult(err);
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            err = vkBeginCommandBuffer(commandBuffer, &beginInfo);
            checkVkResult(err);

            ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

            VkSubmitInfo endInfo = {};
            endInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            endInfo.commandBufferCount = 1;
            endInfo.pCommandBuffers = &commandBuffer;
            err = vkEndCommandBuffer(commandBuffer);
            checkVkResult(err);
            err = vkQueueSubmit(g_Queue, 1, &endInfo, VK_NULL_HANDLE);
            checkVkResult(err);

            err = vkDeviceWaitIdle(g_Device);
            checkVkResult(err);
            ImGui_ImplVulkan_DestroyFontUploadObjects();
        }
    }
}
