#ifndef COMMANDS_H
#define COMMANDS_H

#include "log_funcs.h"

void shutdown_mode(int s3);
void add_mode(int s3, char* source, char* target);

#endif