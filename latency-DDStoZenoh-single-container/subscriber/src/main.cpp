#include <stdio.h>
#include <string.h>

#include "zenoh.h"

void callback(const z_sample_t *sample, void *context)
{
    z_publisher_t pub = z_loan(*(z_owned_publisher_t *)context);
#ifdef ZENOH_C // The zc_owned_payload_t API is exclusive to zenoh-c, but allows avoiding some copies.
    zc_owned_payload_t payload = zc_sample_payload_rcinc(sample);
    zc_publisher_put_owned(pub, z_move(payload), NULL);
#else
    z_publisher_put(pub, sample->payload.start, sample->payload.len, NULL);
#endif
}
void drop(void *context)
{
    z_owned_publisher_t *pub = (z_owned_publisher_t *)context;
    z_drop(pub);
    // A note on lifetimes:
    //  here, `sub` takes ownership of `pub` and will drop it before returning from its own `drop`,
    //  which makes passing a pointer to the stack safe as long as `sub` is dropped in a scope where `pub` is still
    //  valid.
}

const char *DEFAULT_PING_KEYEXPR = "DDSPerfRPingKS";
const char *DEFAULT_PONG_KEYEXPR = "*/DDSPerfRPongKS";
struct args_t
{
    uint8_t help_requested;   // -h
    char *config_path;        // -c
    const char *ping_keyexpr; // -p
    const char *pong_keyexpr; // -P
};
struct args_t parse_args(int argc, char **argv);

int main(int argc, char **argv)
{
    struct args_t args = parse_args(argc, argv);
    if (args.help_requested)
    {
        printf("-c (optional, string): the path to a configuration file for the session. If this option isn't passed, the default configuration will be used.\n");
        printf("-p (optional, string): the key expression for the ping topic. Default: DDSPerfRPingKS\n");
        printf("-P (optional, string): the key expression for the pong topic. Default: */DDSPerfRPongKS\n");
        return 1;
    }

    // Check key expressions and assign defaults if necessary
    if (!args.ping_keyexpr)
    {
        args.ping_keyexpr = DEFAULT_PING_KEYEXPR;
    }
    if (!args.pong_keyexpr)
    {
        args.pong_keyexpr = DEFAULT_PONG_KEYEXPR;
    }

    printf("Ping key Expr: %s\n", args.ping_keyexpr);
    printf("Pong key Expr: %s\n", args.pong_keyexpr);

    z_owned_config_t config = args.config_path ? zc_config_from_file(args.config_path) : z_config_default();
    z_owned_session_t session = z_open(z_move(config));
    z_keyexpr_t ping = z_keyexpr_unchecked(args.ping_keyexpr);
    z_keyexpr_t pong = z_keyexpr_unchecked(args.pong_keyexpr);
    z_owned_publisher_t pub = z_declare_publisher(z_loan(session), pong, NULL);
    z_owned_closure_sample_t respond = z_closure(callback, drop, (void *)z_move(pub));
    z_owned_subscriber_t sub = z_declare_subscriber(z_loan(session), ping, z_move(respond), NULL);
    printf("subscriber is listening. Press 'q' to quit\n");
    while (getchar() != 'q')
    {
    }
    z_drop(z_move(sub));
    z_close(z_move(session));
}

char *getopt(int argc, char **argv, char option)
{
    for (int i = 0; i < argc; i++)
    {
        size_t len = strlen(argv[i]);
        if (len >= 2 && argv[i][0] == '-' && argv[i][1] == option)
        {
            if (len > 2 && argv[i][2] == '=')
            {
                return argv[i] + 3;
            }
            else if (i + 1 < argc)
            {
                return argv[i + 1];
            }
        }
    }
    return NULL;
}

struct args_t parse_args(int argc, char **argv)
{
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-h") == 0)
        {
            return (struct args_t){.help_requested = 1, .config_path = nullptr};
        }
    }
    return (struct args_t){
        .help_requested = 0,
        .config_path = getopt(argc, argv, 'c'),
        .ping_keyexpr = getopt(argc, argv, 'p') ? getopt(argc, argv, 'p') : DEFAULT_PING_KEYEXPR,
        .pong_keyexpr = getopt(argc, argv, 'P') ? getopt(argc, argv, 'P') : DEFAULT_PONG_KEYEXPR,
    };
}