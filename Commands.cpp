#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}
//
//int _parseCommandLine(const char* cmd_line, char** args) {
//  FUNC_ENTRY()
//  int i = 0;
//  std::istringstream iss(_trim(string(cmd_line)).c_str());
//  for(std::string s; iss >> s; ) {
//    args[i] = (char*)malloc(s.length()+1);
//    memset(args[i], 0, s.length()+1);
//    strcpy(args[i], s.c_str());
//    args[++i] = NULL;
//  }
//  return i;
//  FUNC_EXIT()
//}

vector<string> SmallShell::convertToVector(const string cmd_line) {
    vector<string> vec;
    stringstream ss(cmd_line);
    string word;
    while(ss >> word){
        vec.push_back(word);
    }
    return vec;
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

//void freeArgs(char** args, int size){
//    if(!args)
//        return;
//    for(int i = 0; i < size; i++){
//        if(args[i]) {
//            free(args[i]);
//        }
//    }
//}

bool isComplex(std::string cmd_line){
    if(cmd_line.find("*") != string::npos || cmd_line.find("?") != string::npos)
        return true;
    return false;
}

char** vectorToArgs(const vector<string> vec){
    char** args = new char*[vec.size() + 1];
    for(int i=0; i<vec.size();i++){
            args[i] = new char[vec[i].length()];
            strcpy(args[i],vec[i].c_str());
    }
    args[vec.size()] = nullptr;
    return args;
}


void clearArgs(char** args, int size){
    for(int i = 0; i<size;i++){
        if(args[i])
            delete []args[i];
    }
    delete []args;
}


//====================================BUILT IN COMMAND===========================================//

BuiltInCommand::BuiltInCommand(const std::string cmd_line) : Command(cmd_line){}

//====================================chpromptCommand Implementation===========================================//

chpromptCommand::chpromptCommand(const std::string cmd_line) : BuiltInCommand(cmd_line){};

void chpromptCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    vector<string> args = smash.convertToVector(cmd_line);
    int num_of_arguments = args.size();
    //reset prompt
    if(num_of_arguments == 1){
        smash.smash_prompt = "smash";
    }
    else{
        smash.smash_prompt = args[1];
    }
}

//==================================== showPidCommand Implementation===========================================//

ShowPidCommand::ShowPidCommand(const string cmd_line) : BuiltInCommand(cmd_line){};

void ShowPidCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    std::cout << "smash pid is " << smash.pid << endl;
}

//==================================== getCurrentDir(pwd) Implementation ===========================================//

GetCurrDirCommand::GetCurrDirCommand(const string cmd_line) : BuiltInCommand(cmd_line){};

void GetCurrDirCommand::execute() {
    //syscall fail
    char* path = getcwd(nullptr,0);
    if(!path) {
        perror("smash error: getcwd failed");
        free(path);
        return;
    }
    std::cout << path << endl;
    free(path);
}

//==================================== changeDirCommand Implementation ===========================================//

ChangeDirCommand::ChangeDirCommand(const string cmd_line, string* plastPwd) : BuiltInCommand(cmd_line), plastPwd(plastPwd){}

void ChangeDirCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    vector<string> args = smash.convertToVector(cmd_line);
    char* current_path = getcwd(nullptr,0);
    //syscall fail
    if (!current_path) {
        perror("smash error: getcwd failed");
        free(current_path);
        return;
    }
    int num_of_args = args.size();
    //invalid parameters
    if(num_of_args == 1){
        std::cout<< "smash error:> \"cd \"" << endl;
        free(current_path);
        return;
    }
    //too many arguments
    if (num_of_args > 2) {
        std::cerr << "smash error: cd: too many arguments" << endl;
        free(current_path);
        return;
    }
        //old pwd not set
    else if (num_of_args == 2 && args[1] == "-" && plastPwd->size()==0) {
        std::cerr << "smash error: cd: OLDPWD not set" << endl;
        free(current_path);
        return;
    }
        //execute chdir
    else if(args[1] == "-") {
        if (chdir(plastPwd->c_str()) == -1) {
            std::cerr << "smash error: chdir failed" << endl;
            free(current_path);
            return;
        }
        else{
            smash.last_dir = current_path;
            free(current_path);
            return;
        }
    }
    else if(chdir(args[1].c_str()) == -1){
        std::cerr << "smash error: chdir failed" << endl;
        free(current_path);
        return;
    }
    else{
        smash.last_dir = current_path;
        free(current_path);
        return;
    }
}
//====================================JobsCommand===================================================//

