/*
 * Configuration Manager Header
 * 
 * This header defines the configuration management interface for loading
 * and saving application settings from/to JSON files.
 * 
 * Author: Face Detection Demo Team
 * License: MIT
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <map>
#include <vector>
#include <memory>

// Forward declaration
struct FaceDetectionConfig;

// Configuration value types
enum class ConfigValueType {
    INTEGER,
    DOUBLE,
    BOOLEAN,
    STRING,
    ARRAY
};

// Generic configuration value
class ConfigValue {
public:
    ConfigValue() = default;
    explicit ConfigValue(int value) : type_(ConfigValueType::INTEGER), int_value_(value) {}
    explicit ConfigValue(double value) : type_(ConfigValueType::DOUBLE), double_value_(value) {}
    explicit ConfigValue(bool value) : type_(ConfigValueType::BOOLEAN), bool_value_(value) {}
    explicit ConfigValue(const std::string& value) : type_(ConfigValueType::STRING), string_value_(value) {}
    explicit ConfigValue(const std::vector<std::string>& value) : type_(ConfigValueType::ARRAY), array_value_(value) {}
    
    ConfigValueType getType() const { return type_; }
    
    int asInt() const { return int_value_; }
    double asDouble() const { return double_value_; }
    bool asBool() const { return bool_value_; }
    const std::string& asString() const { return string_value_; }
    const std::vector<std::string>& asArray() const { return array_value_; }
    
    bool isValid() const { return type_ != ConfigValueType::INTEGER || int_value_ != 0; }
    
private:
    ConfigValueType type_ = ConfigValueType::INTEGER;
    int int_value_ = 0;
    double double_value_ = 0.0;
    bool bool_value_ = false;
    std::string string_value_;
    std::vector<std::string> array_value_;
};

// Configuration section
class ConfigSection {
public:
    ConfigSection() = default;
    explicit ConfigSection(const std::string& name) : name_(name) {}
    
    void setValue(const std::string& key, const ConfigValue& value);
    ConfigValue getValue(const std::string& key) const;
    bool hasValue(const std::string& key) const;
    
    void setInt(const std::string& key, int value);
    void setDouble(const std::string& key, double value);
    void setBool(const std::string& key, bool value);
    void setString(const std::string& key, const std::string& value);
    void setArray(const std::string& key, const std::vector<std::string>& value);
    
    int getInt(const std::string& key, int default_value = 0) const;
    double getDouble(const std::string& key, double default_value = 0.0) const;
    bool getBool(const std::string& key, bool default_value = false) const;
    std::string getString(const std::string& key, const std::string& default_value = "") const;
    std::vector<std::string> getArray(const std::string& key) const;
    
    const std::string& getName() const { return name_; }
    std::vector<std::string> getKeys() const;
    
private:
    std::string name_;
    std::map<std::string, ConfigValue> values_;
};

// Main configuration manager class
class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();
    
    // File operations
    bool loadFromFile(const std::string& filename);
    bool saveToFile(const std::string& filename) const;
    
    // Section management
    ConfigSection& getSection(const std::string& section_name);
    const ConfigSection& getSection(const std::string& section_name) const;
    bool hasSection(const std::string& section_name) const;
    void addSection(const std::string& section_name);
    std::vector<std::string> getSectionNames() const;
    
    // Direct value access (section.key format)
    void setValue(const std::string& path, const ConfigValue& value);
    ConfigValue getValue(const std::string& path) const;
    bool hasValue(const std::string& path) const;
    
    // Convenience methods
    void setInt(const std::string& path, int value);
    void setDouble(const std::string& path, double value);
    void setBool(const std::string& path, bool value);
    void setString(const std::string& path, const std::string& value);
    
    int getInt(const std::string& path, int default_value = 0) const;
    double getDouble(const std::string& path, double default_value = 0.0) const;
    bool getBool(const std::string& path, bool default_value = false) const;
    std::string getString(const std::string& path, const std::string& default_value = "") const;
    
    // Application-specific configuration
    bool loadConfig(const std::string& filename, FaceDetectionConfig& config);
    bool saveConfig(const std::string& filename, const FaceDetectionConfig& config);
    
    // Validation
    bool validateConfig() const;
    std::vector<std::string> getValidationErrors() const;
    
    // Defaults
    void loadDefaults();
    void resetToDefaults();
    
    // Utility
    void clear();
    bool isEmpty() const;
    std::string toString() const;
    
    // Error handling
    std::string getLastError() const { return last_error_; }
    
private:
    std::map<std::string, ConfigSection> sections_;
    mutable std::string last_error_;
    
    // Helper methods
    std::pair<std::string, std::string> parsePath(const std::string& path) const;
    void setError(const std::string& error) const;
    
    // JSON parsing (if available)
#ifdef HAVE_JSON
    bool loadFromJSON(const std::string& filename);
    bool saveToJSON(const std::string& filename) const;
#endif
    
    // INI parsing (fallback)
    bool loadFromINI(const std::string& filename);
    bool saveToINI(const std::string& filename) const;
    
    // Configuration validation rules
    struct ValidationRule {
        std::string path;
        ConfigValueType type;
        bool required;
        ConfigValue min_value;
        ConfigValue max_value;
        std::vector<std::string> allowed_values;
    };
    
    std::vector<ValidationRule> validation_rules_;
    void setupValidationRules();
    bool validateValue(const ValidationRule& rule, const ConfigValue& value) const;
    
    // Non-copyable
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
};

// Utility functions
namespace ConfigUtils {
    // File format detection
    enum class ConfigFormat {
        JSON,
        INI,
        XML,
        UNKNOWN
    };
    
    ConfigFormat detectFormat(const std::string& filename);
    bool isValidConfigFile(const std::string& filename);
    
    // Path utilities
    std::string getConfigDirectory();
    std::string getDefaultConfigPath();
    std::vector<std::string> getConfigSearchPaths();
    
    // Backup and versioning
    bool createBackup(const std::string& filename);
    bool restoreBackup(const std::string& filename);
    std::vector<std::string> getBackupFiles(const std::string& filename);
    
    // Migration
    bool migrateConfig(const std::string& old_filename, const std::string& new_filename);
    int getConfigVersion(const std::string& filename);
    bool upgradeConfig(const std::string& filename, int target_version);
    
    // Validation helpers
    bool isValidPath(const std::string& path);
    bool isValidSectionName(const std::string& section_name);
    bool isValidKeyName(const std::string& key_name);
    
    // Type conversion
    std::string toString(const ConfigValue& value);
    ConfigValue fromString(const std::string& str, ConfigValueType type);
    
    // Environment variable substitution
    std::string expandEnvironmentVariables(const std::string& str);
    bool hasEnvironmentVariables(const std::string& str);
}

// Configuration templates
namespace ConfigTemplates {
    // Create default configuration for different use cases
    ConfigManager createDefaultConfig();
    ConfigManager createMinimalConfig();
    ConfigManager createPerformanceConfig();
    ConfigManager createDebugConfig();
    
    // Predefined configurations
    FaceDetectionConfig getDefaultFaceDetectionConfig();
    FaceDetectionConfig getHighPerformanceConfig();
    FaceDetectionConfig getLowResourceConfig();
    FaceDetectionConfig getDebugConfig();
}

// Constants
namespace ConfigConstants {
    // Default file names
    const std::string DEFAULT_CONFIG_FILE = "config.json";
    const std::string DEFAULT_CONFIG_DIR = ".face_detection_demo";
    const std::string BACKUP_SUFFIX = ".backup";
    
    // Section names
    const std::string CAMERA_SECTION = "camera";
    const std::string DETECTION_SECTION = "detection";
    const std::string DISPLAY_SECTION = "display";
    const std::string PERFORMANCE_SECTION = "performance";
    const std::string LOGGING_SECTION = "logging";
    
    // Configuration version
    constexpr int CONFIG_VERSION = 1;
    
    // Limits
    constexpr int MAX_BACKUP_FILES = 5;
    constexpr size_t MAX_CONFIG_FILE_SIZE = 1024 * 1024; // 1MB
}

// Macros for easy configuration access
#define CONFIG_GET_INT(manager, path, default_val) \
    manager.getInt(path, default_val)

#define CONFIG_GET_DOUBLE(manager, path, default_val) \
    manager.getDouble(path, default_val)

#define CONFIG_GET_BOOL(manager, path, default_val) \
    manager.getBool(path, default_val)

#define CONFIG_GET_STRING(manager, path, default_val) \
    manager.getString(path, default_val)

#define CONFIG_SET_INT(manager, path, value) \
    manager.setInt(path, value)

#define CONFIG_SET_DOUBLE(manager, path, value) \
    manager.setDouble(path, value)

#define CONFIG_SET_BOOL(manager, path, value) \
    manager.setBool(path, value)

#define CONFIG_SET_STRING(manager, path, value) \
    manager.setString(path, value)

#endif // CONFIG_MANAGER_H
