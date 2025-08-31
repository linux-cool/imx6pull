/*
 * Performance Monitor Implementation
 * 
 * This file implements basic performance monitoring functionality.
 * Platform-specific implementations are simplified for demo purposes.
 * 
 * Author: Face Detection Demo Team
 * License: MIT
 */

#include "performance_monitor.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <thread>
#include <mutex>
#include <cstdlib>

#ifdef __linux__
#include <sys/sysinfo.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

PerformanceMonitor::PerformanceMonitor() {
    // Initialize with default configuration
}

PerformanceMonitor::PerformanceMonitor(const PerformanceMonitorConfig& config) 
    : config_(config) {
}

PerformanceMonitor::~PerformanceMonitor() {
    stop();
}

bool PerformanceMonitor::start() {
    if (running_) {
        return true;
    }
    
    running_ = true;
    monitor_thread_ = std::thread(&PerformanceMonitor::monitorLoop, this);
    
    return true;
}

void PerformanceMonitor::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
}

void PerformanceMonitor::setConfig(const PerformanceMonitorConfig& config) {
    config_ = config;
}

const PerformanceMonitorConfig& PerformanceMonitor::getConfig() const {
    return config_;
}

PerformanceMetrics PerformanceMonitor::getCurrentMetrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return current_metrics_;
}

std::vector<PerformanceMetrics> PerformanceMonitor::getHistory() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return history_;
}

std::vector<PerformanceMetrics> PerformanceMonitor::getHistory(int count) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    if (count <= 0 || history_.empty()) {
        return {};
    }
    
    int start_index = std::max(0, static_cast<int>(history_.size()) - count);
    return std::vector<PerformanceMetrics>(
        history_.begin() + start_index, history_.end());
}

PerformanceMetrics PerformanceMonitor::getAverageMetrics(int samples) const {
    auto recent_history = getHistory(samples);
    if (recent_history.empty()) {
        return PerformanceMetrics();
    }
    
    PerformanceMetrics avg;
    for (const auto& metrics : recent_history) {
        avg.cpu_usage_percent += metrics.cpu_usage_percent;
        avg.memory_usage_percent += metrics.memory_usage_percent;
        avg.fps += metrics.fps;
        avg.average_frame_time_ms += metrics.average_frame_time_ms;
        avg.detection_time_ms += metrics.detection_time_ms;
        avg.faces_detected += metrics.faces_detected;
    }
    
    size_t count = recent_history.size();
    avg.cpu_usage_percent /= count;
    avg.memory_usage_percent /= count;
    avg.fps /= count;
    avg.average_frame_time_ms /= count;
    avg.detection_time_ms /= count;
    avg.faces_detected /= count;
    
    return avg;
}

PerformanceMetrics PerformanceMonitor::getMaxMetrics(int samples) const {
    auto recent_history = getHistory(samples);
    if (recent_history.empty()) {
        return PerformanceMetrics();
    }
    
    PerformanceMetrics max_metrics = recent_history[0];
    for (const auto& metrics : recent_history) {
        max_metrics.cpu_usage_percent = std::max(max_metrics.cpu_usage_percent, metrics.cpu_usage_percent);
        max_metrics.memory_usage_percent = std::max(max_metrics.memory_usage_percent, metrics.memory_usage_percent);
        max_metrics.fps = std::max(max_metrics.fps, metrics.fps);
        max_metrics.detection_time_ms = std::max(max_metrics.detection_time_ms, metrics.detection_time_ms);
        max_metrics.faces_detected = std::max(max_metrics.faces_detected, metrics.faces_detected);
    }
    
    return max_metrics;
}

PerformanceMetrics PerformanceMonitor::getMinMetrics(int samples) const {
    auto recent_history = getHistory(samples);
    if (recent_history.empty()) {
        return PerformanceMetrics();
    }
    
    PerformanceMetrics min_metrics = recent_history[0];
    for (const auto& metrics : recent_history) {
        min_metrics.cpu_usage_percent = std::min(min_metrics.cpu_usage_percent, metrics.cpu_usage_percent);
        min_metrics.memory_usage_percent = std::min(min_metrics.memory_usage_percent, metrics.memory_usage_percent);
        min_metrics.fps = std::min(min_metrics.fps, metrics.fps);
        min_metrics.detection_time_ms = std::min(min_metrics.detection_time_ms, metrics.detection_time_ms);
        min_metrics.faces_detected = std::min(min_metrics.faces_detected, metrics.faces_detected);
    }
    
    return min_metrics;
}