JobsCommand::JobsCommand(const std::string cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs){}

void JobsCommand::execute(){
    SmallShell& smash = SmallShell::getInstance();
    vector<string> args = smash.convertToVector(cmd_line);
    smash.jobs_list.removeFinishedJobs();
    smash.jobs_list.printJobsList();
}

//====================================ForeGround Command===================================================//
ForegroundCommand::ForegroundCommand(const std::string cmd_line) : BuiltInCommand(cmd_line){}

void ForegroundCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    vector<string> args = smash.convertToVector(cmd_line);
    int status;
    int job_id = 0;
    //no jobs
    if(args.size() == 1 && smash.jobs_list.jobs_list.size() == 0) {
        perror("smash error: fg: jobs list is empty");
        return;
    }
    //invalid args
    job_id =  atoi(args[1].c_str());
    if(args.size() > 2 || (args.size() == 2 && job_id == 0)){
        perror("smash error: fg: invalid arguments");
        return;
    }
    //specific job
    if(args.size() == 2 && !smash.jobs_list.getJobById(job_id)){
        cerr << "smash error: fg: job-id " << job_id << " does not exist" << endl;
        return;
    }
    //max job_id job
    else if(args.size() == 1)
        job_id = smash.jobs_list.max_job_id - 1;

    //send signal to goto fg
    if(kill(smash.current_process,SIGCONT) == -1){
        perror("smash error: kill failed");
        return;
    }

    //take relevant pid and cmd line
    smash.current_process = smash.jobs_list.getJobById(job_id)->pid;
    smash.current_cmd_line = smash.jobs_list.getJobById(job_id)->cmd;
    //remove from jobs list
    smash.jobs_list.removeJobById(job_id);
    //wait because fg
    if (waitpid(smash.current_process, &status, WUNTRACED) == -1) {
        perror("smash error: waitpid failed");
        return;
    }
    //update current values after finish
    smash.current_process = -1;
    smash.current_cmd_line = "";
}
//====================================  Background COMMAND ===========================================//
BackgroundCommand::BackgroundCommand(const std::string cmd_line) : BuiltInCommand(cmd_line){}

void BackgroundCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    vector<string> args = smash.convertToVector(cmd_line);
    int job_id = 0;
    int last_stopped_job = 0;
    job_id =  atoi(args[1].c_str());
    /////////errors////////
    //inval args
    if(args.size() > 2 || (args.size() == 2 && job_id == 0)){
        perror("smash error: bg: invalid arguments");
        return;
    }
    //no job like this
    if(args.size() == 2 && !smash.jobs_list.getJobById(job_id)){
        std::cerr << "smash error: bg: job-id " << job_id << " does not exist" << endl;
        return;
    }
    //exists but already running
    if(args.size() == 2 && smash.jobs_list.getJobById(job_id)->is_stopped == false){
        std::cerr << "smash error: bg: job-id " << job_id << " is already running in the background" << endl;
        return;
    }
    //no stopped jobs
    smash.jobs_list.getLastStoppedJob(&last_stopped_job);
    if(args.size() == 1 && last_stopped_job == 0){
        std::cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
        return;
    }
    ////////function///////

    //specific job id
    if(args.size() == 2){
        std::cout << smash.jobs_list.getJobById(job_id)->cmd << " : " << smash.jobs_list.getJobById(job_id)->pid << endl;
        smash.jobs_list.getJobById(job_id)->is_stopped = false;
        kill(smash.jobs_list.getJobById(job_id)->pid,SIGCONT);
        smash.last_stopped_job_id = smash.jobs_list.getLastStoppedJob(&job_id)->job_id;
        return;
    }
    //last stopped job
    if(args.size() == 1){
        std::cout << smash.jobs_list.getJobById(last_stopped_job)->cmd << " : " << smash.jobs_list.getJobById(last_stopped_job)->pid << endl;
        smash.jobs_list.getJobById(last_stopped_job)->is_stopped = false;
        kill(smash.jobs_list.getJobById(last_stopped_job)->pid,SIGCONT);
        if(smash.jobs_list.getLastStoppedJob(&last_stopped_job))
            smash.last_stopped_job_id = smash.jobs_list.getLastStoppedJob(&last_stopped_job)->job_id;
        else
            smash.last_stopped_job_id = 0;
        return;
    }
}
//==================================== EXTERNAL COMMAND ===========================================//


