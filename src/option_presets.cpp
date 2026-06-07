
#include "common.h"
#include "options.h"

using namespace std;

// See: https://github.com/Optiroc/SuperFamiconv/blob/68e3477d27b80da6e921dcceaa334cc234e591ad/src/Mode.h
#define PRESET_GBC_BG_NAME  "gbc-bg"
#define PRESET_GBC_BG_OPTS            "-num_pals 8 -cols_per_pal 4 -bits_per_chan 5"
#define PRESET_GBC_SPR_NAME "gbc-spr"
#define PRESET_GBC_SPR_OPTS           "-num_pals 1 -cols_per_pal 4 -bits_per_chan 5 -col_zero transp_color"

// Doesn't account for fixed palette of NES
#define PRESET_NES_BG_NAME  "nes-bg"
#define PRESET_NES_BG_OPTS            "-num_pals 4 -cols_per_pal 4 -col_zero shared -tile_w 16 -tile_h 16"
#define PRESET_NES_SPR_NAME "nes-spr"
#define PRESET_NES_SPR_OPTS           "-num_pals 4 -cols_per_pal 4 -col_zero transp_color"

#define PRESET_SMS_BG_NAME  "sms-bg"
#define PRESET_SMS_BG_OPTS            "-num_pals 2 -cols_per_pal 16 -bits_per_chan 2"
#define PRESET_SMS_SPR_NAME "sms-spr"
#define PRESET_SMS_SPR_OPTS           "-num_pals 1 -cols_per_pal 16 -bits_per_chan 2 -col_zero transp_color"

#define PRESET_GG_BG_NAME  "gg-bg"
#define PRESET_GG_BG_OPTS            "-num_pals 2 -cols_per_pal 16 -bits_per_chan 3"
#define PRESET_GG_SPR_NAME "gg-spr"
#define PRESET_GG_SPR_OPTS           "-num_pals 1 -cols_per_pal 16 -bits_per_chan 3 -col_zero transp_color"

#define PRESET_MD_BG_NAME  "md-bg"
#define PRESET_MD_BG_OPTS            "-num_pals 4 -cols_per_pal 16 -bits_per_chan 3 -col_zero shared"
#define PRESET_MD_SPR_NAME "md-spr"
#define PRESET_MD_SPR_OPTS           "-num_pals 1 -cols_per_pal 16 -bits_per_chan 3 -col_zero transp_color"

#define PRESET_PCE_BG_NAME  "pce-bg"
#define PRESET_PCE_BG_OPTS            "-num_pals 16 -cols_per_pal 16 -bits_per_chan 3 -col_zero shared"
#define PRESET_PCE_SPR_NAME "pce-spr"
#define PRESET_PCE_SPR_OPTS           "-num_pals 1 -cols_per_pal 16 -bits_per_chan 3 -col_zero transp_color"

#define PRESET_COUNT 12

// Presets
const char * presets[PRESET_COUNT][2] = {
    {PRESET_GBC_BG_NAME,  PRESET_GBC_BG_OPTS},
    {PRESET_GBC_SPR_NAME, PRESET_GBC_SPR_OPTS},
    {PRESET_NES_BG_NAME,  PRESET_NES_BG_OPTS},
    {PRESET_NES_SPR_NAME, PRESET_NES_SPR_OPTS},
    {PRESET_SMS_BG_NAME,  PRESET_SMS_BG_OPTS},
    {PRESET_SMS_SPR_NAME, PRESET_SMS_SPR_OPTS},
    {PRESET_GG_BG_NAME,   PRESET_GG_BG_OPTS},
    {PRESET_GG_SPR_NAME,  PRESET_GG_SPR_OPTS},
    {PRESET_MD_BG_NAME,   PRESET_MD_BG_OPTS},
    {PRESET_MD_SPR_NAME,  PRESET_MD_SPR_OPTS},
    {PRESET_PCE_BG_NAME,   PRESET_PCE_BG_OPTS},
    {PRESET_PCE_SPR_NAME,  PRESET_PCE_SPR_OPTS}
};


// Invoked by "-help-presets"
void showHelpPresets(void) {
    printf(    "tilepalquant presets (use with -preset <mode>\n"
               "  Mode       Settings\n"
               "  ---------  -------------\n");
    for (int c=0; c < PRESET_COUNT; c++) {
        printf("  %-8s   %s\n", presets[c][0], presets[c][1]);
    }
}


const char * getPresetOptionStr(const char * presetStr, bool & status) {
    for (int c=0; c < PRESET_COUNT; c++) {
        if (!strcmp(presetStr, presets[c][0])) {
            status = true;
            return presets[c][1];
        }
    }

    // No match
    status = false;
    return "";
}