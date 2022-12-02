#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
using namespace std;

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command {
protected:
    const std::string  cmd_line;
 public:
  Command(const std::string cmd_line) : cmd_line(cmd_line) {};
  virtual ~Command() = default;
  virtual void execute() = 0;
  const std::string getCmdLine(){return cmd_line;};
  //virtual void prepare();
  //virtual void cleanup();
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const std::string cmd_line);
  virtual ~BuiltInCommand() {};
};

class ExternalCommand : public Command {
 public:
  ExternalCommand(const std::string cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override;
};

class chpromptCommand : public BuiltInCommand{
public:
    chpromptCommand(const std::string cmd_line);
    virtual ~chpromptCommand()  = default;
    void execute() override;
};


class PipeCommand : public Command {
    int fd_pipe[2] = {0,0};
    bool is_err;
    int fd_copy;
public:
  PipeCommand(const std::string cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
//  void prepare();
//  void cleanup();
};

class RedirectionCommand : public Command {
    bool append;
    int stdout_copy;
    int fd_copy;
public:
  explicit RedirectionCommand(const std::string cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  void prepare() ;
  void cleanup() ;
};

class ChangeDirCommand : public BuiltInCommand {
public:
    string* plastPwd;
  ChangeDirCommand(const std::string cmd_line, string* plastPwd);
  virtual ~ChangeDirCommand() {}
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const std::string cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const std::string cmd_line);
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members
public:
  QuitCommand(const std::string cmd_line);
  virtual ~QuitCommand() {}
  void execute() override;
};


class JobsList {
 public:
  class JobEntry {
  public:
   // TODO: Add your data members
   int job_id;
   pid_t pid;
   bool is_stopped;
   std::string cmd;
   int entry_time;
   JobEntry(int job_id, pid_t pid,bool is_stopped,std::string cmd,int entry_time) : job_id(job_id),
                                pid(pid), is_stopped(is_stopped) ,cmd(cmd),entry_time(entry_time){};
  };
  vector<JobEntry*> jobs_list;
  int max_job_id;
 public:
  JobsList();
  ~JobsList() = default;
  void addJob(const std::string cmd_line,pid_t pid, bool isStopped = false);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob(int* lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
public:
    JobsList* jobs;
    JobsCommand(const std::string cmd_line, JobsList* jobs);
  virtual ~JobsCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 JobsList* jobs;
 public:
  ForegroundCommand(const std::string cmd_line);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  BackgroundCommand(const std::string cmd_line);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class TimeoutCommand : public BuiltInCommand {
/* Optional */
// TODO: Add your data members
 public:
  explicit TimeoutCommand(const std::string cmd_line);
  virtual ~TimeoutCommand() {}
  void execute() override;
};

class FareCommand : public BuiltInCommand {
    std::string  word_to_replace;
    std::string replacement;
    int counter;
 public:
  FareCommand(const std::string cmd_line);
  virtual ~FareCommand() {}
  void execute() override;
};

class SetcoreCommand : public BuiltInCommand {
  /* Optional */
  // TODO: Add your data members
 public:
  SetcoreCommand(const std::string cmd_line);
  virtual ~SetcoreCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {

public:
  KillCommand(const std::string cmd_line);
  virtual ~KillCommand() {}
  void execute() override;
};

class SmallShell {
 private:
  // TODO: Add your data members
  SmallShell();
 public:
    string last_dir;
    pid_t pid;
    string smash_prompt;
    JobsList jobs_list;
    string current_cmd_line;
    pid_t current_process;
    pid_t last_stopped_job_id;
    pid_t pid_for_pipe;
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line);
  vector<string> convertToVector(const string cmd_line);
  // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
