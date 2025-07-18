#pragma once

#include <netinet/in.h> // **WAIT**
#include <arpa/inet.h>  // **WAIT**
#include <cstring>      // memset

#include "npshell.hpp"

constexpr int MAX_CLIENT = 40;

constexpr int FD_UNDEFINED = -1;

void singleProcessConcurrentServer(int port);
void dup2Client(int fd);
void broadcast(string msg);
