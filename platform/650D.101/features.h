#include "all_features.h"

//~ #define FEATURE_LV_3RD_PARTY_FLASH // requires props
#define FEATURE_EYEFI_TRICKS

// Disable all audio stuff
#undef FEATURE_WAV_RECORDING
#undef FEATURE_FPS_WAV_RECORD
#undef FEATURE_BEEP
#undef FEATURE_VOICE_TAGS
#undef FEATURE_AUDIO_REMOTE_SHOT
#undef FEATURE_AUDIO_METERS

#undef FEATURE_AUTO_BURST_PICQ // maybe not working on 650D

// No DISP button
#undef FEATURE_OVERLAYS_IN_PLAYBACK_MODE
#undef FEATURE_ARROW_SHORTCUTS

// Not working :(
#undef FEATURE_IMAGE_EFFECTS
#undef FEATURE_DEFISHING_PREVIEW
#undef FEATURE_ANAMORPHIC_PREVIEW

#undef FEATURE_LV_BUTTON_PROTECT
#undef FEATURE_LV_BUTTON_RATE

#undef FEATURE_MOVIE_RESTART
#undef FEATURE_WARNINGS_FOR_BAD_SETTINGS

#undef FEATURE_TRAP_FOCUS

// Glitchy
#undef FEATURE_STICKY_DOF
#undef FEATURE_STICKY_HALFSHUTTER
// #undef FLEXINFO_DEVELOPER_MENU - disable from flexinfo.h

#define FEATURE_INTERMEDIATE_ISO_PHOTO_DISPLAY
