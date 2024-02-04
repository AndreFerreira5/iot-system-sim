#include "alerts_watcher.h"
#include "unistd.h"

_Noreturn void init_alerts_watcher(){
    while(1) sleep(10000);
}