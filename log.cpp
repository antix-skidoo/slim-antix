#include "log.h"
#include <iostream>
#include <stdlib.h>

/* ....ref:   http://www.cplusplus.com/reference/iostream/cerr/     */

bool LogUnit::openLog(const char * filename)
{
    if (logFile.is_open()) {
        std::cerr << "SLiM: opening a new Log file, while another is already open" << std::endl;
        logFile.close();
    }
    logFile.open(filename, ios_base::app);

    return !(logFile.fail());
}

void LogUnit::closeLog()
{
    if (logFile.is_open()) {
        logFile.close();
    }
}
