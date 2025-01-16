//
// Created by darko on 11/24/24.
//

#ifndef PROFILEMANAGER_H
#define PROFILEMANAGER_H
#include <string>
#include <vector>

struct KeyMapping;

namespace Keybinder {
    class ProfileManager {
    protected:
        const std::string PROFILES_DIR;

    public:
        std::string get_profiles_dir();

        std::vector<std::pair<std::string, std::string>>load_profile(const std::string &profile_name);

        void create_profile(const std::string &profile_name);

        void delete_profile(const std::string &profile_name);

        void list_profiles();

    };
} // Keybinder

#endif //PROFILEMANAGER_H
