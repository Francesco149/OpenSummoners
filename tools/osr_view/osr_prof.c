/* tools/osr_view/osr_prof.c — console profiler for the scrub engine.
 *
 * Opens a .osr, then renders a spread of frames and reports where the time goes:
 * the one-time index/open vs the per-frame clear / ddraw blit-replay / RGBA
 * readback.  Console subsystem so the numbers print.  Build: make -C tools/osr_view prof
 *   build/osr_prof.exe C:\oss-osr\retail.osr [nsamples]
 */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "osr_scrub.h"

static void write_bmp(const char *path, const uint32_t *rgba, int w, int h)
{
    FILE *f = fopen(path, "wb");
    if (!f) return;
    uint32_t row = (uint32_t)((w * 3 + 3) & ~3), pix = row * (uint32_t)h;
    uint8_t fh[14] = {0}, ih[40] = {0};
    fh[0]='B'; fh[1]='M';
    *(uint32_t*)(fh+2) = 14 + 40 + pix; *(uint32_t*)(fh+10) = 14 + 40;
    *(uint32_t*)(ih+0) = 40; *(int32_t*)(ih+4) = w; *(int32_t*)(ih+8) = h;
    *(uint16_t*)(ih+12) = 1; *(uint16_t*)(ih+14) = 24;
    fwrite(fh,1,14,f); fwrite(ih,1,40,f);
    uint8_t *line = (uint8_t*)calloc(row,1);
    for (int y = h-1; y >= 0; y--) {        /* bottom-up */
        for (int x = 0; x < w; x++) {
            uint32_t p = rgba[y*w+x];        /* 0xAABBGGRR (R low) */
            line[x*3+0] = (p >> 16) & 0xff;  /* B */
            line[x*3+1] = (p >> 8) & 0xff;   /* G */
            line[x*3+2] = p & 0xff;          /* R */
        }
        fwrite(line,1,row,f);
    }
    free(line); fclose(f);
}

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "C:\\oss-osr\\retail.osr";
    /* dump mode: osr_prof <osr> dump <frame_index> <out.bmp> */
    if (argc >= 5 && !strcmp(argv[2], "dump")) {
        WNDCLASSA wc; memset(&wc,0,sizeof wc); wc.lpfnWndProc=DefWindowProcA;
        wc.hInstance=GetModuleHandle(NULL); wc.lpszClassName="osr_prof";
        RegisterClassA(&wc);
        HWND hw = CreateWindowA("osr_prof","p",0,0,0,16,16,HWND_MESSAGE,NULL,wc.hInstance,NULL);
        osr_scrub *s = osr_scrub_open((void*)hw, path);
        if (!s) { printf("open failed\n"); return 1; }
        int idx = atoi(argv[3]), w = osr_scrub_width(s), h = osr_scrub_height(s);
        uint32_t *b = (uint32_t*)malloc((size_t)w*h*4);
        osr_scrub_render_rgba(s, idx, b);
        write_bmp(argv[4], b, w, h);
        uint32_t fl=0,tk=0; osr_scrub_frame_info(s, idx, &fl, &tk);
        printf("dumped frame idx %d (flip=%u tick=%u) -> %s\n", idx, fl, tk, argv[4]);
        free(b); osr_scrub_close(s); return 0;
    }
    /* pick mode: osr_prof <osr> pick <frame_index> <px> <py> — which draw last
     * painted (px,py), plus the ±2 neighbours in the ordered draw list. */
    if (argc >= 6 && !strcmp(argv[2], "pick")) {
        WNDCLASSA wc; memset(&wc,0,sizeof wc); wc.lpfnWndProc=DefWindowProcA;
        wc.hInstance=GetModuleHandle(NULL); wc.lpszClassName="osr_prof";
        RegisterClassA(&wc);
        HWND hw = CreateWindowA("osr_prof","p",0,0,0,16,16,HWND_MESSAGE,NULL,wc.hInstance,NULL);
        osr_scrub *s = osr_scrub_open((void*)hw, path);
        if (!s) { printf("open failed\n"); return 1; }
        int idx = atoi(argv[3]), px = atoi(argv[4]), py = atoi(argv[5]);
        int d = osr_scrub_pick_draw(s, idx, px, py);
        int nd = osr_scrub_frame_ndraws(s, idx);
        osr_draw_info *info = (osr_draw_info*)calloc(nd > 0 ? nd : 1, sizeof *info);
        osr_scrub_frame_draws(s, idx, info, nd);
        uint32_t fl=0,tk=0; osr_scrub_frame_info(s, idx, &fl, &tk);
        printf("frame idx %d (flip=%u tick=%u) ndraws=%d\n", idx, fl, tk, nd);
        printf("pixel (%d,%d) last changed by draw #%d\n", px, py, d);
        for (int k = (d>2?d-2:0); k <= d+2 && k < nd; k++)
            printf("  %c#%-3d %s\n", k==d?'*':' ', k, info[k].label);
        free(info); osr_scrub_close(s); return 0;
    }

    int nsamp = argc > 2 ? atoi(argv[2]) : 300;

    /* a message-only window for the ddraw cooperative level */
    WNDCLASSA wc; memset(&wc, 0, sizeof wc);
    wc.lpfnWndProc = DefWindowProcA; wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "osr_prof";
    RegisterClassA(&wc);
    HWND hwnd = CreateWindowA("osr_prof", "p", 0, 0, 0, 16, 16, HWND_MESSAGE, NULL, wc.hInstance, NULL);

    LARGE_INTEGER freq; QueryPerformanceFrequency(&freq);
    LARGE_INTEGER a, b;

    QueryPerformanceCounter(&a);
    osr_scrub *s = osr_scrub_open((void *)hwnd, path);
    QueryPerformanceCounter(&b);
    double open_ms = (double)(b.QuadPart - a.QuadPart) * 1000.0 / freq.QuadPart;
    if (!s) { printf("open failed: %s\n", path); return 1; }

    int n = osr_scrub_frame_count(s);
    int w = osr_scrub_width(s), h = osr_scrub_height(s);
    uint32_t *buf = (uint32_t *)malloc((size_t)w * h * 4);

    double idx_ms = 0;
    osr_scrub_prof(s, &idx_ms, NULL, NULL, NULL, NULL);
    printf("opened %s: %d frames %dx%d\n", path, n, w, h);
    printf("OPEN total = %.0f ms  (index pass = %.0f ms)\n", open_ms, idx_ms);

    /* render a spread of DISTINCT frames (step so they don't collapse to one) */
    if (nsamp > n) nsamp = n;
    int step = n > nsamp ? n / nsamp : 1;
    QueryPerformanceCounter(&a);
    int done = 0;
    for (int i = 0; i < n && done < nsamp; i += step, done++)
        osr_scrub_render_rgba(s, i, buf);
    QueryPerformanceCounter(&b);
    double wall_ms = (double)(b.QuadPart - a.QuadPart) * 1000.0 / freq.QuadPart;

    double clear_ms = 0, replay_ms = 0, readback_ms = 0; long renders = 0;
    osr_scrub_prof(s, NULL, &clear_ms, &replay_ms, &readback_ms, &renders);

    printf("\nRENDERED %d sampled frames (step %d), %ld actual renders\n", done, step, renders);
    printf("  wall total      = %.1f ms  (%.2f ms/sampled-frame, %.1f fps)\n",
           wall_ms, wall_ms / (done ? done : 1), done * 1000.0 / (wall_ms ? wall_ms : 1));
    if (renders) {
        printf("  per RENDER avg:  clear=%.3f ms  blit-replay=%.3f ms  readback=%.3f ms\n",
               clear_ms / renders, replay_ms / renders, readback_ms / renders);
        printf("  totals:          clear=%.0f ms  blit-replay=%.0f ms  readback=%.0f ms\n",
               clear_ms, replay_ms, readback_ms);
    }

    free(buf);
    osr_scrub_close(s);
    return 0;
}
