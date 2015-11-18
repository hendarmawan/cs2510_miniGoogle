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
    const char *ip = NULL;
    int port = 0;
    const char *master_ip = NULL;
    int master_port = 0;

    int threads_num = 4;
    int consumers_num = 10;

    /* command line options */
    while (true) {
        static struct option long_options[] = {
            { "help", no_argument, 0, 'h'},
            { "ip", required_argument, 0, 'i'},
            { "port", required_argument, 0, 'p'},
            { "master-ip", required_argument, 0, 1},
            { "master-port", required_argument, 0, 2},
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
    if (0 == port || ip == NULL || 0 == master_port || master_ip == NULL) {
        usage(argc, argv);
        exit(0);
    }

    /* start service */
    RPC_DEBUG("threads num=%d", threads_num);
    RPC_INFO("my ip=%s", ip);
    RPC_INFO("my port=%d", port);
    RPC_INFO("master ip=%s", master_ip);
    RPC_INFO("master port=%d", master_port);

    signal(SIGINT, signal_proc);
    signal(SIGTERM, signal_proc);

    file_mngr::create_instance(true);

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
    tc->set_master_addr(master_ip, master_port);
    tc->set_slave_addr(ip, port);

    if (0 != tc->run(consumers_num)) {
        RPC_WARNING("create threads error");
        exit(-1);
    }

    unsigned long long prev_msec = get_cur_msec();
    while (running) {
        svr->run_routine(10);
    }

    tc->stop();
    tc->join();

    svr->stop();
    svr->join();

    delete svr;
    delete tc;

    file_mngr::destroy_instance();

    RPC_WARNING("server exit");
    return 0;
}
