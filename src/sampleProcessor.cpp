
#include <iomanip>
#include <ctime>
#include <sstream>
#include "sampleProcessor.h"

const string sampleProcessor::CompileSuccess = "Successfully compiled.";
const string sampleProcessor::CycleFound = "Cycle found.";
const string sampleProcessor::DependencyFailed = "Dependency failed.";

using namespace std;

bool sampleLogger::stop()
{
    if(processFinished.load() && _logMsg.empty())
        return true;
    else
        return false;
}
void sampleLogger::log(Level l, const std::string& str)
{
    if(l <= _logLevel)
    {
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S:");
        auto currTS = oss.str();
        string logLevel("GOOD");
        if( l == Logger::BAD)
            logLevel = "BAD";
        else if ( l == Logger::UGLY)
            logLevel = "UGLY";

        std::unique_lock<std::mutex> lock1(_mtx);
        _logMsg.append("\n");
        _logMsg.append(currTS);
        _logMsg.append(":");
        _logMsg.append(logLevel);
        _logMsg.append(":");
        _logMsg.append(str);
    }

}
void sampleLogger::write(std::ostream& out_stream)
{
    if(!_logMsg.empty())
    {
        std::unique_lock<std::mutex> lock1(_mtx);
        out_stream << _logMsg;
        _logMsg = "";
    }
    out_stream.flush();
}

void trim(std::string &s)
{
     s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), [](char c){ return std::isspace(c); }));
     s.erase(std::find_if_not(s.rbegin(), s.rend(), [](char c){ return std::isspace(c); }).base(), s.end());
}

