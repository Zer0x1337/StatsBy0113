#include "SystemMonitor.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <unistd.h> 
#include <array>
#include <cstdio>
#include <algorithm>
#include <memory>
#include <dirent.h> 


std::string exec(const char* cmd) {
    // Runs a shell command and returns its output. Useful for getting system info.
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        
        return ""; 
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

SystemMonitor::SystemMonitor()
    : cpuUsage(0.0),
      cpuTemperature(0),
      totalMemory(0),
      usedMemory(0),
      availableMemory(0),
      downloadSpeed(0.0),
      uploadSpeed(0.0),
      pingLatency(0.0),
      gpuUsage(0.0),
      gpuMemoryUsed(0),
      gpuMemoryTotal(0),
      gpuTemperature(0),
      fps(0.0),
      processCpuUsage(0.0),
      processMemoryUsage(0),
      prevProcessCpuUserTime(0),
      prevProcessCpuKernelTime(0),
      prevProcessCpuTotalTime(0)
{
    lastUpdateTime = std::chrono::steady_clock::now();
    
    updateCpuStats();
    updateNetworkStats();
    updateProcessCpuStats(); 
    lastPingUpdateTime = std::chrono::steady_clock::now();
    lastGpuUpdateTime = std::chrono::steady_clock::now();
}

CpuStats SystemMonitor::parseCpuStats(const std::string& line) {
    std::istringstream iss(line);
    std::string cpuLabel;
    CpuStats stats{};
    iss >> cpuLabel >> stats.user >> stats.nice >> stats.system >> stats.idle
        >> stats.iowait >> stats.irq >> stats.softirq >> stats.steal
        >> stats.guest >> stats.guest_nice;
    return stats;
}

void SystemMonitor::updateCpuStats() {
    std::ifstream file("/proc/stat");
    std::string line;
    if (std::getline(file, line)) {
        CpuStats currentCpuStats = parseCpuStats(line);

        long long prevIdle = prevCpuStats.idle + prevCpuStats.iowait;
        long long currentIdle = currentCpuStats.idle + currentCpuStats.iowait;

        long long prevNonIdle = prevCpuStats.user + prevCpuStats.nice + prevCpuStats.system + prevCpuStats.irq + prevCpuStats.softirq + prevCpuStats.steal;
        long long currentNonIdle = currentCpuStats.user + currentCpuStats.nice + currentCpuStats.system + currentCpuStats.irq + currentCpuStats.softirq + currentCpuStats.steal;

        long long prevTotal = prevIdle + prevNonIdle;
        long long currentTotal = currentIdle + currentNonIdle;

        long long totalDiff = currentTotal - prevTotal;
        long long idleDiff = currentIdle - prevIdle;

        if (totalDiff > 0) {
            cpuUsage = (double)(totalDiff - idleDiff) / totalDiff * 100.0;
        } else {
            cpuUsage = 0.0;
        }
        prevCpuStats = currentCpuStats;
    }
}

void SystemMonitor::updateCpuTemperature() {
    cpuTemperature = 0; 
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir("/sys/class/thermal/")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            std::string entry_name = ent->d_name;
            if (entry_name.rfind("thermal_zone", 0) == 0) { 
                std::string temp_path = std::string("/sys/class/thermal/") + entry_name + "/temp";
                std::ifstream temp_file(temp_path);
                if (temp_file.is_open()) {
                    int temp_milli_celsius;
                    temp_file >> temp_milli_celsius;
                    cpuTemperature = temp_milli_celsius / 1000; 
                    temp_file.close();
                    
                    closedir(dir);
                    return;
                }
            }
        }
        closedir(dir);
    }
}