ExternalCommand::ExternalCommand(const std::string cmd_line) : Command(cmd_line){}

void ExternalCommand::execute() {
    int status;
    SmallShell& smash = SmallShell::getInstance();
    char cmd_copy[COMMAND_MAX_ARGS] = {""};
    strcpy(cmd_copy,cmd_line.c_str());
    bool is_background = _isBackgroundComamnd(cmd_copy);
    if(is_background){
        _removeBackgroundSign(cmd_copy);
    }
    vector<string> args = smash.convertToVector(cmd_copy);
    char command[COMMAND_ARGS_MAX_LENGTH] = {""};
    strcpy(command,args[0].c_str());
    char complex_path[] = "/bin/bash";
    char flag[] = "-c";
    char** arguments = vectorToArgs(args);
    //fork
    pid_t pid = fork();
    if(pid == -1){
        perror("smash error: fork failed");
        return;
    }
    //son
    if(pid == 0){
        if(setpgrp() == -1){
            perror("smash error: setpgrp failed");
            return;
        }
        //complex external
        if(isComplex(cmd_copy) == true){
            char* complex_args[] = {complex_path,flag,cmd_copy, nullptr};
            if(execvp(complex_path,complex_args) == -1){
                clearArgs(arguments, args.size()+1);
                perror("smash error: execvp failed complex");
                return;
            }
        }
            //rest of externals
        else{
            if(execvp(command,arguments) == -1){
                clearArgs(arguments, args.size()+1);
                perror("smash error: execvp failed");
                return;
            }
        }
        clearArgs(arguments, args.size()+1);
    }
    //daddy
    //foreground
    if(!is_background) {
        smash.current_process = pid;
        smash.current_cmd_line = cmd_line;
        if (waitpid(pid, &status, WUNTRACED) == -1) {
            perror("smash error: waitpid failed");
            return;
        }
        smash.current_cmd_line = "";
        smash.current_process = -1;
    }
    //background
    else{
        smash.jobs_list.removeFinishedJobs();
        smash.jobs_list.addJob(cmd_line, pid, false);
    }
    return;
}

//==================================== JOBS LIST IMPLEMENTATION ===========================================//

JobsList::JobsList() : jobs_list(), max_job_id(1){}

void JobsList::addJob(const std::string cmd_line,pid_t pid, bool isStopped) {
    SmallShell& smash = SmallShell::getInstance();
    JobEntry* job = new JobEntry(smash.jobs_list.max_job_id,pid,isStopped,cmd_line,time(nullptr));
    jobs_list.push_back(job);
    smash.jobs_list.max_job_id++;
}

void JobsList::printJobsList() {
    for(int i=0; i < jobs_list.size();i++){
        if(jobs_list[i]->is_stopped == true){
            std::cout << "[" << jobs_list[i] -> job_id << "] " << jobs_list[i] ->cmd << ": " << jobs_list[i] ->pid << " " <<
                                        difftime(time(nullptr),jobs_list[i]->entry_time)<< " secs (stopped)" << endl;
        }
        else{
            std::cout << "[" << jobs_list[i] -> job_id << "] " << jobs_list[i] ->cmd << ": " << jobs_list[i] ->pid << " " <<
                      difftime(time(nullptr),jobs_list[i]->entry_time)<< " secs"<<endl;
        }
    }
}


