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

    struct SslContext: public mbedtls_ssl_context {
        SslContext() noexcept {
            mbedtls_ssl_init(this);
        }

        ~SslContext() noexcept {
            mbedtls_ssl_free(this);
        }
    };

    struct SslSocketImpl: public SslSocket {
        SslContext ssl;
        Input* in;
        Output* out;

        SslSocketImpl(mbedtls_ssl_config* conf, Input* in, Output* out);

        size_t readImpl(void* data, size_t len) override;
        size_t writeImpl(const void* data, size_t len) override;

        static int recv(void* ctx, unsigned char* buf, size_t len);
        static int send(void* ctx, const unsigned char* buf, size_t len);
    };

    struct SslConf: public mbedtls_ssl_config {
        SslConf() noexcept {
            mbedtls_ssl_config_init(this);
        }

        ~SslConf() noexcept {
            mbedtls_ssl_config_free(this);
        }
    };

    struct SslCert: public mbedtls_x509_crt {
        SslCert() noexcept {
            mbedtls_x509_crt_init(this);
        }

        ~SslCert() noexcept {
            mbedtls_x509_crt_free(this);
        }
    };

    struct SslKey: public mbedtls_pk_context {
        SslKey() noexcept {
            mbedtls_pk_init(this);
        }

        ~SslKey() noexcept {
            mbedtls_pk_free(this);
        }
    };

    struct SslEntropy: public mbedtls_entropy_context {
        SslEntropy() noexcept {
            mbedtls_entropy_init(this);
        }

        ~SslEntropy() noexcept {
            mbedtls_entropy_free(this);
        }
    };

    struct SslCtrDrbg: public mbedtls_ctr_drbg_context {
        SslCtrDrbg() noexcept {
            mbedtls_ctr_drbg_init(this);
        }

        ~SslCtrDrbg() noexcept {
            mbedtls_ctr_drbg_free(this);
        }
    };

    struct SslCtxImpl: public SslCtx {
        SslConf conf;
        SslCert cert;
        SslKey key;
        SslEntropy entropy;
        SslCtrDrbg ctr_drbg;

        SslCtxImpl(StringView certData, StringView keyData);

        SslSocket* create(ObjPool* pool, Input* in, Output* out) override;
    };
}

int SslSocketImpl::recv(void* ctx, unsigned char* buf, size_t len) {
    size_t n = ((SslSocketImpl*)ctx)->in->read(buf, len);

    if (n == 0) {
        return MBEDTLS_ERR_SSL_WANT_READ;
    }

    return (int)n;
}

int SslSocketImpl::send(void* ctx, const unsigned char* buf, size_t len) {
    ((SslSocketImpl*)ctx)->out->write(buf, len);

    return (int)len;
}

SslSocketImpl::SslSocketImpl(mbedtls_ssl_config* conf, Input* in, Output* out)
    : in(in)
    , out(out)
{
    checkSsl(mbedtls_ssl_setup(&ssl, conf));
    mbedtls_ssl_set_bio(&ssl, this, send, recv, nullptr);
    checkSsl(mbedtls_ssl_handshake(&ssl));
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
    checkSsl(mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, nullptr, 0));
    checkSsl(mbedtls_x509_crt_parse(&cert, (const unsigned char*)certData.data(), certData.length() + 1));
    checkSsl(mbedtls_pk_parse_key(&key, (const unsigned char*)keyData.data(), keyData.length() + 1, nullptr, 0, mbedtls_ctr_drbg_random, &ctr_drbg));
    checkSsl(mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT));
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
    mbedtls_ssl_conf_ca_chain(&conf, cert.next, nullptr);
    checkSsl(mbedtls_ssl_conf_own_cert(&conf, &cert, &key));
}

SslSocket* SslCtxImpl::create(ObjPool* pool, Input* in, Output* out) {
    return pool->make<SslSocketImpl>(&conf, in, out);
}

SslCtx* stl::SslCtx::create(ObjPool* pool, StringView cert, StringView key) {
    return pool->make<SslCtxImpl>(cert, key);
}
