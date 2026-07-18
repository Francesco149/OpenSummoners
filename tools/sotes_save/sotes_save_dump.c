/* sotes_save_dump — standalone .sdt inspector (host tool; libc-only).
 *
 * Usage:
 *   sotes_save_dump [--json] FILE.sdt [FILE.sdt ...]   inspect specific files
 *   sotes_save_dump [--json] [--dir DIR]               scan DIR/savedata*.sdt (00..15)
 * Default DIR = $SOTES_SAVE_DIR or the Steam EN-SE user dir.
 *
 * Reusable verification of tools/sotes_save.  Build (host, in nix develop):
 *   cc -O2 -Wall -o build/sotes_save_dump \
 *      tools/sotes_save/sotes_save_dump.c tools/sotes_save/sotes_save.c
 */
#include "sotes_save.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_DIR "/mnt/c/Program Files (x86)/Steam/steamapps/common/sotes/user"

static void print_text(const char *label, const sotes_save_info *s) {
    if (!s->ok) { printf("%-14s  <unreadable / not a .sdt>\n", label); return; }
    printf("%-14s  %s  handle=%#x  file=%zuB body=%zuB  key=%#04x checksum=%#010x\n",
           label, s->valid ? "VALID(MainQuest)" : "INVALID",
           s->handle, s->file_size, s->body_size, s->hdr.key, s->checksum);
    printf("                party(%d):", s->party_count);
    if (s->party_count == 0) printf(" (none found)");
    for (int i = 0; i < s->party_count; ++i)
        printf(" %s(Lv.base %d)", s->party[i].name[0] ? s->party[i].name : "?",
               s->party[i].combat_level_max);
    printf("\n");
    if (s->ph_present) {
        printf("                header grid:");
        for (int k = 0; k < 16; ++k) printf(" %u", s->ph[k]);
        printf("\n");
    }
}

static void print_json(const char *label, const sotes_save_info *s) {
    printf("{\"name\":\"%s\",\"ok\":%s,\"valid\":%s,\"handle\":%u,"
           "\"file_size\":%zu,\"body_size\":%zu,\"key\":%u,\"checksum\":%u,\"party\":[",
           label, s->ok ? "true" : "false", s->valid ? "true" : "false",
           s->handle, s->file_size, s->body_size, s->hdr.key, s->checksum);
    for (int i = 0; i < s->party_count; ++i)
        printf("%s{\"name\":\"%s\",\"code\":%u,\"combat_level_max\":%d}",
               i ? "," : "", s->party[i].name, s->party[i].code, s->party[i].combat_level_max);
    printf("],\"header_grid\":[");
    for (int k = 0; k < 16 && s->ph_present; ++k) printf("%s%u", k ? "," : "", s->ph[k]);
    printf("]}\n");
}

int main(int argc, char **argv) {
    int json = 0;
    const char *dir = getenv("SOTES_SAVE_DIR");
    if (!dir) dir = DEFAULT_DIR;
    const char *files[64]; int nfiles = 0;

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--json")) json = 1;
        else if (!strcmp(argv[i], "--dir") && i + 1 < argc) dir = argv[++i];
        else if (nfiles < 64) files[nfiles++] = argv[i];
    }

    if (nfiles > 0) {
        for (int i = 0; i < nfiles; ++i) {
            sotes_save_info s;
            sotes_save_read(files[i], &s);
            (json ? print_json : print_text)(files[i], &s);
        }
        return 0;
    }

    /* scan DIR/savedata00..15.sdt */
    for (int n = 0; n < 16; ++n) {
        char path[512], label[24];
        snprintf(path, sizeof path, "%s/savedata%02d.sdt", dir, n);
        snprintf(label, sizeof label, "savedata%02d", n);
        FILE *f = fopen(path, "rb");
        if (!f) continue;
        fclose(f);
        sotes_save_info s;
        sotes_save_read(path, &s);
        (json ? print_json : print_text)(label, &s);
    }
    return 0;
}