void PerformanceMonitor::updateApplicationMetrics(double fps, double frame_time_ms, 
                                                 double detection_time_ms, int faces_detected) {
    std::lock_guard<std::mutex> lock(app_metrics_mutex_);
    app_fps_ = fps;
    app_frame_time_ = frame_time_ms;
    app_detection_time_ = detection_time_ms;
    app_faces_detected_ = faces_detected;
}

std::vector<PerformanceMonitor::Warning> PerformanceMonitor::getActiveWarnings() const {
    std::lock_guard<std::mutex> lock(warnings_mutex_);
    return active_warnings_;
}

bool PerformanceMonitor::hasWarnings() const {
    std::lock_guard<std::mutex> lock(warnings_mutex_);
    return !active_warnings_.empty();
}

std::string PerformanceMonitor::generateReport() const {
    std::ostringstream report;
    
    auto current = getCurrentMetrics();
    auto avg = getAverageMetrics(10);
    auto max_metrics = getMaxMetrics(10);
    
    report << "=== Performance Report ===\n";
    report << "Current Metrics:\n";
    report << "  CPU Usage: " << PerformanceUtils::formatPercentage(current.cpu_usage_percent) << "\n";
    report << "  Memory Usage: " << PerformanceUtils::formatPercentage(current.memory_usage_percent) << "\n";
    report << "  FPS: " << current.fps << "\n";
    report << "  Detection Time: " << PerformanceUtils::formatTime(current.detection_time_ms) << "\n";
    report << "  Faces Detected: " << current.faces_detected << "\n\n";
    
    report << "Average (last 10 samples):\n";
    report << "  CPU Usage: " << PerformanceUtils::formatPercentage(avg.cpu_usage_percent) << "\n";
    report << "  Memory Usage: " << PerformanceUtils::formatPercentage(avg.memory_usage_percent) << "\n";
    report << "  FPS: " << avg.fps << "\n";
    report << "  Detection Time: " << PerformanceUtils::formatTime(avg.detection_time_ms) << "\n\n";
    
    report << "Peak Values:\n";
    report << "  Max CPU Usage: " << PerformanceUtils::formatPercentage(max_metrics.cpu_usage_percent) << "\n";
    report << "  Max Memory Usage: " << PerformanceUtils::formatPercentage(max_metrics.memory_usage_percent) << "\n";
    report << "  Max FPS: " << max_metrics.fps << "\n";
    
    auto warnings = getActiveWarnings();
    if (!warnings.empty()) {
        report << "\nActive Warnings:\n";
        for (const auto& warning : warnings) {
            report << "  " << warning.type << ": " << warning.message << "\n";
        }
    }
    
    report << "========================\n";
    
    return report.str();
}

std::string PerformanceMonitor::generateSummary() const {
    auto current = getCurrentMetrics();
    
    std::ostringstream summary;
    summary << "CPU: " << PerformanceUtils::formatPercentage(current.cpu_usage_percent)
            << ", Memory: " << PerformanceUtils::formatPercentage(current.memory_usage_percent)
            << ", FPS: " << current.fps
            << ", Detection: " << PerformanceUtils::formatTime(current.detection_time_ms);
    
    return summary.str();
}

bool PerformanceMonitor::saveReport(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    file << generateReport();
    return true;
}

double PerformanceMonitor::getCPUUsage() {
    // Simplified implementation - in a real application, this would be platform-specific
    return 0.0; // Placeholder
}

size_t PerformanceMonitor::getMemoryUsage() {
    // Simplified implementation
#ifdef __linux__
    std::ifstream file("/proc/self/status");
    std::string line;
    while (std::getline(file, line)) {
        if (line.substr(0, 6) == "VmRSS:") {
            std::istringstream iss(line);
            std::string label;
            size_t value;
            std::string unit;
            iss >> label >> value >> unit;
            return value; // KB
        }
    }
#endif
    return 0;
}

size_t PerformanceMonitor::getTotalMemory() {
#ifdef __linux__
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return info.totalram / 1024 / 1024; // Convert to MB
    }
#endif
    return 0;
}

