/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include "../include/config_parser.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace AICopilot {

bool ConfigParser::parse(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    clear();
    std::string currentSection = "";
    std::string line;
    
    while (std::getline(file, line)) {
        // Trim whitespace
        line = trim(line);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue;
        }
        
        // Check for section header
        if (line[0] == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.length() - 2);
            currentSection = trim(currentSection);
            data_[currentSection] = std::map<std::string, std::string>();
            continue;
        }
        
        // Parse key-value pair
        size_t equalPos = line.find('=');
        if (equalPos != std::string::npos) {
            std::string key = trim(line.substr(0, equalPos));
            std::string value = trim(line.substr(equalPos + 1));
            
            // Remove comments from value
            size_t commentPos = value.find(';');
            if (commentPos != std::string::npos) {
                value = trim(value.substr(0, commentPos));
            }
            
            // Remove quotes if present
            if (value.length() >= 2 && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.length() - 2);
            }
            
            if (!currentSection.empty() && !key.empty()) {
                data_[currentSection][key] = value;
            }
        }
    }
    
    return true;
}

std::string ConfigParser::getValue(const std::string& section, const std::string& key, 
                                  const std::string& defaultValue) const {
    auto sectionIt = data_.find(section);
    if (sectionIt != data_.end()) {
        auto keyIt = sectionIt->second.find(key);
        if (keyIt != sectionIt->second.end()) {
            return keyIt->second;
        }
    }
    return defaultValue;
}

int ConfigParser::getIntValue(const std::string& section, const std::string& key, 
                              int defaultValue) const {
    std::string value = getValue(section, key);
    if (value.empty()) {
        return defaultValue;
    }
    try {
        return std::stoi(value);
    } catch (...) {
        return defaultValue;
    }
}

double ConfigParser::getDoubleValue(const std::string& section, const std::string& key, 
                                   double defaultValue) const {
    std::string value = getValue(section, key);
    if (value.empty()) {
        return defaultValue;
    }
    try {
        return std::stod(value);
    } catch (...) {
        return defaultValue;
    }
}

bool ConfigParser::getBoolValue(const std::string& section, const std::string& key, 
                               bool defaultValue) const {
    std::string value = getValue(section, key);
    if (value.empty()) {
        return defaultValue;
    }
    std::string lowerValue = toLower(value);
    return (lowerValue == "true" || lowerValue == "1" || lowerValue == "yes" || lowerValue == "on");
}

bool ConfigParser::hasSection(const std::string& section) const {
    return data_.find(section) != data_.end();
}

bool ConfigParser::hasKey(const std::string& section, const std::string& key) const {
    auto sectionIt = data_.find(section);
    if (sectionIt != data_.end()) {
        return sectionIt->second.find(key) != sectionIt->second.end();
    }
    return false;
}

std::vector<std::string> ConfigParser::getSections() const {
    std::vector<std::string> sections;
    for (const auto& pair : data_) {
        sections.push_back(pair.first);
    }
    return sections;
}

std::vector<std::string> ConfigParser::getKeys(const std::string& section) const {
    std::vector<std::string> keys;
    auto sectionIt = data_.find(section);
    if (sectionIt != data_.end()) {
        for (const auto& pair : sectionIt->second) {
            keys.push_back(pair.first);
        }
    }
    return keys;
}

void ConfigParser::clear() {
    data_.clear();
}

std::string ConfigParser::trim(const std::string& str) const {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

std::string ConfigParser::toLower(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                  [](unsigned char c) { return std::tolower(c); });
    return result;
}

} // namespace AICopilot
