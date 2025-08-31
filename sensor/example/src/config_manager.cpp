/*
 * Configuration Manager Implementation
 * 
 * This file implements configuration management functionality.
 * JSON support is optional and falls back to simple INI format.
 * 
 * Author: Face Detection Demo Team
 * License: MIT
 */

#include "config_manager.h"
#include "face_detection_demo.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

#ifdef HAVE_JSON
#include <nlohmann/json.hpp>
#endif

// ConfigValue implementation
void ConfigSection::setValue(const std::string& key, const ConfigValue& value) {
    values_[key] = value;
}

ConfigValue ConfigSection::getValue(const std::string& key) const {
    auto it = values_.find(key);
    return (it != values_.end()) ? it->second : ConfigValue();
}

bool ConfigSection::hasValue(const std::string& key) const {
    return values_.find(key) != values_.end();
}

void ConfigSection::setInt(const std::string& key, int value) {
    setValue(key, ConfigValue(value));
}

void ConfigSection::setDouble(const std::string& key, double value) {
    setValue(key, ConfigValue(value));
}

void ConfigSection::setBool(const std::string& key, bool value) {
    setValue(key, ConfigValue(value));
}

void ConfigSection::setString(const std::string& key, const std::string& value) {
    setValue(key, ConfigValue(value));
}

void ConfigSection::setArray(const std::string& key, const std::vector<std::string>& value) {
    setValue(key, ConfigValue(value));
}

int ConfigSection::getInt(const std::string& key, int default_value) const {
    auto value = getValue(key);
    return value.isValid() ? value.asInt() : default_value;
}

double ConfigSection::getDouble(const std::string& key, double default_value) const {
    auto value = getValue(key);
    return value.isValid() ? value.asDouble() : default_value;
}

bool ConfigSection::getBool(const std::string& key, bool default_value) const {
    auto value = getValue(key);
    return value.isValid() ? value.asBool() : default_value;
}

std::string ConfigSection::getString(const std::string& key, const std::string& default_value) const {
    auto value = getValue(key);
    return value.isValid() ? value.asString() : default_value;
}

std::vector<std::string> ConfigSection::getArray(const std::string& key) const {
    auto value = getValue(key);
    return value.isValid() ? value.asArray() : std::vector<std::string>();
}

std::vector<std::string> ConfigSection::getKeys() const {
    std::vector<std::string> keys;
    for (const auto& pair : values_) {
        keys.push_back(pair.first);
    }
    return keys;
}

// ConfigManager implementation
ConfigManager::ConfigManager() {
    setupValidationRules();
}

ConfigManager::~ConfigManager() = default;

bool ConfigManager::loadFromFile(const std::string& filename) {
    // Detect format and load accordingly
    ConfigUtils::ConfigFormat format = ConfigUtils::detectFormat(filename);

    switch (format) {
#ifdef HAVE_JSON
    case ConfigUtils::ConfigFormat::JSON:
        return loadFromJSON(filename);
#endif
    case ConfigUtils::ConfigFormat::INI:
        return loadFromINI(filename);
    default:
        // Default to INI format for unknown extensions
        return loadFromINI(filename);
    }
}

bool ConfigManager::saveToFile(const std::string& filename) const {
    ConfigUtils::ConfigFormat format = ConfigUtils::detectFormat(filename);

    switch (format) {
#ifdef HAVE_JSON
    case ConfigUtils::ConfigFormat::JSON:
        return saveToJSON(filename);
#endif
    case ConfigUtils::ConfigFormat::INI:
        return saveToINI(filename);
    default:
        // Default to INI format for unknown extensions
        return saveToINI(filename);
    }
}

ConfigSection& ConfigManager::getSection(const std::string& section_name) {
    return sections_[section_name];
}

const ConfigSection& ConfigManager::getSection(const std::string& section_name) const {
    static ConfigSection empty_section;
    auto it = sections_.find(section_name);
    return (it != sections_.end()) ? it->second : empty_section;
}

bool ConfigManager::hasSection(const std::string& section_name) const {
    return sections_.find(section_name) != sections_.end();
}

void ConfigManager::addSection(const std::string& section_name) {
    sections_[section_name] = ConfigSection(section_name);
}