void JobsList::killAllJobs() {
    std::cout << "smash: sending SIGKILL signal to " << jobs_list.size() << " jobs:" << endl;
    for(int i = 0; i < jobs_list.size(); i++){
            std::cout << jobs_list[i]->pid << ": " << jobs_list[i]->cmd << endl;
            kill(jobs_list[i]->pid, SIGKILL);
            delete jobs_list[i];
    }
}

void JobsList::removeFinishedJobs() {
    SmallShell& smash = SmallShell::getInstance();
    if (smash.jobs_list.jobs_list.size() == 0) {
        smash.jobs_list.max_job_id = 1;
        return;
    }
    for (auto it = jobs_list.begin(); it != jobs_list.end(); ++it) {
        int status;
        int ret_wait = waitpid((*it)->pid, &status, WNOHANG);
        if (ret_wait == (*it)->pid || ret_wait == -1) {
            jobs_list.erase(it);
            --it;
            //todo IMPLEMENT DELETE
        }
    }
    int temp;
    getLastJob(&temp);
}

JobsList::JobEntry* JobsList::getJobById(int jobId) {
    for(int i = 0; i<jobs_list.size();i++){
        if(jobs_list[i]->job_id == jobId)
            return jobs_list[i];
    }
    return nullptr;
}

void JobsList::removeJobById(int jobId) {
    for(int i = 0;i < jobs_list.size(); i++){
        if(jobId == jobs_list[i]->job_id){
            jobs_list.erase(jobs_list.begin() + i);
        }
    }
    int temp;
    getLastJob(&temp);
}

JobsList::JobEntry *JobsList::getLastJob(int *lastJobId) {
    SmallShell& smash = SmallShell::getInstance();
    smash.jobs_list.max_job_id=1;
    for(int i=0; i< jobs_list.size(); i++){
        if(jobs_list[i]->job_id >= smash.jobs_list.max_job_id)
            smash.jobs_list.max_job_id = jobs_list[i]->job_id + 1;
    }
    *lastJobId = smash.jobs_list.max_job_id;
    return getJobById(smash.jobs_list.max_job_id);
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId) {
    for(int i= jobs_list.size() - 1; i>=0 ; i--){
        if(jobs_list[i]->is_stopped) {
            *jobId = jobs_list[i]->job_id;
            return jobs_list[i];
        }
    }
    return nullptr;
}

SmallShell::SmallShell() : last_dir(""), pid(getpid()), smash_prompt("smash"), current_process(-1), current_cmd_line(""), jobs_list(JobsList()), last_stopped_job_id(0){
// TODO: add your implementation
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/

bool inputCheck(string cmd) {
    return ((cmd == "chprompt") || (cmd == "showpid") || (cmd == "pwd") || (cmd == "cd") || (cmd == "jobs") ||
            (cmd == "fg") || (cmd == "bg") || (cmd == "quit") || (cmd == "kill"));
}


Command * SmallShell::CreateCommand(const char* cmd_line) {
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    if(firstWord.compare("chprompt") == 0){
        return new chpromptCommand(cmd_s);
    }
    if(firstWord.compare("showpid") == 0){
        return new ShowPidCommand(cmd_s);
    }
    if(firstWord.compare("pwd") == 0){
        return new GetCurrDirCommand(cmd_s);
    }
    if(firstWord.compare("cd") == 0){
        return new ChangeDirCommand(cmd_s,&last_dir);
    }
    if(firstWord.compare("jobs") == 0){
        return new JobsCommand(cmd_s,&jobs_list);
    }
    if(firstWord.compare("fg") == 0){
        return new ForegroundCommand(cmd_s);
    }
    if(firstWord.compare("bg") == 0){
        return new BackgroundCommand(cmd_s);
    }
    else{
        return new ExternalCommand(cmd_line);
    }
	// For example:
/*
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if ...
  .....
  else {
    return new ExternalCommand(cmd_line);
  }
  */
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
   Command* cmd = CreateCommand(cmd_line);
   if(!cmd){
       return;
   }
   SmallShell &smash = SmallShell::getInstance();
   smash.jobs_list.removeFinishedJobs();
   cmd->execute();
   delete cmd;
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}


//this is a test gadol