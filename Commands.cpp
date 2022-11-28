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

// TODO: Add your implementation for classes in Commands.h 

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


SmallShell::SmallShell() : last_dir(""), pid(getpid()), smash_prompt("smash") {
// TODO: add your implementation
};

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