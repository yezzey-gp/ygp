#include <stdio.h>
#include <unistd.h>
#include "cdb/yezzeyLog.h"

EXTERNC void yezzeyLog() {
    int sleep_cnt_log = 1e5;
    while (sleep_cnt_log--) {
        //sleep(1);
    }
    const char* log_file_name = "log.txt";
    FILE* log_file = fopen(log_file_name, "a");
    fprintf(log_file, "yeZZey\n");
    fsync(fileno(log_file));
    fclose(log_file);
}