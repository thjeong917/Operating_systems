#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX 4096

int print_prompt()
{
    printf("prompt>");

    return 0;
} // function for showing "prompt>"

int main(int argc, char *argv[])
{
    char buf[MAX];
    char *ptr;
    char *ptr2;
    char *tkn[255];// tokenize ";"
    char *tkn2[255];// tokenize " "
    int status;
    int i;
    int cnt=0; //counting strings tokenized by ";"
    int cnt2=0; //counting strings tokenized by " "
    pid_t pid;
    
    if(argc==2)
    {
        freopen(argv[1],"r",stdin);//in case of using 'batch mode', changing stream from 'stdin' to 'argv[1]'
   
    }

    while(1)
    {
        if(feof(stdin)!=0)
                return 0;  //when file ends, return 0;      

        memset(buf,0,sizeof(char)*MAX); //initializing buf[]
        memset(tkn,0,255); //initialuze tkn[]
        
        if(argc<2)
            print_prompt(); // show 'prompt>' at interactive mode

        fgets(buf,MAX,stdin); //accepting commmands
        
        if(argc==2)
            printf("%s", buf); // showing commands at batch mode
 
        for(i=0; i<=(int)strlen(buf); i++)
        {
            if(buf[i] == '\n'){
                buf[i] = 0;
            }
        }// delete "\n"

        cnt=0;
        ptr=strtok(buf,";");

        while(ptr!=NULL)
        {
            tkn[cnt]=(char*)malloc(sizeof(char)*strlen(ptr));
            strcpy(tkn[cnt],ptr);
            cnt++;                      
            ptr=strtok(NULL,";");
            
        } //tokenize with ";"

        for(i=0;i<cnt;i++)
        {
            if(!strcmp(tkn[i],"quit"))
                exit(0);    //exiting './shell' when command line sees 'quit'
            
            pid=fork(); //makes child process
            memset(tkn2,0,255); //initialize tkn2 with '0'
            
            cnt2=0;
            ptr2=strtok(tkn[i]," ");
          
            while(ptr2!=NULL)
            {
                tkn2[cnt2]=(char*)malloc(sizeof(char)*strlen(ptr2));
                strcpy(tkn2[cnt2],ptr2);  
                cnt2++;
                ptr2=strtok(NULL," ");
            } //tokenize with " "
 
            if(pid<0)
            {
                printf("fork failed\n");
            } //when fork failed

            else if(pid==0)
            {   
                if(tkn2[1]==NULL)
                    execlp(tkn2[0],tkn2[0],NULL);

                execvp(tkn2[0],tkn2);

                exit(0);
            } //execute commands with 'exec' functions

            else
                wait(&status); //parent process waiting
        }
    }

    return 0;
}
