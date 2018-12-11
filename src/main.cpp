#include <iostream>
#include <iomanip>
#include <string>
#include <stdio.h>
#include "lib.h"
#include <unistd.h>
using namespace std;

#define OUT(x) cout << #x << " = " << x << endl
#define O(x) cout << x << endl

void display_usage(char **argv)
{
    printf("Usage: %s lib1 lib2 lib3 ...\n", argv[0]);
    printf(" or\n");
    printf("Usage: ls libclang*.a | xargs %s\n", argv[0]);
    printf("\n");
    printf("\t-h\t\tprint help info\n");
    printf("\t-w\t\tuse weak symbol\n");
    printf("\t-v\t\tverbose\n");
    exit(1);
}

int main(int argc, char** argv) {
    OUT(argc);
    if (argc == 1) {
        display_usage(argv);
    }

    const char* optString = "hwv";
	int opt = getopt(argc, argv, optString);
    while (opt != -1) {
        switch (opt) {
            case 'w':
                g_opt.useWeakSymbol = true;
                break;

            case 'v':
                g_opt.verbose = true;
                break;
                 
            case 'h':   /* fall-through is intentional */
            case '?':
                display_usage(argv);
                break;
                 
            default:
                /* You won't actually get here. */
                break;
        }
         
        opt = getopt(argc, argv, optString);
    }

    for (int i = optind; i < argc; ++i) {
        LibManager::getInstance().addLib(argv[i]);
    }

    LibManager::getInstance().run();
    LibManager::getInstance().dumpDeps();
    LibManager::getInstance().dumpLinkArgs();
    return 0;
}

