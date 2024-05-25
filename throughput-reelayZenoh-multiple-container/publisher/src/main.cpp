#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>  // for getopt
#include <zenoh.h>

void print_usage() {
    std::cerr << "Usage: publisher -r <replay_file> -d <period_us> -k <key> [-c <config_file>]\n";
}

int main(int argc, char *argv[]) {
    std::string replay_file = "/app/timescales/largesuite/AbsentAQ/AbsentAQ10.jsonl";
    int period_us = 10000;
    std::string key = "bounverif/timescales";
    std::string config_file;

    int opt;
    while ((opt = getopt(argc, argv, "r:d:k:c:h")) != -1) {
        switch (opt) {
            case 'r':
                replay_file = optarg;
                break;
            case 'd':
                period_us = std::stoi(optarg);
                break;
            case 'k':
                key = optarg;
                break;
            case 'c':
                config_file = optarg;
                break;
            case 'h':
            default:
                print_usage();
                return EXIT_FAILURE;
        }
    }

    // Create Zenoh Config
    z_owned_config_t config = z_config_default();
    if (!config_file.empty()) {
        if (zc_config_insert_json(z_loan(config), Z_CONFIG_CONNECT_KEY, config_file.c_str()) < 0) {
            std::cerr << "Couldn't insert value `" << config_file << "` in configuration at `" << Z_CONFIG_CONNECT_KEY << "`. This is likely because `" << Z_CONFIG_CONNECT_KEY << "` expects a JSON-serialized list of strings\n";
            return EXIT_FAILURE;
        }
    }

    // Open Zenoh Session
    z_owned_session_t session = z_open(z_move(config));
    if (!z_check(session)) {
        std::cerr << "Unable to open session!\n";
        return EXIT_FAILURE;
    }

    // Declare a publisher
    z_publisher_options_t options_pub = z_publisher_options_default();
    options_pub.congestion_control = Z_CONGESTION_CONTROL_BLOCK;

    z_owned_publisher_t pub = z_declare_publisher(z_loan(session), z_keyexpr(key.c_str()), &options_pub);
    if (!z_check(pub)) {
        std::cerr << "Unable to declare publisher for key expression!\n";
        return EXIT_FAILURE;
    }

    double publish_period = period_us / 1e6;

    std::ifstream file(replay_file);
    if (!file) {
        std::cerr << "Unable to open replay file: " << replay_file << std::endl;
        return EXIT_FAILURE;
    }

    std::string line;
    while (std::getline(file, line)) {
        z_publisher_put(z_loan(pub), reinterpret_cast<const uint8_t*>(line.c_str()), line.size(), NULL);
        // Uncomment the next line if you want to add a delay
        // std::this_thread::sleep_for(std::chrono::duration<double>(publish_period));
    }

    z_undeclare_publisher(z_move(pub));
    z_close(z_move(session));

    return EXIT_SUCCESS;
}
