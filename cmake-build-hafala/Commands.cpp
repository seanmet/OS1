#include <unistd.h>
#include <string.h>
#include <iostream>
#include "fstream"
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <fcntl.h>
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
        std::cerr<< "smash error:> \"cd \"" << endl;
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
            perror("smash error: chdir failed");
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
        perror("smash error: chdir failed");
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
        std::cerr << "smash error: fg: jobs list is empty" << endl;
        return;
    }
    //invalid args
    job_id =  atoi(args[1].c_str());
    if(args.size() > 2 || (args.size() == 2 && job_id == 0)){
        std::cerr <<"smash error: fg: invalid arguments" << endl;
        return;
    }
    //specific job
    if(args.size() == 2 && !smash.jobs_list.getJobById(job_id)){
        std::cerr << "smash error: fg: job-id " << job_id << " does not exist" << endl;
        return;
    }
    //max job_id job
    else if(args.size() == 1)
        job_id = smash.jobs_list.max_job_id - 1;

    std::cout << smash.jobs_list.getJobById(job_id)->cmd << ": " << smash.jobs_list.getJobById(job_id)->pid;
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
        std::cerr << "smash error: bg: invalid arguments" << endl;
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
        if(smash.jobs_list.getLastStoppedJob(&last_stopped_job))
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
//==================================== Quit COMMAND ===========================================//
QuitCommand::QuitCommand(const std::string cmd_line) : BuiltInCommand(cmd_line) {}

void QuitCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    vector<string> args = smash.convertToVector(cmd_line);
    //sigkill
    if(args.size() > 1 && args[1] == "kill"){
        smash.jobs_list.killAllJobs();
        exit(0);
    }
    //normal quit
    for(int i = 0; i < smash.jobs_list.jobs_list.size(); i++){
        kill(smash.jobs_list.jobs_list[i]->pid, SIGKILL);
        delete smash.jobs_list.jobs_list[i];
    }
    delete this;
    exit(0);
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
                exit(1);
            }
        }
            //rest of externals
        else{
            if(execvp(command,arguments) == -1){
                clearArgs(arguments, args.size()+1);
                perror("smash error: execvp failed");
                exit(1);
            }
        }
