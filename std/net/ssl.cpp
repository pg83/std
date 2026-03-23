#include "ssl.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/mem/obj_pool.h>
#include <std/str/builder.h>

#include <mbedtls/ssl.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/x509_crt.h>

using namespace stl;

namespace {
    [[noreturn]]
    void raiseSsl(int err) {
        Errno(err).raise(StringBuilder() << StringView(u8"ssl error"));
    }

    void checkSsl(int r) {
        if (r != 0) {
            raiseSsl(r);
        }
    }

    struct SslSocketImpl: public SslSocket {
        mbedtls_ssl_context ssl;
        Input* in;
        Output* out;

        SslSocketImpl(mbedtls_ssl_config* conf, Input* in, Output* out);
        ~SslSocketImpl() noexcept;

        size_t readImpl(void* data, size_t len) override;
        size_t writeImpl(const void* data, size_t len) override;

        static int recv(void* ctx, unsigned char* buf, size_t len);
        static int send(void* ctx, const unsigned char* buf, size_t len);
    };

    struct SslCtxImpl: public SslCtx {
        mbedtls_ssl_config conf;
        mbedtls_x509_crt cert;
        mbedtls_pk_context key;
        mbedtls_entropy_context entropy;
        mbedtls_ctr_drbg_context ctr_drbg;

        SslCtxImpl(StringView certData, StringView keyData);
        ~SslCtxImpl() noexcept;

        SslSocket* create(ObjPool* pool, Input* in, Output* out) override;
    };
}

int SslSocketImpl::recv(void* ctx, unsigned char* buf, size_t len) {
    auto* sock = (SslSocketImpl*)ctx;
    size_t n = sock->in->read(buf, len);

    if (n == 0) {
        return MBEDTLS_ERR_SSL_WANT_READ;
    }

    return (int)n;
}

int SslSocketImpl::send(void* ctx, const unsigned char* buf, size_t len) {
    auto* sock = (SslSocketImpl*)ctx;

    sock->out->write(buf, len);

    return (int)len;
}

SslSocketImpl::SslSocketImpl(mbedtls_ssl_config* conf, Input* in, Output* out)
    : in(in)
    , out(out)
{
    mbedtls_ssl_init(&ssl);
    checkSsl(mbedtls_ssl_setup(&ssl, conf));
    mbedtls_ssl_set_bio(&ssl, this, send, recv, nullptr);
}

SslSocketImpl::~SslSocketImpl() noexcept {
    mbedtls_ssl_free(&ssl);
}

size_t SslSocketImpl::readImpl(void* data, size_t len) {
    int r = mbedtls_ssl_read(&ssl, (unsigned char*)data, len);

    if (r == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY || r == 0) {
        return 0;
    }

    if (r < 0) {
        raiseSsl(r);
    }

    return (size_t)r;
}

size_t SslSocketImpl::writeImpl(const void* data, size_t len) {
    int r = mbedtls_ssl_write(&ssl, (const unsigned char*)data, len);

    if (r < 0) {
        raiseSsl(r);
    }

    return (size_t)r;
}

SslCtxImpl::SslCtxImpl(StringView certData, StringView keyData) {
    mbedtls_ssl_config_init(&conf);
    mbedtls_x509_crt_init(&cert);
    mbedtls_pk_init(&key);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    checkSsl(mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, nullptr, 0));
    checkSsl(mbedtls_x509_crt_parse(&cert, (const unsigned char*)certData.data(), certData.length() + 1));
    checkSsl(mbedtls_pk_parse_key(&key, (const unsigned char*)keyData.data(), keyData.length() + 1, nullptr, 0, mbedtls_ctr_drbg_random, &ctr_drbg));
    checkSsl(mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT));

    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
    mbedtls_ssl_conf_ca_chain(&conf, cert.next, nullptr);
    checkSsl(mbedtls_ssl_conf_own_cert(&conf, &cert, &key));
}

SslCtxImpl::~SslCtxImpl() noexcept {
    mbedtls_ssl_config_free(&conf);
    mbedtls_x509_crt_free(&cert);
    mbedtls_pk_free(&key);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);
}

SslSocket* SslCtxImpl::create(ObjPool* pool, Input* in, Output* out) {
    auto* sock = pool->make<SslSocketImpl>(&conf, in, out);

    checkSsl(mbedtls_ssl_handshake(&sock->ssl));

    return sock;
}

SslCtx* stl::SslCtx::create(ObjPool* pool, StringView cert, StringView key) {
    return pool->make<SslCtxImpl>(cert, key);
}
