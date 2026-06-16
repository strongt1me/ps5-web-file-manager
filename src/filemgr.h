#pragma once

#include <microhttpd.h>

enum MHD_Result filemgr_api_request(struct MHD_Connection *conn,
                                    const char *url);
enum MHD_Result filemgr_fs_request(struct MHD_Connection *conn);
