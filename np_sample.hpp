#pragma once

#include <netinet/in.h> // **WAIT**
#include <arpa/inet.h>  // **WAIT**
#include <cstring>      // memset

#include "npshell.hpp"

#define BACKLOG 10

void concurentConnectionOrientedServer(int port);
