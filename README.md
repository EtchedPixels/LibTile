# LibTile
Some experiments in a portable tile based graphcis library

Nothing to see here, just some scribbling

## Concept

A small library that can render tile based graphics on a large number of
output targets with some level of efficiency. Also something small enough
to use on 8bit micros.

## Model

The display consists of a grid of fixed sized tiles. In most cases these
will line up with the physical display, but in other cases they might be
imposed arbitrarily on a bitmap or similar.

Why tiles - because they are a common format between a lot of devices. In
addition because if you need backing store for write only devices, or very
slow to read back devices, it uses little memory. It also makes some types
of updating and second level backing store much much more memory efficient.

The API provides a minimal set of interfaces to manipulate tiles. It allows
you to set a tile in the display buffer or optional backbuffer. It also has
some optimized paths for setting a group of tiles, and for filling a block
with the same tile.

Colours are attached to the tile not to the display square. This is a lowest
common denominator thing as some displays embed the colour in the tile
itself and others group tiles together in blocks with a single colour
setting.

Tile numbering is done by the library, or more accurately by the output
driver. ASCII tiles 32-126 are always the same logical numbering. The other
tile numbers are assigned by the driver so it can match hardware behaviour,
colour grouping rules and so on.

## Tile Formats

Tiles end up needing to be available in multiple formats. At the moment the
library design anticipates the following

- ASCII - a literal mapping of ASCII symbols
- ANSI - likewise for the ANSI graphic set along with a colour for each tile
- 8x8 pixels - monochrome bitmap with an attached colour
- 8x12 pixels - likewise but taller
- 6x8 pixels - likewise but squashed up (notably for MSX text mode and for things like the 480x320 TFT panels doing 80 column text)
- RGB332 - PropGFX, some TFT panels and fancier displays

The tiles (and fonts) are kept in a resource file with a very simple
structure so that a program can remain small and portable by loading a
suitable resource set.

The resource API loads a block of tiles from the resource file and assigns
them into an array. If there are more tiles than resources then they are
loaded until the resource is consumed. Tile blocks can be freed and
allocated but freeing tiles and allocating new ones may or may not change
the existing display so this is more relevant to things like theming
or game level changes.

The intended use case is an array of ascii fallback symbols which will then
be replaced (in part, whole or none) by tile numbers. Thus a program can
fall back to ASCII or partly ASCII cleanly but also gets enough info if it
wants to be clever about different tile space limits.

## Output Drivers

Experimenting at the moment:
- VT100 - ASCII mode (TODO: VT220/3xx downloadable fonts)
- ANSI - Likewise with colour and more symbols
- VT52 - Fuzix console (optional UDG TODO)
- Sixels - because

To add:
- Termcap
- Bitmap in memory space
- Bitmap with write command
- SPI/direct attach TFT panels (ILI9341 etc)
- RCbus Propeller GFX (RGB222)

There are also some displays that suffer from being weird enough they might
need their own special case driver (eg EF9345 quadrichrome)

## Resource Files

These are a very simple header followed by data blocks. Nothing is
compressed currently but the header format doesn't care about that. The
header follows a magic number (or will do) and consists of simple series
of six bytes. Byte 1 is the resource type (0 = EOF) 1 is the resource format
and 2-5 give the file offset.

To keep things sane weirder directly convertible formats (like sixel) do
format conversion as they load.
