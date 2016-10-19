#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int pass = 0;
    char *crash=NULL;
    char buff[15];

    crash = buff;
    printf("\n Enter the password : \n");
    strcpy(buff, argv[1]);

    if(strcmp(buff, "thegeekstuff"))
    {   
        printf ("\n Wrong Password \n");
    }   
    else{
        printf ("\n Correct Password \n");
        pass = 1;
    }   

    if(pass)
    {   
         /* Now Give root or admin rights to user*/
         printf ("\n Root privileges given to the user \n");
    }   

    printf("crash %c", (*crash));
    return 0;
}           
