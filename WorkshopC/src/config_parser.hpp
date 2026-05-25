#pragma once

#include <fstream>
#include <sstream>
#include <algorithm>

#include "config.hpp"

class ConfigParser {
private:
    static std::string trim(const std::string &str) {
        size_t first = str.find_first_not_of(" \t\r\n");

        if (first == std::string::npos) {
            return "";
        }

        size_t last = str.find_last_not_of(" \t\r\n");

        return str.substr(first, last - first + 1);
    }

    static RuleLevel parseLevel(const std::string &str) {
        std::string lower = str;

        std::transform(
            lower.begin(),
            lower.end(),
            lower.begin(),
            ::tolower
        );

        if (lower == "off") {
            return RuleLevel::Off;
        }

        if (lower == "warning") {
            return RuleLevel::Warning;
        }

        if (lower == "error") {
            return RuleLevel::Error;
        }

        return RuleLevel::Warning;
    }

public:
    static bool loadFromFile(
        const std::string &filepath,
        Config &config
    ) {
        std::ifstream file(filepath);

        if (!file.is_open()) {
            return false;
        }

        std::string line;

        std::string currentSection;
        std::string currentRule;

        bool inProjectIncludes = false;
        bool inThirdPartyIncludes = false;

        while (std::getline(file, line)) {
            line = trim(line);

            if (line.empty() || line[0] == '#') {
                continue;
            }

            // sections
            if (line == "paths:") {
                currentSection = "paths";
                continue;
            }

            if (line == "rules:") {
                currentSection = "rules";
                continue;
            }

            // include lists
            if (line == "project_includes:") {
                inProjectIncludes = true;
                inThirdPartyIncludes = false;
                continue;
            }

            if (line == "third_party_includes:") {
                inProjectIncludes = false;
                inThirdPartyIncludes = true;
                continue;
            }

            // list item
            if (line.starts_with("-")) {
                std::string value = trim(line.substr(1));

                if (inProjectIncludes) {
                    config.projectIncludes.push_back(value);
                }
                else if (inThirdPartyIncludes) {
                    config.thirdPartyIncludes.push_back(value);
                }

                continue;
            }

            // rule
            if (currentSection == "rules" && line.back() == ':') {
                currentRule = trim(
                    line.substr(0, line.size() - 1)
                );

                config.rules[currentRule] = RuleConfig();

                continue;
            }

            // rule settings
            size_t colon = line.find(':');

            if (colon != std::string::npos && !currentRule.empty()) {
                std::string key =
                    trim(line.substr(0, colon));

                std::string value =
                    trim(line.substr(colon + 1));

                if (key == "level") {
                    config.rules[currentRule].level =
                        parseLevel(value);
                }
                else {
                    config.rules[currentRule]
                        .options[key] = value;
                }
            }
        }

        return true;
    }
};