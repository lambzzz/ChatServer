#ifndef CLIENTSSL_H
#define CLIENTSSL_H

#include <stdio.h> 
#include <sys/socket.h> 
#include "openssl/ssl.h"
#include "openssl/err.h"

enum SSL_MODE {
    SSL_MODE_SERVER,
    SSL_MODE_CLIENT
};

SSL* sync_initialize_ssl(const char* cert_path, const char* key_path, SSL_MODE mode, int fd);

#endif