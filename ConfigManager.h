#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "imgui.h" 
#include <string>
#include <vector>
#include <map>

struct AppConfig {
    // Holds all the app settings. Pretty self-explanatory.
    bool enable_fading_colors = true;
    float hue_speed = 0.05f;
    ImVec4 text_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); 

    
    bool show_cpu_usage = true;
    bool show_cpu_temp = true;
    bool show_memory_stats = true;
    bool show_net_down = true;
    bool show_net_up = true;
    bool show_ping = true;
    bool show_gpu_usage = true;
    bool show_gpu_mem = true;
    bool show_gpu_temp = true;
    bool show_fps = true;

    
    bool show_app_cpu = true;
    bool show_app_mem = true;
};

class ConfigManager {
public:
    ConfigManager(const std::string& filename);
    void loadConfig(AppConfig& config);
    void saveConfig(const AppConfig& config);

private:
    std::string configFilename;
    std::string get_config_path();
};

#endif 
