
#define SERVER_SOCK_PATH "/tmp/test_serv"
#define SERVER_MAX_CLIENT (10)

/* GCC visibility */                                                                                                                                                                                           
#if defined(__GNUC__) && __GNUC__ >= 4
#define SRV_EXPORT __attribute__ ((visibility("default")))
#else
#define SRV_EXPORT
#endif

void loglog(const char *fmt, ...);
