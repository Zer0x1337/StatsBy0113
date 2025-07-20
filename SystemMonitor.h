#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include <string>
#include <vector>
#include <map>
#include <chrono>

struct CpuStats {
    long long user;
    long long nice;
    long long system;
    long long idle;
    long long iowait;
    long long irq;
    long long softirq;
    long long steal;
    long long guest;
    long long guest_nice;
};

struct NetworkInterfaceStats {
    long long rx_bytes;
    long long tx_bytes;
};

class SystemMonitor {
public:
    SystemMonitor();
    void update();

    // CPU
    double getCpuUsage() const { return cpuUsage; }
    int getCpuTemperature() const { return cpuTemperature; }

    // Memory
    long long getTotalMemory() const { return totalMemory; }
    long long getUsedMemory() const { return usedMemory; }
    long long getAvailableMemory() const { return availableMemory; }

    // Network
    double getDownloadSpeed() const { return downloadSpeed; } // Bytes/sec
    double getUploadSpeed() const { return uploadSpeed; }     // Bytes/sec
    double getPingLatency() const { return pingLatency; }     // ms

    // GPU
    double getGpuUsage() const { return gpuUsage; }
    long long getGpuMemoryUsed() const { return gpuMemoryUsed; }
    long long getGpuMemoryTotal() const { return gpuMemoryTotal; }
    int getGpuTemperature() const { return gpuTemperature; }

    // FPS (calculated externally, but stored here for convenience)
    double getFps() const { return fps; }
    void setFps(double newFps) { fps = newFps; }

private:
    // CPU
    CpuStats prevCpuStats;
    double cpuUsage;
    int cpuTemperature;
    void updateCpuStats();
    void updateCpuTemperature();
    CpuStats parseCpuStats(const std::string& line);

    // Memory
    long long totalMemory;
    long long usedMemory;
    long long availableMemory;
    void updateMemoryStats();

    // Network
    std::map<std::string, NetworkInterfaceStats> prevNetworkStats;
    double downloadSpeed;
    double uploadSpeed;
    double pingLatency;
    void updateNetworkStats();
    void updatePingStats();

    // GPU
    double gpuUsage;
    long long gpuMemoryUsed;
    long long gpuMemoryTotal;
    int gpuTemperature;
    void updateGpuStats();

    // Time tracking for speed calculations
    std::chrono::steady_clock::time_point lastUpdateTime;

    // FPS
    double fps;
};

#endif // SYSTEM_MONITOR_H
