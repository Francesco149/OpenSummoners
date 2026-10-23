/*
 * win_app.{c,h} — the application shell under WinMain (config + text files, version checks, the asset DLL load,
 * the launcher dialog, the window, the message pump, the shutdown, the window procedure, the mutex, GDI helpers).
 * Retail layout; rhost-verified inside retail (REPLACE).  Names by VA.
 *
 *   FUN_005afb00  wm_5afb00   FUN_005afb90  wm_5afb90   FUN_005afbf0  wm_5afbf0   FUN_005afc90  wm_5afc90
 *   FUN_005aff00  wm_5aff00   FUN_005aff20  wm_5aff20   FUN_005affe0  wm_5affe0   FUN_005b01e0  wm_5b01e0
 *   FUN_005b0260  wm_5b0260   FUN_005b0280  wm_5b0280   FUN_005b0390  wm_5b0390   FUN_005b03e0  wm_5b03e0
 *   FUN_005b0450  wm_5b0450   FUN_005b0890  wm_5b0890   FUN_005b0c60  wm_5b0c60   FUN_005b0ce0  wm_5b0ce0
 *   FUN_005b0d70  wm_5b0d70   FUN_005b0dd0  wm_5b0dd0   FUN_005b0ee0  wm_5b0ee0   FUN_005b1030  wm_5b1030
 *   FUN_005b10d0  wm_5b10d0   FUN_005b12e0  wm_5b12e0   FUN_005b14c0  wm_5b14c0   FUN_005b5a60  wm_5b5a60
 *   FUN_005b5a90  wm_5b5a90   FUN_005b5ac0  wm_5b5ac0   FUN_005b5af0  wm_5b5af0   FUN_005b5d50  wm_5b5d50
 *   FUN_005b5d90  wm_5b5d90   FUN_005b5dc0  wm_5b5dc0   FUN_005b5ef0  wm_5b5ef0   FUN_005b5f50  wm_5b5f50
 */
#ifndef OSS_WIN_APP_H
#define OSS_WIN_APP_H

#include <stdint.h>

#include <windows.h>

#include "world_layout.h"

int32_t wm_5afb00(wl_char *self, char * a2, uint16_t a3, int32_t a4);
int32_t wm_5afb90(wl_char *self, uint16_t a2, int32_t a3, int32_t a4, void * a5);
int32_t wm_5afbf0(wl_char *self, uint16_t a2, int32_t a3, int32_t a4);
int32_t wm_5afc90(wl_char *self);
void wm_5aff00(wl_char *self);
int32_t wm_5aff20(wl_char *self, char * a2, char * a3);
int32_t wm_5affe0(wl_char *self, char * FileName, uint32_t a3);
void wm_5b01e0(wl_char *self, int32_t hInstance);
int32_t wm_5b0260(wl_char *self, char a2);
int32_t wm_5b0280(wl_char *self, OSVERSIONINFOA * lpVersionInformation);
int32_t wm_5b0390(int32_t * a1, int32_t * a2);
int32_t wm_5b03e0(uint16_t * a1, uint16_t * a2);
int32_t wm_5b0450(wl_char *self, char * lptstrFilename, uint16_t * a3);
int32_t wm_5b0890(char * a1, int32_t a2, int32_t a3);
int32_t wm_5b0c60(wl_char *self, char * a2, char * a3);
int32_t wm_5b0ce0(wl_char *self, char * a2, char * a3);
int32_t wm_5b0d70(wl_char *self, int32_t hInstance, char * a3);
int32_t wm_5b0dd0(wl_char *self, char * a2, int32_t a3);
int32_t wm_5b0ee0(wl_char *self, int32_t hInstance);
int32_t wm_5b1030(void);
void wm_5b10d0(void);
int32_t wm_5b12e0(int32_t hWnd, uint32_t Msg, uint32_t wParam, int32_t lParam);
void wm_5b14c0(wl_char *self);
int32_t wm_5b5a60(wl_char *self, char * lpName);
int32_t wm_5b5a90(wl_char *self);
int32_t wm_5b5ac0(uint32_t a1);
int32_t wm_5b5af0(int32_t a1);
int32_t wm_5b5d50(uint32_t a1);
int32_t wm_5b5d90(int32_t a1, int32_t a2);
int32_t wm_5b5dc0(int32_t a1, int32_t a2, int32_t a3);
int32_t wm_5b5ef0(int32_t hdc, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t wm_5b5f50(uint32_t a1, int32_t a2, int32_t a3, int32_t a4);

#endif /* OSS_WIN_APP_H */
