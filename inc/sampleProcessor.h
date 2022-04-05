#ifndef SAMPLEPROCESSOR_H_INCLUDED
#define SAMPLEPROCESSOR_H_INCLUDED

#include <iostream>
#include <map>
#include <set>
#include <queue>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <mutex>
#include <atomic>

#include "processor.h"

using namespace std;

class sampleLogger : public Logger
{
public:
   sampleLogger(Level l):_logLevel(l),processFinished(false){}
   void log (Level l, const std::string & str);
   void write (std::ostream& out_stream);
   bool stop();
   inline void setProcessFinished(){ processFinished.store(true);}
private:
    Logger::Level _logLevel;
    string _logMsg;
    mutex _mtx;
    std::atomic<bool> processFinished;

};

class sampleCompiler : public Compiler
{
   bool compile (const std::string & file, std::string & error_message)
   {
       if(file.rfind("FAIL", 0) == 0)
       {
           error_message = "Compilation Failed: File name starts with FAIL.";
           return false;
       }
       else
           return true;

   }
};


class sampleProcessor :public MakeFileProcessor
{
   public:
       bool Run(Logger& log, Compiler& compiler, const std::string& file);
       string getFileStatus(const std::string& file){ return fileCompileStatus[file];}
   private:
       fstream fMakefile;
       queue<string> fileList;
       map<string,string> fileCompileStatus;
       multimap<string,string> dependencyMap;
       bool logDependencyTree = false;
       bool initialize(const std::string& file,Logger& log);
       bool checkDependencyCompile(const string& file, Compiler& compiler,Logger& log,
            set<string>& cycleList, string& msg);


       static const string CompileSuccess;
       static const string CycleFound;
       static const string DependencyFailed;
};



#endif // SAMPLEPROCESSOR_H_INCLUDED
