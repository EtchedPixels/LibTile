#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define TILE_DRIVER

#include "tile.h"

/* FIXME: allocation dynamic so drivers don't waste buffer space if
   not in use */

uint8_t tilebits[128 * 16];

/*
 *	Sixel graphics driver. Mostly intended for Linux debug/test
 *	of the features.
 */

/* Seems we have to put an entire slice alas */
static void tile_sixel_put(uint8_t *tp)
{
    uint_fast8_t x;
    /* Write the upper half of each tile of the row */
    for (x = 0; x < tile_stride; x++)
        write(vid_fd, tilebits + 16 * *tp++, 8);
    tp -= tile_stride;
    write(vid_fd, "-", 1);
    /* And then the lower half */
    for (x = 0; x < tile_stride; x++)
        write(vid_fd, tilebits + 8 + 16 * *tp++, 8);
    write(vid_fd, "-", 1);
}

static void tile_sync_sixel(void)
{
    uint8_t *p = tile_dirty;
    uint_fast8_t y;
    uint8_t *tp = tile_front;

    /* Enter sixel mode top left */
    write(vid_fd, "\033[0;0H\033Pq", 9);
    for (y = 0; y < tile_rows; y++) {
        if (*p)
            tile_sixel_put(tp);
        else
            /* Next sixel row */
            write(vid_fd, "-", 2);
        *p++ = 0;
        tp += tile_stride;
        
    }
    write(vid_fd, "\033\\", 2);
}

/* Format conversions - minimal for now */
static void tile8x12_to_sixel(uint8_t *out, uint8_t *in)
{
    uint_fast8_t i, j;
    uint8_t *op = out;
    uint8_t m = 0x80;
    memset(out, 0, 12);
    /* 8 vertical stripes of 2 sets of 6 pixels */
    for (j = 0; j < 8; j++) {
        uint8_t v = 1;
        for (i = 0; i < 6; i++) {
            if (in[i] & m)
                *op |= v;
            v <<= 1;
        }
        v = 1;
        for (i = 6; i < 12; i++) {
            if (in[i] & m)
                op[8] |= v;
            v <<= 1;
        }
        m >>= 1;
        op++;
    }
    op = out;
    for (j = 0; j < 16; j++) {
        *op += 0x3F;
        op++;
    }
}        

/* FIXME: need to add load/unload support */
static int tile_load_sixel(uint8_t *bits)
{
    if (tile_next > 31)
        return -1;
    tile8x12_to_sixel(tilebits + 16 * tile_next, bits);
    return tile_next++;
}

static void tile_font_sixel(uint_fast8_t tile, uint8_t *bits)
{
    tile8x12_to_sixel(tilebits + 16 * tile, bits);
}

struct tile_driver sixel_ops = {
    tile_set_buffered,
    tile_get_buffered,
    tile_sync_sixel,
    tile_font_sixel,
    tile_load_sixel,
    tile_colour_none,
    tile_init_none,
    "sixel"
};

void tile_dev_sixel(void)
{
    tile_rows = 25;
    tile_cols = 80;
    tile_fmt = TILE_FMT_8X12;
    tile_size = 12;
    tile_next = 1;
    tile_last = 31;
    need_font = 1;
    memcpy(&tile_ops, &sixel_ops, sizeof(tile_ops));
}
