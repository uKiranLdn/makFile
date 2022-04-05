#ifndef PROCESSOR_H_INCLUDED
#define PROCESSOR_H_INCLUDED

class Logger
{
public:
   enum Level {GOOD, BAD, UGLY};
   virtual void log (Level l, const std::string & str) = 0;
   virtual void write (std::ostream & out_stream) = 0;
};

class Compiler
{
public:
/**
* Compiles a file
* @param file the file to compile
* @param error_message An error message set if the compilation fails
* @returns True for success, false for failure
*/
  virtual bool compile (const std::string & file, std::string & error_message) = 0;
};

class MakeFileProcessor
{
public:
   virtual bool Run (Logger & log, Compiler & compiler, const std::string & file) = 0;

};


#endif // PROCESSOR_H_INCLUDED
