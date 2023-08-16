#define LOG_TAG "SystemControl"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef MTD_OLD
#include <linux/mtd/mtd.h>
#else
#define __user /* nothing */
#include <mtd/mtd-user.h>
#endif

#include "ubootenv.h"
#define SYS_LOGI(x...) printf(x)
#define ERROR(x...) printf(x)
#define NOTICE(x...) printf(x)
#define INFO(x...) printf(x)

#define MAX_UBOOT_RWRETRY 5

typedef struct env_image {
  uint32_t crc; /* CRC32 over data bytes */
  char data[];  /* Environment data */
} env_image_t;

typedef struct environment {
  void *image;
  uint32_t *crc;
  char *data;
} environment_t;

typedef struct env_kv {
  struct env_kv *next;
  char key[256];
  char *value;
} env_kv;

static char gs_partition_name[32] = {0};

static unsigned int gs_env_partition_size = 0;
static unsigned int gs_env_erase_size = 0;
static unsigned int gs_env_data_size = 0;

static bool gs_init_done = false;

static struct environment gs_env_data;
static struct env_kv gs_kv_header;
static pthread_mutex_t gs_kv_lock = PTHREAD_MUTEX_INITIALIZER;

//#define ENV_IMG_SHM_NAME "/uenv_shm"
#define ENV_SHM_PATH "/dev/shm"
#define ENV_IMG_SHM_NAME ENV_SHM_PATH "/uenvShm"

static struct uenv_img_shm_info {
  char lock;
  int32_t refcnt;
  uint32_t revision;
  char imgdata[0];
} *gs_env_shm_info = NULL;

static uint32_t gs_current_revision = 0;
// check the existence of /dev/shm,
// to prevent malfunctions when running in a ramdisk environment
static bool gs_shm_available = true;

#define CRC32_POLYNOMIAL 0xEDB88320

static uint32_t crc32(const char *buffer, size_t size) {
  uint32_t crc = 0xFFFFFFFF;
  size_t i, j;

  for (i = 0; i < size; i++) {
    crc ^= buffer[i];
    for (j = 0; j < 8; j++) {
      if (crc & 1) {
        crc = (crc >> 1) ^ CRC32_POLYNOMIAL;
      } else {
        crc >>= 1;
      }
    }
  }

  return ~crc;
}

static void envimg_buffer_lock() {
  int max_retry_cnt = 1000;
  while (__sync_val_compare_and_swap(&gs_env_shm_info->lock, 0, 1) &&
         max_retry_cnt-- > 0)
    usleep(1000);
}

static void envimg_buffer_unlock() {
  assert(__sync_val_compare_and_swap(&gs_env_shm_info->lock, 1, 0));
}

