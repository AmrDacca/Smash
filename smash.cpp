#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"



int main(int argc, char* argv[]) {
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }
    struct sigaction sa ;
    sa.sa_handler = alarmHandler;
    sa.sa_flags = SA_RESTART;
    if(sigaction( SIGALRM,&sa, &sa)==-1) {
        perror("smash error: failed to set SIGALRM handler");
    }

    SmallShell& smash = SmallShell::getInstance();
    while(true) {
        try {
            std::cout << smash.chprompt << "> ";
            std::string cmd_line;
            std::getline(std::cin, cmd_line);
            if (cmd_line == ""){
                continue;
            }
            smash.executeCommand(cmd_line.c_str());
        }
        catch(std::logic_error){

        }
    }
    return 0;
}