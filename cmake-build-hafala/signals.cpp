#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include "unistd.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    std::cout << "smash: got ctrl-Z"<< endl;
    SmallShell& smash = SmallShell::getInstance();
    pid_t current_pid = smash.current_process;
    std::string current_cmd_line = smash.current_cmd_line;
    if(current_pid != -1) {
        smash.jobs_list.removeFinishedJobs();
        smash.jobs_list.addJob(current_cmd_line,current_pid,true);
        smash.jobs_list.getLastStoppedJob(&smash.last_stopped_job_id);
        kill(current_pid,SIGSTOP);
        smash.current_process = -1;
        smash.current_cmd_line = "";
        std:: cout<< "smash: process " << current_pid << " was stopped" << endl;
    }
}

void ctrlCHandler(int sig_num) {
  std::cout << "smash: got ctrl-C"<< endl;
  SmallShell& smash = SmallShell::getInstance();
  pid_t current = smash.current_process;

  if(current != -1){
      kill(current, SIGKILL);
      smash.current_process = -1;
      std:: cout<< "smash: process " << current << " was killed" << endl;
  }
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

