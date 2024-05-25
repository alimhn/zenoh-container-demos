#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fstream>
#include <string>
#include <iostream>

#include "zenoh.h"

#define DEFAULT_JSON_FILE "/app/publisher/timescales/largesuite/AbsentAQ/AbsentAQ10.jsonl"
// #define DEFAULT_JSON_FILE "/app/timescales/largesuite/AbsentBQR/AbsentBQR10.jsonl"
// #define DEFAULT_JSON_FILE "/app/timescales/largesuite/AbsentBR/AbsentBR10.jsonl"
// #define DEFAULT_JSON_FILE "/app/timescales/largesuite/RecurGLB/RecurGLB10.jsonl"
// #define DEFAULT_JSON_FILE "/app/timescales/largesuite/RespondGLB/RespondGLB10.jsonl"
// #define DEFAULT_JSON_FILE "/app/timescales/largesuite/AlwaysAQ/AlwaysAQ10.jsonl"
// #define DEFAULT_JSON_FILE "/app/timescales/largesuite/AlwaysBR/AlwaysBR10.jsonl"
// #define DEFAULT_JSON_FILE "/app/timescales/largesuite/AlwaysBQR/AlwaysBQR10.jsonl"
// #define DEFAULT_JSON_FILE "/app/timescales/largesuite/RecurBQR/RecurBQR10.jsonl"
// #define DEFAULT_JSON_FILE "/app/timescales/largesuite/RecurBQR/RecurBQR10.jsonl"


#define DEFAULT_PKT_SIZE 64
#define DEFAULT_PING_NB 100
#define DEFAULT_WARMUP_MS 1000
#define PING_TIMEOUT_SEC 1
#define DEFAULT_MSG_PER_SEC 1  // Default message rate per second

#define handle_error_en(en, msg) \
    do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

pthread_cond_t cond;
pthread_mutex_t mutex;

void callback(const z_sample_t* sample, void* context) { pthread_cond_signal(&cond); }
void drop(void* context) { pthread_cond_destroy(&cond); }

struct args_t {
    std::string json_file;
    unsigned int size;
    unsigned int number_of_pings;
    unsigned int warmup_ms;
    char* config_path;
    uint8_t help_requested;
    unsigned int msg_per_second;  // New field for message rate per second
};

struct args_t parse_args(int argc, char** argv);

// New function to calculate the sleep interval for the given message rate
void precise_sleep(int messages_per_second) {
    static struct timespec last_time = {0, 0};
    struct timespec current_time, sleep_duration;
    long interval_ns = 1000000000 / messages_per_second;

    clock_gettime(CLOCK_MONOTONIC, &current_time);
    if (last_time.tv_sec != 0) { // Check if last_time is initialized
        long elapsed_ns = (current_time.tv_sec - last_time.tv_sec) * 1000000000L + (current_time.tv_nsec - last_time.tv_nsec);
        long sleep_ns = interval_ns - elapsed_ns;
        if (sleep_ns > 0) {
            sleep_duration.tv_sec = sleep_ns / 1000000000L;
            sleep_duration.tv_nsec = sleep_ns % 1000000000L;
            nanosleep(&sleep_duration, NULL);
        }
    }
    last_time = current_time;
}

