#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define TILE_DRIVER

#include "tile.h"

/* We should be able to convert easily

    6x8 font base to 8x8, 8x12 (just space a lot), sixel, rgb232

    8x8 to 8x12 tile - stretch (iffy)
    8x8 to 6x tile - merge two rows (not ideal)
    8x12 to sixel (thus 8x8 to sixel)
    8x8 to RGB232 8x8 
    2x2 and 2x3 to any other (just make char of the blocks) */

uint8_t tile_workspace[512];	/* Disk sector, usable as workspace by
                                   anything needing scratch space within
                                   the operation only */
static int workleft;		/* Used for block disk I/O */
static uint8_t *workptr;

/* Some rigging for test purposes */

uint_fast8_t draw_bg = 1;
uint_fast8_t draw_fg = 1;
uint_fast8_t off_y, off_x;
uint_fast8_t clip_left = 0, clip_right = 79;
uint_fast8_t clip_top = 0, clip_bottom = 24;
uint_fast8_t tile_rows = 25;
uint_fast8_t tile_cols = 80;

uint_fast8_t tile_fmt;
uint_fast8_t tile_size;
uint_fast8_t need_font;

uint8_t tile_front[80 * 25];
uint8_t tile_back[80 * 25];
unsigned tile_stride = 80;
int vid_fd = 1;
uint8_t tile_cmap[256];	/* IRGB fg << 4 | bg */
        /* probably will become 'per video system' for speed */

uint8_t tile_dirty[25];		/* FIXME: max lines ? */

uint8_t tile_next;		/* 0 is used for magic */
uint8_t tile_last;

struct tile_driver tile_ops;

void tile_sync_none(void)
{
}

void tile_set_buffered(uint_fast8_t y, uint_fast8_t x, uint_fast8_t tile)
{
    uint8_t *p = tile_front + y * tile_stride + x;

    /* Skip existing tile */
    if (*p == tile)
        return;
    *p = tile;
    tile_dirty[y] = 1;    
}

int tile_colour_direct(uint8_t *tile, uint_fast8_t colour)
{
    tile_cmap[*tile] = colour;
    return 1;
}

int tile_colour_none(uint_fast8_t *tile, uint_fast8_t colour)
{
    return 0;
}

int tile_init_none(void)
{
    return 0;
}

uint8_t tile_get_buffered(uint_fast8_t y, uint_fast8_t x)
{
    return tile_front[y * tile_stride + x];
}

static uint8_t do_tile_get(uint_fast8_t y, uint_fast8_t x)
{
    return tile_ops.tile_get(y, x);
}

static void do_tile_set(uint_fast8_t y, uint_fast8_t x, uint_fast8_t tile)
{
    if (draw_bg)
        tile_back[y * tile_stride + x] = tile;
    if (draw_fg)
        tile_ops.tile_set(y, x, tile);
}

void tile_set(uint_fast8_t y, uint_fast8_t x, uint_fast8_t tile)
{
    y += off_y;
    x += off_x;
    if (y < clip_top || y > clip_bottom)
        return;
    if (x < clip_left || x > clip_right)
        return;
    if (draw_bg)
        tile_back[y * tile_stride + x] = tile;
    if (draw_fg)
        tile_ops.tile_set(y, x, tile);
}

void tile_rect(uint_fast8_t y, uint_fast8_t x, uint_fast8_t y2, uint_fast8_t x2, uint_fast8_t tile)
{
    uint_fast8_t xn;

    y += off_y;
    x += off_x;
    y2 += off_y;
    x2 += off_x;

    if (y > clip_bottom || y2 < clip_top)
        return;
    if (x > clip_right || x2 < clip_left)
        return;
    /* At least partially visible */
    if (y2 > clip_bottom)
        y2 = clip_bottom;
    if (y < clip_top)
        y = clip_top;
    if (x2 > clip_right)
        x2 = clip_right;
    if (x < clip_left)
        x = clip_left;
    /* Draw clipped area */    
    while(y <= y2) {
        for (xn = x; xn < x2; xn++)
            do_tile_set(y, xn, tile);
        y++;
    }
}

void tile_row(uint_fast8_t y, uint_fast8_t x, uint8_t *ptr, uint_fast8_t len)
{
    y += off_y;
    x += off_x;

    if (y < clip_top || y > clip_bottom)
        return;
    if (x > clip_right)
        return;
    if (x + len < clip_left)
        return;
    /* Partially clipped - draw only relevant section */
    if (x < clip_left) {
        ptr += clip_left - x;
        len -= clip_left - x;
        x = clip_left;
    }
    while(len--)
        do_tile_set(y, x++, *ptr++);
}

