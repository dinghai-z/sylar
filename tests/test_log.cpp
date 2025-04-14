#include "sylar/log.hpp"
#include <string>
#include <memory>
#include "sylar/thread.hpp"
#include <stdlib.h>

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("root");

int main(){
    
    SYLAR_LOG_DEBUG(g_logger) << " stream test1";   
    SYLAR_LOG_DEBUG(g_logger) << " stream test2";
    
    return 0;
}