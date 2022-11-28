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

bool _isBackgroundComamnd(const std::string cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(std::string cmd_line) {
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
    args[vec.size() + 1] = NULL;
    return args;
}


void clearArgs(char** args, int size){
    for(int i = 0; i<size;i++){
        if(args[i])
            delete []args[i];
    }
    if(args)
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

//==================================== EXTERNAL COMMAND ===========================================//


ExternalCommand::ExternalCommand(const std::string cmd_line) : Command(cmd_line){}

void ExternalCommand::execute() {
    int status;
    SmallShell& smash = SmallShell::getInstance();
    vector<string> args = smash.convertToVector(cmd_line);
    bool is_background = _isBackgroundComamnd(cmd_line);
    if(is_background){
        _removeBackgroundSign(cmd_line);
    }
    char command[args[0].length()];
    strcpy(command,args[0].c_str());
    char cmd[COMMAND_MAX_ARGS];
    strcpy(cmd,cmd_line.c_str());
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
//        sleep(5);
        //complex external
        if(isComplex(cmd_line) == true){
            char* complex_args[] = {complex_path,flag,cmd, nullptr};
            if(execvp(complex_path,complex_args) == -1){
                perror("smash error: execvp failed complex");
                return;
            }
        }
            //rest of externals
        else{
            if(execvp(command,arguments) == -1){
                perror("smash error: execvp failed");
                return;
            }
        }
        clearArgs(arguments, args.size());
    }
    //daddy
    //foreground
    if(!is_background)
        if(waitpid(pid,&status,WUNTRACED) == -1){
            perror("smash error: waitpid failed");
            return;
        }
    //background
    else{
        ///future
    }
    return;
}



SmallShell::SmallShell() : last_dir(""), pid(getpid()), smash_prompt("smash") {
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
   cmd->execute();
   delete cmd;
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}


//this is a test gadol