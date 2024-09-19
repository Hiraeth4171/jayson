#include "./include/jayson/logging.h"
#include <stdio.h>

void log_err(const char* str) {
    perror(str);
}
