#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#ifdef __linux__
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <string.h>
#endif
#include "ConfigManager.h"
#include "SystemMonitor.h"
#include <chrono>
#include <cmath>
#include <thread>

SystemMonitor systemMonitor;
AppConfig appConfig;
ConfigManager configManager("config.ini");

bool show_stats_window = true;
bool show_config_window = true;
float window_alpha = 1.0f;
float target_alpha = 1.0f;
double fade_start_time = 0.0;
const double fade_duration = 0.2;

float current_hue = 0.0f;
const float hue_speed = 0.05f;

static void glfw_error_callback(int error, const char *description) {
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

static void glfw_key_callback(GLFWwindow *window, int key, int scancode,
                              int action, int mods) {
  ImGuiIO &io = ImGui::GetIO();
  if (io.WantCaptureKeyboard) {

    return;
  }

  if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
    printf("F1 key pressed!\n");
    show_stats_window = !show_stats_window;
    fade_start_time = glfwGetTime();
    target_alpha = show_stats_window ? 1.0f : 0.0f;
  }
}

#include <string>
// #include <vector> Not used currently

bool debug_mode = false;
bool config_mode = false;

int main(int argc, char **argv) {

  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
    return 1;

  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--debug") {
      debug_mode = true;
    } else if (std::string(argv[i]) == "--config") {
      config_mode = true;
    }
  }

  configManager.loadConfig(appConfig);

  if (config_mode) {
  }

#if defined(IMGUI_IMPL_OPENGL_ES2)

  const char *glsl_version = "#version 100";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)

  const char *glsl_version = "#version 150";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else

  const char *glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

#endif

  glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
  glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
  glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
  if (!config_mode) {
    glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);
  } else {
    glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_FALSE);
  }
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
  glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);

  GLFWmonitor *primary_monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode *mode = glfwGetVideoMode(primary_monitor);

  GLFWwindow *window = glfwCreateWindow(mode->width, mode->height,
                                        "System Stats Overlay", NULL, NULL);
  if (window == NULL)
    return 1;

  glfwSetKeyCallback(window, glfw_key_callback);

#ifdef __linux__
  if (!config_mode) {
    // X11 stuff to make the window behave like an overlay/dock.
    // Basically, makes it always on top, sticky, and not show in taskbars.
    // Not working on KDE Plasma(KWin), ...
    Display *dpy = glfwGetX11Display();
    Window w = glfwGetX11Window(window);
    Window root = DefaultRootWindow(dpy);

    if (dpy && w) {
      Atom kde_net_wm_window_type_override =
          XInternAtom(dpy, "_KDE_NET_WM_WINDOW_TYPE_OVERRIDE", False);
      XChangeProperty(dpy, w, kde_net_wm_window_type_override, XA_ATOM, 32,
                      PropModeReplace,
                      (unsigned char *)&kde_net_wm_window_type_override, 1);

      Atom wm_window_type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
      Atom type_dock = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
      XChangeProperty(dpy, w, wm_window_type, XA_ATOM, 32, PropModeReplace,
                      (unsigned char *)&type_dock, 1);

      Atom wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
      Atom states[] = {XInternAtom(dpy, "_NET_WM_STATE_ABOVE", False),
                       XInternAtom(dpy, "_NET_WM_STATE_STICKY", False),
                       XInternAtom(dpy, "_NET_WM_STATE_SKIP_TASKBAR", False),
                       XInternAtom(dpy, "_NET_WM_STATE_SKIP_PAGER", False)};
      XChangeProperty(dpy, w, wm_state, XA_ATOM, 32, PropModeReplace,
                      (unsigned char *)states, 4);

      XEvent ev;
      memset(&ev, 0, sizeof(ev));
      ev.xclient.type = ClientMessage;
      ev.xclient.window = w;
      ev.xclient.message_type = XInternAtom(dpy, "_NET_WM_STATE", False);
      ev.xclient.format = 32;
      ev.xclient.data.l[0] = 1;
      ev.xclient.data.l[1] = XInternAtom(dpy, "_NET_WM_STATE_ABOVE", False);
      ev.xclient.data.l[2] = XInternAtom(dpy, "_NET_WM_STATE_STICKY", False);
      ev.xclient.data.l[3] = 0;
      ev.xclient.data.l[4] = 0;

      XSendEvent(dpy, root, False,
                 SubstructureRedirectMask | SubstructureNotifyMask, &ev);

      XFlush(dpy);
    }
  }
