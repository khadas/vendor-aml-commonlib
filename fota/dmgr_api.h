#ifndef __DMGR_API_H
#define __DMGR_API_H

#include <stdarg.h>

typedef int dmgr_t;
struct notifier_block;

enum NOTIFY {
    NOTIFY_USER_BEGIN = 0,
    NOTIFY_CHECKVERSION_PREPARE,
    NOTIFY_CHECKVERSION_ABORTED,
    NOTIFY_CHECKVERSION_FINISHED,
    NOTIFY_DOWNLOAD_PREPARE,
    NOTIFY_DOWNLOAD_ABORTED,
    NOTIFY_DOWNLOAD_TRANSLATING,
    NOTIFY_DOWNLOAD_FINISHED,
    NOTIFY_DOWNLOAD_PARTTIAL_FINISHED,
    NOTIFY_SOCKET_CONNECT_PREPARE,
    NOTIFY_SOCKET_CONNECT_ABORTED,
    NOTIFY_SOCKET_CONNECT_FINISHED,
    NOTIFY_SOCKET_CONNECT_CLOSED,
    NOTIFY_SOCKET_SEND_PREPARE,
    NOTIFY_SOCKET_SEND_TRANSLATING,
    NOTIFY_SOCKET_SEND_ABORTED,
    NOTIFY_SOCKET_SEND_FINISHED,
    NOTIFY_SOCKET_RECV_PREPARE,
    NOTIFY_SOCKET_RECV_TRANSLATING,
    NOTIFY_SOCKET_RECV_ABORTED,
    NOTIFY_SOCKET_RECV_FINISHED,
    NOTIFY_USER_END
};

enum NOTIFIER_ID {
    NOTIFY_ID_CHECK_VERSION = 0,
    NOTIFY_ID_DOWNLOAD,
    NOTIFY_ID_SOCKET,
    MAX_USER_NOTIFIERS
};

enum _DMGR_ERRNO {
    E_OK = 0,
    E_BEGIN = 300,
    E_NOMEM,
    E_INVAL_PARAM,
    E_INVAL_OPERATION,
    E_IO_FAILED,
    E_IO_TIMEDOUT,
    E_VERIFIED_FAILED,
    E_CANCELLED,
    E_NESTING_CALLED,
    E_RESERVE1 = 400,
    /* Error from server */
    E_INVAL_TOKEN,
    E_INVAL_PLATFORM,
    E_PARAM_MISSING,
    E_UNCONFIGURED_VERSION,
    E_INTERNAL_ERROR,
    E_RESERVE2 = 500,
    E_URL_MALFORMAT,
    E_COULDNT_RESOLVE_HOST,
    E_COULDNT_CONNECT,
    E_HTTP_RETURNED_ERROR,
    E_RANGE_ERROR,
    E_HTTP_POST_ERROR,
    E_BAD_DOWNLOAD_RESUME,
    E_ABORTED,
    E_AGAIN,
    E_TOO_MANY_REDIRECTS,
    E_GOT_NOTHING,
    E_SEND_ERROR,
    E_RECV_ERROR,
    E_NO_CONNECTION_AVAILABLE,
    E_END
};

#define DMGR_ERRNO(err)                 (int)(-(err))
#define SOCKET_WAIT_FOREVER             ((int)-1)

#define VERINFO_FLAGS_FORCE_PACKAGE     (int)(0x01 << 0)
typedef struct version_info {
    char *version_name;
    int  file_size;
    char *delta_id;
    char *md5sum;
    char *delta_url;
    char *release_note;
    int flags;
    char *upgrade_from_time;
    char *upgrade_to_time;
    char *upgrade_gap;
    char *meta_data;
    char *event_id;
} version_info_t;

typedef struct download_info {
    long total_bytes;
    long break_bytes;
    long saved_bytes;
    long now_bytes;
} download_info_t;

typedef struct error_value {
    int error;
} error_value_t;

typedef struct socket_sbuf {
    char *data;
    size_t size;
} socket_sbuf_t;

typedef struct notifier_data {
    union {
        struct error_value errvalue;
        struct download_info dlinfo;
        struct version_info verinfo;
        struct socket_sbufs {
            struct socket_sbuf *sock_sbufs;
            size_t count;
        } ssbufs;
    } u;
} notifier_data_t;

typedef int (*notifier_fn_t)(struct notifier_block *nb,
                             unsigned long action,
                             void *data);

typedef struct socket_connect {
    int (*create)(struct socket_connect *conn,
                  char *hostname,
                  int port,
                  int recv_timeout,
                  int send_timeout,
                  int connect_timeout,
                  int flags);
    int (*destory)(struct socket_connect *conn);
    int (*connect)(struct socket_connect *conn);
    int (*send)(struct socket_connect *conn,
                char *buf,
                size_t size,
                size_t *sent_size);
    int (*send_sbufs)(struct socket_connect *conn,
                      struct socket_sbuf *sbufs,
                      size_t count,
                      size_t *sent_size);
    int (*recv)(struct socket_connect *conn,
                char *buf,
                size_t size,
                size_t *recved_size);
    int (*close)(struct socket_connect *conn);
} socket_connect_t;

typedef struct notifier_block {
    notifier_fn_t notifier_call;
    struct notifier_block *next;
    int priority;
    int alloc_set;
} notifier_block_t;

#define init_notifier_block(nb, call, pri) \
    do {  \
        (nb)->notifier_call = call; \
        (nb)->next = NULL; \
        (nb)->priority = pri; \
        (nb)->alloc_set= 0; \
    } while (0)

