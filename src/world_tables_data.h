/* world_tables_data.h — accessors for the generated in-game world tables.
 *
 * The two compiled-in tables the in-game engine (FUN_0059f2c0) reads on map
 * entry, lifted verbatim from the retail EXE's .rdata by
 * tools/extract/game_world_tables.py --emit-c.  The raw bytes live in the
 * generated world_tables_data.c; game_world.c parses them the way the engine
 * does (copy + FUN_00585000 cross-reference).
 *
 *   AREA registry  (&DAT_00693848)  0x40-byte stride, zero-dword0 terminated.
 *   ROOM registry  (&DAT_006940c8)  0x150-byte stride, zero-dword0 terminated;
 *                  entry [0] is a header sentinel (dword0 == 0xf423f).
 *
 * Both blobs INCLUDE the trailing all-zero terminator entry, so a faithful
 * zero-terminated walk (as the engine does) sees the end-of-table marker.
 */
#ifndef OSS_WORLD_TABLES_DATA_H
#define OSS_WORLD_TABLES_DATA_H

/* Raw AREA table bytes (incl. terminator entry).  *len = byte length. */
const unsigned char *world_tables_area(unsigned long *len);

/* Raw ROOM registry bytes (incl. terminator entry).  *len = byte length. */
const unsigned char *world_tables_room(unsigned long *len);

#endif /* OSS_WORLD_TABLES_DATA_H */