static char *acquire_envimg_buffer() {
  if (gs_env_shm_info && gs_env_shm_info->imgdata != NULL) {
    return gs_env_shm_info->imgdata;
  }

  uint32_t size = gs_env_partition_size + sizeof(struct uenv_img_shm_info);

  gs_shm_available = access(ENV_SHM_PATH, F_OK) != -1;

  if (!gs_shm_available) {
    gs_env_shm_info = (struct uenv_img_shm_info *)malloc(size);
    return gs_env_shm_info->imgdata;
  }

  mode_t old_umask = umask(0);
  int32_t fd = open(ENV_IMG_SHM_NAME, O_CREAT | O_RDWR, 0666);
  umask(old_umask);

  if (fd == -1) {
    fd = open(ENV_IMG_SHM_NAME, O_RDWR);
    if (fd == -1) {
      perror("open");
      goto failure;
    }
  }

  if (ftruncate(fd, size) == -1) {
    perror("ftruncate");
    goto failure;
  }

  gs_env_shm_info = (struct uenv_img_shm_info *)mmap(
      NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (gs_env_shm_info == MAP_FAILED) {
    perror("mmap");
    goto failure;
  }
  close(fd);
  return gs_env_shm_info->imgdata;
failure:
  if (gs_env_shm_info)
    munmap((char *)gs_env_shm_info, size);
  if (fd)
    close(fd);
  return NULL;
}

static void release_envimg_buffer() {
  if (gs_env_shm_info == NULL)
    return;

  if (!gs_shm_available) {
    free(gs_env_shm_info);
    gs_env_shm_info = NULL;
    return;
  }

  envimg_buffer_lock();
  bool unlink_finally = --gs_env_shm_info->refcnt <= 0;
  envimg_buffer_unlock();
  if (munmap((char *)gs_env_shm_info,
             gs_env_partition_size + sizeof(struct uenv_img_shm_info)) == -1) {
    goto failure;
  }

  if (unlink_finally && unlink(ENV_IMG_SHM_NAME) == -1) {
    goto failure;
  }
  gs_env_shm_info = NULL;
failure:
  return;
}

/* Parse a session kv */
static env_kv *env_parse_kv(void) {
  char *ptr = gs_env_data.data;
  env_kv *attr = &gs_kv_header;

  pthread_mutex_lock(&gs_kv_lock);
  bool bproc_key = true;
  bool bproc_next = false;
  while (ptr < gs_env_data.data + gs_env_data_size) {
    if (*ptr == '\0')
      break;

    if (bproc_next) {
      bproc_next = false;
      attr->next = (env_kv *)malloc(sizeof(env_kv));
      if (attr->next == NULL) {
        ERROR("[ubootenv] exit malloc error \n");
        break;
      }
      attr = attr->next;
      attr->next = NULL;
      attr->value = NULL;
      attr->key[0] = '\0';
    }

    char *pend = strchr(ptr, bproc_key ? '=' : '\0');
    if (!pend) {
      ERROR("[ubootenv] error '%s' not found, end parsing\n",
            bproc_key ? "=" : "\\0");
      break;
    }
    size_t plen = pend - ptr + 1;
    size_t maxlen = bproc_key ? sizeof(attr->key) : 4096;
    if (plen > maxlen) {
      ERROR("[ubootenv] warning '%s' too long, truncated\n",
            bproc_key ? "key" : "value");
      plen = maxlen;
    }
    if (bproc_key) {
      snprintf(attr->key, plen, "%s", ptr);
      // value should be processed
      bproc_key = false;
    } else {
      attr->value = (char *)malloc(plen);
      snprintf(attr->value, plen, "%s", ptr);
      bproc_key = true;
      bproc_next = true;
    }
    ptr = pend + 1;
  }
  pthread_mutex_unlock(&gs_kv_lock);

  return &gs_kv_header;
}

/* serialize the kv list into raw env data */
static int env_serialize_data(void) {
  int len;
  env_kv *attr = &gs_kv_header;
  char *data = gs_env_data.data;
  memset(gs_env_data.data, 0, gs_env_data_size);
  pthread_mutex_lock(&gs_kv_lock);
  do {
    len = sprintf(data, "%s=%s", attr->key, attr->value);
    if (len < 3) {
      ERROR("[ubootenv] Invalid data\n");
    } else
      data += len + 1;

    attr = attr->next;
  } while (attr);
  pthread_mutex_unlock(&gs_kv_lock);
  // update revision
  gs_current_revision++;
  gs_env_shm_info->revision = gs_current_revision;
  return 0;
}

static void env_release_kv() {
  pthread_mutex_lock(&gs_kv_lock);
  if (gs_kv_header.value) {
    free(gs_kv_header.value);
    gs_kv_header.value = NULL;
  }
  for (env_kv *pAttr = gs_kv_header.next, *pTmp; pAttr != NULL; pAttr = pTmp) {
    pTmp = pAttr->next;
    if (pAttr->value) {
      free(pAttr->value);
    }
    free(pAttr);
  }
  gs_kv_header.next = NULL;
  pthread_mutex_unlock(&gs_kv_lock);
}

static void check_load_kv() {
  if (gs_current_revision != gs_env_shm_info->revision) {
    env_release_kv();
    env_parse_kv();
    gs_current_revision = gs_env_shm_info->revision;
  }
  return;
}

static int load_bootenv() {
  int ret = 0;
  struct env_image *image;
  char *addr;

  addr = acquire_envimg_buffer();
  if (addr == NULL) {
    ERROR("[ubootenv] Not enough memory for environment (%u bytes)\n",
          gs_env_partition_size);
    return -1;
  }

  envimg_buffer_lock();
  gs_env_data.image = addr;
  image = (struct env_image *)addr;
  gs_env_data.crc = &(image->crc);
  gs_env_data.data = image->data;

  if (gs_env_shm_info->refcnt == 0) {
    int fd;
    if ((fd = open(gs_partition_name, O_RDONLY)) < 0) {
      ERROR("[ubootenv] open devices error: %s\n", strerror(errno));
      ret = -1;
      goto failure;
    }
    int32_t bytes = read(fd, gs_env_data.image, gs_env_partition_size);
    close(fd);
    if (bytes != (int32_t)gs_env_partition_size) {
      NOTICE("[ubootenv] read error 0x%x \n", bytes);
      ret = -2;
      goto failure;
    }
  }

  gs_current_revision = gs_env_shm_info->revision;
  uint32_t crc_calc = crc32(gs_env_data.data, gs_env_data_size);
  if (crc_calc != *(gs_env_data.crc)) {
    ERROR("[ubootenv] CRC Check ERROR save_crc=%08x,calc_crc = %08x \n",
          *gs_env_data.crc, crc_calc);
    ret = -3;
    goto failure;
  }
  if (env_parse_kv() == NULL) {
    ret = -4;
    goto failure;
  }
  // bootenv_print();
failure:
  if (ret == 0) {
    gs_env_shm_info->refcnt++;
  } else {
    // reset to zero if error happens for the next retries
    gs_env_shm_info->refcnt = 0;
  }
  envimg_buffer_unlock();
  return ret;
}

static const char *bootenv_get_value(const char *key) {
  if (!gs_init_done) {
    return NULL;
  }

  pthread_mutex_lock(&gs_kv_lock);
  env_kv *attr;
  for (attr = &gs_kv_header; attr && strcmp(key, attr->key); attr = attr->next)
    ;
  pthread_mutex_unlock(&gs_kv_lock);
  return attr ? attr->value : NULL;
}

/*
creat_args_flag : if true , if envvalue don't exists Creat it .
              if false , if envvalue don't exists just exit .
*/
static int bootenv_set_value(const char *key, const char *value,
                             int creat_args_flag) {
  int ret = 0;
  env_kv *attr = &gs_kv_header;
  env_kv *last = attr;
  pthread_mutex_lock(&gs_kv_lock);
  while (attr) {
    if (!strcmp(key, attr->key)) {
      if (attr->value != NULL) {
        free(attr->value);
      }
      attr->value = (char *)malloc(strlen(value) + 1);
      strcpy(attr->value, value);
      ret = 2;
      goto out;
    }
    last = attr;
    attr = attr->next;
  }

  if (creat_args_flag) {
    NOTICE("[ubootenv] ubootenv.var.%s not found, create it.\n", key);
    /*******Creat a New args*********************/
    attr = (env_kv *)malloc(sizeof(env_kv));
    attr->next = NULL;
    last->next = attr;
    snprintf(attr->key, sizeof(attr->key), "%s", key);
    attr->value = (char *)malloc(strlen(value) + 1);
    strcpy(attr->value, value);
    ret = 1;
  }
out:
  pthread_mutex_unlock(&gs_kv_lock);
  return ret;
}

static int save_bootenv() {
  int fd;
  int err;
  struct erase_info_user erase;
  struct mtd_info_user info;
  unsigned char *data = NULL;
  env_serialize_data();
  *(gs_env_data.crc) = crc32(gs_env_data.data, gs_env_data_size);

  if ((fd = open(gs_partition_name, O_RDWR)) < 0) {
    ERROR("[ubootenv] open devices error\n");
    return -1;
  }

  if (strstr(gs_partition_name, "mtd")) {
    memset(&info, 0, sizeof(info));
    err = ioctl(fd, MEMGETINFO, &info);
    if (err < 0) {
      ERROR("[ubootenv] Get MTD info error\n");
      close(fd);
      return -4;
    }

    erase.start = 0;
    if (info.erasesize > gs_env_partition_size) {
      data = (unsigned char *)malloc(info.erasesize);
      if (data == NULL) {
        ERROR("[ubootenv] Out of memory!!!\n");
        close(fd);
        return -5;
      }
      memset(data, 0, info.erasesize);
      err = read(fd, (void *)data, info.erasesize);
      if (err != (int)info.erasesize) {
        ERROR("[ubootenv] Read access failed !!!\n");
        free(data);
        close(fd);
        return -6;
      }
      memcpy(data, gs_env_data.image, gs_env_partition_size);
      erase.length = info.erasesize;
    } else {
      erase.length = gs_env_partition_size;
    }

    err = ioctl(fd, MEMERASE, &erase);
    if (err < 0) {
      ERROR("[ubootenv] MEMERASE ERROR %d\n", err);
      close(fd);
      if (info.erasesize > gs_env_partition_size) {
        free(data);
      }
      return -2;
    }

    if (info.erasesize > gs_env_partition_size) {
      lseek(fd, 0L, SEEK_SET);
      err = write(fd, data, info.erasesize);
      free(data);
    } else {
      err = write(fd, gs_env_data.image, gs_env_partition_size);
    }

  } else {
    // emmc and nand needn't erase
    err = write(fd, gs_env_data.image, gs_env_partition_size);
  }

  FILE *fp = NULL;
  fp = fdopen(fd, "r+");
  if (fp == NULL) {
    ERROR("fdopen failed!\n");
    close(fd);
    return -3;
  }

  fflush(fp);
  fsync(fd);
  fclose(fp);
  if (err < 0) {
    ERROR("[ubootenv] ERROR write, size %d \n", gs_env_partition_size);
    return -3;
  }
  return 0;
}

#define MAX_MTD_PARTITIONS 20
static struct {
    char name[16];
    int number;
} mtd_part_map[MAX_MTD_PARTITIONS];

static int mtd_part_count = -1;

static void find_mtd_partitions(void)
{
    int fd;
    char buf[1024];
    char *pmtdbufp;
    ssize_t pmtdsize;
    int r;
    fd = open("/proc/mtd", O_RDONLY);
    if (fd < 0)
        return;
    buf[sizeof(buf) - 1] = '\0';
    pmtdsize = read(fd, buf, sizeof(buf) - 1);
    pmtdbufp = buf;
    while (pmtdsize > 0) {
        int mtdnum, mtdsize, mtderasesize;
        char mtdname[16];
        mtdname[0] = '\0';
        mtdnum = -1;
        r = sscanf(pmtdbufp, "mtd%d: %x %x %15s",
                   &mtdnum, &mtdsize, &mtderasesize, mtdname);
        if ((r == 4) && (mtdname[0] == '"')) {
            char *x = strchr(mtdname + 1, '"');
            if (x) {
                *x = 0;
            }
            INFO("mtd partition %d, %s\n", mtdnum, mtdname + 1);
            if (mtd_part_count < MAX_MTD_PARTITIONS) {
                strcpy(mtd_part_map[mtd_part_count].name, mtdname + 1);
                mtd_part_map[mtd_part_count].number = mtdnum;
                mtd_part_count++;
            } else {
                ERROR("too many mtd partitions\n");
            }
        }
        while (pmtdsize > 0 && *pmtdbufp != '\n') {
            pmtdbufp++;
            pmtdsize--;
        }
        if (pmtdsize > 0) {
            pmtdbufp++;
            pmtdsize--;
        }
    }
    close(fd);
}
int mtd_name_to_number(const char *name)
{
    int n;
    if (mtd_part_count < 0) {
        mtd_part_count = 0;
        find_mtd_partitions();
    }
    for (n = 0; n < mtd_part_count; n++) {
        if (!strcmp(name, mtd_part_map[n].name)) {
            return mtd_part_map[n].number;
        }
    }
    return -1;
}

int bootenv_init(void) {
  struct stat st;
  struct mtd_info_user info;
  int err;
  int fd;
  int ret = -1;
  int i = 0;
  if (gs_init_done)
    return 0;
  int id = mtd_name_to_number("ubootenv");
  if (id >= 0) {
  sprintf(gs_partition_name, "/dev/mtd%d", id);
  if ((fd = open (gs_partition_name, O_RDWR)) < 0) {
    ERROR("open device(%s) error : %s \n",gs_partition_name, strerror(errno) );
    return -2;
  }

  memset(&info, 0, sizeof(info));
  err = ioctl(fd, MEMGETINFO, &info);
  if (err < 0) {
    ERROR("get MTD info error\n");
    close(fd);
    return -3;
  }

  gs_env_erase_size = info.erasesize;
  gs_env_partition_size = info.size;
  gs_env_data_size = gs_env_partition_size - sizeof(long);
  close(fd);
  } else if (!stat("/dev/nand_env", &st)) {
    sprintf(gs_partition_name, "/dev/nand_env");
    gs_env_partition_size = 0x10000;
#if defined(MESON8_ENVSIZE) || defined(GXBABY_ENVSIZE) ||                      \
    defined(GXTVBB_ENVSIZE) || defined(GXL_ENVSIZE)
    gs_env_partition_size = 0x10000;
#elif defined(A1_ENVSIZE)
    gs_env_partition_size = 0x2000;
#endif
    gs_env_data_size = gs_env_partition_size - sizeof(uint32_t);
    INFO("[ubootenv] using /dev/nand_env with size(%d)(%d)\n",
         gs_env_partition_size, gs_env_data_size);
  } else if (!stat("/dev/env", &st)) {
    INFO("[ubootenv] stat /dev/env OK\n");
    sprintf(gs_partition_name, "/dev/env");
    gs_env_partition_size = 0x10000;
#if defined(MESON8_ENVSIZE) || defined(GXBABY_ENVSIZE) ||                      \
    defined(GXTVBB_ENVSIZE) || defined(GXL_ENVSIZE)
    gs_env_partition_size = 0x10000;
#endif
    gs_env_data_size = gs_env_partition_size - sizeof(uint32_t);
    INFO("[ubootenv] using /dev/env with size(%d)(%d)\n", gs_env_partition_size,
         gs_env_data_size);
  } else if (!stat("/dev/block/env", &st)) {
    INFO("[ubootenv] stat /dev/block/env OK\n");
    sprintf(gs_partition_name, "/dev/block/env");
    gs_env_partition_size = 0x10000;
#if defined(MESON8_ENVSIZE) || defined(GXBABY_ENVSIZE) ||                      \
    defined(GXTVBB_ENVSIZE) || defined(GXL_ENVSIZE)
    gs_env_partition_size = 0x10000;
#endif
    gs_env_data_size = gs_env_partition_size - sizeof(uint32_t);
    INFO("[ubootenv] using /dev/block/env with size(%d)(%d)\n",
         gs_env_partition_size, gs_env_data_size);
  } else if (!stat("/dev/block/ubootenv", &st)) {
    sprintf(gs_partition_name, "/dev/block/ubootenv");
    if ((fd = open(gs_partition_name, O_RDWR)) < 0) {
      ERROR("[ubootenv] open device(%s) error\n", gs_partition_name);
      return -2;
    }

    memset(&info, 0, sizeof(info));
    err = ioctl(fd, MEMGETINFO, &info);
    if (err < 0) {
      fprintf(stderr, "get MTD info error\n");
      close(fd);
      return -3;
    }

    gs_env_erase_size = info.erasesize; // 0x20000;//128K
    gs_env_partition_size = info.size;  // 0x8000;
    gs_env_data_size = gs_env_partition_size - sizeof(long);
    close(fd);
  }

  while (i < MAX_UBOOT_RWRETRY && ret < 0) {
    i++;
    ret = load_bootenv();
    if (ret < 0)
      ERROR("[ubootenv] Cannot read %s: %d.\n", gs_partition_name, ret);
    if (ret < -2)
      release_envimg_buffer();
  }

  if (i >= MAX_UBOOT_RWRETRY) {
    ERROR("[ubootenv] read %s failed \n", gs_partition_name);
    return -2;
  }

  gs_init_done = true;

  return 0;
}

int bootenv_update(const char *name, const char *value) {
  if (!gs_init_done) {
    ERROR("[ubootenv] not inited\n");
    return -1;
  }

  envimg_buffer_lock();
  check_load_kv();

  INFO("[ubootenv] update_bootenv_variable name [%s]: value [%s] \n", name,
       value);

  const char *variable_value = bootenv_get_value(name);
  if (!variable_value)
    variable_value = "";

  if (!strcmp(value, variable_value)) {
    envimg_buffer_unlock();
    return 0;
  }

  bootenv_set_value(name, value, 1);

  int i = 0;
  int ret = -1;
  while (i < MAX_UBOOT_RWRETRY && ret < 0) {
    i++;
    ret = save_bootenv();
    if (ret < 0)
      ERROR("[ubootenv] Cannot write %s: %d.\n", gs_partition_name, ret);
  }

  if (i < MAX_UBOOT_RWRETRY) {
    INFO("[ubootenv] Save ubootenv to %s succeed!\n", gs_partition_name);
  }

  envimg_buffer_unlock();
  return ret;
}

const char *bootenv_get(const char *key) {
  if (!gs_init_done) {
    ERROR("[ubootenv] not inited\n");
    return NULL;
  }
  envimg_buffer_lock();
  check_load_kv();
  const char *v = bootenv_get_value(key);
  envimg_buffer_unlock();
  return v;
}

void bootenv_print(void) {
  pthread_mutex_lock(&gs_kv_lock);
  env_kv *attr = &gs_kv_header;
  while (attr != NULL) {
    SYS_LOGI("[ubootenv] key: [%s]\n", attr->key);
    SYS_LOGI("[ubootenv] value: [%s]\n\n", attr->value);
    attr = attr->next;
  }
  pthread_mutex_unlock(&gs_kv_lock);
}

__attribute__((destructor)) void _deinit() {
  if (!gs_init_done)
    return;
  release_envimg_buffer();
  gs_env_data.image = NULL;
  gs_env_data.crc = NULL;
  gs_env_data.data = NULL;
  env_release_kv();
}

