/*
 * Performance Monitor Header
 * 
 * This header defines the performance monitoring interface for tracking
 * system resources and application performance metrics.
 * 
 * Author: Face Detection Demo Team
 * License: MIT
 */

#ifndef PERFORMANCE_MONITOR_H
#define PERFORMANCE_MONITOR_H

#include <chrono>
#include <atomic>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <cmath>

// Performance metrics structure
struct PerformanceMetrics {
    // CPU metrics
    double cpu_usage_percent = 0.0;
    double cpu_temperature = 0.0;
    
    // Memory metrics
    size_t memory_used_mb = 0;
    size_t memory_total_mb = 0;
    double memory_usage_percent = 0.0;
    
    // Application metrics
    double fps = 0.0;
    double average_frame_time_ms = 0.0;
    double detection_time_ms = 0.0;
    int faces_detected = 0;
    
    // System metrics
    double disk_usage_percent = 0.0;
    double network_usage_mbps = 0.0;
    
    // Timestamp
    std::chrono::steady_clock::time_point timestamp;
    
    PerformanceMetrics() {
        timestamp = std::chrono::steady_clock::now();
    }
};

// Performance monitor configuration
struct PerformanceMonitorConfig {
    bool enable_cpu_monitoring = true;
    bool enable_memory_monitoring = true;
    bool enable_disk_monitoring = false;
    bool enable_network_monitoring = false;
    bool enable_temperature_monitoring = false;
    
    int update_interval_ms = 1000;  // Update interval in milliseconds
    int history_size = 60;          // Number of historical samples to keep
    
    // Thresholds for warnings
    double cpu_warning_threshold = 80.0;
    double memory_warning_threshold = 90.0;
    double temperature_warning_threshold = 70.0;
    
    // Logging
    bool enable_logging = false;
    std::string log_file_path = "performance.log";
};

// Performance monitor class
class PerformanceMonitor {
public:
    PerformanceMonitor();
    explicit PerformanceMonitor(const PerformanceMonitorConfig& config);
    ~PerformanceMonitor();
    
    // Control
    bool start();
    void stop();
    bool isRunning() const { return running_; }
    
    // Configuration
    void setConfig(const PerformanceMonitorConfig& config);
    const PerformanceMonitorConfig& getConfig() const;
    
    // Metrics access
    PerformanceMetrics getCurrentMetrics() const;
    std::vector<PerformanceMetrics> getHistory() const;
    std::vector<PerformanceMetrics> getHistory(int count) const;
    
    // Statistics
    PerformanceMetrics getAverageMetrics(int samples = 10) const;
    PerformanceMetrics getMaxMetrics(int samples = 10) const;
    PerformanceMetrics getMinMetrics(int samples = 10) const;
    
    // Application-specific metrics
    void updateApplicationMetrics(double fps, double frame_time_ms, 
                                 double detection_time_ms, int faces_detected);
    
    // Warnings and alerts
    struct Warning {
        std::string type;
        std::string message;
        double value;
        double threshold;
        std::chrono::steady_clock::time_point timestamp;
    };
    
    std::vector<Warning> getActiveWarnings() const;
    bool hasWarnings() const;
    
    // Reporting
    std::string generateReport() const;
    std::string generateSummary() const;
    bool saveReport(const std::string& filename) const;
    
    // Utility methods
    static double getCPUUsage();
    static size_t getMemoryUsage();
    static size_t getTotalMemory();
    static double getCPUTemperature();
    static double getDiskUsage(const std::string& path = "/");
    
private:
    // Configuration
    PerformanceMonitorConfig config_;
    
    // State
    std::atomic<bool> running_{false};
    std::atomic<bool> initialized_{false};
    
    // Metrics storage
    mutable std::mutex metrics_mutex_;
    std::vector<PerformanceMetrics> history_;
    PerformanceMetrics current_metrics_;
    
