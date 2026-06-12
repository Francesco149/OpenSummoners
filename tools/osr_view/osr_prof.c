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

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "C:\\oss-osr\\retail.osr";
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
