A c++ console port of rilden's excellent tile based palette quantizer for images.

For the original javascript/web version see:
https://github.com/rilden/tiledpalettequant

----

One change from the original is defaulting to deterministic
output instead of slightly different (randomly driven) output
each time. This helps for integrating into build tool-chains
so that images get converted in exactly the same way, which
means builds are consistent and reproducible.

The behavior can be changed to non-deterministic by using the
`-rand_on` option to randomly seed the RNG. This allows for
experimenting with different processing runs to see if a more
optimal output image can be found.

To reproduce the output from a given `-rand_on` run make
sure to have verbose enabled (`-v`). Then the generated RNG
seed will be visible in the console output as a
pre-formulated `-rand_seed <nnn>` argument which can be used
in place of `-rand_on`.

----

```
usage: tilepalquant <file>.png [options]
-o <filename>         Ouput file (if not used then default is <png file>_out.png)
-h                    Show this help output
-v                    Verbose output (-vv for extra debug output)
-tile_w <width>       Width  of tiles in pixels    (default: 8, range: 1-32)
-tile_h <height>      Height of tiles in pixels    (default: 8, range: 1-32)
-num_pals <num>       Number of palettes           (default: 8, range: 1-16)
-cols_per_pal <num>   Number of colors per palette (default: 4, range: 2-256)
-bits_per_chan <num>  Bits per RGB color channel   (default: 5, range: 2-8)
-frac_of_px <num>                                  (default: 0.1, range: 0.01- 10)
-col_zero <mode>      Color index zero behavior (default: unique)
                        unique:
                        shared: (may specify -shared_col)
                        transp: transparent, from transparent pixels
                        transp_color: (may specify -transp_col)
-shared_col <col>    RGB Color used by "-col_zero shared" (default: 0,0,0)
                        Entered as: r,g,b. Example: "255,128,0"
-transp_col <col>    RGB Color used by "-col_zero transp_color" (default: 255,0,255)
                        Entered as: r,g,b. Example: "255,128,0"
-dither <mode>       Dithering (off, fast, slow) (default: off)
-dither_pat <pat>    Dither pattern (default: diag4)
                        (diag4, horiz4, vert4, diag2, horiz2, vert2)
-dither_wt <num>     Dither weight                  (default: 0.5, range: 0.01-1))
-use_metafile        Read extra options from file <inputfile>.meta (missing not an error)
-rand_seed <num>     Specify random number seed for conversion (default: 0)
-rand_on             Use a random value for conversion instead of fixed seed,
                         meaning output may not be the same each time.
                         Use -v to view generated seed (for later re-use).
-export_previews     Export multiple png previews during processing

Example usage: tilepalquant in.png -cols_per_pal 16 -num_pals 2 -o out.png

```