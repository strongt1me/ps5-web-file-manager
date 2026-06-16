#include "app_installer.h"

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <ps5/kernel.h>

#define INCASSET(name, file)                    \
  __asm__(".section .rodata\n"                  \
          ".global " #name "\n"                 \
          ".global " #name "_end\n"             \
          ".global " #name "_size\n"            \
          ".align 16\n"                         \
          #name ":\n"                           \
          ".incbin \"" file "\"\n"              \
          #name "_end:\n"                       \
          #name "_size:\n"                      \
          ".quad " #name "_end - " #name "\n"   \
          ".previous\n");                       \
  extern const uint8_t name[];                   \
  extern const size_t name##_size;

INCASSET(param_json, "assets/param.json");
INCASSET(icon0_png, "assets/icon0.png");

int sceAppInstUtilInitialize(void);
int sceAppInstUtilAppInstallAll(void *);

static int
install_file(const char *path, const uint8_t *data, size_t size) {
  FILE *f;

  if(!(f = fopen(path, "w"))) {
    return -1;
  }
  if(fwrite(data, size, 1, f) != 1) {
    fclose(f);
    return -1;
  }
  fclose(f);
  return 0;
}

static int
install_app(const char *title_id, const char *dir) {
  int (*sceAppInstUtilAppInstallTitleDir)(const char *, const char *, void *) = 0;
  const char *nid = "Wudg3Xe3heE";
  uint32_t handle;

  if(!kernel_dynlib_handle(-1, "libSceAppInstUtil.sprx", &handle)) {
    sceAppInstUtilAppInstallTitleDir =
      (void *)kernel_dynlib_resolve(-1, handle, nid);
  }

  if(sceAppInstUtilAppInstallTitleDir) {
    return sceAppInstUtilAppInstallTitleDir(title_id, dir, 0);
  }

  return sceAppInstUtilAppInstallAll(0);
}

static int
needs_update(const char *path, const uint8_t *expected, size_t expected_size) {
  struct stat st;
  uint8_t *buf;
  FILE *f;
  int mismatch;

  if(stat(path, &st) || st.st_size != (off_t)expected_size) {
    return 1;
  }
  if(!(f = fopen(path, "r"))) {
    return 1;
  }
  if(!(buf = malloc(expected_size))) {
    fclose(f);
    return 1;
  }
  if(fread(buf, 1, expected_size, f) != expected_size) {
    free(buf);
    fclose(f);
    return 1;
  }
  fclose(f);

  mismatch = memcmp(buf, expected, expected_size);
  free(buf);
  return mismatch != 0;
}

int
app_install_if_needed(void) {
  const char *title_id = TITLE_ID;
  char base_dir[256];
  char sce_sys_dir[256];
  char param_path[256];
  char icon_path[256];
  struct stat st;
  int update_needed = 0;
  int err;

  snprintf(base_dir, sizeof(base_dir), "/user/app/%s", title_id);
  snprintf(sce_sys_dir, sizeof(sce_sys_dir), "/user/app/%s/sce_sys", title_id);
  snprintf(param_path, sizeof(param_path), "%s/param.json", sce_sys_dir);
  snprintf(icon_path, sizeof(icon_path), "%s/icon0.png", sce_sys_dir);

  if(stat(base_dir, &st)) {
    update_needed = 1;
  } else if(needs_update(param_path, param_json, param_json_size) ||
            needs_update(icon_path, icon0_png, icon0_png_size)) {
    update_needed = 1;
  }

  if(!update_needed) {
    return 0;
  }

  printf("Installing launcher app %s\n", title_id);

  if((err = sceAppInstUtilInitialize())) {
    printf("sceAppInstUtilInitialize: error 0x%08X\n", err);
    return -1;
  }
  if(mkdir(base_dir, 0755) && errno != EEXIST) {
    perror("mkdir app dir");
    return -1;
  }
  if(mkdir(sce_sys_dir, 0755) && errno != EEXIST) {
    perror("mkdir sce_sys dir");
    return -1;
  }
  if(install_file(param_path, param_json, param_json_size) ||
     install_file(icon_path, icon0_png, icon0_png_size)) {
    perror("install launcher assets");
    return -1;
  }
  if((err = install_app(title_id, "/user/app/"))) {
    printf("install_app: error 0x%08X\n", err);
    return -1;
  }

  return 0;
}
