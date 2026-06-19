#pragma once

#include <microhttpd.h>

enum MHD_Result filemgr_api_request(struct MHD_Connection *conn,
                                    const char *url, const char *method,
                                    const char *body, size_t body_size);
enum MHD_Result filemgr_fs_request(struct MHD_Connection *conn);
