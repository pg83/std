#include "ssl.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/mem/obj_pool.h>
#include <std/str/builder.h>

#include <stdlib.h>
#include <string.h>

using namespace stl;

#if __has_include(<openssl/ssl.h>)
    #include <openssl/ssl.h>
    #include <openssl/err.h>
    #include <openssl/bio.h>
    #define STD_HAVE_OPENSSL 1
#endif

#if __has_include(<mbedtls/ssl.h>)
    #include <mbedtls/ssl.h>
    #include <mbedtls/ctr_drbg.h>
    #include <mbedtls/entropy.h>
    #include <mbedtls/x509_crt.h>
    #define STD_HAVE_MBEDTLS 1
#endif

#if defined(STD_HAVE_OPENSSL)
namespace {
    namespace ossl {
        [[noreturn]]
        void raiseSsl() {
            unsigned long err = ERR_get_error();
            char buf[256];

            ERR_error_string_n(err, buf, sizeof(buf));
            Errno((int)err).raise(StringBuilder() << StringView(buf));
        }

        void checkSsl(int r) {
            if (r <= 0) {
                raiseSsl();
            }
        }

        struct BioMethod {
            BIO_METHOD* method;

            BioMethod();
            ~BioMethod() noexcept;
        };

        struct SslSocketImpl: public SslSocket {
            SSL* ssl;
            Input* in;
            Output* out;

            SslSocketImpl(SSL_CTX* ctx, BIO_METHOD* bioMethod, Input* in, Output* out);

            ~SslSocketImpl() noexcept {
                SSL_free(ssl);
            }

            size_t readImpl(void* data, size_t len) override;
            size_t writeImpl(const void* data, size_t len) override;
            void flushImpl() override;

            static int bioRead(BIO* bio, char* buf, int len);
            static int bioWrite(BIO* bio, const char* buf, int len);
            static long bioCtrl(BIO* bio, int cmd, long num, void* ptr);
        };

        struct SslCtxImpl: public SslCtx {
            SSL_CTX* ctx;
            BioMethod bioMethod;

            SslCtxImpl(StringView certData, StringView keyData);

            ~SslCtxImpl() noexcept {
                SSL_CTX_free(ctx);
            }

            SslSocket* create(ObjPool* pool, Input* in, Output* out) override;
        };
    }
}

ossl::BioMethod::BioMethod() {
    method = BIO_meth_new(BIO_get_new_index() | BIO_TYPE_SOURCE_SINK, "stl");

    BIO_meth_set_read(method, SslSocketImpl::bioRead);
    BIO_meth_set_write(method, SslSocketImpl::bioWrite);
    BIO_meth_set_ctrl(method, SslSocketImpl::bioCtrl);
}

ossl::BioMethod::~BioMethod() noexcept {
    BIO_meth_free(method);
}

int ossl::SslSocketImpl::bioRead(BIO* bio, char* buf, int len) {
    auto sock = (SslSocketImpl*)BIO_get_data(bio);

    sock->out->flush();

    size_t n = sock->in->read(buf, (size_t)len);

    if (n == 0) {
        BIO_set_retry_read(bio);

        return -1;
    }

    return (int)n;
}

int ossl::SslSocketImpl::bioWrite(BIO* bio, const char* buf, int len) {
    auto sock = (SslSocketImpl*)BIO_get_data(bio);

    sock->out->write(buf, (size_t)len);

    return len;
}

long ossl::SslSocketImpl::bioCtrl(BIO* bio, int cmd, long num, void* ptr) {
    (void)bio;
    (void)num;
    (void)ptr;

    if (cmd == BIO_CTRL_FLUSH) {
        return 1;
    }

    return 0;
}

ossl::SslSocketImpl::SslSocketImpl(SSL_CTX* ctx, BIO_METHOD* bioMethod, Input* in, Output* out)
    : in(in)
    , out(out)
{
    ssl = SSL_new(ctx);

    BIO* bio = BIO_new(bioMethod);

    BIO_set_data(bio, this);
    BIO_set_init(bio, 1);
    SSL_set_bio(ssl, bio, bio);
    checkSsl(SSL_accept(ssl));
}

size_t ossl::SslSocketImpl::readImpl(void* data, size_t len) {
    int r = SSL_read(ssl, data, (int)len);

    if (r <= 0) {
        int err = SSL_get_error(ssl, r);

        if (err == SSL_ERROR_ZERO_RETURN) {
            return 0;
        }

        raiseSsl();
    }

    return (size_t)r;
}

size_t ossl::SslSocketImpl::writeImpl(const void* data, size_t len) {
    int r = SSL_write(ssl, data, (int)len);

    if (r <= 0) {
        raiseSsl();
    }

    return (size_t)r;
}

void ossl::SslSocketImpl::flushImpl() {
    out->flush();
}

