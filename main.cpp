//#include <libevdev/libevdev.h>

#include <libevdev-1.0/libevdev/libevdev.h>
#include <linux/uinput.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <sstream>
#include <filesystem>
#include <vector>
#include <cstring>

#include <pwd.h>

#include "ProfileManager.h"


// Key mapping structure
struct KeyMapping {
    int source_code;
    int target_code;
};

// Helper to get key codes from names
int get_key_code(const std::string& key_name) {

    std::cout << "key_name: " << key_name << std::endl;
    std::cout << "key max: " << KEY_MAX << std::endl;

    for (int key = 0; key < KEY_MAX; ++key) {

        const char * found_key_name = libevdev_event_code_get_name(EV_KEY, key);

        if (nullptr == found_key_name) {
            continue;
        }

        std::cout << key << ": " << found_key_name << std::endl;
        if (key_name == found_key_name) {
            return key;
        }
    }
    std::cerr << "Error: Invalid key name '" << key_name << "'" << std::endl;
    return -1; // Invalid key
}

// Load mappings from a profile
std::unordered_map<int, KeyMapping> parse_mappings(const std::vector<std::pair<std::string, std::string> >& profile) {
    std::unordered_map<int, KeyMapping> mappings;

    for (const auto& pair : profile) {
        KeyMapping mapping{};
        mapping.source_code = get_key_code(pair.first);
        mapping.target_code = get_key_code(pair.second);
        mappings[mapping.source_code] = mapping;

    }
    return mappings;
}

// Setup uinput device
int setup_uinput_device() {
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("Failed to open uinput");
        return -1;
    }

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    for (int key = 0; key < KEY_MAX; ++key) {
        ioctl(fd, UI_SET_KEYBIT, key);
    }

    struct uinput_setup usetup = {};
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1; // Vendor ID
    usetup.id.product = 0x1; // Product ID
    strcpy(usetup.name, "Input Remapper");

    ioctl(fd, UI_DEV_SETUP, &usetup);
    ioctl(fd, UI_DEV_CREATE);

    return fd;
}

// Emit event using uinput
void emit_event(int fd, int type, int code, int value) {
    struct input_event ev = {};
    ev.type = type;
    ev.code = code;
    ev.value = value;
    ev.time.tv_sec = 0;
    ev.time.tv_usec = 0;
    write(fd, &ev, sizeof(ev));
}

// Detect all input devices
std::vector<std::string> detect_input_devices() {
    std::vector<std::string> devices;
    for (const auto& entry : std::filesystem::directory_iterator("/dev/input/")) {
        if (entry.path().string().find("event") != std::string::npos) {
            devices.push_back(entry.path().string());
        }
    }
    return devices;
}

// Check if a device is a keyboard
bool is_keyboard(const std::string& device_path) {
    struct libevdev* dev = nullptr;
    int fd = open(device_path.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) return false;

    if (libevdev_new_from_fd(fd, &dev) < 0) {
        close(fd);
        return false;
    }

    bool keyboard = libevdev_has_event_type(dev, EV_KEY) && libevdev_has_event_code(dev, EV_KEY, KEY_A);
    libevdev_free(dev);
    close(fd);
    return keyboard;
}

// Read input events, remap, and emit
void remap_events(const std::string& device_path, int uinput_fd, const std::unordered_map<int, KeyMapping>& mappings) {
    struct libevdev* dev = nullptr;
    int fd = open(device_path.c_str(), O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        std::cerr << "Failed to open device: " << device_path << std::endl;
        return;
    }

    if (libevdev_new_from_fd(fd, &dev) < 0) {
        std::cerr << "Failed to initialize libevdev." << std::endl;
        close(fd);
        return;
    }

    // Grab the input device for exclusive access
    if (libevdev_grab(dev, LIBEVDEV_GRAB) != 0) {
        std::cerr << "Failed to grab device: " << device_path << std::endl;
        libevdev_free(dev);
        close(fd);
        return;
    }

    std::cout << "Remapping input device: " << libevdev_get_name(dev) << std::endl;

    struct input_event ev;
    while (true) {
        int rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
        if (rc == 0) {
            if (ev.type == EV_KEY) {
                // If the key is mapped, emit the remapped event instead
                if (mappings.count(ev.code)) {
                    KeyMapping mapping = mappings.at(ev.code);

                    if(mapping.target_code >= 0 && mapping.target_code < KEY_MAX) {

                        // Emit remapped key event
                        emit_event(uinput_fd, EV_KEY, mapping.target_code, ev.value);

                        std::cout << "Mapped "
                                  << libevdev_event_code_get_name(EV_KEY, ev.code)
                                  << " to "
                                  << libevdev_event_code_get_name(EV_KEY, mapping.target_code)
                                  << " (Value: " << ev.value << ")" << std::endl;

                        // Suppress the original key event by skipping it
                        continue;
                    } else {
                        std::cerr << "Error: Invalid target key code for mapping " << ev.code << std::endl;
                    }

  
                } else {
                    
                    // For unmapped keys, emit the original event
                    emit_event(uinput_fd, EV_KEY, ev.code, ev.value);
                }

            } else {
                // Forward non-key events as is (e.g., EV_REL for mouse movements)
                emit_event(uinput_fd, ev.type, ev.code, ev.value);
            }

            // Synchronize the event
            emit_event(uinput_fd, EV_SYN, SYN_REPORT, 0);
        }
    }

    libevdev_free(dev);
    close(fd);
}

int main(int argc, char* argv[]) {

    // Handle command-line arguments
    if (argc > 1) {
        std::string command = argv[1];

        Keybinder::ProfileManager profile_manager;

        if (command == "create" && argc == 3) {
            std::string profile_name = argv[2];
            profile_manager.create_profile(profile_name);
            return 0;
        }
        if (command == "load" && argc == 3) {
            std::string profile_name = argv[2];
            std::vector<std::pair<std::string, std::string> > contents = profile_manager.load_profile(profile_name);
            std::unordered_map<int, KeyMapping> mappings = parse_mappings(contents);
            if (mappings.empty()) {
                std::cerr << "No valid mappings in profile.\n";
                return 1;
            }

            // Detect a keyboard
            auto devices = detect_input_devices();
            std::string selected_device;
            for (const auto& device : devices) {
                if (is_keyboard(device)) {
                    selected_device = device;
                    break;
                }
            }

            if (selected_device.empty()) {
                std::cerr << "No keyboard detected.\n";
                return 1;
            }

            // Setup uinput device
            int uinput_fd = setup_uinput_device();
            if (uinput_fd < 0) {
                return 1;
            }

            // Remap input events
            remap_events(selected_device, uinput_fd, mappings);

            // Cleanup
            ioctl(uinput_fd, UI_DEV_DESTROY);
            close(uinput_fd);
            return 0;
        }
    }

    std::cerr << "Usage:\n"
              << "  " << argv[0] << " create <profile_name> - Create a new profile\n"
              << "  " << argv[0] << " load <profile_name>   - Load a profile and start remapping\n";
    return 1;
}