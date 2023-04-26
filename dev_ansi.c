#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define TILE_DRIVER

#include "tile.h"

static uint8_t last_x = 255;
static uint8_t last_y = 255;
static uint8_t last_colour = 255;

static uint8_t ansi_cmap[] = "04152637";
    
/* Really mostly for Linux testing */
static void tile_set_ansi(uint_fast8_t y, uint_fast8_t x, uint_fast8_t tile)
{
    char buf[32];
    uint8_t *p = tile_front + y * tile_stride + x;
    uint8_t c;

    /* Skip existing tile */
    if (*p == tile)
        return;
    *p = tile;
    c = tile_cmap[tile];
    /* Need to do cursor pos optimizing and maybe buffering */
    /* Also need to look up colour of tile if in tile not text range */
    if (c != last_colour) {
        sprintf(buf, "\033[3%c;4%cm", ansi_cmap[(c >> 4) & 7], ansi_cmap[c & 7]);
        write(vid_fd, buf, strlen(buf));
    }
    if (last_y == y && last_x == x) {
        write(vid_fd, &tile, 1);
    } else {
        sprintf(buf, "\033[%d;%dH%c", y, x, tile);
        write(vid_fd, buf, strlen(buf));
    }
    last_y = y;
    last_x = x + 1;
}

struct tile_driver ansi_ops = {
    tile_set_ansi,
    tile_get_buffered,
    tile_sync_none,
    tile_font_none,
    tile_load_none,
    tile_colour_direct,
    tile_init_none,
    "ansi"
};

struct tile_driver vt100_ops = {
    tile_set_ansi,
    tile_get_buffered,
    tile_sync_none,
    tile_font_none,
    tile_load_none,
    tile_colour_none,
    tile_init_none,
    "ansi"
};

void tile_dev_ansi(void)
{
    tile_fmt = TILE_FMT_ANSI;
    tile_size = 0;
    tile_rows = 25;
    tile_cols = 80;
    memcpy(&tile_ops, &ansi_ops, sizeof(tile_ops));
}

void tile_dev_vt100(void)
{
    tile_fmt = TILE_FMT_COLOUR;
    tile_size = 0;
    tile_rows = 25;
    tile_cols = 80;
    memcpy(&tile_ops, &vt100_ops, sizeof(tile_ops));
}
