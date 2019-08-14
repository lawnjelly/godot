// Single Compilation Unit
#define SCU_IDENT(x) x
#define SCU_XSTR(x) #x
#define SCU_STR(x) SCU_XSTR(x)
#define SCU_PATH(x,y) SCU_STR(SCU_IDENT(x)SCU_IDENT(y))
#define SCU_DIR editor/doc/

#include SCU_PATH(SCU_DIR,doc_data.cpp)
#include SCU_PATH(SCU_DIR,doc_dump.cpp)
