#include<thread>
#include <fstream>
#include <future>
#include "sampleProcessor.h"

void logWrite(sampleLogger& log)
{
    std::ofstream ofs;
    ofs.open ("sampleProcessor.log", std::ofstream::out | std::ofstream::app);
    log.log(Logger::GOOD, "Logging started ...");
    while (!log.stop())
    {
        log.write(ofs);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main()
{
    sampleLogger logger(Logger::GOOD);
    sampleCompiler compiler;
    sampleProcessor processor;

    std::thread loggerThread(logWrite,std::ref(logger));
    processor.Run(logger,compiler,"F:/CBlocksProjects/ADSS_Test/bin/Debug/testMakeFile");
    std::cout << "Finished compiling. Pls check log file sampleProcessor.log for Final report."<< endl;
    logger.setProcessFinished();
    loggerThread.join();
    return 0;
}
