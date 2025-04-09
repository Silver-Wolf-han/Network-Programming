#pragma once

#include <netinet/in.h> // **WAIT**
#include <arpa/inet.h>  // **WAIT**
#include <cstring>      // memset

#include "npshell.hpp"

#define MAX_CLIENT 30

void concurentConnectionOrientedServer(int port);
