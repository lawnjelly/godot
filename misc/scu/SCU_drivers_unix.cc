// Single Compilation Unit
#define SCU_IDENT(x) x
#define SCU_XSTR(x) #x
#define SCU_STR(x) SCU_XSTR(x)
#define SCU_PATH(x,y) SCU_STR(SCU_IDENT(x)SCU_IDENT(y))
#define SCU_DIR drivers/unix/

#include SCU_PATH(SCU_DIR,dir_access_unix.cpp)
#include SCU_PATH(SCU_DIR,file_access_unix.cpp)
#include SCU_PATH(SCU_DIR,ip_unix.cpp)
#include SCU_PATH(SCU_DIR,mutex_posix.cpp)
#include SCU_PATH(SCU_DIR,net_socket_posix.cpp)
#include SCU_PATH(SCU_DIR,os_unix.cpp)
#include SCU_PATH(SCU_DIR,rw_lock_posix.cpp)
#include SCU_PATH(SCU_DIR,semaphore_posix.cpp)
#include SCU_PATH(SCU_DIR,syslog_logger.cpp)
#include SCU_PATH(SCU_DIR,thread_posix.cpp)
