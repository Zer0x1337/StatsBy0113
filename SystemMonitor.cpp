#include "SystemMonitor.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <unistd.h> // For sysconf(_SC_CLK_TCK)
#include <array>
#include <cstdio>
#include <algorithm>
#include <memory>
#include <dirent.h> // For directory listing

// Helper function to execute a shell command and capture its output
std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        // std::cerr << "popen() failed for command: " << cmd << std::endl;
        return ""; // Return empty string on failure
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
      fps(0.0)
{
    lastUpdateTime = std::chrono::steady_clock::now();
    // Initialize prevCpuStats and prevNetworkStats on first run
    updateCpuStats();
    updateNetworkStats();
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
    cpuTemperature = 0; // Default to 0 if not found
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir("/sys/class/thermal/")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            std::string entry_name = ent->d_name;
            if (entry_name.rfind("thermal_zone", 0) == 0) { // Check if it starts with "thermal_zone"
                std::string temp_path = std::string("/sys/class/thermal/") + entry_name + "/temp";
                std::ifstream temp_file(temp_path);
                if (temp_file.is_open()) {
                    int temp_milli_celsius;
                    temp_file >> temp_milli_celsius;
                    cpuTemperature = temp_milli_celsius / 1000; // Convert to Celsius
                    temp_file.close();
                    // Assuming the first valid temperature found is sufficient for CPU temp
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
            totalMemory = value; // in KB
        } else if (key == "MemAvailable:") {
            availableMemory = value; // in KB
        }
    }
    usedMemory = totalMemory - availableMemory;
}

void SystemMonitor::updateNetworkStats() {
    std::ifstream file("/proc/net/dev");
    std::string line;
    std::getline(file, line); // Skip header line 1
    std::getline(file, line); // Skip header line 2

    long long currentRxBytes = 0;
    long long currentTxBytes = 0;

    std::map<std::string, NetworkInterfaceStats> currentNetworkStats;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string interfaceName;
        iss >> interfaceName; // Read interface name (e.g., "eth0:")
        interfaceName.pop_back(); // Remove trailing ':'

        NetworkInterfaceStats stats;
        // Read values: Rx bytes, Rx packets, Rx errs, Rx drop, Rx fifo, Rx frame, Rx compressed, Rx multicast
        //               Tx bytes, Tx packets, Tx errs, Tx drop, Tx fifo, Tx colls, Tx carrier, Tx compressed
        iss >> stats.rx_bytes; // Rx bytes
        for (int i = 0; i < 7; ++i) { std::string dummy; iss >> dummy; } // Skip other Rx stats
        iss >> stats.tx_bytes; // Tx bytes

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
    // Ping Google's DNS server (8.8.8.8) as a common reliable target
    std::string pingOutput = exec("ping -c 1 -W 1 8.8.8.8");
    std::istringstream iss(pingOutput);
    std::string line;
    pingLatency = 0.0; // Default to 0 if ping fails

    while (std::getline(iss, line)) {
        if (line.find("time=") != std::string::npos) {
            size_t timePos = line.find("time=") + 5; // +5 to skip "time="
            size_t msPos = line.find(" ms", timePos);
            if (msPos != std::string::npos) {
                std::string timeStr = line.substr(timePos, msPos - timePos);
                try {
                    pingLatency = std::stod(timeStr);
                } catch (const std::exception& e) {
                    // Handle conversion error, keep latency as 0.0
                    std::cerr << "Error parsing ping time: " << e.what() << std::endl;
                }
            }
            break; // Found the time line, no need to parse further
        }
    }
}

void SystemMonitor::updateGpuStats() {
    // Reset GPU stats
    gpuUsage = 0.0;
    gpuMemoryUsed = 0;
    gpuMemoryTotal = 0;
    gpuTemperature = 0;

    // Try to get NVIDIA GPU stats using nvidia-smi
    std::string nvidiaSmiOutput = exec("nvidia-smi --query-gpu=utilization.gpu,memory.used,memory.total,temperature.gpu --format=csv,noheader,nounits");
    if (!nvidiaSmiOutput.empty()) {
        std::istringstream iss(nvidiaSmiOutput);
        std::string line;
        if (std::getline(iss, line)) {
            std::replace(line.begin(), line.end(), ',', ' '); // Replace commas with spaces for easier parsing
            std::istringstream lineStream(line);
            lineStream >> gpuUsage >> gpuMemoryUsed >> gpuMemoryTotal >> gpuTemperature;
        }
    } else {
        // Fallback or N/A if nvidia-smi is not available or fails
        // You could add logic here to try other tools for AMD/Intel GPUs
        // For now, just leave as 0 or indicate N/A
    }
}

void SystemMonitor::update() {
    updateCpuStats();
    updateCpuTemperature(); // Call CPU temperature update
    updateMemoryStats();
    updateNetworkStats();
    updatePingStats();
    updateGpuStats(); // Call GPU stats update
    lastUpdateTime = std::chrono::steady_clock::now();
}
