#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include "rpc_log.h"
#include "rpc_net.h"
#include "rpc_http.h"
#include "rpc_common.h"
#include "mini_google_master_svr.h"
#include "mini_google_common.h"

static bool running = true;
static void signal_proc(int signo) {
    running = false;
}

static void usage(int argc, char *argv[]) {
    printf("Usage: %s [options]\n", argv[0]);
    printf("-h/--help:      show this help\n");
    printf("-t/--threads:   specify threads number\n");
    printf("-i/--ip:        specify service ip\n");
    printf("-p/--port:      specify service port\n");
    printf("--master-ip:    specify master ip\n");
    printf("--master-port:  specify master port\n");
}

int main(int argc, char *argv[]) {
    int threads_num = 4;

    const char *ip = NULL;
    int port = 0;

    const char *master_ip = NULL;
    int master_port = 0;

    /* command line options */
    while (true) {
        static struct option long_options[] = {
            { "help", no_argument, 0, 'h'},
            { "ip", required_argument, 0, 'i'},
            { "port", required_argument, 0, 'p'},
            { "threads", required_argument, 0, 't'},
            { "master-ip", required_argument, 0, 1},
            { "master-port", required_argument, 0, 2},
            { 0, 0, 0, 0 }
        };
        int option_index = 0;
        int c = getopt_long(argc, argv, "ht:i:p:", long_options, &option_index);
        if (-1 == c) {
            break;
        }
        switch (c) {
            case 'h':
                usage(argc, argv);
                exit(0);
            case 't':
                threads_num = atoi(optarg);
                break;
            case 'i':
                ip = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 1:
                master_ip = optarg;
                break;
            case 2:
                master_port = atoi(optarg);
                break;
            default:
                usage(argc, argv);
                exit(0);
        }
    }
    if (NULL == ip && 0 == port) {
        usage(argc, argv);
        exit(0);
    }

    /* start service */
    RPC_DEBUG("threads num=%d", threads_num);
    RPC_INFO("register port=%d", port);

    signal(SIGINT, signal_proc);
    signal(SIGTERM, signal_proc);

    /* initialize server */
    mini_google_svr *svr = new mini_google_svr;

    if (0 != svr->run(threads_num)) {
        RPC_WARNING("create threads error");
        exit(-1);
    }
    if (0 != svr->bind(port)) {
        RPC_WARNING("bind error");
        exit(-1);
    }

    if (master_ip && master_port) {
        /* register */
        if (0 != register_to_master(master_ip, master_port, ip, port)) {
            RPC_WARNING("register error");
        }

        /* backup master's memory */
        if (0 != svr->backup_lookup_table(master_ip, master_port)) {
            RPC_WARNING("backup_lookup_table error");
            exit(-1);
        }
        if (0 != svr->backup_invert_table(master_ip, master_port)) {
            RPC_WARNING("backup_invert_table error");
            exit(-1);
        }
    }

    unsigned long long prev_msec = get_cur_msec();
    while (running) {
        svr->run_routine(10);

        unsigned long long curr_msec = get_cur_msec();
        if (curr_msec - prev_msec >= 10 * 1000) {

            /* update register information every 10 secs */
            if (master_ip && master_port) {
                if (0 != register_to_master(master_ip, master_port, ip, port)) {
                    RPC_WARNING("register error");
                }
            }

            /* check timeout */
            svr->check_timeout(30);

            prev_msec = curr_msec;
        }
    }

    /* unregister */
    if (master_ip && master_port) {
        if (0 != unregister_to_master(master_ip, master_port, ip, port)) {
            RPC_WARNING("unregister error");
        }
    }

    svr->stop();
    svr->join();

    delete svr;

    RPC_WARNING("server exit");
    return 0;
}
