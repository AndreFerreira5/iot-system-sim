#include "worker.h"
#include <unistd.h>

_Noreturn void init_worker(){
    while(1) sleep(100000);
}