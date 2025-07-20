#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <stdio.h>
#include <GLFW/glfw3.h>
#ifdef __linux__
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <string.h> // For memset
#endif
#include "SystemMonitor.h"
#include <chrono>
#include <thread>
#include <cmath>

// Global SystemMonitor instance
SystemMonitor systemMonitor;

// Visibility and Alpha for the stats window
bool show_stats_window = true;
float window_alpha = 1.0f; // Current alpha for fading
float target_alpha = 1.0f; // Target alpha for fading
double fade_start_time = 0.0; // Time when fading started
const double fade_duration = 0.2; // Fade duration in seconds

// Color cycling variables
float current_hue = 0.0f; // Current hue for text color (0.0 to 1.0)
const float hue_speed = 0.05f; // Speed of hue change per second

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

// Key callback function
static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // We explicitly do NOT pass key events to ImGui here, as we want all input to pass through
    // except for F1, which is handled directly by GLFW.
    // ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);

    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        printf("F1 key pressed!\n"); // Debug print
        show_stats_window = !show_stats_window;
        fade_start_time = glfwGetTime();
        target_alpha = show_stats_window ? 1.0f : 0.0f;
    }
}

int main(int, char**)
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Configure GLFW for an overlay window
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); // No window decorations (border, title bar)
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE); // Enable transparent framebuffer
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE); // Keep window always on top (might not work on all WMs)
    glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE); // Allow mouse events to pass through the window
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // Make window non-resizable, often helps with taskbar hiding
    glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE); // Do not focus the window when shown
    glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE); // Do not iconify the window automatically

    // Get primary monitor and its video mode to create a full-screen window
    GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary_monitor);

    // Create window with graphics context
    // Create a window that covers the entire screen, but is not truly fullscreen (no mode change)
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "System Stats Overlay", NULL, NULL);
    if (window == NULL)
        return 1;

#ifdef __linux__
    // Final attempt: Use KDE-specific hints for KWin, while keeping EWMH hints for compatibility.
    Display* dpy = glfwGetX11Display();
    Window w = glfwGetX11Window(window);
    Window root = DefaultRootWindow(dpy);

    if (dpy && w) {
        // KDE-specific hint to treat this as an override-redirect window, giving us more control.
        Atom kde_net_wm_window_type_override = XInternAtom(dpy, "_KDE_NET_WM_WINDOW_TYPE_OVERRIDE", False);
        XChangeProperty(dpy, w, kde_net_wm_window_type_override, XA_ATOM, 32, PropModeReplace, (unsigned char *)&kde_net_wm_window_type_override, 1);

        // Set window type to DOCK as a strong hint for the window's purpose.
        Atom wm_window_type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
        Atom type_dock = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
        XChangeProperty(dpy, w, wm_window_type, XA_ATOM, 32, PropModeReplace, (unsigned char *)&type_dock, 1);

        // Set properties to keep the window above others and on all desktops.
        Atom wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
        Atom states[] = {
            XInternAtom(dpy, "_NET_WM_STATE_ABOVE", False),
            XInternAtom(dpy, "_NET_WM_STATE_STICKY", False),
            XInternAtom(dpy, "_NET_WM_STATE_SKIP_TASKBAR", False),
            XInternAtom(dpy, "_NET_WM_STATE_SKIP_PAGER", False)
        };
        XChangeProperty(dpy, w, wm_state, XA_ATOM, 32, PropModeReplace, (unsigned char *)states, 4);

        // Send a client message to the window manager to actively request the state change.
        XEvent ev;
        memset(&ev, 0, sizeof(ev));
        ev.xclient.type = ClientMessage;
        ev.xclient.window = w;
        ev.xclient.message_type = XInternAtom(dpy, "_NET_WM_STATE", False);
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = 1; // _NET_WM_STATE_ADD
        ev.xclient.data.l[1] = XInternAtom(dpy, "_NET_WM_STATE_ABOVE", False);
        ev.xclient.data.l[2] = XInternAtom(dpy, "_NET_WM_STATE_STICKY", False);
        ev.xclient.data.l[3] = 0;
        ev.xclient.data.l[4] = 0;

        XSendEvent(dpy, root, False, SubstructureRedirectMask | SubstructureNotifyMask, &ev);

        XFlush(dpy);
    }
