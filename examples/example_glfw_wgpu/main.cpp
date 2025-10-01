// Dear ImGui: standalone example application for using GLFW + WebGPU
// - Emscripten is supported for publishing on web. See https://emscripten.org.
// - Dawn is used as a WebGPU implementation on desktop.

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_wgpu.h"
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>
#else
#include <glfw3webgpu.h>
#endif

#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu_cpp.h>

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

// Global WebGPU required states
static wgpu::Instance      wgpu_instance = nullptr;
static wgpu::Device        wgpu_device = nullptr;
static wgpu::Surface       wgpu_surface = nullptr;
static wgpu::TextureFormat wgpu_preferred_fmt = wgpu::TextureFormat::RGBA8Unorm;
static wgpu::SurfaceConfiguration wgpu_surface_config = {};

// static WGPUSwapChain     wgpu_swap_chain = nullptr;
static int               wgpu_swap_chain_width = 1280;
static int               wgpu_swap_chain_height = 800;

// Forward declarations
static bool InitWGPU(GLFWwindow* window);
// static void CreateSwapChain(int width, int height);

static void glfw_error_callback(int error, const char* description)
{
    printf("GLFW Error %d: %s\n", error, description);
}

// Main code
int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Make sure GLFW does not initialize any graphics context.
    // This needs to be done explicitly later.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(wgpu_swap_chain_width, wgpu_swap_chain_height, "Dear ImGui GLFW+WebGPU example", nullptr, nullptr);
    if (window == nullptr)
        return 1;

    // Initialize the WebGPU environment
    if (!InitWGPU(window))
    {
        if (window)
            glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    // CreateSwapChain(wgpu_swap_chain_width, wgpu_swap_chain_height);
    glfwShowWindow(window);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOther(window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
#endif
    ImGui_ImplWGPU_InitInfo init_info;
    init_info.Device = wgpu_device.Get();
    init_info.NumFramesInFlight = 3;
    init_info.RenderTargetFormat = (WGPUTextureFormat) wgpu_preferred_fmt;
    init_info.DepthStencilFormat = WGPUTextureFormat_Undefined;
    ImGui_ImplWGPU_Init(&init_info);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details. If you like the default font but want it to scale better, consider using the 'ProggyVector' from the same author!
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Emscripten allows preloading a file or folder to be accessible at runtime. See Makefile for details.
    //io.Fonts->AddFontDefault();
    //style.FontSizeBase = 20.0f;
#ifndef IMGUI_DISABLE_FILE_FUNCTIONS
    //io.Fonts->AddFontFromFileTTF("fonts/segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("fonts/Cousine-Regular.ttf");
    //io.Fonts->AddFontFromFileTTF("fonts/ProggyTiny.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("fonts/ArialUni.ttf");
    //IM_ASSERT(font != nullptr);
#endif

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!glfwWindowShouldClose(window))
#endif
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // React to changes in screen size
        int width, height;
        glfwGetFramebufferSize((GLFWwindow*)window, &width, &height);
        if (width != wgpu_swap_chain_width || height != wgpu_swap_chain_height)
        {
            ImGui_ImplWGPU_InvalidateDeviceObjects();
            // CreateSwapChain(width, height);
            ImGui_ImplWGPU_CreateDeviceObjects();
        }

        // Start the Dear ImGui frame
        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                                // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");                     // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);            // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);                  // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color);       // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                                  // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);         // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // Rendering
        ImGui::Render();

#ifndef __EMSCRIPTEN__
        // Tick needs to be called in Dawn to display validation errors
        wgpu_device.Tick();
#endif
        wgpu::SurfaceTexture st;
        wgpu_surface.GetCurrentTexture(&st);
        if (st.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal &&
            st.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal) {
            // handle lost/suboptimal (reconfigure if needed)
        }
        wgpu::TextureView view = st.texture.CreateView();


        wgpu::RenderPassColorAttachment color_attachments = {};
        color_attachments.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        color_attachments.loadOp = wgpu::LoadOp::Clear;
        color_attachments.storeOp = wgpu::StoreOp::Store;
        color_attachments.clearValue = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        color_attachments.view = view;

        wgpu::RenderPassDescriptor render_pass_desc = {};
        render_pass_desc.colorAttachmentCount = 1;
        render_pass_desc.colorAttachments = &color_attachments;
        render_pass_desc.depthStencilAttachment = nullptr;

        wgpu::CommandEncoderDescriptor enc_desc = {};
        wgpu::CommandEncoder encoder = wgpu_device.CreateCommandEncoder(&enc_desc);

        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&render_pass_desc);
        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass.Get());

        pass.End();

        wgpu::CommandBufferDescriptor cmd_buffer_desc = {};
        wgpu::CommandBuffer cmd_buffer = encoder.Finish(&cmd_buffer_desc);
        wgpu::Queue queue = wgpu_device.GetQueue();
        queue.Submit(1, &cmd_buffer);

#ifndef __EMSCRIPTEN__
    wgpu_surface.Present();