std::vector<std::string> ConfigManager::getSectionNames() const {
    std::vector<std::string> names;
    for (const auto& pair : sections_) {
        names.push_back(pair.first);
    }
    return names;
}

void ConfigManager::setValue(const std::string& path, const ConfigValue& value) {
    auto path_parts = parsePath(path);
    getSection(path_parts.first).setValue(path_parts.second, value);
}

ConfigValue ConfigManager::getValue(const std::string& path) const {
    auto path_parts = parsePath(path);
    return getSection(path_parts.first).getValue(path_parts.second);
}

bool ConfigManager::hasValue(const std::string& path) const {
    auto path_parts = parsePath(path);
    return hasSection(path_parts.first) && getSection(path_parts.first).hasValue(path_parts.second);
}

void ConfigManager::setInt(const std::string& path, int value) {
    setValue(path, ConfigValue(value));
}

void ConfigManager::setDouble(const std::string& path, double value) {
    setValue(path, ConfigValue(value));
}

void ConfigManager::setBool(const std::string& path, bool value) {
    setValue(path, ConfigValue(value));
}

void ConfigManager::setString(const std::string& path, const std::string& value) {
    setValue(path, ConfigValue(value));
}

int ConfigManager::getInt(const std::string& path, int default_value) const {
    auto path_parts = parsePath(path);
    return getSection(path_parts.first).getInt(path_parts.second, default_value);
}

double ConfigManager::getDouble(const std::string& path, double default_value) const {
    auto path_parts = parsePath(path);
    return getSection(path_parts.first).getDouble(path_parts.second, default_value);
}

bool ConfigManager::getBool(const std::string& path, bool default_value) const {
    auto path_parts = parsePath(path);
    return getSection(path_parts.first).getBool(path_parts.second, default_value);
}

std::string ConfigManager::getString(const std::string& path, const std::string& default_value) const {
    auto path_parts = parsePath(path);
    return getSection(path_parts.first).getString(path_parts.second, default_value);
}

bool ConfigManager::loadConfig(const std::string& filename, FaceDetectionConfig& config) {
    if (!loadFromFile(filename)) {
        return false;
    }
    
    // Load camera settings
    config.camera_id = getInt("camera.device_id", config.camera_id);
    config.device_path = getString("camera.device_path", config.device_path);
    config.width = getInt("camera.width", config.width);
    config.height = getInt("camera.height", config.height);
    config.fps = getInt("camera.fps", config.fps);
    
    // Load detection settings
    config.scale_factor = getDouble("detection.scale_factor", config.scale_factor);
    config.min_neighbors = getInt("detection.min_neighbors", config.min_neighbors);
    config.min_size = getInt("detection.min_size", config.min_size);
    config.max_size = getInt("detection.max_size", config.max_size);
    
    // Load display settings
    config.show_fps = getBool("display.show_fps", config.show_fps);
    config.show_detection_info = getBool("display.show_detection_info", config.show_detection_info);
    config.show_confidence = getBool("display.show_confidence", config.show_confidence);
    config.window_title = getString("display.window_title", config.window_title);
    
    // Load performance settings
    config.enable_multithreading = getBool("performance.enable_multithreading", config.enable_multithreading);
    config.max_queue_size = getInt("performance.max_queue_size", config.max_queue_size);
    config.enable_performance_monitor = getBool("performance.enable_performance_monitor", config.enable_performance_monitor);
    
    // Load output settings
    config.save_video = getBool("output.save_video", config.save_video);
    config.output_filename = getString("output.filename", config.output_filename);
    
    // Load debug settings
    config.verbose = getBool("debug.verbose", config.verbose);
    config.enable_debug_display = getBool("debug.enable_debug_display", config.enable_debug_display);
    
    return true;
}