uint8_t tile_get(uint_fast8_t y, uint_fast8_t x)
{
    y += off_y;
    x += off_x;
    if (draw_bg)
        return  tile_back[y * tile_stride + x];
    return do_tile_get(y, x);
}

void tile_to_back(uint_fast8_t y, uint_fast8_t x, uint_fast8_t y2, uint_fast8_t x2)
{
    uint_fast8_t xn;
    uint8_t *bp, *lp;

    y += off_y;
    x += off_x;
    y2 += off_y;
    x2 += off_x;

    if (y > clip_bottom || y2 < clip_top)
        return;
    if (x > clip_right || x2 < clip_left)
        return;
    /* At least partially visible */
    if (y2 > clip_bottom)
        y2 = clip_bottom;
    if (y < clip_top)
        y = clip_top;
    if (x2 > clip_right)
        x2 = clip_right;
    if (x < clip_left)
        x = clip_left;
    /* Process the area. Might be worth having a fast path for
       'yes there is a front buffer' */
    lp = tile_back + tile_stride * y;
    while(y <= y2) {
        bp = lp;
        for (xn = x; xn < x2; xn++)
            *bp++ = do_tile_get(y, xn);
        y++;
        lp += tile_stride;
    }    
}

void tile_to_front(uint_fast8_t y, uint_fast8_t x, uint_fast8_t y2, uint_fast8_t x2)
{
    uint_fast8_t xn;
    uint8_t *bp, *lp;

    y += off_y;
    x += off_x;
    y2 += off_y;
    x2 += off_x;

    if (y > clip_bottom || y2 < clip_top)
        return;
    if (x > clip_right || x2 < clip_left)
        return;
    /* At least partially visible */
    if (y2 > clip_bottom)
        y2 = clip_bottom;
    if (y < clip_top)
        y = clip_top;
    if (x2 > clip_right)
        x2 = clip_right;
    if (x < clip_left)
        x = clip_left;
    /* Process the area. Might be worth having a fast path for
       'yes there is a front buffer' */
    lp = tile_back + tile_stride * y;
    while(y <= y2) {
        bp = lp;
        for (xn = x; xn < x2; xn++)
            do_tile_set(y, xn, *bp++);
        y++;
        lp += tile_stride;
    }    
}

void tile_clip(uint_fast8_t y, uint_fast8_t x, uint_fast8_t y2, uint_fast8_t x2)
{
    /* TODO clip against screen size */
    clip_top = y;
    clip_bottom = y2;
    clip_left = x;
    clip_right = x2;
}

void tile_set_offset(uint_fast8_t y, uint_fast8_t x)
{
    off_y = y;
    off_x = x;
}

/* Sync pending to display */
void tile_sync(void)
{
    tile_ops.tile_sync();
}

void tile_cleanup(void)
{
}

/* Setting a tile colour may renumber it so pass a pointer to the tile tab
   entry */
int tile_colour(uint_fast8_t *tile, uint_fast8_t colour)
{
    return tile_ops.tile_colour(tile, colour);
}

/* ANSI tile format is ANSI 8bit code, tile (and ASCII works same way) */
int tile_load_ansi(uint8_t *bits)
{
    tile_cmap[*bits] = bits[1];
    return *bits;
}

int tile_load_none(uint8_t *bits)
{
    return -1;
}

void tile_font_none(uint_fast8_t tile, uint8_t *bits)
{
}

static uint_fast8_t res_read(int fd, uint8_t *ptr, unsigned len)
{
    while(1) {
        if (workleft >= len) {
            memcpy(ptr, workptr, len);
            workptr += len;
            workleft -= len;
            return 0;
        }
        if (workleft) {
            memcpy(ptr, workptr, workleft);
            len -= workleft;
            ptr += workleft;
        }
        workptr = tile_workspace;
        workleft = read(fd, tile_workspace, sizeof(tile_workspace));
        if (workleft <= 0)
            return 1;
    }
}

static uint_fast8_t res_find(int fd, uint8_t code, uint8_t sub, unsigned off)
{
    static uint8_t r[6];
    off_t pos;
    if (lseek(fd, 0L, SEEK_SET) < 0)
        return 1;
    workleft = 0;
    while(res_read(fd, r, 6) == 0) {
        if (r[0] == 0)
            return 1;
        if (r[0] == code && r[1] == sub) {
            pos = r[2] | (r[3] << 8) | (r[4] << 16) | (r[5] << 24);
            pos += off;
            if (lseek(fd, pos, SEEK_SET) < 0)
                return -1;
            workleft = 0;
            return 0;
        }
    }
    return 1;
}


