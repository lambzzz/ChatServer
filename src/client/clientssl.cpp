#include "clientssl.h"


SSL* sync_initialize_ssl(const char* cert_path, const char* key_path, SSL_MODE mode, int fd) {
    const SSL_METHOD *method;
    SSL_CTX *ctx;
    SSL *ssl = NULL;
    // 初始化OpenSSL库
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    // 根据模式(客户端/服务端)选择合适的方法
    if (mode == SSL_MODE_SERVER) {
        method = SSLv23_server_method();
    } else if (mode == SSL_MODE_CLIENT) {
        method = SSLv23_client_method();
    } else {
        // 未知模式
        printf("Not found method");
        return NULL;
    }

    // 创建SSL上下文
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        printf("Unable to create SSL context");
        return NULL;
    }

    // // 配置SSL上下文
    // if (SSL_CTX_use_certificate_file(ctx, cert_path, SSL_FILETYPE_PEM) <= 0 || SSL_CTX_use_PrivateKey_file(ctx, key_path, SSL_FILETYPE_PEM) <= 0 ) {
    //     printf("Not found certificate or private key");
    //     SSL_CTX_free(ctx);
    //     return NULL;
    // }
    // // SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);

    // 创建SSL对象
    ssl = SSL_new(ctx);
    if (!ssl) {
        printf("Failed to create SSL object.");
        return NULL;
    }

    // 设置文件描述符
    if (SSL_set_fd(ssl, fd) == 0) {
        printf("Failed to set fd to SSL object.");
        return NULL;
    }

    // SSL握手
    if ((mode == SSL_MODE_CLIENT && SSL_connect(ssl) <= 0) ||
        (mode == SSL_MODE_SERVER && SSL_accept(ssl) <= 0)) {
        int ssl_result;
        if (mode == SSL_MODE_CLIENT) {
            ssl_result = SSL_connect(ssl);
        } else if (mode == SSL_MODE_SERVER) {
            ssl_result = SSL_accept(ssl);
        }

        int ssl_err = SSL_get_error(ssl, ssl_result);

        const char *err_str;
        switch (ssl_err) {
            case SSL_ERROR_NONE:
                err_str = "No error";
                break;
            case SSL_ERROR_SSL:
                err_str = "Error in the SSL protocol";
                break;
            case SSL_ERROR_WANT_READ:
                err_str = "SSL read operation did not complete";
                break;
            case SSL_ERROR_WANT_WRITE:
                err_str = "SSL write operation did not complete";
                break;
            case SSL_ERROR_WANT_X509_LOOKUP:
                err_str = "SSL X509 lookup operation did not complete";
                break;
            case SSL_ERROR_SYSCALL:
                err_str = "Syscall error";
                break;
            case SSL_ERROR_ZERO_RETURN:
                err_str = "SSL connection was shut down cleanly";
                break;
            case SSL_ERROR_WANT_CONNECT:
                err_str = "SSL connect operation did not complete";
                break;
            case SSL_ERROR_WANT_ACCEPT:
                err_str = "SSL accept operation did not complete";
                break;
            default:
                err_str = "Unknown error";
                break;
        }
        printf("===============SSL handshake failed. Error: %s========!\n", err_str ? err_str : "Unknown");
        unsigned long err_code = ERR_get_error();
        char* err_msg = ERR_error_string(err_code, NULL);
        printf("SSL error: %s\n", err_msg);
        return nullptr;
    }

    return ssl;
}