#endif

    glfwSetWindowPos(window, 0, 0); // Ensure window is at top-left of the screen
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Set key callback
    glfwSetKeyCallback(window, glfw_key_callback);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // REMOVED: Only F1 keybind should be active
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_NoMouse; // Tell ImGui not to capture mouse input
    io.ConfigFlags |= ImGuiConfigFlags_NoKeyboard; // Tell ImGui not to capture keyboard input

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    // Pass false to ImGui_ImplGlfw_InitForOpenGL to prevent it from installing its own key/mouse callbacks
    ImGui_ImplGlfw_InitForOpenGL(window, false); 
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Main loop
    double last_frame_time = glfwGetTime();
    while (!glfwWindowShouldClose(window))
    {
        // Calculate FPS
        double current_time = glfwGetTime();
        double delta_time = current_time - last_frame_time;
        last_frame_time = current_time;
        if (delta_time > 0.0) {
            systemMonitor.setFps(1.0 / delta_time);
        }

        // Update system stats every second (or so)
        static double last_stat_update_time = 0.0;
        if (current_time - last_stat_update_time > 1.0) {
            systemMonitor.update();
            last_stat_update_time = current_time;
        }

        // Handle fading effect
        if (current_time < fade_start_time + fade_duration) {
            float t = (current_time - fade_start_time) / fade_duration;
            // Use a simple linear interpolation for fading
            window_alpha = (1.0f - t) * (show_stats_window ? 0.0f : 1.0f) + t * (show_stats_window ? 1.0f : 0.0f);
        } else {
            window_alpha = target_alpha;
        }

        // Update hue for color cycling
        current_hue += hue_speed * delta_time;
        if (current_hue >= 1.0f) {
            current_hue -= 1.0f;
        }
        ImVec4 text_color;
        ImGui::ColorConvertHSVtoRGB(current_hue, 1.0f, 1.0f, text_color.x, text_color.y, text_color.z);
        text_color.w = window_alpha; // Apply window alpha to text color

        // Poll and handle events (inputs, window resize, etc.)
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Render system stats window
        if (window_alpha > 0.0f) // Only render if not fully transparent
        {
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(window_alpha); // Apply fading alpha
            ImGui::Begin("System Stats", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);

            ImGui::TextColored(text_color, "CPU: %.2f%%", systemMonitor.getCpuUsage());
            ImGui::TextColored(text_color, "CPU Temp: %d C", systemMonitor.getCpuTemperature());
            ImGui::TextColored(text_color, "Mem: %lld MB / %lld MB (%.2f%%)",
                        systemMonitor.getUsedMemory() / 1024, systemMonitor.getTotalMemory() / 1024,
                        (double)systemMonitor.getUsedMemory() / systemMonitor.getTotalMemory() * 100.0);
            ImGui::TextColored(text_color, "Net Down: %.2f KB/s", systemMonitor.getDownloadSpeed() / 1024.0);
            ImGui::TextColored(text_color, "Net Up: %.2f KB/s", systemMonitor.getUploadSpeed() / 1024.0);
            ImGui::TextColored(text_color, "Ping: %.2f ms", systemMonitor.getPingLatency());
            ImGui::TextColored(text_color, "GPU Usage: %.2f%%", systemMonitor.getGpuUsage());
            ImGui::TextColored(text_color, "GPU Mem: %lld MB / %lld MB", systemMonitor.getGpuMemoryUsed(), systemMonitor.getGpuMemoryTotal());
            ImGui::TextColored(text_color, "GPU Temp: %d C", systemMonitor.getGpuTemperature());
            ImGui::TextColored(text_color, "FPS: %.2f", systemMonitor.getFps());

            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Clear with transparent black
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}