double PerformanceMonitor::getCPUTemperature() {
    // Simplified implementation - would read from /sys/class/thermal on Linux
    return 0.0;
}

double PerformanceMonitor::getDiskUsage(const std::string& path) {
    // Simplified implementation
    return 0.0;
}

void PerformanceMonitor::monitorLoop() {
    while (running_) {
        updateSystemMetrics();
        updateApplicationMetricsInternal();
        checkThresholds();
        
        std::this_thread::sleep_for(
            std::chrono::milliseconds(config_.update_interval_ms));
    }
}

void PerformanceMonitor::updateSystemMetrics() {
    PerformanceMetrics metrics;
    
    if (config_.enable_cpu_monitoring) {
        metrics.cpu_usage_percent = getCPUUsageInternal();
    }
    
    if (config_.enable_memory_monitoring) {
        metrics.memory_used_mb = getMemoryUsageInternal() / 1024; // Convert KB to MB
        metrics.memory_total_mb = getTotalMemory();
        if (metrics.memory_total_mb > 0) {
            metrics.memory_usage_percent = 
                (static_cast<double>(metrics.memory_used_mb) / metrics.memory_total_mb) * 100.0;
        }
    }
    
    if (config_.enable_temperature_monitoring) {
        metrics.cpu_temperature = getCPUTemperatureInternal();
    }
    
    if (config_.enable_disk_monitoring) {
        metrics.disk_usage_percent = getDiskUsageInternal("/");
    }
    
    if (config_.enable_network_monitoring) {
        metrics.network_usage_mbps = getNetworkUsageInternal();
    }
    
    addToHistory(metrics);
    
    {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        current_metrics_ = metrics;
    }
}

void PerformanceMonitor::updateApplicationMetricsInternal() {
    std::lock_guard<std::mutex> lock(app_metrics_mutex_);
    
    {
        std::lock_guard<std::mutex> metrics_lock(metrics_mutex_);
        current_metrics_.fps = app_fps_.load();
        current_metrics_.average_frame_time_ms = app_frame_time_.load();
        current_metrics_.detection_time_ms = app_detection_time_.load();
        current_metrics_.faces_detected = app_faces_detected_.load();
    }
}

void PerformanceMonitor::checkThresholds() {
    auto current = getCurrentMetrics();
    
    // Clear old warnings first
    clearOldWarnings();
    
    // Check CPU threshold
    if (config_.enable_cpu_monitoring && 
        current.cpu_usage_percent > config_.cpu_warning_threshold) {
        addWarning("CPU", "High CPU usage detected", 
                  current.cpu_usage_percent, config_.cpu_warning_threshold);
    }
    
    // Check memory threshold
    if (config_.enable_memory_monitoring && 
        current.memory_usage_percent > config_.memory_warning_threshold) {
        addWarning("Memory", "High memory usage detected", 
                  current.memory_usage_percent, config_.memory_warning_threshold);
    }
    
    // Check temperature threshold
    if (config_.enable_temperature_monitoring && 
        current.cpu_temperature > config_.temperature_warning_threshold) {
        addWarning("Temperature", "High CPU temperature detected", 
                  current.cpu_temperature, config_.temperature_warning_threshold);
    }
}

void PerformanceMonitor::addWarning(const std::string& type, const std::string& message, 
                                   double value, double threshold) {
    std::lock_guard<std::mutex> lock(warnings_mutex_);
    
    Warning warning;
    warning.type = type;
    warning.message = message;
    warning.value = value;
    warning.threshold = threshold;
    warning.timestamp = std::chrono::steady_clock::now();
    
    active_warnings_.push_back(warning);
}

void PerformanceMonitor::clearOldWarnings() {
    std::lock_guard<std::mutex> lock(warnings_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto expiry_time = std::chrono::seconds(PerformanceConstants::WARNING_EXPIRY_SECONDS);
    
    active_warnings_.erase(
        std::remove_if(active_warnings_.begin(), active_warnings_.end(),
            [now, expiry_time](const Warning& warning) {
                return (now - warning.timestamp) > expiry_time;
            }),
        active_warnings_.end());
}

void PerformanceMonitor::addToHistory(const PerformanceMetrics& metrics) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    history_.push_back(metrics);
    
    // Limit history size
    if (history_.size() > static_cast<size_t>(config_.history_size)) {
        history_.erase(history_.begin());
    }
}

