#pragma once

#include <iostream>
#include <cstring>
#include <string>
#include <cstdlib>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <fcntl.h>


#include "npshell.hpp"

#define MAX_CLIENT 30

using namespace std;

struct User {
    int id;
    int socket;
    string name;
    string ip;
    int port;
};

void broadcast(const string& msg);
void send_to_user(int scok, const string& msg);