bool sampleProcessor::checkDependencyCompile(const string& file, Compiler& compiler,Logger& log,
            set<string>& cycleList, string& msg)
{
    log.log(Logger::UGLY, "checkDependencyCompile :"+file );
    if( cycleList.find(file) != cycleList.end())//cycle found
    {
        msg = CycleFound;
        fileCompileStatus[file]="F:"+msg;
        return false;
    }
    else
    {
        auto it = dependencyMap.equal_range(file);
        if(it.first == it.second) //No dependencies for this file
        {
            //string errMsg;
            log.log(Logger::GOOD, "Compiling file: "+file);
            if( compiler.compile(file,msg) )
            {
                fileCompileStatus[file]="S:"+CompileSuccess;
                log.log(Logger::GOOD, fileCompileStatus[file]);
                return true;
            }
            else
            {
                fileCompileStatus[file]="F:"+msg;
                log.log(Logger::GOOD, fileCompileStatus[file]);
                return false;
            }
        }

        // Process dependencies
        for (auto itr = it.first; itr != it.second; ++itr)
        {
            string currStatus = fileCompileStatus[itr->second];
            if(logDependencyTree)
                log.log(Logger::GOOD, "Dependent File status:"+itr->second+":"+currStatus);
            log.log(Logger::BAD, "Current File status:"+itr->second+":"+currStatus);
            if(currStatus.empty())
            {
                cycleList.insert(itr->first);
                if ( !checkDependencyCompile(itr->second, compiler,log,cycleList, msg))//failed
                {
                    if(msg != CycleFound)
                        msg = DependencyFailed;
                    else
                        log.log(Logger::GOOD, "Cyclic dependency found for file: "+ file);

                    fileCompileStatus[file]="F:"+msg;
                    cycleList.erase(itr->first);
                    return false;
                }
            }
            else if( currStatus.rfind("F:", 0) == 0)
            {
                log.log(Logger::BAD, "Already failed: "+itr->second);
                msg = DependencyFailed;
                fileCompileStatus[itr->first]= "F:"+msg;
                return false;
            }
            else //Already compiled
            {
                log.log(Logger::BAD, "Already compiled: "+itr->second);
            }
        }


        log.log(Logger::GOOD, "All dependents resolved so Compiling file: "+file);
        if( compiler.compile(file,msg) )
        {
            fileCompileStatus[file]="S:"+CompileSuccess;
            log.log(Logger::GOOD, fileCompileStatus[file]);
            return true;
        }
        else
        {
            fileCompileStatus[file]="F:"+msg;
            log.log(Logger::GOOD, fileCompileStatus[file]);
            return false;
        }
    }
}
bool sampleProcessor::Run(Logger& log, Compiler& compiler, const std::string& file)
{
    log.log(Logger::GOOD, "Processing makefile:" + file);
    bool retStatus;
    retStatus = initialize(file,log);

    log.log(Logger::BAD, "Init File status:");
    for(auto& elem : fileCompileStatus)
        log.log(Logger::BAD, elem.first + ":" + elem.second);

    if(logDependencyTree)
    {
        log.log(Logger::GOOD, "Dependency Tree map:");
        for(auto& elem : dependencyMap)
            log.log(Logger::GOOD, elem.first + ":" + elem.second);
    }

    //start compiling
    log.log(Logger::GOOD, "Start processing ...");
    while( !fileList.empty())
    {
        string fileName = fileList.front();
        fileList.pop();
        log.log(Logger::GOOD, "From Queue:"+fileName );
        string errMsg;
        set<string> cycleList;
        if( fileCompileStatus[fileName].empty() )
            checkDependencyCompile(fileName, compiler,log,cycleList, errMsg); //failed
    }

    log.log(Logger::GOOD, "########### FINAL FILE STATUS ###########");
    int succCount =0, failCount =0, skipCount=0, cycleCount=0,totCount=0;
    for(auto& elem : fileCompileStatus)
    {
        log.log(Logger::GOOD, elem.first + ":" + elem.second);
        if( elem.second.rfind("S:", 0) == 0)
            succCount++;
        else if( elem.second.rfind("F:Compilation Failed", 0) == 0)
            failCount++;
        else if( elem.second.rfind("F:Dep", 0) == 0)
            skipCount++;
        else if( elem.second.rfind("F:Cyc", 0) == 0)
            cycleCount++;

        totCount++;
    }
    log.log(Logger::GOOD, "########### FINAL REPORT ###########");
    log.log(Logger::GOOD, "Total File count:" + to_string(totCount));
    log.log(Logger::GOOD, "Success File count:" + to_string(succCount));
    log.log(Logger::GOOD, "Fail count(Errors):" + to_string(failCount));
    log.log(Logger::GOOD, "Skip count(Dependency failed):" + to_string(skipCount));
    log.log(Logger::GOOD, "Skip count(Cyclic dependency):" + to_string(cycleCount));

    return true;
}
bool sampleProcessor::initialize(const std::string& file,Logger& log)
{
    fMakefile.open(file, fstream::in);
    if(fMakefile.is_open())
    {
        string line;
        string token;
        while(!fMakefile.eof())
        {
            getline(fMakefile,line);
            if(line.rfind("Component:", 0) == 0 &&
               line.length() > 10 )
            {
                stringstream fileSS(line.substr(10));
                while(getline(fileSS,token, ','))
                {
                    trim(token);
                    auto res = fileCompileStatus.emplace(token,"");
                    if(res.second)
                       fileList.push(token);
                    else
                        log.log(Logger::GOOD, "Ignoring Duplicate file: " + token );
                }
            }
            else if (line.rfind("Depends:", 0) == 0 &&
                     line.length() > 8 )
            {
                stringstream depFileSS(line.substr(8));
                string firstFile;
                getline(depFileSS,firstFile, ',');
                trim(firstFile);
                while(getline(depFileSS,token, ','))
                {
                    trim(token);
                    if( dependencyMap.emplace(firstFile,token) == dependencyMap.end() )
                        log.log(Logger::GOOD, "Ignoring Duplicate Dependency: " +
                            firstFile + ":" + token );
                }
            }
            else if (line.rfind("LogDependencyTree:", 0) == 0 &&
                     line.length() > 18)
            {
                string token = line.substr(18);
                log.log(Logger::GOOD, "LogDependencyTree:" + token );
                trim(token);
                std::transform(token.begin(), token.end(), token.begin(), ::toupper);
                if(token == "TRUE")
                    logDependencyTree = true;
            }
            else if( line.rfind("#", 0) == 0 && line.length() > 1)
            {
                log.log(Logger::GOOD, "Makefile comment:" + line.substr(1) );
            }
            else if( line != "")
                log.log(Logger::GOOD, "Makefile Invalid line: " + line );
        }
        fMakefile.close();
        return 0;
    }
    else
        return false;

}
