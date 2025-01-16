//
// Created by darko on 11/24/24.
//

#include "ProfileManager.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <pwd.h>
#include <sstream>

namespace Keybinder {

    // Dynamically set the profiles directory
    std::string ProfileManager::get_profiles_dir() {
        const char *home = nullptr;
        // Check if running with sudo
        if (getenv("SUDO_USER")) {
            // Get the original user's home directory
            const char *user = getenv("SUDO_USER");
            struct passwd *pw = getpwnam(user);
            if (pw) {
                home = pw->pw_dir;
            } else {
                std::cerr << "Error: Failed to retrieve home directory for SUDO_USER." << std::endl;
                exit(1);
            }
        } else {
            // Get the home directory of the current user
            home = getenv("HOME");
        }
        if (!home) {
            std::cerr << "Error: HOME environment variable not set." << std::endl;
            exit(1);
        }
        std::string profiles_dir = std::string(home) + "/.config/keybinder";

        // Create the directory if it doesn't exist
        if (!std::filesystem::exists(profiles_dir)) {
            std::filesystem::create_directories(profiles_dir);
        }

        return profiles_dir;
    }


    std::vector<std::pair<std::string, std::string> > ProfileManager::load_profile(const std::string &profile_name) {
        const std::string PROFILES_DIR = get_profiles_dir();
        std::vector<std::pair<std::string, std::string> > mappings;
        std::string profile_path = PROFILES_DIR + "/" + profile_name + ".txt";

        std::cout << "Loading profile: " << profile_path << "\n";
        std::ifstream infile(profile_path);

        if (!infile.is_open()) {
            std::cerr << "Error: Could not open profile: " << profile_name << std::endl;
            return mappings;
        }

        std::string line;
        while (std::getline(infile, line)) {
            std::istringstream iss(line);
            std::string source_key, target_key;

            if (std::getline(iss, source_key, '=') && std::getline(iss, target_key)) {
                mappings.emplace_back(source_key, target_key);
            }
        }

        infile.close();
        return mappings;
    }


    void ProfileManager::create_profile(const std::string &profile_name) {
        const std::string PROFILES_DIR = get_profiles_dir();
        std::string profile_path = PROFILES_DIR + "/" + profile_name + ".txt";
        std::ofstream outfile(profile_path);

        if (!outfile.is_open()) {
            std::cerr << "Error: Could not create profile at " << profile_path << std::endl;
            return;
        }

        std::cout << "Creating profile: " << profile_name << "\nEnter mappings (KEY=VALUE). Type 'done' to finish:\n";

        std::string line;
        while (true) {
            std::cout << "> ";
            std::getline(std::cin, line);
            if (line == "done") break;

            std::istringstream iss(line);
            std::string source_key, target_key;
            if (std::getline(iss, source_key, '=') && std::getline(iss, target_key)) {
                outfile << line << "\n";
            } else {
                std::cerr << "Invalid format. Use KEY=VALUE.\n";
            }
        }

        outfile.close();
        std::cout << "Profile " << profile_name << " created successfully at " << profile_path << "\n";
    }


    // List available profiles
    void ProfileManager::list_profiles() {
        const std::string PROFILES_DIR = get_profiles_dir();
        for (const auto &entry: std::filesystem::directory_iterator(PROFILES_DIR)) {
            std::cout << entry.path().filename().string() << std::endl;
        }
    }

    // Delete a profile
    void ProfileManager::delete_profile(const std::string &profile_name) {
        const std::string PROFILES_DIR = get_profiles_dir();
        std::string profile_path = PROFILES_DIR + "/" + profile_name + ".txt";
        if (std::filesystem::exists(profile_path)) {
            std::filesystem::remove(profile_path);
            std::cout << "Deleted profile: " << profile_name << std::endl;
        } else {
            std::cerr << "Profile not found: " << profile_name << std::endl;
        }
    }
} // Keybinder