/*
 *	Load a range of tiles. The tiles are assumed to be in priority
 *	order and we will load them until we run out of room in the tile
 *	mapping of the device. Each tile we load we put the assigned id
 *	into the passed array.
 *
 *	The intended use case is that you can pass a table populated with
 *	suitable ASCII fallbacks for text mode then load tiles over the
 *	top in order to get what you can in graphical tiles.
 *
 *	The table pointer can be NULL and this is intenrally used for loading
 *	the font if required.
 *
 *	TODO; fallbacks and conversions
 *
 *	All these functions want turning to use a simple buffering
 *	algorithm for block read size and to do some conversions
 */
static uint_fast8_t res_load_tiles(int fd, uint_fast8_t tile, uint_fast8_t num, uint8_t *ptr)
{
    uint8_t buf[MAX_TILE_SIZE];
    int n;
    uint_fast8_t r = 0;

    if (res_find(fd, RES_TILE, tile_fmt, tile * tile_size))
        return 0;

    while(num--) {
        /* Tiles have an additional colour byte following unlike fonts */
        if (res_read(fd, buf, tile_size + 1))
            break;
        n = tile_ops.tile_load(buf);
        if (n < 0)
            break;
        *ptr++ = n;		/* Update tile table */
        r++;
    }
    return r;
}

/* Load the ASCII 32-127 font. We use a different helper as the ASCII
   set is always logically mapped to tile 32-127, not allocated as needed.
   It might have a different internal map but that is a driver problem */
static uint8_t res_load_font(int fd)
{
    uint_fast8_t n;
    uint8_t buf[MAX_TILE_SIZE];
    unsigned size = tile_size;

    if (res_find(fd, RES_FONT, tile_fmt, 0)) {
        /* Try fallback: blank the rest and expand 6x8 tiles */
        /* Doesn't look pretty in other modes but beats not working */
        memset(buf, 0, MAX_TILE_SIZE);
        size = 8;
        if (res_find(fd, RES_FONT, TILE_FMT_6X8, 0))
            return 1;
    }
    for (n = 32; n < 128; n++) {
        if (res_read(fd, buf, size)) {
            fprintf(stderr, "short font at %d?\n", n);
            return 1;
        }
        tile_ops.tile_font(n, buf);
    }
    return 0;
}

int tile_load_font(void)
{
    int fd;
    /* Font is available via other means (eg ASCII + UDG driver) */
    if (need_font == 0)
        return 0;
    fd = open(PATH_FONTRES, O_RDONLY);
    if (fd == -1) {
        perror(PATH_FONTRES);
        return -1;
    }
    if (res_load_font(fd)) {
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

int tile_load_tiles(const char *path, uint_fast8_t start, uint_fast8_t num, uint8_t *ptr)
{
    int r;
    int fd = open(path, O_RDONLY);
    if (fd == -1)
        return -1;
    r = res_load_tiles(fd, start, num, ptr);
    close(fd);
    return r;
}

void tile_unload_tiles(uint8_t *ptr, uint_fast8_t num)
{
//TODO   while(num--)
//TODO        tile_ops.tile_free(*ptr++);    
}

int tile_initialize(struct tileopt *t)
{
    /* For now we don't do much */
    /* Need to call an interface specific helper */
    atexit(tile_cleanup);
    /* Set some default colour for the font */
    memset(tile_cmap + ' ', (t->ink << 4) | t->paper, 96);
    if (tile_ops.tile_init())
        return -1;
    if (t->opt & OPT_FONT)
        if (tile_load_font() < 0)
            return -1;
    return 0;
}

static struct tileopt myopt = {
    OPT_FONT,
    0,
    7
};

static uint8_t ttab[2] = { ' ', '#' };

int main(int argc, char *argv[])
{
    /* Need to work out how we do autoselect etc */
    tile_dev_sixel();
    if (tile_initialize(&myopt) < 0) {
        fprintf(stderr, "%s: unable to initialize.\n", argv[0]);
        exit(1);
    }
    tile_load_tiles("test.res", 0, 2, ttab);
    /* FIXME: we need to just pull colours from the res table for
       things like ANSI */
    tile_colour(ttab, 7);
    tile_colour(ttab + 1, 0x16);
    tile_rect(0, 0, 24, 79, ttab[0]);
    tile_rect(10, 10, 20, 40, ttab[1]);
    tile_clip(15, 15, 20, 20);
    tile_rect(0, 0, 24, 79, ttab[0]);
    tile_sync();
    getchar();
}