void SystemMonitor::updateMemoryStats() {
    std::ifstream file("/proc/meminfo");
    std::string line;
    totalMemory = 0;
    usedMemory = 0;
    availableMemory = 0;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        long long value;
        std::string unit;
        iss >> key >> value >> unit;

        if (key == "MemTotal:") {
            totalMemory = value; 
        } else if (key == "MemAvailable:") {
            availableMemory = value; 
        }
    }
    usedMemory = totalMemory - availableMemory;
}

void SystemMonitor::updateNetworkStats() {
    std::ifstream file("/proc/net/dev");
    std::string line;
    std::getline(file, line); 
    std::getline(file, line); 

    long long currentRxBytes = 0;
    long long currentTxBytes = 0;

    std::map<std::string, NetworkInterfaceStats> currentNetworkStats;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string interfaceName;
        iss >> interfaceName; 
        interfaceName.pop_back(); 

        NetworkInterfaceStats stats;
        
        
        iss >> stats.rx_bytes; 
        for (int i = 0; i < 7; ++i) { std::string dummy; iss >> dummy; } 
        iss >> stats.tx_bytes; 

        currentNetworkStats[interfaceName] = stats;

        currentRxBytes += stats.rx_bytes;
        currentTxBytes += stats.tx_bytes;
    }

    auto currentTime = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsedSeconds = currentTime - lastUpdateTime;

    if (elapsedSeconds.count() > 0) {
        long long prevRxBytes = 0;
        long long prevTxBytes = 0;

        for (const auto& pair : prevNetworkStats) {
            prevRxBytes += pair.second.rx_bytes;
            prevTxBytes += pair.second.tx_bytes;
        }

        downloadSpeed = (currentRxBytes - prevRxBytes) / elapsedSeconds.count();
        uploadSpeed = (currentTxBytes - prevTxBytes) / elapsedSeconds.count();
    }

    prevNetworkStats = currentNetworkStats;
}

void SystemMonitor::updatePingStats() {
    auto currentTime = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsedSeconds = currentTime - lastPingUpdateTime;

    if (elapsedSeconds.count() < 5.0) { 
        return;
    }

    
    std::string pingOutput = exec("ping -c 1 -W 1 8.8.8.8");
    lastPingUpdateTime = currentTime;
    std::istringstream iss(pingOutput);
    std::string line;
    pingLatency = 0.0; 

    while (std::getline(iss, line)) {
        if (line.find("time=") != std::string::npos) {
            size_t timePos = line.find("time=") + 5; 
            size_t msPos = line.find(" ms", timePos);
            if (msPos != std::string::npos) {
                std::string timeStr = line.substr(timePos, msPos - timePos);
                try {
                    pingLatency = std::stod(timeStr);
                } catch (const std::exception& e) {
                    
                    std::cerr << "Error parsing ping time: " << e.what() << std::endl;
                }
            }
            break; 
        }
    }
}

void SystemMonitor::updateGpuStats() {
    auto currentTime = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsedSeconds = currentTime - lastGpuUpdateTime;

    if (elapsedSeconds.count() < 5.0) { 
        return;
    }

    
    gpuUsage = 0.0;
    gpuMemoryUsed = 0;
    gpuMemoryTotal = 0;
    gpuTemperature = 0;

    
    std::string nvidiaSmiOutput = exec("nvidia-smi --query-gpu=utilization.gpu,memory.used,memory.total,temperature.gpu --format=csv,noheader,nounits");
    lastGpuUpdateTime = currentTime;
    if (!nvidiaSmiOutput.empty()) {
        std::istringstream iss(nvidiaSmiOutput);
        std::string line;
        if (std::getline(iss, line)) {
            std::replace(line.begin(), line.end(), ',', ' '); 
            std::istringstream lineStream(line);
            lineStream >> gpuUsage >> gpuMemoryUsed >> gpuMemoryTotal >> gpuTemperature;
        }
    } else {
        
        
        
    }
}

long SystemMonitor::getSystemUptime() {
    std::ifstream file("/proc/uptime");
    std::string line;
    if (std::getline(file, line)) {
        std::istringstream iss(line);
        double uptime_seconds;
        iss >> uptime_seconds;
        return static_cast<long>(uptime_seconds);
    }
    return 0;
}