//        clearArgs(arguments, args.size()+1);
//        exit(0);
    }
    //daddy
    //foreground
    if(!is_background) {
        smash.current_process = pid;
        smash.current_cmd_line = cmd_line;
        if (waitpid(pid, &status,WUNTRACED) == -1) {
            perror("smash error: waitpid failed");
            return;
        }
        smash.pid_for_pipe = pid;
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
//======================================== Special Commands ===================================//
//======================================== Redirection Command ===================================//
RedirectionCommand::RedirectionCommand(const std::string cmd_line) : Command(cmd_line), stdout_copy(1), fd_copy(-1){
    if(strstr(cmd_line.c_str(),">>"))
        append = true;
    else
        append = false;
}

void RedirectionCommand::execute() {
    prepare();
    SmallShell& smash = SmallShell::getInstance();
    char cmd_copy[COMMAND_MAX_ARGS] = {""};
    strcpy(cmd_copy,cmd_line.c_str());
    bool is_background = _isBackgroundComamnd(cmd_copy);
    string command = _trim(cmd_line.substr(0,cmd_line.find(">") - 1));
    if(is_background)
        command += " &";
    smash.executeCommand(command.c_str());
    cleanup();
    return;
}
void RedirectionCommand::prepare() {
    if((stdout_copy = dup(1)) == -1){
        perror( "smash error: dup failed");
        return;
    }
    //close stdout
    if(close(1) == -1){
        perror("smash error: close failed");
        return;
    }
    char temp[COMMAND_ARGS_MAX_LENGTH];
    char * path;
    int fd;
    if(append){
        strcpy(temp, strstr(cmd_line.c_str(),">>"));
        path = temp + 2*sizeof(char);
        fd = open(_trim(string(path)).c_str(),   O_WRONLY | O_APPEND | O_CREAT, 0666);
    }
    else{
        strcpy(temp, strstr(cmd_line.c_str(),">"));
        path = temp + 1*sizeof(char);
        fd = open(_trim(string(path)).c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0666);
    }
    if(fd == -1){
        perror("smash error: open failed");
        return;
    }
    return;
}

void RedirectionCommand::cleanup() {
    if(close(1) == -1){
        perror("smash error: close failed");
        return;
    }
    if(dup2(stdout_copy,1) == -1){
        perror("smash error: dup2 failed");
        return;
    }
    if(close(stdout_copy) == -1){
        perror("smash error: close failed");
        return;
    }
}
//======================================== Pipe Command ===================================//
PipeCommand::PipeCommand(const std::string cmd_line): Command(cmd_line),fd_copy(-1) {
    if(strstr(cmd_line.c_str(),"|&")){
        is_err = true;
    }
    else
        is_err = false;
}

void PipeCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    string command_line_1, command_line_2;
    command_line_1 = _trim(cmd_line.substr(0,cmd_line.find("|") - 1));
    Command* cmd1 = smash.CreateCommand(command_line_1.c_str());
    int fd[2];
    pipe(fd);
    if (is_err == false) {
        command_line_2 = _trim(cmd_line.substr(cmd_line.find("|") + 1,cmd_line.length() - cmd_line.find("|") - 1));
        Command* cmd2 = smash.CreateCommand(command_line_2.c_str());
        int new_stdout_fd = dup(STDOUT_FILENO);
        if(dup2(fd[1], STDOUT_FILENO) == -1){
            perror("smash error: dup2 failed");
            return;
        }
        if(close(fd[1]) == -1){
            perror("smash error: close failed");
            return;
        }
        cmd1->execute();
        if(dup2(new_stdout_fd, STDOUT_FILENO) == -1){
            perror("smash error: dup2 failed");
            return;
        }
        if(close(new_stdout_fd) == -1){
            perror("smash error: close failed");
            return;
        }
        int new_stdin_fd = dup(STDIN_FILENO);
        if(dup2(fd[0], STDIN_FILENO) == -1){
            perror("smash error: dup2 failed");
            return;
        }
        cmd2->execute();
        if(dup2(new_stdin_fd, STDIN_FILENO) == -1){
            perror("smash error: dup2 failed");
            return;
        }
        if(close(new_stdin_fd) == -1){
            perror("smash error: close failed");
            return;
        }
    } else {
        command_line_2 = _trim(cmd_line.substr(cmd_line.find("|&") + 2,cmd_line.length() - cmd_line.find("|&") - 2));
        Command* cmd2 = smash.CreateCommand(command_line_2.c_str());
        int new_stderr_fd = dup(STDERR_FILENO);
        if(new_stderr_fd == -1){
            perror("smash error: dup failed");
        }
        if(dup2(fd[1], STDERR_FILENO)==-1){
            perror("smash error: dup2 failed");
        }
        if(close(fd[1]) == -1){
            perror("smash error: close failed");
            return;
        }
        cmd1->execute();
        if(dup2(new_stderr_fd, STDERR_FILENO) == -1){
            perror("smash error: dup2 failed");
            return;
        }
        if(close(new_stderr_fd) == -1){
            perror("smash error: close failed");
            return;
        }
        int new_stdin_fd = dup(STDIN_FILENO);
        if(dup2(fd[0], STDIN_FILENO) == -1){
            perror("smash error: dup2 failed");
            return;
        }
        cmd2->execute();
        if(dup2(new_stdin_fd, STDIN_FILENO) == -1){
            perror("smash error: dup2 failed");
            return;
        }
        if(close(new_stdin_fd) == -1){
            perror("smash error: close failed");
            return;
        }
    }
}
//======================================== Fare Command ===================================//
FareCommand::FareCommand(const std::string cmd_line) : BuiltInCommand(cmd_line) ,counter(0){
    SmallShell& smash = SmallShell::getInstance();
    vector<string> args = smash.convertToVector(cmd_line);
    if(args.size() != 4 || args[1].length() <= 4 || args[1][args[1].length() - 1] != 't' || args[1][args[1].length() - 2] != 'x' ||
                                                    args[1][args[1].length() - 3] != 't' ||args[1][args[1].length() - 4] != '.' ){
        cerr << "smash error: fare: invalid arguments" << endl;
    }
    word_to_replace = args[2];
    replacement = args[3];
}

void FareCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    vector<string> args = smash.convertToVector(cmd_line);
//    char args1[args[1].length()-4];
//    for(int i = 0; i < args[1].length() - 4;i ++)
//        args1[i] = args[1][i];
    int temp_fd = 0;
    int file_fd = 0;
//    remove(args[1].c_str());
//    if((file_fd = open(args[1].c_str(), O_CREAT | O_TRUNC  | O_RDWR, 777)) == -1){
////        remove(args1);
//        perror("smash error: open failed");
//        return;
//    }
    string path = "fare_temp.txt" ;
    remove("fare_temp.txt");
    if((temp_fd = open( path.c_str(),O_CREAT | O_APPEND  | O_RDWR, 0777)) == -1){
        perror("smash error: open failed");
        return;
    }

    ifstream file(args[1].c_str());
    if(!file){
        perror("smash error: open failed");
        return;
    }
    string line;
    vector<string> file_by_lines;
    while(getline(file,line)){
        std::string::size_type pos = 0;
        while ((pos = line.find(word_to_replace, pos)) != std::string::npos) {
            ++ counter;
            pos += word_to_replace.length();
        }
        file_by_lines.push_back(line);
    }

    for(int i = 0; i < file_by_lines.size();i ++){
        int start_pos = 0;
        while((start_pos = file_by_lines[i].find(word_to_replace, start_pos)) != std::string::npos) {
            file_by_lines[i].replace(start_pos, word_to_replace.length(), replacement);
            start_pos += replacement.length();
        }
        if(!write(temp_fd,file_by_lines[i].c_str() + '\n',file_by_lines[i].length())){
            perror("smash error: write failed");
            return;
        }
    }
    ofstream file2(args[1].c_str());
    for(int i = 0; i < file_by_lines.size();i ++){
        file2 << file_by_lines[i] << "\n";
    }
    file.close();
    file2.close();


//    rename("fare_temp.txt",args[1].c_str());
//    file.close();
    close(temp_fd);
//    dup2(temp_fd,file_fd);
    cout << "replaced " << counter << " instances of the string \"" << word_to_replace << "\"" << endl;
    return;
}
//======================================== Kill Command ===================================//
KillCommand::KillCommand(const std::string cmd_line) : BuiltInCommand(cmd_line) {}

void KillCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    vector<string> args = smash.convertToVector(cmd_line);
    if(args.size() != 3){
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    int sig = atoi(args[1].c_str() + 1);
    int job_id = atoi(args[2].c_str());
    if(sig == -1 || job_id < 0 || args[1][0] != '-'){
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    if(!smash.jobs_list.getJobById(job_id)){
        cerr << "smash error: kill: job-id: " << job_id << " does not exist" << endl;
        return;
    }
    int pid = smash.jobs_list.getJobById(job_id)->pid;
    if(kill(pid,sig) == -1){
        perror("smash error: kill failed");
        return; //:)
    }

    cout << "signal number " << sig << " was send to pid " << pid << endl;
}
SmallShell::SmallShell() : last_dir(""), pid(getpid()), smash_prompt("smash"), current_process(-1), current_cmd_line(""),
                                jobs_list(JobsList()), last_stopped_job_id(0), pid_for_pipe(0){
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
    if(strstr(cmd_line,">") || strstr(cmd_line,">>")){
        return new RedirectionCommand(cmd_s);
    }
    if(strstr(cmd_line,"|") || strstr(cmd_line,"|&")){
        return new PipeCommand(cmd_s);
    }
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
    if(firstWord.compare("quit") == 0){
        return new QuitCommand(cmd_s);
    }
    if(firstWord.compare("fare") == 0){
        return new FareCommand(cmd_s);
    }
    if(firstWord.compare("kill") == 0){
        return new KillCommand(cmd_s);
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