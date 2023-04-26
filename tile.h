struct tileopt {
    unsigned opt;
#define OPT_FONT	1	/* Needs a font */
    uint_fast8_t	ink;
    uint_fast8_t	paper;	
};

#define RES_EOF		0
#define RES_TILE	1
#define RES_FONT	2

#define PATH_FONTRES	"font.res"

#define MAX_TILE_SIZE	64	/* 8x8 RGB332 */

#define TILE_FMT_8X8	0
#define TILE_FMT_6X8	1
#define TILE_FMT_8X12	2	/* Sixel expands this format */
#define TILE_FMT_RGB8x8	3	/* RGB 332 8x8 */
#define TILE_FMT_2X2	4	/* Block graphic tiles */
#define TILE_FMT_2x3	5
#define TILE_FMT_ANSI	6	/* ANSI symbol mapping */
#define TILE_FMT_COLOUR	7	/* Colour table only map (eg vt132) */

#ifdef TILE_DRIVER

struct tile_driver {
    void (*tile_set)(uint_fast8_t y, uint_fast8_t x, uint_fast8_t tile);
    uint8_t (*tile_get)(uint_fast8_t y, uint_fast8_t x);
    void (*tile_sync)(void);
    void (*tile_font)(uint_fast8_t, uint8_t *);
    int (*tile_load)(uint8_t *);
    int (*tile_colour)(uint8_t *, uint_fast8_t);
    int (*tile_init)(void);
    const char *name;
    /* May want property info ? */
};

extern struct tile_driver tile_ops;

extern uint8_t tile_workspace[512];

extern uint_fast8_t off_y, off_x;
extern uint_fast8_t clip_left, clip_right;
extern uint_fast8_t clip_top, clip_bottom;

extern uint_fast8_t tile_size;
extern uint_fast8_t need_font;

extern uint8_t tile_dirty[25];		/* FIXME: max lines ? */
extern uint8_t tile_cmap[256];
extern int vid_fd;

extern void tile_sync_none(void);
extern void tile_set_buffered(uint_fast8_t y, uint_fast8_t x, uint_fast8_t tile);
extern int tile_colour_direct(uint_fast8_t *tile, uint_fast8_t colour);
extern int tile_colour_none(uint_fast8_t *tile, uint_fast8_t colour);
extern int tile_init_none(void);
extern uint8_t tile_get_buffered(uint_fast8_t y, uint_fast8_t x);
extern int tile_load_ansi(uint8_t *bits);
extern int tile_load_none(uint8_t *bits);
extern void tile_font_none(uint_fast8_t tile, uint8_t *bits);
extern int tile_load_font(void);
extern int tile_load_tiles(const char *path, uint_fast8_t start, uint_fast8_t num, uint8_t *ptr);
extern void tile_unload_tiles(uint8_t *ptr, uint_fast8_t num);
extern int tile_initialize(struct tileopt *t);

extern uint8_t tile_next;
extern uint8_t tile_last;

#endif

extern uint_fast8_t draw_bg;
extern uint_fast8_t draw_fg;
extern uint_fast8_t tile_rows;
extern uint_fast8_t tile_cols;
extern uint_fast8_t tile_fmt;

extern void tile_set(uint_fast8_t y, uint_fast8_t x, uint_fast8_t tile);
extern void tile_rect(uint_fast8_t y, uint_fast8_t x, uint_fast8_t y2, uint_fast8_t x2, uint_fast8_t tile);
extern void tile_row(uint_fast8_t y, uint_fast8_t x, uint8_t *ptr, uint_fast8_t len);
extern uint8_t tile_get(uint_fast8_t y, uint_fast8_t x);
extern void tile_to_front(uint_fast8_t y, uint_fast8_t x, uint_fast8_t y2, uint_fast8_t x2);
extern void tile_clip(uint_fast8_t y, uint_fast8_t x, uint_fast8_t y2, uint_fast8_t x2);
extern void tile_set_offset(uint_fast8_t y, uint_fast8_t x);
extern void tile_sync(void);
extern void tile_cleanup(void);
extern int tile_colour(uint_fast8_t *tile, uint_fast8_t colour);


/* TODO sizing on these and ownership */
extern uint8_t tile_front[80 * 25];
extern uint8_t tile_back[80 * 25];
extern unsigned tile_stride;
