#ifndef LOGGER_H_
#define LOGGER_H_

struct Log
{
};

class Logger
{
  public:
    static Logger &getInstance();
    void addLog(const Log&){}

  private:
    Logger() {}
};

#endif