bool ConfigManager::saveConfig(const std::string& filename, const FaceDetectionConfig& config) {
    clear();
    
    // Save camera settings
    setInt("camera.device_id", config.camera_id);
    setString("camera.device_path", config.device_path);
    setInt("camera.width", config.width);
    setInt("camera.height", config.height);
    setInt("camera.fps", config.fps);
    
    // Save detection settings
    setDouble("detection.scale_factor", config.scale_factor);
    setInt("detection.min_neighbors", config.min_neighbors);
    setInt("detection.min_size", config.min_size);
    setInt("detection.max_size", config.max_size);
    
    // Save display settings
    setBool("display.show_fps", config.show_fps);
    setBool("display.show_detection_info", config.show_detection_info);
    setBool("display.show_confidence", config.show_confidence);
    setString("display.window_title", config.window_title);
    
    // Save performance settings
    setBool("performance.enable_multithreading", config.enable_multithreading);
    setInt("performance.max_queue_size", config.max_queue_size);
    setBool("performance.enable_performance_monitor", config.enable_performance_monitor);
    
    // Save output settings
    setBool("output.save_video", config.save_video);
    setString("output.filename", config.output_filename);
    
    // Save debug settings
    setBool("debug.verbose", config.verbose);
    setBool("debug.enable_debug_display", config.enable_debug_display);
    
    return saveToFile(filename);
}

bool ConfigManager::validateConfig() const {
    // Basic validation - could be expanded
    return true;
}

std::vector<std::string> ConfigManager::getValidationErrors() const {
    std::vector<std::string> errors;
    
    // Validate camera settings
    if (getInt("camera.width", 0) <= 0) {
        errors.push_back("Invalid camera width");
    }
    
    if (getInt("camera.height", 0) <= 0) {
        errors.push_back("Invalid camera height");
    }
    
    if (getInt("camera.fps", 0) <= 0) {
        errors.push_back("Invalid camera FPS");
    }
    
    // Validate detection settings
    double scale_factor = getDouble("detection.scale_factor", 1.1);
    if (scale_factor <= 1.0 || scale_factor > 2.0) {
        errors.push_back("Invalid scale factor");
    }
    
    return errors;
}

void ConfigManager::loadDefaults() {
    clear();
    
    // Load default configuration
    FaceDetectionConfig default_config;
    
    // Camera defaults
    setInt("camera.device_id", default_config.camera_id);
    setString("camera.device_path", default_config.device_path);
    setInt("camera.width", default_config.width);
    setInt("camera.height", default_config.height);
    setInt("camera.fps", default_config.fps);
    
    // Detection defaults
    setDouble("detection.scale_factor", default_config.scale_factor);
    setInt("detection.min_neighbors", default_config.min_neighbors);
    setInt("detection.min_size", default_config.min_size);
    setInt("detection.max_size", default_config.max_size);
    
    // Display defaults
    setBool("display.show_fps", default_config.show_fps);
    setBool("display.show_detection_info", default_config.show_detection_info);
    setBool("display.show_confidence", default_config.show_confidence);
    setString("display.window_title", default_config.window_title);
}

void ConfigManager::resetToDefaults() {
    loadDefaults();
}

void ConfigManager::clear() {
    sections_.clear();
}

bool ConfigManager::isEmpty() const {
    return sections_.empty();
}

std::string ConfigManager::toString() const {
    std::ostringstream oss;
    
    for (const auto& section_pair : sections_) {
        oss << "[" << section_pair.first << "]\n";
        
        for (const auto& key : section_pair.second.getKeys()) {
            auto value = section_pair.second.getValue(key);
            oss << key << " = ";
            
            switch (value.getType()) {
            case ConfigValueType::INTEGER:
                oss << value.asInt();
                break;
            case ConfigValueType::DOUBLE:
                oss << value.asDouble();
                break;
            case ConfigValueType::BOOLEAN:
                oss << (value.asBool() ? "true" : "false");
                break;
            case ConfigValueType::STRING:
                oss << value.asString();
                break;
            case ConfigValueType::ARRAY:
                // Simple array representation
                oss << "[";
                auto array = value.asArray();
                for (size_t i = 0; i < array.size(); ++i) {
                    if (i > 0) oss << ", ";
                    oss << array[i];
                }
                oss << "]";
                break;
            }
            oss << "\n";
        }
        oss << "\n";
    }
    
    return oss.str();
}

std::pair<std::string, std::string> ConfigManager::parsePath(const std::string& path) const {
    size_t dot_pos = path.find('.');
    if (dot_pos == std::string::npos) {
        return {"default", path};
    }
    
    return {path.substr(0, dot_pos), path.substr(dot_pos + 1)};
}

