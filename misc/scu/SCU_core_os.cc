// Single Compilation Unit
#define SCU_IDENT(x) x
#define SCU_XSTR(x) #x
#define SCU_STR(x) SCU_XSTR(x)
#define SCU_PATH(x,y) SCU_STR(SCU_IDENT(x)SCU_IDENT(y))
#define SCU_DIR core/os/

#include SCU_PATH(SCU_DIR,dir_access.cpp)
#include SCU_PATH(SCU_DIR,file_access.cpp)
#include SCU_PATH(SCU_DIR,input.cpp)
#include SCU_PATH(SCU_DIR,input_event.cpp)
#include SCU_PATH(SCU_DIR,keyboard.cpp)
#include SCU_PATH(SCU_DIR,main_loop.cpp)
#include SCU_PATH(SCU_DIR,memory.cpp)
#include SCU_PATH(SCU_DIR,midi_driver.cpp)
#include SCU_PATH(SCU_DIR,mutex.cpp)
#include SCU_PATH(SCU_DIR,os.cpp)
#include SCU_PATH(SCU_DIR,rw_lock.cpp)
#include SCU_PATH(SCU_DIR,semaphore.cpp)
#include SCU_PATH(SCU_DIR,thread.cpp)
#include SCU_PATH(SCU_DIR,thread_dummy.cpp)
#include SCU_PATH(SCU_DIR,thread_safe.cpp)
