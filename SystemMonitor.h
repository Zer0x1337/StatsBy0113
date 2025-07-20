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
    // This class handles all the system stat gathering. Where the evil black magic happens.
    SystemMonitor();
    void update();

    
    double getCpuUsage() const { return cpuUsage; }
    int getCpuTemperature() const { return cpuTemperature; }

    
    long long getTotalMemory() const { return totalMemory; }
    long long getUsedMemory() const { return usedMemory; }
    long long getAvailableMemory() const { return availableMemory; }

    
    double getDownloadSpeed() const { return downloadSpeed; } 
    double getUploadSpeed() const { return uploadSpeed; }     
    double getPingLatency() const { return pingLatency; }     

    
    double getGpuUsage() const { return gpuUsage; }
    long long getGpuMemoryUsed() const { return gpuMemoryUsed; }
    long long getGpuMemoryTotal() const { return gpuMemoryTotal; }
    int getGpuTemperature() const { return gpuTemperature; }

    
    double getFps() const { return fps; }
    void setFps(double newFps) { fps = newFps; }

    
    double getProcessCpuUsage() const { return processCpuUsage; }
    long long getProcessMemoryUsage() const { return processMemoryUsage; }

private:
    
    CpuStats prevCpuStats;
    double cpuUsage;
    int cpuTemperature;
    void updateCpuStats();
    void updateCpuTemperature();
    CpuStats parseCpuStats(const std::string& line);

    
    long long prevProcessCpuUserTime;
    long long prevProcessCpuKernelTime;
    long long prevProcessCpuTotalTime;
    double processCpuUsage;
    long long processMemoryUsage; 
    void updateProcessCpuStats();
    void updateProcessMemoryStats();
    long getSystemUptime();
    long getProcessStartTime();
    long getClockTicksPerSecond();

    
    long long totalMemory;
    long long usedMemory;
    long long availableMemory;
    void updateMemoryStats();

    
    std::map<std::string, NetworkInterfaceStats> prevNetworkStats;
    double downloadSpeed;
    double uploadSpeed;
    double pingLatency;
    void updateNetworkStats();
    void updatePingStats();

    
    double gpuUsage;
    long long gpuMemoryUsed;
    long long gpuMemoryTotal;
    int gpuTemperature;
    void updateGpuStats();

    
    std::chrono::steady_clock::time_point lastUpdateTime;
    std::chrono::steady_clock::time_point lastPingUpdateTime;
    std::chrono::steady_clock::time_point lastGpuUpdateTime;

    
    double fps;
};

#endif 
