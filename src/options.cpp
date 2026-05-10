#include <vector>
#include <string>
#include <cstring>
#include <stdio.h>
#include <fstream>
#include <cstdint>
#include <cstdlib>
#include <sstream>

#include "options.h"


#define ARG_SKIP_NONE            0
#define ARG_AT_INPUT_FILENAME    1
#define ARG_AFTER_INPUT_FILENAME 2

using namespace std;


static string str_remove_path(string str_in);
static void   logArgs(int startIndex, int argc, const char* argv[], quantOptions * options);
static void   initArgs(quantOptions * options);
static void   showHelp(void);
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
    options->tileWidth        = 8; // TODO: ? #defines or CONSTS?
    options->tileHeight       = 8;
    options->numPalettes      = 8;
    options->colorsPerPalette = 4;
    options->bitsPerChannel   = 5;
    
    options->fractionOfPixels = 0.50;
    
    options->colorZeroBehaviour = Opts::indexZeroUnique;
    // options->colorZeroRGB; // TODO: = hexToColor(colorInput.value); RGB(0,0,0)
    // options->sharedColorRGB;
    // options->transparentColorRGB;
        
    options->ditherMethod = Opts::ditherOff;
    options->ditherPattern = Opts::ditherDiagonal4;
    options->ditherWeight  = 0.50;
       
    // Options unique to the console port
    options->argsForLoggingToOutput = "";
    
    options->use_metafile = false;
}


void showHelp(void) {
    printf(
        "tilepalquant: console port of rilden's js tiledpalettequant\n"
        "              see https://github.com/rilden/tiledpalettequant\n"
        "\n"
        "usage: tilepalquant <file>.png [options]\n"
        "-o <filename>         Ouput file (if not used then default is <png file>_out.png)\n"       
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
    );
        
        // rgbColor colorZeroValue; // TODO: = hexToColor(colorInput.value);
        // rgbColor sharedColor;  // specify -shared_col
        // rgbColor transparentColor;  -transp_col
        
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
        if (!strcmp(argv[i], "-o")) {
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
            if      (mode_str == "diag4")  options->ditherPattern = Opts::ditherDiagonal4;
            else if (mode_str == "horiz4") options->ditherPattern = Opts::ditherHorizontal4;
            else if (mode_str == "vert4")  options->ditherPattern = Opts::ditherVertical4;
            else if (mode_str == "diag2")  options->ditherPattern = Opts::ditherDiagonal2;
            else if (mode_str == "horiz2") options->ditherPattern = Opts::ditherHorizontal2;
            else if (mode_str == "vert2")  options->ditherPattern = Opts::ditherVertical2;
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

    // Finalize remaining options
    options->totalPaletteColors    = options->numPalettes * options->colorsPerPalette;
    options->outputLogArgsFilename = options->outputImageFilename + ".convert_args.txt";
    
    // TODO:
    //     if (totalPaletteColors > 256) {  -> Emit png in RGB instead Indexed
    
    return EXIT_SUCCESS;
}