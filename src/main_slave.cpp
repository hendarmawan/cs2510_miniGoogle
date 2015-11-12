#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include "rpc_log.h"
#include "rpc_net.h"
#include "rpc_http.h"
#include "rpc_common.h"
#include "mini_google_slave_svr.h"
#include "task_consumer.h"
#include "file_mngr.h"

#define DS_IP "127.0.0.1"
#define DS_PORT 8000
#define RPC_ID 520
#define RPC_NAME "mini_google_slave"
#define RPC_VERSION "1.0.0"

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
}

int main(int argc, char *argv[]) {
    const char *ip = NULL;
    int port = 0;
    int threads_num = 4;
    int consumers_num = 1;

    /* command line options */
    while (true) {
        static struct option long_options[] = {
            { "help", no_argument, 0, 'h'},
            { "ip", required_argument, 0, 'i'},
            { "port", required_argument, 0, 'p'},
            { "threads", required_argument, 0, 't'},
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
            default:
                usage(argc, argv);
                exit(0);
        }
    }
    if (0 == port || ip == NULL) {
        usage(argc, argv);
        exit(0);
    }

    /* start service */
    RPC_DEBUG("threads num=%d", threads_num);
    RPC_INFO("register ip=%s", ip);
    RPC_INFO("register port=%d", port);

    signal(SIGINT, signal_proc);
    signal(SIGTERM, signal_proc);

    file_mngr::create_instance();

    /* initialize server */
    mini_google_slave_svr *svr = new mini_google_slave_svr;

    if (0 != svr->run(threads_num)) {
        RPC_WARNING("create threads error");
        exit(-1);
    }
    if (0 != svr->bind(port)) {
        RPC_WARNING("bind error");
        exit(-1);
    }

    /* initialize task consumer */
    task_consumer *tc = new task_consumer;
    tc->set_slave_addr(ip, port);

    if (0 != tc->run(consumers_num)) {
        RPC_WARNING("create threads error");
        exit(-1);
    }

    /* register information */
    if (0 != register_information(DS_IP, DS_PORT,
                RPC_ID, RPC_NAME, RPC_VERSION, ip, port)) {
        RPC_WARNING("register error");
    }

    unsigned long long prev_msec = get_cur_msec();
    while (running) {
        svr->run_routine(10);

        unsigned long long curr_msec = get_cur_msec();
        if (curr_msec - prev_msec >= 30 * 1000) {
            /* update register information every 10 secs */
            if (0 != register_information(DS_IP, DS_PORT,
                        RPC_ID, RPC_NAME, RPC_VERSION, ip, port)) {
                RPC_WARNING("update register information error");
            }
            prev_msec = curr_msec;
        }
    }

    tc->stop();
    tc->join();

    svr->stop();
    svr->join();

    /* delete */
    if (0 != unregister_information(DS_IP, DS_PORT,
                RPC_ID, RPC_NAME, RPC_VERSION, ip, port)) {
        RPC_WARNING("unregister error");
    }

    delete svr;
    delete tc;

    file_mngr::destroy_instance();

    RPC_WARNING("server exit");
    return 0;
}
