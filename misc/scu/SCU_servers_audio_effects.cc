// Single Compilation Unit
#define SCU_IDENT(x) x
#define SCU_XSTR(x) #x
#define SCU_STR(x) SCU_XSTR(x)
#define SCU_PATH(x,y) SCU_STR(SCU_IDENT(x)SCU_IDENT(y))
#define SCU_DIR servers/audio/effects/

#include SCU_PATH(SCU_DIR,audio_effect_amplify.cpp)
#include SCU_PATH(SCU_DIR,audio_effect_chorus.cpp)
#include SCU_PATH(SCU_DIR,audio_effect_compressor.cpp)
#include SCU_PATH(SCU_DIR,audio_effect_delay.cpp)
#include SCU_PATH(SCU_DIR,audio_effect_distortion.cpp)
#include SCU_PATH(SCU_DIR,audio_effect_eq.cpp)
#include SCU_PATH(SCU_DIR,audio_effect_filter.cpp)
#include SCU_PATH(SCU_DIR,audio_effect_limiter.cpp)
#include SCU_PATH(SCU_DIR,audio_effect_panner.cpp)
#include SCU_PATH(SCU_DIR,audio_effect_phaser.cpp)
#include SCU_PATH(SCU_DIR,audio_effect_pitch_shift.cpp)
#include SCU_PATH(SCU_DIR,audio_effect_record.cpp)
#include SCU_PATH(SCU_DIR,audio_effect_reverb.cpp)
#include SCU_PATH(SCU_DIR,audio_effect_spectrum_analyzer.cpp)
#include SCU_PATH(SCU_DIR,audio_effect_stereo_enhance.cpp)
#include SCU_PATH(SCU_DIR,audio_stream_generator.cpp)
#include SCU_PATH(SCU_DIR,eq.cpp)
#include SCU_PATH(SCU_DIR,reverb.cpp)
