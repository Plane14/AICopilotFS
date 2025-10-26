/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <string>
#include <map>
#include <vector>

namespace AICopilot {

/**
 * Generic INI/CFG file parser for aircraft.cfg and .FLT files
 */
class ConfigParser {
public:
    ConfigParser() = default;
    
    // Parse a configuration file
    bool parse(const std::string& filePath);
    
    // Get value from section
    std::string getValue(const std::string& section, const std::string& key, 
                        const std::string& defaultValue = "") const;
    
    int getIntValue(const std::string& section, const std::string& key, 
                    int defaultValue = 0) const;
    
    double getDoubleValue(const std::string& section, const std::string& key, 
                         double defaultValue = 0.0) const;
    
    bool getBoolValue(const std::string& section, const std::string& key, 
                     bool defaultValue = false) const;
    
    // Check if section exists
    bool hasSection(const std::string& section) const;
    
    // Check if key exists in section
    bool hasKey(const std::string& section, const std::string& key) const;
    
    // Get all sections
    std::vector<std::string> getSections() const;
    
    // Get all keys in a section
    std::vector<std::string> getKeys(const std::string& section) const;
    
    // Clear all data
    void clear();
    
private:
    // Map of section -> (key -> value)
    std::map<std::string, std::map<std::string, std::string>> data_;
    
    std::string trim(const std::string& str) const;
    std::string toLower(const std::string& str) const;
};

} // namespace AICopilot

#endif // CONFIG_PARSER_H