#endif

  glfwSetWindowPos(window, 0, 0);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  glfwSetKeyCallback(window, glfw_key_callback);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  if (config_mode) {
    io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
    io.ConfigFlags &= ~ImGuiConfigFlags_NoKeyboard;
  }

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(window, config_mode);
  ImGui_ImplOpenGL3_Init(glsl_version);

  double last_frame_time = glfwGetTime();
  while (!glfwWindowShouldClose(window)) {

    double current_time = glfwGetTime();
    double delta_time = current_time - last_frame_time;
    last_frame_time = current_time;
    if (delta_time > 0.0) {
      systemMonitor.setFps(1.0 / delta_time);
    }

    static double last_stat_update_time = 0.0;
    if (current_time - last_stat_update_time > 1.0) {
      systemMonitor.update();
      last_stat_update_time = current_time;
    }

    if (current_time < fade_start_time + fade_duration) {
      float t = (current_time - fade_start_time) / fade_duration;

      window_alpha = (1.0f - t) * (show_stats_window ? 0.0f : 1.0f) +
                     t * (show_stats_window ? 1.0f : 0.0f);
    } else {
      window_alpha = target_alpha;
    }

    ImVec4 text_color = appConfig.text_color;
    if (appConfig.enable_fading_colors) {
      current_hue += appConfig.hue_speed * delta_time;
      if (current_hue >= 1.0f) {
        current_hue -= 1.0f;
      }
      ImGui::ColorConvertHSVtoRGB(current_hue, 1.0f, 1.0f, text_color.x,
                                  text_color.y, text_color.z);
    } else {
      text_color = appConfig.text_color;
    }
    text_color.w = window_alpha;

    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (window_alpha > 0.0f) {
      ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
      ImGui::SetNextWindowBgAlpha(window_alpha);
      ImGui::Begin("System Stats", NULL,
                   ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                       ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                       ImGuiWindowFlags_AlwaysAutoResize |
                       ImGuiWindowFlags_NoSavedSettings |
                       ImGuiWindowFlags_NoBackground);

      if (appConfig.show_cpu_usage)
        ImGui::TextColored(text_color, "CPU: %.2f%%",
                           systemMonitor.getCpuUsage());
      if (appConfig.show_cpu_temp)
        ImGui::TextColored(text_color, "CPU Temp: %d C",
                           systemMonitor.getCpuTemperature());
      if (appConfig.show_memory_stats)
        ImGui::TextColored(text_color, "Mem: %lld MB / %lld MB (%.2f%%)",
                           systemMonitor.getUsedMemory() / 1024,
                           systemMonitor.getTotalMemory() / 1024,
                           (double)systemMonitor.getUsedMemory() /
                               systemMonitor.getTotalMemory() * 100.0);
      if (appConfig.show_net_down)
        ImGui::TextColored(text_color, "Net Down: %.2f KB/s",
                           systemMonitor.getDownloadSpeed() / 1024.0);
      if (appConfig.show_net_up)
        ImGui::TextColored(text_color, "Net Up: %.2f KB/s",
                           systemMonitor.getUploadSpeed() / 1024.0);
      if (appConfig.show_ping)
        ImGui::TextColored(text_color, "Ping: %.2f ms",
                           systemMonitor.getPingLatency());
      if (appConfig.show_gpu_usage)
        ImGui::TextColored(text_color, "GPU Usage: %.2f%%",
                           systemMonitor.getGpuUsage());
      if (appConfig.show_gpu_mem)
        ImGui::TextColored(text_color, "GPU Mem: %lld MB / %lld MB",
                           systemMonitor.getGpuMemoryUsed(),
                           systemMonitor.getGpuMemoryTotal());
      if (appConfig.show_gpu_temp)
        ImGui::TextColored(text_color, "GPU Temp: %d C",
                           systemMonitor.getGpuTemperature());
      if (appConfig.show_fps)
        ImGui::TextColored(text_color, "FPS: %.2f", systemMonitor.getFps());

      ImGui::End();
    }

    if (debug_mode && !config_mode) {
      ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x, 0), ImGuiCond_Always,
                              ImVec2(1, 0));
      ImGui::SetNextWindowBgAlpha(window_alpha);
      ImGui::Begin("Debug Stats", NULL,
                   ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                       ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                       ImGuiWindowFlags_AlwaysAutoResize |
                       ImGuiWindowFlags_NoSavedSettings |
                       ImGuiWindowFlags_NoBackground);

      if (appConfig.show_app_cpu)
        ImGui::TextColored(text_color, "App CPU: %.2f%%",
                           systemMonitor.getProcessCpuUsage());
      if (appConfig.show_app_mem)
        ImGui::TextColored(text_color, "App Mem: %lld KB",
                           systemMonitor.getProcessMemoryUsage());

      ImGui::End();
    }

    if (config_mode && show_config_window) {
      ImGui::SetNextWindowPos(
          ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2), ImGuiCond_Always,
          ImVec2(0.5, 0.5));
      ImGui::Begin("Configuration", &show_config_window,
                   ImGuiWindowFlags_AlwaysAutoResize |
                       ImGuiWindowFlags_NoCollapse);

      if (ImGui::CollapsingHeader("General Display")) {
        ImGui::Checkbox("Enable Fading Colors",
                        &appConfig.enable_fading_colors);
        ImGui::SliderFloat("Hue Speed", &appConfig.hue_speed, 0.01f, 0.5f,
                           "%.2f");
        ImGui::ColorEdit4("Text Color", (float *)&appConfig.text_color);
      }

      if (ImGui::CollapsingHeader("Stat Visibility")) {
        ImGui::Checkbox("Show CPU Usage", &appConfig.show_cpu_usage);
        ImGui::Checkbox("Show CPU Temp", &appConfig.show_cpu_temp);
        ImGui::Checkbox("Show Memory Stats", &appConfig.show_memory_stats);
        ImGui::Checkbox("Show Net Down", &appConfig.show_net_down);
        ImGui::Checkbox("Show Net Up", &appConfig.show_net_up);
        ImGui::Checkbox("Show Ping", &appConfig.show_ping);
        ImGui::Checkbox("Show GPU Usage", &appConfig.show_gpu_usage);
        ImGui::Checkbox("Show GPU Memory", &appConfig.show_gpu_mem);
        ImGui::Checkbox("Show GPU Temp", &appConfig.show_gpu_temp);
        ImGui::Checkbox("Show FPS", &appConfig.show_fps);
      }

      if (ImGui::CollapsingHeader("App Debug Stats (requires --debug)")) {
        ImGui::Checkbox("Show App CPU", &appConfig.show_app_cpu);
        ImGui::Checkbox("Show App Memory", &appConfig.show_app_mem);
      }

      if (ImGui::Button("Save Configuration")) {
        configManager.saveConfig(appConfig);
      }

      ImGui::End();
      if (!show_config_window) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
      }
    }

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
