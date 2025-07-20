#include "ConfigManager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <limits.h> 
#include <unistd.h> 

namespace fs = std::filesystem;

ConfigManager::ConfigManager(const std::string& filename)
    : configFilename(filename) {}

std::string ConfigManager::get_config_path() {
    
    
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count != -1) {
        return fs::path(std::string(result, count)).parent_path() / configFilename;
    }
    
    return configFilename;
}

void ConfigManager::loadConfig(AppConfig& config) {
    // Tries to load settings from the config file. If it's not there, just uses defaults.
    std::string path = get_config_path();
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Config file not found or could not be opened: " << path << ". Using default settings." << std::endl;
        return; 
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        if (std::getline(iss, key, '=')) {
            std::string value;
            if (std::getline(iss, value)) {
                
                key.erase(0, key.find_first_not_of(" \t\r\n"));
                key.erase(key.find_last_not_of(" \t\r\n") + 1);
                value.erase(0, value.find_first_not_of(" \t\r\n"));
                value.erase(value.find_last_not_of(" \t\r\n") + 1);

                if (key == "enable_fading_colors") config.enable_fading_colors = (value == "true");
                else if (key == "hue_speed") config.hue_speed = std::stof(value);
                else if (key == "text_color_r") config.text_color.x = std::stof(value);
                else if (key == "text_color_g") config.text_color.y = std::stof(value);
                else if (key == "text_color_b") config.text_color.z = std::stof(value);
                else if (key == "text_color_a") config.text_color.w = std::stof(value);
                else if (key == "show_cpu_usage") config.show_cpu_usage = (value == "true");
                else if (key == "show_cpu_temp") config.show_cpu_temp = (value == "true");
                else if (key == "show_memory_stats") config.show_memory_stats = (value == "true");
                else if (key == "show_net_down") config.show_net_down = (value == "true");
                else if (key == "show_net_up") config.show_net_up = (value == "true");
                else if (key == "show_ping") config.show_ping = (value == "true");
                else if (key == "show_gpu_usage") config.show_gpu_usage = (value == "true");
                else if (key == "show_gpu_mem") config.show_gpu_mem = (value == "true");
                else if (key == "show_gpu_temp") config.show_gpu_temp = (value == "true");
                else if (key == "show_fps") config.show_fps = (value == "true");
                else if (key == "show_app_cpu") config.show_app_cpu = (value == "true");
                else if (key == "show_app_mem") config.show_app_mem = (value == "true");
            }
        }
    }
    file.close();
}

void ConfigManager::saveConfig(const AppConfig& config) {
    // Writes the current app settings to the config file. Simple stuff.
    std::string path = get_config_path();
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Could not open config file for writing: " << path << std::endl;
        return;
    }

    file << "enable_fading_colors=" << (config.enable_fading_colors ? "true" : "false") << "\n";
    file << "hue_speed=" << config.hue_speed << "\n";
    file << "text_color_r=" << config.text_color.x << "\n";
    file << "text_color_g=" << config.text_color.y << "\n";
    file << "text_color_b=" << config.text_color.z << "\n";
    file << "text_color_a=" << config.text_color.w << "\n";
    file << "show_cpu_usage=" << (config.show_cpu_usage ? "true" : "false") << "\n";
    file << "show_cpu_temp=" << (config.show_cpu_temp ? "true" : "false") << "\n";
    file << "show_memory_stats=" << (config.show_memory_stats ? "true" : "false") << "\n";
    file << "show_net_down=" << (config.show_net_down ? "true" : "false") << "\n";
    file << "show_net_up=" << (config.show_net_up ? "true" : "false") << "\n";
    file << "show_ping=" << (config.show_ping ? "true" : "false") << "\n";
    file << "show_gpu_usage=" << (config.show_gpu_usage ? "true" : "false") << "\n";
    file << "show_gpu_mem=" << (config.show_gpu_mem ? "true" : "false") << "\n";
    file << "show_gpu_temp=" << (config.show_gpu_temp ? "true" : "false") << "\n";
    file << "show_fps=" << (config.show_fps ? "true" : "false") << "\n";
    file << "show_app_cpu=" << (config.show_app_cpu ? "true" : "false") << "\n";
    file << "show_app_mem=" << (config.show_app_mem ? "true" : "false") << "\n";

    file.close();
}
