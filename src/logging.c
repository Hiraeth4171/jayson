#include "./include/jayson/logging.h"
#include <stdio.h>

void jayson_log_err(const char* str) {
    perror(str);
}
