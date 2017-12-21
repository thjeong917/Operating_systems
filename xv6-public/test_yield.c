#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[])
{
    int pid;
    int i;
    pid = fork();

       
        if(pid < 0)
        {
            printf(1,"fork failed!\n");
            exit();   
        }

        else if(pid == 0)   //child
        {
            for(i=0;i<50;i++){
                printf(1,"Child\n");
                yield();
            }
        }

        else    // parent
        {
            for(i=0;i<50;i++){
                printf(1,"Parent\n");
                yield();
            }
        }
        
    wait();
    exit();

    return 0;
}
