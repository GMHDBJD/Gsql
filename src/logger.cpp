#include "logger.h"

Logger& Logger::getInstance(){
    static Logger logger;
    return logger;
}