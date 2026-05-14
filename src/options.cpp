#include <vector>
#include <string>
#include <cstring>
#include <stdio.h>
#include <fstream>
#include <cstdint>
#include <cstdlib>
#include <sstream>

#include "options.h"

using namespace std;

#define MIN(A,B)    ({ __typeof__(A) __a = (A); __typeof__(B) __b = (B); __a < __b ? __a : __b; })
#define MAX(A,B)    ({ __typeof__(A) __a = (A); __typeof__(B) __b = (B); __a < __b ? __b : __a; })

#define CLAMP(x, low, high) ({\
  __typeof__(x) __x = (x); \
  __typeof__(low) __low = (low);\
  __typeof__(high) __high = (high);\
  __x > __high ? __high : (__x < __low ? __low : __x);\
  })

// Order should match: Opts::ditherPatternValues
const uint8_t ditherPatterns[Opts::ditherPatternsCount][DITHER_WIDTH][DITHER_HEIGHT] = {
    {{0, 2}, {3, 1}},     // Diagonal4
    {{0, 3}, {1, 2}},     // Horizontal4
    {{0, 1}, {3, 2}},     // Vertical4
    {{0, 1}, {1, 0}},     // Diagonal2
    {{0, 1}, {0, 1}},     // Horizontal2
    {{0, 0}, {1, 1}}      // Vertical2
 };

const uint8_t ditherPixelsSz[Opts::ditherPatternsCount] = {
    DITHER_PIXELS_4, // Diagonal4
    DITHER_PIXELS_4, // Horizontal4
    DITHER_PIXELS_4, // Vertical4
    DITHER_PIXELS_2, // Diagonal2
    DITHER_PIXELS_2, // Horizontal2
    DITHER_PIXELS_2  // Vertical2
 };


static string str_remove_path(string str_in);
static void   initArgs(quantOptions * options);
static void   showHelp(void);
static void   checkLogRandArgs(quantOptions * options);
static void   logArgs(int startIndex, int argc, const char* argv[], quantOptions * options);
static int    processArgs(int startIndex, int argc, const char* argv[], quantOptions * options);
static int    handleMetaFileArgs(quantOptions * options);


// Strip any leading path and slashes
static string str_remove_path(string str_in) {
    size_t slash_pos = str_in.find_last_of('/');
    if (slash_pos != str_in.npos)
        str_in = str_in.substr(slash_pos, str_in.length() - slash_pos);

    slash_pos = str_in.find_last_of('\\');
    if (slash_pos != str_in.npos)
        str_in = str_in.substr(slash_pos, str_in.length() - slash_pos);

    return str_in;
}


static void initArgs(quantOptions * options) {

    //default values for some params
    options->tileWidth        = TILE_WIDTH_DEFAULT;
    options->tileHeight       = TILE_HEIGHT_DEFAULT;
    options->numPalettes      = NUM_PALETTES_DEFAULT;
    options->colorsPerPalette = COLORS_PER_PALETTE_DEFAULT;
    options->bitsPerChannel   = BITS_PER_CHANNEL_DEFAULT;

    options->fractionOfPixels        = FRACTION_OF_PIXELS_DEFAULT;

    options->colorZeroBehaviour      = COLOR_ZERO_BEHAVIOUR_DEFAULT;
    // options->colorZeroRGB = COLOR_ZERO_RGB_DEFAULT; // TODO: = hexToColor(colorInput.value); RGB(0,0,0)
    // options->sharedColorRGB = SHARED_COLOR_RGB_DEFAULT;
    // options->transparentColorRGB = TRANSPARENT_COLOR_RGB_DEFAULT;

    options->ditherMethod            = DITHER_METHOD_DEFAULT;
    options->ditherPatternType       = DITHER_PATTERN_DEFAULT;
    options->ditherWeight            = DITHER_WEIGHT_DEFAULT;

    // Options unique to the console port
    options->argsForLoggingToOutput  = "";
    options->randomSeed              = RAND_SEED_DEFAULT;
    options->use_metafile            = false;
    options->verbose                 = false;
}


