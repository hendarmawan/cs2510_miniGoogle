#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include "rpc_log.h"
#include "rpc_net.h"
#include "rpc_http.h"
#include "rpc_common.h"
#include "mini_google_master_svr.h"
#include "uishell.h"

static void usage(int argc, char *argv[]) {
    printf("Usage: %s [options]\n", argv[0]);
    printf("-h/--help:      show this help\n");
    printf("-t/--threads:   specify threads number\n");
    printf("-i/--ip:        specify service ip\n");
    printf("-p/--port:      specify service port\n");
    printf("-n/--index:     specify that it is a task\n");
    printf("-d/--directory: specify path that should be indexed\n");
}

static void cmd_usage() {
    printf("help:           show this help\n");
    printf("query:          query for documents\n");
    printf("search:         query for documents while display the abstract\n");
    printf("retrieve:       retrieve document by its file_id\n");
    printf("abstract:       abstract document by its file_id\n");
    printf("quit:           quit the program\n");
}

/**
 * @brief index a given directory
 *
 * @param ip
 * @param port
 * @param path
 */
static void proc_index(const std::string &ip, 
        unsigned short port, const std::string &path) {
        
}

/**
 * @brief process command line
 *
 * @param ip
 * @param port
 */
static void proc_commandline(const std::string &ip, 
        unsigned short port) {

    cmd_usage();

    char line[1024] = { 0 };
    char cmd[128] = { 0 };
    char arg[128] = { 0 };
    char tmp[128] = { 0 };

    uishell ui;
    ui.open(ip, port);

    while (true) {
        printf("uishell > ");
        if (NULL != gets(line)) {
            sscanf(line, "%s%[ \t]%[^\n]", cmd, tmp, arg);

            /* show help */
            if (0 == strcasecmp(cmd, "help")) {
                cmd_usage();
            } 
            /* quit program */
            else if (0 == strcasecmp(cmd, "quit")) {
                printf("bye!\n");
                break;
            } 
            /* query for documents */
            else if (0 == strcasecmp(cmd, "query")) {
                std::vector<query_result_t> query_result;
                std::string query;
                for (const char *ptr = arg; *ptr != '\0'; ++ptr) {
                    if (*ptr == ' ') {
                        query += "%20";
                    } else {
                        query += *ptr;
                    }
                }

                if (0 > ui.query(query, query_result)) {
                    printf("query failed!\n");
                } else {
                    printf("+-------------------------------------------------------+\n");
                    printf("|  file_id                         | number | frequency |\n");
                    printf("+-------------------------------------------------------+\n");
                    for (int i = 0; i < query_result.size(); ++i) {
                        printf("| %s |%8d|   %8d|\n", query_result[i].file_id.c_str(), 
                                query_result[i].num, query_result[i].freq);
                    }
                    printf("+-------------------------------------------------------+\n");
                }
            }
            /* retrive for specific document */
            else if (0 == strcasecmp(cmd, "retrieve")) {
                std::string file_content;
                if (0 > ui.retrieve(arg, file_content)) {
                    printf("retrieve failed!\n");
                } else {
                    printf("%s\n", file_content.c_str());
                }
            }
            /* abstract for specific document */
            else if (0 == strcasecmp(cmd, "abstract")) {
                std::string file_content;
                if (0 > ui.retrieve(arg, file_content)) {
                    printf("retrieve failed!\n");
                } else {
                    std::vector<std::string> sentence_list;
                    html_to_sentences(file_content, sentence_list);
                    for (int i = 0; i < sentence_list.size(); ++i) {
                        printf("%s\n", sentence_list[i].c_str());
                    }
                    printf("\n");
                }
            }
            /* unrecognized cmd */
            else {
                printf("unrecognized command, use help to see supported commands\n");
            }
        }
    }
}

int main(int argc, char *argv[]) {
    const char *ip = NULL;
    int port = 0;
    int threads_num = 4;
    bool index = false;
    const char *path = NULL;

    /* command line options */
    while (true) {
        static struct option long_options[] = {
            { "help", no_argument, 0, 'h'},
            { "ip", required_argument, 0, 'i'},
            { "port", required_argument, 0, 'p'},
            { "threads", required_argument, 0, 't'},
            { "index", required_argument, 0, 'n'},
            { "directory", required_argument, 0, 'd'},
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
            case 'n':
                index = true;
                break;
            case 'd':
                path = optarg;
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

    if (index && !path) {
        usage(argc, argv);
        exit(0);
    }

    if (index) {
        /* handling index mode */
        proc_index(ip, port, path);
    } else {
        /* handling command line mode */
        proc_commandline(ip, port);
    }

    return 0;
}