ossl::SslCtxImpl::SslCtxImpl(StringView certData, StringView keyData) {
    ctx = SSL_CTX_new(TLS_server_method());

    BIO* certBio = BIO_new_mem_buf(certData.data(), (int)certData.length());
    X509* x509 = PEM_read_bio_X509(certBio, nullptr, nullptr, nullptr);

    checkSsl(SSL_CTX_use_certificate(ctx, x509));
    X509_free(x509);
    BIO_free(certBio);

    BIO* keyBio = BIO_new_mem_buf(keyData.data(), (int)keyData.length());
    EVP_PKEY* pkey = PEM_read_bio_PrivateKey(keyBio, nullptr, nullptr, nullptr);

    checkSsl(SSL_CTX_use_PrivateKey(ctx, pkey));
    EVP_PKEY_free(pkey);
    BIO_free(keyBio);
}

SslSocket* ossl::SslCtxImpl::create(ObjPool* pool, Input* in, Output* out) {
    return pool->make<SslSocketImpl>(ctx, bioMethod.method, in, out);
}
#endif

#if defined(STD_HAVE_MBEDTLS)
namespace {
    namespace mbed {
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
            void flushImpl() override;

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
}

int mbed::SslSocketImpl::recv(void* ctx, unsigned char* buf, size_t len) {
    auto sock = (SslSocketImpl*)ctx;

    sock->out->flush();

    size_t n = sock->in->read(buf, len);

    if (n == 0) {
        return MBEDTLS_ERR_SSL_WANT_READ;
    }

    return (int)n;
}

int mbed::SslSocketImpl::send(void* ctx, const unsigned char* buf, size_t len) {
    ((SslSocketImpl*)ctx)->out->write(buf, len);

    return (int)len;
}

mbed::SslSocketImpl::SslSocketImpl(mbedtls_ssl_config* conf, Input* in, Output* out)
    : in(in)
    , out(out)
{
    checkSsl(mbedtls_ssl_setup(&ssl, conf));
    mbedtls_ssl_set_bio(&ssl, this, send, recv, nullptr);
    checkSsl(mbedtls_ssl_handshake(&ssl));
}

size_t mbed::SslSocketImpl::readImpl(void* data, size_t len) {
    int r = mbedtls_ssl_read(&ssl, (unsigned char*)data, len);

    if (r == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY || r == 0) {
        return 0;
    }

    if (r < 0) {
        raiseSsl(r);
    }

    return (size_t)r;
}

size_t mbed::SslSocketImpl::writeImpl(const void* data, size_t len) {
    int r = mbedtls_ssl_write(&ssl, (const unsigned char*)data, len);

    if (r < 0) {
        raiseSsl(r);
    }

    return (size_t)r;
}

void mbed::SslSocketImpl::flushImpl() {
    out->flush();
}

mbed::SslCtxImpl::SslCtxImpl(StringView certData, StringView keyData) {
    checkSsl(mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, nullptr, 0));
    checkSsl(mbedtls_x509_crt_parse(&cert, (const unsigned char*)certData.data(), certData.length() + 1));
    checkSsl(mbedtls_pk_parse_key(&key, (const unsigned char*)keyData.data(), keyData.length() + 1, nullptr, 0, mbedtls_ctr_drbg_random, &ctr_drbg));
    checkSsl(mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT));
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
    mbedtls_ssl_conf_ca_chain(&conf, cert.next, nullptr);
    checkSsl(mbedtls_ssl_conf_own_cert(&conf, &cert, &key));
}

SslSocket* mbed::SslCtxImpl::create(ObjPool* pool, Input* in, Output* out) {
    return pool->make<SslSocketImpl>(&conf, in, out);
}
#endif

namespace {
    SslCtx* createOpenSsl([[maybe_unused]] ObjPool* pool, [[maybe_unused]] StringView cert, [[maybe_unused]] StringView key) {
#if defined(STD_HAVE_OPENSSL)
        return pool->make<ossl::SslCtxImpl>(cert, key);
#else
        return nullptr;
#endif
    }

    SslCtx* createMbedTls([[maybe_unused]] ObjPool* pool, [[maybe_unused]] StringView cert, [[maybe_unused]] StringView key) {
#if defined(STD_HAVE_MBEDTLS)
        return pool->make<mbed::SslCtxImpl>(cert, key);
#else
        return nullptr;
#endif
    }
}

SslCtx* stl::SslCtx::create(ObjPool* pool, StringView cert, StringView key) {
    auto env = getenv("USE_SSL_ENGINE");

    if (env) {
        if (strcmp(env, "openssl") == 0) {
            return createOpenSsl(pool, cert, key);
        }

        if (strcmp(env, "mbedtls") == 0) {
            return createMbedTls(pool, cert, key);
        }
    }

    if (auto ctx = createOpenSsl(pool, cert, key); ctx) {
        return ctx;
    }

    return createMbedTls(pool, cert, key);
}
