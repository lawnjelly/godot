// Single Compilation Unit
#define SCU_IDENT(x) x
#define SCU_XSTR(x) #x
#define SCU_STR(x) SCU_XSTR(x)
#define SCU_PATH(x,y) SCU_STR(SCU_IDENT(x)SCU_IDENT(y))
#define SCU_DIR servers/audio/

#include SCU_PATH(SCU_DIR,audio_driver_dummy.cpp)
#include SCU_PATH(SCU_DIR,audio_effect.cpp)
#include SCU_PATH(SCU_DIR,audio_filter_sw.cpp)
#include SCU_PATH(SCU_DIR,audio_rb_resampler.cpp)
#include SCU_PATH(SCU_DIR,audio_stream.cpp)
#include SCU_PATH(SCU_DIR,reverb_sw.cpp)