#define SERVER_TRANSOPT_FOTA            ((0x00 << 0) & 0x0f)
#define SERVER_TRANSOPT_RELIANCE        ((0x01 << 0) & 0x0f)
#define SERVER_TRANSOPT_RELIANCE_ST     ((0x02 << 0) & 0x0f)
#define SERVER_TRANSOPT_VEHICLE         ((0x03 << 0) & 0x0f)
#define SERVER_TRANSOPT_IOT             ((0x04 << 0) & 0x0f)
#define SERVER_TRANSOPT_HTTPS           ((0x01 << 4) & 0xf0)
#define SERVER_TRANSOPT_HTTP            ((0x02 << 4) & 0xf0)
#define SERVER_TRANSOPT_COMPACT_HTTP    ((0x03 << 4) & 0xf0)

typedef struct policy_info {
    int so_transfer_timeout;        /* The tcp/ip receive/send data timeout */
    int so_connect_timeout;         /* The tcp/ip connection timeout */
    int so_recv_buffer_size;        /* The max size of receive buffer */
    int download_retry;             /* The retry count for download failed */
    int download_retry_time;        /* The retry delay time(seconds) for download failed */
    int server_transfer_opt;        /* The type or opt of sepcific server */
    char *sstate_cached_path;       /* The path used for cache dmgr shared-state */
    char *root_certificates;        /* Root certificates are self-signed and form
                                       the basis of an X.509-based public key
                                       infrastructure (PKI) */
    char *sub_certificates;         /* The path used for cache dmgr shared-state */
    int freespace_for_file_frac;    /* file bavail safe proportion */
} policy_info_t;

#define POLICY_INFO_INIT(name)      \
    { .so_transfer_timeout = -1,     \
      .so_connect_timeout = -1,      \
      .so_recv_buffer_size = 8192,  \
      .download_retry = 10,          \
      .download_retry_time = 5,          \
      .server_transfer_opt = SERVER_TRANSOPT_FOTA| \
                             SERVER_TRANSOPT_COMPACT_HTTP, \
      .sstate_cached_path = "/var", \
      .root_certificates  = NULL,   \
      .sub_certificates   = NULL, \
      .freespace_for_file_frac = 90 \
    }

#define POLICY_INFO(name)           \
    struct policy_info name = POLICY_INFO_INIT(name)

enum LOGGER {
    LOGGER_STDIO = 0,
    LOGGER_BUFFER,
    LOGGER_FILE
};

enum LOG_LEVEL {
    LOG_VERBOSE = 0,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_FATAL,
    LOG_OFF
};

enum LOG_TYPE {
    LOG_TYPE_CHECK = 0,
    LOG_TYPE_DOWNLOAD,
    LOG_TYPE_HTTP,
    LOG_TYPE_LAST
};

dmgr_t dmgr_alloc(struct policy_info *policy,
                  enum LOG_LEVEL default_log_level,
                  enum LOGGER logger, ...);
int dmgr_free(dmgr_t dm_id);
int dmgr_keepalive_with_server(dmgr_t dm_id);
int dmgr_register_device_to_server(dmgr_t dm_id);
int dmgr_register_socket(dmgr_t dm_id, struct socket_connect *conn);
int dmgr_alloc_notifier(dmgr_t dm_id,
                        enum NOTIFIER_ID nh_id,
                        notifier_fn_t notifier_call,
                        int priority,
                        int extra_bytes);
int dmgr_register_notifier(dmgr_t dm_id,
                           struct notifier_block *nb,
                           enum NOTIFIER_ID nh_id);
int dmgr_register_deviceinfo(dmgr_t dm_id,
                             const char *name,
                             const char *value);
int dmgr_register_salesinfo(dmgr_t dm_id,
                            const char *name,
                            const char *value);

#define SERI_KEEP_URL               "KEEP_URL"
#define SERI_REG_URL                "REG_URL"
#define SERI_CHK_URL                "CHK_URL"
#define SERI_DL_URL                 "DL_URL"
#define SERI_REPORT_DLR_URL         "REPORT_DLR_URL"
#define SERI_REPORT_UPGR_URL        "REPORT_UPGR_URL"
#define SERI_REPORT_SALES_URL       "REPORT_SALES_URL"

int dmgr_register_serverinfo(dmgr_t dm_id,
                             const char *name,
                             const char *value);

/* The public for checking version */
int dmgr_check_version(dmgr_t dm_id);
int dmgr_async_check_version(dmgr_t dm_id);
int dmgr_async_cancel_checking(dmgr_t dm_id);

/* The public interface for downloading */
#define DOWNLOAD_FILE_SEEK_BEGIN    ((long)0)
#define DOWNLOAD_FILE_SEEK_END      ((long)-1)

int dmgr_filmap_version(dmgr_t dm_id,
                        struct version_info *version,
                        char *file_path,
                        long file_seek,
                        long map_addr_start,
                        long map_size);
int dmgr_async_filmap_version(dmgr_t dm_id,
                              struct version_info *version,
                              char *file_path,
                              long file_seek,
                              long map_addr_start,
                              long map_size);

int dmgr_download_version(dmgr_t dm_id,
                          struct version_info *version,
                          char *file_path,
                          long file_seek,
                          long start_from,
                          long max_size);
int dmgr_async_download_version(dmgr_t dm_id,
                                struct version_info *version,
                                char *file_path,
                                long file_seek,
                                long start_from,
                                long max_size);

int dmgr_async_cancel_downloading(dmgr_t dm_id);

/* The public for reporting interface */
int dmgr_report_upgraded_version(dmgr_t dm_id,
                                 struct version_info *version,
                                 const char *upgraded_desc);

int dmgr_report_sales_tracker(dmgr_t dm_id);
int dmgr_set_log_level(int dm_id,
                       enum LOG_TYPE type,
                       enum LOG_LEVEL level);
int dmgr_version(char *version_buf, int size);

#endif