static void showHelp(void) {
    printf(
        "tilepalquant: console port of rilden's js tiledpalettequant\n"
        "              see https://github.com/rilden/tiledpalettequant\n"
        "\n"
        "usage: tilepalquant <file>.png [options]\n"
        "-o <filename>         Ouput file (if not used then default is <png file>_out.png)\n"
        "-h                    Show this help output\n"
        "-v                    Verbose output\n"
        "-tile_w <width>       Width  of tiles in pixels (default: 8)\n"
        "-tile_h <height>      Height of tiles in pixels (default: 8)\n"
        "-num_pals <num>       Number of palettes (default: 8)\n"
        "-cols_per_pal <num>   Number of colors per palette (default: 4)\n"
        "-bits_per_chan <num>  Bits per RGB color channel (default: 5, meaning RGB555)\n"
        "-fract_of_px <num>    TODO\n"
        "-col_zero <mode>      Color index zero behavior (default: unique)\n"
        "                        unique:\n"
        "                        shared: (may specify -shared_col)\n"
        "                        transp: transparent, from transparent pixels\n"
        "                        transp_color: (may specify -transp_col)\n"
        "-dither <mode>       Dithering (off, fast, slow) (default: off)\n"
        "-dither_pat <pat>    Dither pattern (default: diag4)\n"
        "                        (diag4, horiz4, vert4, diag2, horiz2, vert2)\n"
        "-dither_wt <num>     Dither weight (range: TODO) (default: 0.5)\n"
        "-use_metafile        Read extra options from file <inputfile>.meta (file missing not an error)\n"
        "-rand_seed <num>     Specify random number seed for conversion (default: 0 TODO)\n"
        "-rand_on             Use a random value for conversion instead of fixed seed,\n"
        "                         meaning output may not be the same each time\n"
        "\n"
    );

        // rgbColor colorZeroValue; // TODO: = hexToColor(colorInput.value);
        // rgbColor sharedColor;  // specify -shared_col
        // rgbColor transparentColor;  -transp_col

}


// If random number seed generation was turned on then
// log the generated number to the argument output as an argument
static void checkLogRandArgs(quantOptions * options) {
    if (options->randomSeed != RAND_SEED_DEFAULT) {
        // "-rand_seed " + options->randomSeed
        // Build argv style array
        string rand_arg_str = "-rand_seed " + std::to_string(options->randomSeed);
        int rand_argc = 1;
        static std::vector<char const*> rand_argv;
        rand_argv.clear();
        rand_argv.reserve(rand_argc + 1); // +1 for null terminator entry (optional with our usage)
        rand_argv.push_back(rand_arg_str.c_str());
        rand_argv.push_back(nullptr);
        logArgs(ARG_SKIP_NONE, rand_argc, rand_argv.data(), options);
    }
}


static void logArgs(int startIndex, int argc, const char* argv[], quantOptions * options) {

    // Save all args for logging into output files
    for (int i = startIndex; i < argc; ++i) {
        options->argsForLoggingToOutput.append(" ").append( str_remove_path((string)argv[i]) );
    }
}


static int processArgs(int startIndex, int argc, const char* argv[], quantOptions * options) {

    //Parse argv
    for (int i = startIndex; i < argc; ++i)
    {
        if (!strcmp(argv[i], "-h")) {
            showHelp();
        }
        else if (!strcmp(argv[i], "-v")) {
            options->verbose = true;
        }
        else if (!strcmp(argv[i], "-o")) {
            if ((i + 1) >= argc) {
                printf("Error: -o requires a filename, none specified\n");
                return EXIT_FAILURE;
            } else if (argv[i+1][0] == '-') {
                printf("Error: next argument after -o looks like an option instead of a filename (\"%s\")\n", argv[i + 1]);
                return EXIT_FAILURE;
            }
            options->outputImageFilename = argv[++i];
        }
        else if (!strcmp(argv[i], "-tile_w")) {
            options->tileWidth = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-tile_h")) {
            options->tileHeight = atoi(argv[++i]);
        }

        else if (!strcmp(argv[i], "-num_pals")) {
            options->numPalettes = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-cols_per_pal")) {
            options->colorsPerPalette = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-bits_per_chan")) {
            options->bitsPerChannel = atoi(argv[++i]);
            options->bitsPerChannel = CLAMP(options->bitsPerChannel, BITS_PER_CHANNEL_MIN, BITS_PER_CHANNEL_MAX);
        }
        else if (!strcmp(argv[i], "-fract_of_px")) {
            options->fractionOfPixels = atof(argv[++i]);
        }

        else if (!strcmp(argv[i], "-col_zero")) {
            std::string mode_str = argv[++i];
            if      (mode_str == "unique") options->colorZeroBehaviour       = Opts::indexZeroUnique;
            else if (mode_str == "shared") options->colorZeroBehaviour       = Opts::indexZeroShared;
            else if (mode_str == "transp") options->colorZeroBehaviour       = Opts::indexZeroTranspFromTransp;
            else if (mode_str == "transp_color") options->colorZeroBehaviour = Opts::indexZeroTranspFromColor;
            else {
                printf("-col_zero must be one of: unique, shared, transp, trans_color\n");
                return EXIT_FAILURE;
            }
        }

        // TODO:
        // rgbColor colorZeroValue; // TODO: = hexToColor(colorInput.value);
        // rgbColor sharedColor;
        // rgbColor transparentColor;

        else if(!strcmp(argv[i], "-dither")) {
            std::string mode_str = argv[++i];
            if      (mode_str == "off")  options->ditherMethod = Opts::ditherOff;
            else if (mode_str == "fast") options->ditherMethod = Opts::ditherFast;
            else if (mode_str == "slow") options->ditherMethod = Opts::ditherSlow;
            else {
                printf("-dither must be one of: off, fast, slow\n");
                return EXIT_FAILURE;
            }
        }

        else if(!strcmp(argv[i], "-dither_pat")) {
            std::string mode_str = argv[++i];
            if      (mode_str == "diag4")  options->ditherPatternType = Opts::ditherDiagonal4;
            else if (mode_str == "horiz4") options->ditherPatternType = Opts::ditherHorizontal4;
            else if (mode_str == "vert4")  options->ditherPatternType = Opts::ditherVertical4;
            else if (mode_str == "diag2")  options->ditherPatternType = Opts::ditherDiagonal2;
            else if (mode_str == "horiz2") options->ditherPatternType = Opts::ditherHorizontal2;
            else if (mode_str == "vert2")  options->ditherPatternType = Opts::ditherVertical2;
            else {
                printf("-dither_pat must be one of: diag4, horiz4, vert4, diag2, horiz2, vert2\n");
                return EXIT_FAILURE;
            }
        }

        else if (!strcmp(argv[i], "-dither_wt")) {
            options->ditherWeight = atof(argv[++i]);
        }

        else if(!strcmp(argv[i], "-use_metafile")) {
            options->use_metafile = true;
        }

        else if(!strcmp(argv[i], "-rand_seed")) {
            options->randomSeed = atof(argv[++i]);
        }

        else if(!strcmp(argv[i], "-rand_on")) {
            srand (time(NULL));
            options->randomSeed = rand() % 32000;
        }

        else {
            printf("Warning: Argument \"%s\" not recognized\n", argv[i]);
        }
    }

    return EXIT_SUCCESS;
}