long SystemMonitor::getProcessStartTime() {
    std::ifstream file("/proc/self/stat");
    std::string line;
    if (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string dummy;
        long start_time_jiffies;
        for (int i = 0; i < 21; ++i) iss >> dummy; 
        iss >> start_time_jiffies;
        return start_time_jiffies;
    }
    return 0;
}

long SystemMonitor::getClockTicksPerSecond() {
    return sysconf(_SC_CLK_TCK);
}

void SystemMonitor::updateProcessCpuStats() {
    // Calculates how much CPU this specific app is using. Really annoying.
    std::ifstream file("/proc/self/stat");
    std::string line;
    if (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string dummy;
        long utime, stime, cutime, cstime;

        
        for (int i = 0; i < 13; ++i) iss >> dummy;
        iss >> utime >> stime >> cutime >> cstime;

        long currentProcessCpuUserTime = utime + cutime;
        long currentProcessCpuKernelTime = stime + cstime;
        long currentProcessCpuTotalTime = currentProcessCpuUserTime + currentProcessCpuKernelTime;

        if (prevProcessCpuTotalTime > 0) {
            long long totalCpuTimeDiff = currentProcessCpuTotalTime - prevProcessCpuTotalTime;
            long long totalSystemCpuTimeDiff = 0; 

            
            std::ifstream statFile("/proc/stat");
            std::string statLine;
            if (std::getline(statFile, statLine)) {
                CpuStats currentSystemCpuStats = parseCpuStats(statLine);
                long long currentSystemTotal = currentSystemCpuStats.user + currentSystemCpuStats.nice +
                                               currentSystemCpuStats.system + currentSystemCpuStats.idle +
                                               currentSystemCpuStats.iowait + currentSystemCpuStats.irq +
                                               currentSystemCpuStats.softirq + currentSystemCpuStats.steal +
                                               currentSystemCpuStats.guest + currentSystemCpuStats.guest_nice;

                long long prevSystemTotal = prevCpuStats.user + prevCpuStats.nice +
                                            prevCpuStats.system + prevCpuStats.idle +
                                            prevCpuStats.iowait + prevCpuStats.irq +
                                            prevCpuStats.softirq + prevCpuStats.steal +
                                            prevCpuStats.guest + prevCpuStats.guest_nice;
                totalSystemCpuTimeDiff = currentSystemTotal - prevSystemTotal;
            }

            if (totalSystemCpuTimeDiff > 0) {
                processCpuUsage = (static_cast<double>(totalCpuTimeDiff) / totalSystemCpuTimeDiff) * 100.0;
            } else {
                processCpuUsage = 0.0;
            }
        } else {
            processCpuUsage = 0.0;
        }

        prevProcessCpuUserTime = currentProcessCpuUserTime;
        prevProcessCpuKernelTime = currentProcessCpuKernelTime;
        prevProcessCpuTotalTime = currentProcessCpuTotalTime;
    }
}

void SystemMonitor::updateProcessMemoryStats() {
    std::ifstream file("/proc/self/status");
    std::string line;
    processMemoryUsage = 0; 

    while (std::getline(file, line)) {
        if (line.rfind("VmRSS:", 0) == 0) { 
            std::istringstream iss(line);
            std::string key;
            long long value;
            std::string unit;
            iss >> key >> value >> unit;
            processMemoryUsage = value; 
            break;
        }
    }
}

void SystemMonitor::update() {
    updateCpuStats();
    updateCpuTemperature(); 
    updateMemoryStats();
    updateNetworkStats();
    updatePingStats();
    updateGpuStats(); 
    updateProcessCpuStats(); 
    updateProcessMemoryStats(); 
    lastUpdateTime = std::chrono::steady_clock::now();
}
