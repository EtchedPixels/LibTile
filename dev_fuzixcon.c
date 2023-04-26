#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define TILE_DRIVER

#include "tile.h"


/* Fuzix console - like VT we probably want to buffer */
static uint8_t last_x = 255;
static uint8_t last_y = 255;
static uint8_t last_colour = 255;

static char stc_buf[] = { "\033Yxxc" };
static char stc_col[] = { "\033bX\033cX" };

static void tile_set_fcon(uint_fast8_t y, uint_fast8_t x, uint_fast8_t tile)
{
    uint8_t *p = tile_front + y * tile_stride + x;
    uint8_t c;
    /* Skip existing tile */
    if (*p == tile)
        return;
    *p = tile;
    c = tile_cmap[tile];
    if (last_colour != c) {
        stc_col[2] = (c >> 4) | 0x40;
        stc_col[5] = (c & 0x0F) | 0x40;
        write(vid_fd, stc_col, 6);
        last_colour = c;
    }
    if (last_y == y && last_x == x) {
        write(vid_fd, &tile, 1);
    } else {
        stc_buf[2] = 32 + y;
        stc_buf[3] = 32 + x;
        stc_buf[5] = tile;
        last_y = y;
        last_x = x;
        write(vid_fd, stc_buf, 5);
    }
    last_x++;
}


static int tile_load_fcon(uint8_t *bits)
{
    if (tile_next == tile_last)
        return -1;
    /* Load the UDG : need to tweak the UDG ioctl a bit yet */
    /* TODO: sort out non 8x8 size */
    fcon_udg_load(tile_next, bits);
    tile_cmap[tile_next] = bits[8];	/* Colour info follows 8x8 */
    return tile_next++;
}

static uint8_t tile_init_fcon(void)
{
    struct fontinfo fi;
    uint16_t sz = ioctl(vid_fd, VTGETSIZE, 0);
    if (sz == (uint16_t)-1)
        return 1;
    /* It is a console at least */
    tile_rows = sz >> 8;
    tile_cols = sz & 0xFF;
    /* Library will eventually dynamically allocate the buffer */
    tile_stride = tile_cols;
    tile_next = tile_last = 0;

    /* TODO: Q? look through modes for best option if mode change options
       - one with supported udg formats for example */

    /* See what user defined graphics options we have */
    if (ioctl(vid_fd, VTFONTINFO, &fi) == 0) {
        tile_next = fi.udg_low;
        tile_last = fi.udg_high + 1;
        /* TODO - formats */
        swich(fi.format) {
        case FONT_INFO_8X8:
            tile_next = fi.udg_low;
            tile_last = fi.udg_high + 1;
            tile_fmt = TILE_FMT8X8;
            break;
        case FONT_INFO_6X8:
            tile_next = fi.udg_low;
            tile_last = fi.udg_high + 1;
            tile_fmt = TILE_FMT6X8;
            break;
        /* TODO */
        case FONT_INFO_8X11P16
        case FONT_INFO_8X16:
        default:
    }
    /* TODO: check if we can memory map the text console for speed */
    /* We assume any eventual probe code will try other fuzix console
       things first: bitmap modes, ADRAW etc */
    return 0;
}

#define tile_set_op	tile_set_fcon
#define tile_get_op	tile_get_buffered
#define tile_sync_op	tile_sync_none
#define tile_font_op	tile_font_none
#define tile_load_op	tile_load_fcon
#define tile_colour_op	tile_colour_direct
#define tile_init_op	tile_init_fcon

struct tile_driver fcon_ops = {
    tile_set_fcon,
    tile_get_buffered,
    tile_sync_none,
    tile_font_none,
    tile_load_fcon,
    tile_colour_direct,
    tile_init_fcon,
    "fcon"
};

void tile_dev_fcon(void)
{
    memcpy(&tile_ops, &fcon_ops, sizeof(fcon_ops));
}