    // Application metrics
    mutable std::mutex app_metrics_mutex_;
    std::atomic<double> app_fps_{0.0};
    std::atomic<double> app_frame_time_{0.0};
    std::atomic<double> app_detection_time_{0.0};
    std::atomic<int> app_faces_detected_{0};
    
    // Monitoring thread
    std::thread monitor_thread_;
    
    // Warnings
    mutable std::mutex warnings_mutex_;
    std::vector<Warning> active_warnings_;
    
    // Private methods
    void monitorLoop();
    void updateSystemMetrics();
    void updateApplicationMetricsInternal();
    void checkThresholds();
    void addWarning(const std::string& type, const std::string& message, 
                   double value, double threshold);
    void clearOldWarnings();
    void addToHistory(const PerformanceMetrics& metrics);
    
    // Platform-specific implementations
    double getCPUUsageInternal();
    size_t getMemoryUsageInternal();
    double getCPUTemperatureInternal();
    double getDiskUsageInternal(const std::string& path);
    double getNetworkUsageInternal();
    
    // Non-copyable
    PerformanceMonitor(const PerformanceMonitor&) = delete;
    PerformanceMonitor& operator=(const PerformanceMonitor&) = delete;
};

// Utility functions
namespace PerformanceUtils {
    // Formatting
    std::string formatBytes(size_t bytes);
    std::string formatPercentage(double percentage);
    std::string formatTime(double time_ms);
    std::string formatTemperature(double temperature);
    
    // System information
    std::string getSystemInfo();
    std::string getCPUInfo();
    std::string getMemoryInfo();
    std::string getOSInfo();
    
    // Performance analysis
    double calculateTrend(const std::vector<double>& values);
    double calculateStandardDeviation(const std::vector<double>& values);
    bool isPerformanceStable(const std::vector<PerformanceMetrics>& history, 
                           double threshold = 10.0);
    
    // Benchmarking
    class Timer {
    public:
        Timer() : start_time_(std::chrono::high_resolution_clock::now()) {}
        
        double elapsed() const {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                end_time - start_time_);
            return duration.count() / 1000.0; // Return milliseconds
        }
        
        void reset() {
            start_time_ = std::chrono::high_resolution_clock::now();
        }
        
    private:
        std::chrono::high_resolution_clock::time_point start_time_;
    };
    
    // RAII performance measurement
    class ScopedTimer {
    public:
        ScopedTimer(const std::string& name, double* result = nullptr)
            : name_(name), result_(result) {}
        
        ~ScopedTimer() {
            double elapsed_time = timer_.elapsed();
            if (result_) {
                *result_ = elapsed_time;
            }
            // Could also log the result here
        }
        
    private:
        std::string name_;
        double* result_;
        Timer timer_;
    };
}

// Constants
namespace PerformanceConstants {
    constexpr int DEFAULT_UPDATE_INTERVAL_MS = 1000;
    constexpr int DEFAULT_HISTORY_SIZE = 60;
    constexpr double DEFAULT_CPU_WARNING_THRESHOLD = 80.0;
    constexpr double DEFAULT_MEMORY_WARNING_THRESHOLD = 90.0;
    constexpr double DEFAULT_TEMPERATURE_WARNING_THRESHOLD = 70.0;
    constexpr int WARNING_EXPIRY_SECONDS = 300; // 5 minutes
}

// Macros for easy performance measurement
#define PERF_TIMER(name) PerformanceUtils::Timer name##_timer
#define PERF_MEASURE(name, code) \
    do { \
        PerformanceUtils::Timer timer; \
        code; \
        double elapsed = timer.elapsed(); \
        /* Log or store elapsed time */ \
    } while(0)

#define PERF_SCOPED(name) PerformanceUtils::ScopedTimer scoped_timer(name)
#define PERF_SCOPED_RESULT(name, result) PerformanceUtils::ScopedTimer scoped_timer(name, &result)

#endif // PERFORMANCE_MONITOR_H