void ConfigManager::setError(const std::string& error) const {
    last_error_ = error;
}

bool ConfigManager::loadFromINI(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        setError("Cannot open file: " + filename);
        return false;
    }
    
    std::string line;
    std::string current_section = "default";
    
    while (std::getline(file, line)) {
        // Remove leading/trailing whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        // Check for section header
        if (line[0] == '[' && line.back() == ']') {
            current_section = line.substr(1, line.length() - 2);
            continue;
        }
        
        // Parse key-value pair
        size_t eq_pos = line.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = line.substr(0, eq_pos);
            std::string value = line.substr(eq_pos + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // Try to determine value type and store
            if (value == "true" || value == "false") {
                getSection(current_section).setBool(key, value == "true");
            } else if (value.find('.') != std::string::npos) {
                try {
                    double d = std::stod(value);
                    getSection(current_section).setDouble(key, d);
                } catch (...) {
                    getSection(current_section).setString(key, value);
                }
            } else {
                try {
                    int i = std::stoi(value);
                    getSection(current_section).setInt(key, i);
                } catch (...) {
                    getSection(current_section).setString(key, value);
                }
            }
        }
    }
    
    return true;
}

bool ConfigManager::saveToINI(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        setError("Cannot create file: " + filename);
        return false;
    }
    
    file << toString();
    return true;
}

void ConfigManager::setupValidationRules() {
    // Setup basic validation rules
    // This could be expanded with more sophisticated validation
}

bool ConfigManager::validateValue(const ValidationRule& rule, const ConfigValue& value) const {
    // Basic validation implementation
    (void)rule;  // Suppress unused parameter warning
    (void)value; // Suppress unused parameter warning
    return true;
}

#ifdef HAVE_JSON
bool ConfigManager::loadFromJSON(const std::string& filename) {
    // JSON implementation would go here
    // For now, fall back to INI
    return loadFromINI(filename);
}

bool ConfigManager::saveToJSON(const std::string& filename) const {
    // JSON implementation would go here
    // For now, fall back to INI
    return saveToINI(filename);
}
#endif

// ConfigUtils namespace implementation
namespace ConfigUtils {

ConfigFormat detectFormat(const std::string& filename) {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return ConfigFormat::UNKNOWN;
    }
    
    std::string extension = filename.substr(dot_pos + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (extension == "json") {
        return ConfigFormat::JSON;
    } else if (extension == "ini" || extension == "cfg") {
        return ConfigFormat::INI;
    } else if (extension == "xml") {
        return ConfigFormat::XML;
    }
    
    return ConfigFormat::UNKNOWN;
}

bool isValidConfigFile(const std::string& filename) {
    std::ifstream file(filename);
    return file.good();
}

std::string getConfigDirectory() {
    // Simplified implementation
    return ".";
}

std::string getDefaultConfigPath() {
    return getConfigDirectory() + "/" + ConfigConstants::DEFAULT_CONFIG_FILE;
}

std::vector<std::string> getConfigSearchPaths() {
    return {
        ".",
        getConfigDirectory(),
        "/etc/face_detection_demo",
        "/usr/local/etc/face_detection_demo"
    };
}

} // namespace ConfigUtils

// ConfigTemplates namespace implementation
namespace ConfigTemplates {

FaceDetectionConfig getDefaultFaceDetectionConfig() {
    return FaceDetectionConfig();
}

FaceDetectionConfig getHighPerformanceConfig() {
    FaceDetectionConfig config;
    config.width = 1280;
    config.height = 720;
    config.fps = 60;
    config.enable_multithreading = true;
    config.max_queue_size = 10;
    return config;
}

FaceDetectionConfig getLowResourceConfig() {
    FaceDetectionConfig config;
    config.width = 320;
    config.height = 240;
    config.fps = 15;
    config.scale_factor = 1.3;
    config.min_neighbors = 5;
    config.enable_multithreading = false;
    config.max_queue_size = 2;
    return config;
}

FaceDetectionConfig getDebugConfig() {
    FaceDetectionConfig config;
    config.verbose = true;
    config.enable_debug_display = true;
    config.show_fps = true;
    config.show_detection_info = true;
    config.show_confidence = true;
    return config;
}

} // namespace ConfigTemplates