int main(int argc, char** argv) {
    struct args_t args = parse_args(argc, argv);
    if (args.help_requested) {
        printf(
            "\
        -f (optional, string, default=%s): the path to the JSON file containing the timescales\n\
        -n (optional, int, default=%d): the number of pings to be attempted\n\
        -s (optional, int, default=%d): the size of the payload embedded in the ping and repeated by the pong\n\
        -w (optional, int, default=%d): the warmup time in ms during which pings will be emitted but not measured\n\
        -c (optional, string): the path to a configuration file for the session. If this option isn't passed, the default configuration will be used.\n\
        -r (optional, int, default=%d): the rate of messages per second\n\
        ",
            DEFAULT_JSON_FILE, DEFAULT_PKT_SIZE, DEFAULT_PING_NB, DEFAULT_WARMUP_MS, DEFAULT_MSG_PER_SEC);
        return 1;
    }

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    z_owned_config_t config = args.config_path ? zc_config_from_file(args.config_path) : z_config_default();
    z_owned_session_t session = z_open(z_move(config));
    z_keyexpr_t ping = z_keyexpr_unchecked("bounverif/timescales");
    z_keyexpr_t pong = z_keyexpr_unchecked("bounverif/timescales/response");
    z_owned_publisher_t pub = z_declare_publisher(z_loan(session), ping, NULL);
    z_owned_closure_sample_t respond = z_closure(callback, drop, (void*)(&pub));
    z_owned_subscriber_t sub = z_declare_subscriber(z_loan(session), pong, z_move(respond), NULL);

    std::ifstream file(args.json_file);
    if (!file.is_open()) {
        printf("Error opening file %s\n", args.json_file.c_str());
        return 1;
    }

    std::string line;

    if (!std::getline(file, line)) {
        printf("Error reading file %s\n", args.json_file.c_str());
        return 1;
    }

    size_t data_size = line.size();
    char* data = (char *)malloc(data_size + 1); // +1 for null terminator
    if (data == NULL) {
        printf("Error allocating memory for data\n");
        return 1;
    }
    memcpy(data, line.c_str(), data_size);
    data[data_size] = '\0'; // Ensure null-termination

    pthread_mutex_lock(&mutex);
    // if (args.warmup_ms) {
    //     printf("Warming up for %dms...\n", args.warmup_ms);
    //     struct timespec wmup_start, wmup_stop;
    //     clock_gettime(CLOCK_MONOTONIC, &wmup_start);
    //     unsigned long elapsed_us = 0;
    //     while (elapsed_us < args.warmup_ms * 1000) {
    //         z_publisher_put(z_loan(pub), reinterpret_cast<const uint8_t*>(data), data_size, NULL);
    //         precise_sleep(args.msg_per_second);  // Modified to use precise_sleep
    //         clock_gettime(CLOCK_MONOTONIC, &wmup_stop);
    //         elapsed_us = (1000000 * (wmup_stop.tv_sec - wmup_start.tv_sec) + (wmup_stop.tv_nsec - wmup_start.tv_nsec) / 1000);
    //     }
    // }

    while (true) {
        // size_t data_size = line.size();
        // char* data = (char *)malloc(data_size + 1); // +1 for null terminator
        // if (data == NULL) {
        //     printf("Error allocating memory for data\n");
        //     continue;
        // }
        // memcpy(data, line.c_str(), data_size);
        // data[data_size] = '\0'; // Ensure null-termination

        struct timespec t_start, t_stop, t_timeout;
        clock_gettime(CLOCK_REALTIME, &t_timeout);
        t_timeout.tv_sec += PING_TIMEOUT_SEC;
        clock_gettime(CLOCK_MONOTONIC, &t_start);
        z_publisher_put(z_loan(pub), reinterpret_cast<const uint8_t*>(data), data_size, NULL); //mhn4d --> take care of reintrpret_cast
        precise_sleep(args.msg_per_second);  // Modified to use precise_sleep
        pthread_cond_timedwait(&cond, &mutex, &t_timeout);
        clock_gettime(CLOCK_MONOTONIC, &t_stop);
        int result = (1000000 * (t_stop.tv_sec - t_start.tv_sec) + (t_stop.tv_nsec - t_start.tv_nsec) / 1000);
        printf("%d bytes: rtt=%luµs, lat=%luµs\n", data_size, result, result / 2);
        // free(data);
    }

    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
    // free(result);
    free(data);
    z_drop(z_move(sub));
    z_drop(z_move(pub));
    z_close(z_move(session));
    return 0;
}

char* getopt(int argc, char** argv, char option) {
    for (int i = 0; i < argc; i++) {
        size_t len = strlen(argv[i]);
        if (len >= 2 && argv[i][0] == '-' && argv[i][1] == option) {
            if (len > 2 && argv[i][2] == '=') {
                return argv[i] + 3;
            } else if (i + 1 < argc) {
                return argv[i + 1];
            }
        }
    }
    return NULL;
}

struct args_t parse_args(int argc, char** argv) {
    struct args_t args = {DEFAULT_JSON_FILE, DEFAULT_PKT_SIZE, DEFAULT_PING_NB, DEFAULT_WARMUP_MS, NULL, 0, DEFAULT_MSG_PER_SEC};  // Added default msg_per_second

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            args.help_requested = 1;
        } else if (i + 1 < argc) {
            if (strcmp(argv[i], "-s") == 0) args.size = atoi(argv[++i]);
            else if (strcmp(argv[i], "-n") == 0) args.number_of_pings = atoi(argv[++i]);
            else if (strcmp(argv[i], "-w") == 0) args.warmup_ms = atoi(argv[++i]);
            else if (strcmp(argv[i], "-c") == 0) args.config_path = argv[++i];
            else if (strcmp(argv[i], "-r") == 0) args.msg_per_second = atoi(argv[++i]);  // Parsing new -r option
            else if (strcmp(argv[i], "-f") == 0) args.json_file = argv[++i];
        }
    }

    return args;
}