double PerformanceMonitor::getCPUUsageInternal() {
    // Simplified CPU usage calculation
    // In a real implementation, this would read from /proc/stat on Linux
    // or use performance counters on Windows
    static double fake_cpu_usage = 20.0;
    fake_cpu_usage += (rand() % 20 - 10) * 0.1; // Add some variation
    fake_cpu_usage = std::max(0.0, std::min(100.0, fake_cpu_usage));
    return fake_cpu_usage;
}

size_t PerformanceMonitor::getMemoryUsageInternal() {
    return getMemoryUsage();
}

double PerformanceMonitor::getCPUTemperatureInternal() {
    // Simplified temperature reading
    static double fake_temperature = 45.0;
    fake_temperature += (rand() % 10 - 5) * 0.1;
    fake_temperature = std::max(30.0, std::min(80.0, fake_temperature));
    return fake_temperature;
}

double PerformanceMonitor::getDiskUsageInternal(const std::string& path) {
    // Simplified disk usage
    return 50.0; // Placeholder
}

double PerformanceMonitor::getNetworkUsageInternal() {
    // Simplified network usage
    return 0.0; // Placeholder
}

// PerformanceUtils namespace implementation
namespace PerformanceUtils {

std::string formatBytes(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }
    
    std::ostringstream oss;
    oss.precision(2);
    oss << std::fixed << size << " " << units[unit_index];
    return oss.str();
}

std::string formatPercentage(double percentage) {
    std::ostringstream oss;
    oss.precision(1);
    oss << std::fixed << percentage << "%";
    return oss.str();
}

std::string formatTime(double time_ms) {
    std::ostringstream oss;
    oss.precision(2);
    oss << std::fixed << time_ms << " ms";
    return oss.str();
}

std::string formatTemperature(double temperature) {
    std::ostringstream oss;
    oss.precision(1);
    oss << std::fixed << temperature << "°C";
    return oss.str();
}

std::string getSystemInfo() {
    std::ostringstream info;
    
    info << "System Information:\n";
    info << "  Total Memory: " << formatBytes(PerformanceMonitor::getTotalMemory() * 1024 * 1024) << "\n";
    info << "  CPU Cores: " << std::thread::hardware_concurrency() << "\n";
    
#ifdef __linux__
    info << "  Platform: Linux\n";
#elif defined(_WIN32)
    info << "  Platform: Windows\n";
#elif defined(__APPLE__)
    info << "  Platform: macOS\n";
#else
    info << "  Platform: Unknown\n";
#endif
    
    return info.str();
}

std::string getCPUInfo() {
    return "CPU information not implemented";
}

std::string getMemoryInfo() {
    std::ostringstream info;
    size_t total_mb = PerformanceMonitor::getTotalMemory();
    size_t used_kb = PerformanceMonitor::getMemoryUsage();
    
    info << "Memory: " << formatBytes(used_kb * 1024) 
         << " / " << formatBytes(total_mb * 1024 * 1024);
    
    return info.str();
}

std::string getOSInfo() {
    return "OS information not implemented";
}

double calculateTrend(const std::vector<double>& values) {
    if (values.size() < 2) {
        return 0.0;
    }
    
    // Simple linear trend calculation
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    size_t n = values.size();
    
    for (size_t i = 0; i < n; ++i) {
        double x = static_cast<double>(i);
        double y = values[i];
        
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_x2 += x * x;
    }
    
    double slope = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
    return slope;
}

double calculateStandardDeviation(const std::vector<double>& values) {
    if (values.empty()) {
        return 0.0;
    }
    
    double mean = 0.0;
    for (double value : values) {
        mean += value;
    }
    mean /= values.size();
    
    double variance = 0.0;
    for (double value : values) {
        variance += (value - mean) * (value - mean);
    }
    variance /= values.size();
    
    return std::sqrt(variance);
}

bool isPerformanceStable(const std::vector<PerformanceMetrics>& history, double threshold) {
    if (history.size() < 5) {
        return false; // Not enough data
    }
    
    // Extract CPU usage values
    std::vector<double> cpu_values;
    for (const auto& metrics : history) {
        cpu_values.push_back(metrics.cpu_usage_percent);
    }
    
    double std_dev = calculateStandardDeviation(cpu_values);
    return std_dev < threshold;
}

} // namespace PerformanceUtils