#endif

        // wgpuTextureViewRelease(color_attachments.view);
        // wgpuRenderPassEncoderRelease(pass);
        // wgpuCommandEncoderRelease(encoder);
        // wgpuCommandBufferRelease(cmd_buffer);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    ImGui_ImplWGPU_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

template<typename T>
struct request_userdata {
    T    result{};
    bool request_ended = false;
};

static bool InitWGPU(GLFWwindow* window)
{
    // 1. Instance
    wgpu::InstanceDescriptor desc = {};
    desc.nextInChain = nullptr;
    wgpu::Instance instance = wgpu::CreateInstance(&desc);
    if (!instance) return false;

#ifdef __EMSCRIPTEN__
    wgpu_device = emscripten_webgpu_get_device();
    if (!wgpu_device)
        return false;
#else
    // 2. Request adapter
    request_userdata<wgpu::Adapter> adapter_data;

    instance.RequestAdapter(
        nullptr,
        wgpu::CallbackMode::AllowSpontaneous,
        [&adapter_data](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView message) {
            if (status == wgpu::RequestAdapterStatus::Success) {
                adapter_data.result = adapter;
            } else {
                printf("Could not get WebGPU adapter: %.*s\n", (int)message.length, message.data);
            }
            adapter_data.request_ended = true;
        }
    );

    // Spin until adapter request completes
    while (!adapter_data.request_ended) {
        instance.ProcessEvents();
    }

    wgpu::Adapter adapter = adapter_data.result;
    if (!adapter) return false;

    // 3. Request device
    request_userdata<wgpu::Device> device_data;

    wgpu::DeviceDescriptor device_desc = {};
    device_desc.label = "ImGui Device";
    device_desc.defaultQueue.label = "Main Queue";
    device_desc.SetUncapturedErrorCallback(
        [](wgpu::Device const&, wgpu::ErrorType type, wgpu::StringView message) {
            printf("Uncaptured error %d: %.*s\n", (int)type, (int)message.length, message.data);
        }
    );

    adapter.RequestDevice(
        &device_desc,
        wgpu::CallbackMode::AllowSpontaneous,
        [&device_data](wgpu::RequestDeviceStatus status, wgpu::Device device, wgpu::StringView message) {
            if (status == wgpu::RequestDeviceStatus::Success) {
                device_data.result = device;
            } else {
                printf("Could not get WebGPU device: %.*s\n", (int)message.length, message.data);
            }
            device_data.request_ended = true;
        }
    );

    // Spin until device request completes
    while (!device_data.request_ended) {
        instance.ProcessEvents();
    }

    wgpu::Device device = device_data.result;
    if (!device) return false;

    wgpu_device = device;
#endif

#ifdef __EMSCRIPTEN__
    wgpu::SurfaceDescriptorFromCanvasHTMLSelector html_surface_desc = {};
    html_surface_desc.selector = "#canvas";
    wgpu::SurfaceDescriptor surface_desc = {};
    surface_desc.nextInChain = &html_surface_desc;
    wgpu::Surface surface = instance.CreateSurface(&surface_desc);

    wgpu::Adapter dummy;
    wgpu_preferred_fmt = surface.GetPreferredFormat(dummy);
#else
    // 4) Surface via GLFW
    wgpu::Surface surface = glfwCreateWindowWGPUSurface(instance.Get(), window);
    if (!surface) return false;

    // 5) Choose preferred format from surface+adapter
    wgpu_preferred_fmt = wgpu::TextureFormat::BGRA8Unorm;

    // 6) Configure surface (replaces old swapchain)
    int fbw = 0, fbh = 0;
    glfwGetFramebufferSize(window, &fbw, &fbh);

    wgpu_surface_config.device      = wgpu_device;
    wgpu_surface_config.format      = wgpu_preferred_fmt;
    wgpu_surface_config.usage       = wgpu::TextureUsage::RenderAttachment;
    wgpu_surface_config.width       = (uint32_t)fbw;
    wgpu_surface_config.height      = (uint32_t)fbh;
    wgpu_surface_config.presentMode = wgpu::PresentMode::Fifo;
    wgpu_surface_config.alphaMode   = wgpu::CompositeAlphaMode::Auto;

    surface.Configure(&wgpu_surface_config);
#endif

    // Store globals
    wgpu_instance = instance;
    wgpu_surface = surface;

    return true;
}


// static void CreateSwapChain(int width, int height)
// {
//     if (wgpu_swap_chain)
//         wgpuSwapChainRelease(wgpu_swap_chain);
//     wgpu_swap_chain_width = width;
//     wgpu_swap_chain_height = height;
//     WGPUSwapChainDescriptor swap_chain_desc = {};
//     swap_chain_desc.usage = WGPUTextureUsage_RenderAttachment;
//     swap_chain_desc.format = wgpu_preferred_fmt;
//     swap_chain_desc.width = width;
//     swap_chain_desc.height = height;
//     swap_chain_desc.presentMode = WGPUPresentMode_Fifo;
//     wgpu_swap_chain = wgpuDeviceCreateSwapChain(wgpu_device, wgpu_surface, &swap_chain_desc);
// }
