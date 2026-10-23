/*
 * file_stream.{c,h} — the engine's FILE layer (the file object: resource / memory bank / disk file behind one read /
 * write / close) and the MEMORY BANK (block list, cursors, the scrambled bank file save / load).  Retail layout;
 * rhost-verified inside retail (REPLACE).  Names by VA.
 *
 *   FUN_005b6060  fs_5b6060   FUN_005b60d0  fs_5b60d0   FUN_005b60e0  fs_5b60e0   FUN_005b61b0  fs_5b61b0
 *   FUN_005b6200  fs_5b6200   FUN_005b6250  fs_5b6250   FUN_005b62a0  fs_5b62a0   FUN_005b6340  fs_5b6340
 *   FUN_005b6410  fs_5b6410   FUN_005b64b0  fs_5b64b0   FUN_005b64e0  fs_5b64e0   FUN_005b64f0  fs_5b64f0
 *   FUN_005b6520  fs_5b6520   FUN_005b6580  fs_5b6580   FUN_005b65a0  fs_5b65a0   FUN_005b65b0  fs_5b65b0
 *   FUN_005b6630  fs_5b6630   FUN_005b6650  fs_5b6650   FUN_005b66a0  fs_5b66a0   FUN_005b66b0  fs_5b66b0
 *   FUN_005b66f0  fs_5b66f0   FUN_005b67c0  fs_5b67c0   FUN_005b6890  fs_5b6890   FUN_005b68f0  fs_5b68f0
 *   FUN_005b6940  fs_5b6940   FUN_005b6990  fs_5b6990   FUN_005b6bc0  fs_5b6bc0   FUN_005b6db0  fs_5b6db0
 */
#ifndef OSS_FILE_STREAM_H
#define OSS_FILE_STREAM_H

#include <stdint.h>

#include "world_layout.h"

int32_t fs_5b6060(wl_char *self);
int32_t fs_5b60d0(wl_char *self);
int32_t fs_5b60e0(wl_char *self, char * lpFileName, int32_t a3);
int32_t fs_5b61b0(wl_char *self, char * lpFileName);
int32_t fs_5b6200(wl_char *self, int32_t a2);
int32_t fs_5b6250(wl_char *self, int32_t a2);
int32_t fs_5b62a0(wl_char *self, int32_t hModule, uint16_t a3);
int32_t fs_5b6340(wl_char *self, int32_t lpBuffer, uint32_t nNumberOfBytesToRead);
int32_t fs_5b6410(wl_char *self, int32_t lpBuffer, uint32_t nNumberOfBytesToWrite);
int32_t fs_5b64b0(wl_char *self);
int32_t fs_5b64e0(wl_char *self);
int32_t fs_5b64f0(wl_char *self);
int32_t fs_5b6520(wl_char *self, char * a2);
int32_t fs_5b6580(wl_char *self);
int32_t fs_5b65a0(wl_char *self);
int32_t fs_5b65b0(wl_char *self, int32_t a2, int32_t a3);
int32_t fs_5b6630(wl_char *self);
int32_t fs_5b6650(wl_char *self);
int32_t fs_5b66a0(wl_char *self);
int32_t fs_5b66b0(wl_char *self);
int32_t fs_5b66f0(wl_char *self, int32_t a2);
int32_t fs_5b67c0(wl_char *self, int32_t a2, uint32_t a3, uint32_t a4, int32_t a5);
int32_t fs_5b6890(wl_char *self, int32_t a2, uint32_t a3);
int32_t fs_5b68f0(wl_char *self, int32_t a2, uint32_t a3);
void fs_5b6940(wl_char *self);
int32_t fs_5b6990(wl_char *self, char * lpFileName, int32_t a3);
int32_t fs_5b6bc0(wl_char *self, char * lpFileName);
int32_t fs_5b6db0(wl_char *self, int32_t a2);

#endif /* OSS_FILE_STREAM_H */
