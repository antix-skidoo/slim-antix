#ifndef _LOG_H_
#define _LOG_H_

#ifdef USE_CONSOLEKIT
#include "Ck.h"
#endif

#ifdef USE_PAM
#include "PAM.h"
#endif

#include "const.h"
#include <fstream>
//#i nclude <iomanip>   /*   skidoo  late late TENTATIVE addition   */
#include <ostream>      /*   skidoo  late late addition   */
#include <iostream>     /*   skidoo  1.4.2 addition   */

//       ostream
//   https://www.ntu.edu.sg/home/ehchua/programming/cpp/cp10_IO.html

using namespace std;

static class LogUnit {
    ofstream logFile;
public:
    bool openLog(const char * filename);
    void closeLog();

    ~LogUnit() { closeLog(); }

    template<typename Type>
    LogUnit & operator<<(const Type & text) {
        logFile << text; logFile.flush();
        return *this;
    }

    LogUnit & operator<<(ostream & (*fp)(ostream&)) {
        logFile << fp; logFile.flush();
        return *this;
    }

    LogUnit & operator<<(ios_base & (*fp)(ios_base&)) {
        logFile << fp; logFile.flush();
        return *this;
    }
} logStream;

#endif