// Read in and process a set of args from a file named <inputfile>.meta
static int handleMetaFileArgs(quantOptions * options) {

    string fname = options->sourceImageFilename + ".meta";
    ifstream metaFile(fname);
    if ( metaFile )
    {
        static vector<string> argStrings;
        static std::vector<char const*> metafile_argv; // Static for program scope, const to ensure c_str() pointers remain valid

        // Read file contents
        stringstream metaFileBuffer;
        metaFileBuffer << metaFile.rdbuf();
        metaFile.close();

        // Split strings on spaces/newlines
        string argEntry;
        argStrings.clear();
        while (metaFileBuffer >> argEntry) {
            argStrings.push_back(argEntry);
        }

        // Build argv style array
        int metafile_argc = static_cast<int>(argStrings.size());
        metafile_argv.clear();
        metafile_argv.reserve(metafile_argc + 1); // +1 for null terminator entry (optional with our usage)
        for (const auto& s : argStrings) {
            metafile_argv.push_back(s.c_str());
        }
        metafile_argv.push_back(nullptr);

        // Append args to logged ones and then process them
        logArgs(ARG_SKIP_NONE, metafile_argc, metafile_argv.data(), options);
        if (processArgs(ARG_SKIP_NONE, metafile_argc, metafile_argv.data(), options) == EXIT_FAILURE)
            return EXIT_FAILURE;

    } else {
        printf("Warning: -use_metafile specified but no meta file found at: %s\n", fname.c_str());
    }

    return EXIT_SUCCESS;
}


int processArgs(int argc, char* argv[], quantOptions * options) {

    initArgs(options);

    if (argc < 2) {
        showHelp();
        return EXIT_SUCCESS;
    }

    if (!strcmp(argv[ARG_AT_INPUT_FILENAME], "-h")) {
        showHelp();
    }
    else if (argv[ARG_AT_INPUT_FILENAME][0] == '-') {
        printf("Error: input filename looks like an option instead of a filename (\"%s\")\n", argv[ARG_AT_INPUT_FILENAME]);
        return EXIT_FAILURE;
    }

    //default params
    options->sourceImageFilename = argv[ARG_AT_INPUT_FILENAME];
    options->outputImageFilename = argv[ARG_AT_INPUT_FILENAME];
    options->outputImageFilename = options->outputImageFilename.substr(0, options->outputImageFilename.size() - 4) + "_out.png";

    logArgs(ARG_AT_INPUT_FILENAME, argc, (const char **)argv, options);
    if (processArgs(ARG_AFTER_INPUT_FILENAME, argc, (const char **)argv, options) == EXIT_FAILURE)
        return EXIT_FAILURE;

    if (options->use_metafile) {
        if (handleMetaFileArgs(options) == EXIT_FAILURE)
        return EXIT_FAILURE;
    }


    // Finalize some values based on options
    options->totalPaletteColors    = options->numPalettes * options->colorsPerPalette;
    options->outputLogArgsFilename = options->outputImageFilename + ".convert_args.txt";

    // TODO: ditherPattern is used like so:
    /// const index = ditherPattern[pixel.x & 1][pixel.y & 1];
    memcpy(options->ditherPattern, ditherPatterns[options->ditherPatternType], DITHER_PATTERN_AR_SZ);
    options->ditherPixels = ditherPixelsSz[options->ditherPatternType];

    checkLogRandArgs(options);
    if (options->verbose) {
        printf("Arguments: %s\n", options->argsForLoggingToOutput.c_str());
    }

    // TODO:
    //     if (totalPaletteColors > 256) {  -> Emit png in RGB instead Indexed

    return EXIT_SUCCESS;
}
