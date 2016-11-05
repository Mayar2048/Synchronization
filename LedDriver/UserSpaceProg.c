#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include "DeviceHeaders.h"

char* led[LENGTH] = {"scroll", "num", "caps"};
char* functionality[STATES] = {"set", "get"};
char* state[STATES] = {"off", "on"};
char message[LENGTH];
long ioctl_return;
int file_desc;
int ledNum;
int ledState;

// Functions' Protoypes
int get_functionality(char*);
int get_state(char*);
int get_led(char*);

int main(int argc, char* argv[])
{
    if((argc == 3) || (argc == 4))
    {
        int functionality_flag = get_functionality(argv[1]);
        if(functionality_flag != -1)
        {
            // Open device file
            file_desc = open("/dev/ledModule", O_RDWR);
            if (file_desc < 0)
            {
                perror("Failed to open the device...");
                return errno;
            }

            // set command
            if(functionality_flag == 0)
            {
                if(argc == 4)
                {
                    // Write to device file
                    ledNum = get_led(argv[2]);
                    ledState = get_state(argv[3]);
                    if((ledNum != -1) && (ledState != -1))
                    {
                        int ledIndexState = (ledNum * 10) + ledState;
                        ioctl_return = ioctl(file_desc, IOCTL_SET_LED_STATE, ledIndexState);
                        if (ioctl_return < 0)
                        {
                            perror("Failed to write the message to the device.");
                            return errno;
                        }
                    }
                    else
                        perror("Incorrect Aguments to 'set' Command!!\n");
                }
                else
                    perror("Incorrect Number of Aguments 'set' Command!!\n");
            }
            // get command
            else
            {
                // Read From Device File
                ledNum = get_led(argv[2]);
                if(ledNum != -1)
                {
                    ioctl_return = ioctl(file_desc, IOCTL_GET_LED_STATE, ledNum);
                    if (ioctl_return < 0)
                    {
                        perror("Failed to read the message from the device.");
                        return errno;
                    }
                    else
                    {
                        if(ioctl_return)
                            printf("on\n");
                        else
                            printf("off\n");
                    }
                }
                else
                    perror("Incorrect Aguments to 'get' Command!!\n");
            }

            // Close device file
            close(file_desc);
        }
        else
            perror("Incorrect Command to the Led Driver!!\n");
    }
    else
        perror("Incorrect Number of Aguments to the Led Driver!!\n");
    return 0;
}

/**
*Purpose: indicate whether the user command is set or get
*@param argv[1]
*@return index of set, get or -1 if wrong argument
*/
int get_functionality(char* argv)
{
    int i;
    for(i = 0; i < STATES; i++)
    {
        if(strcmp(argv, functionality[i]) == 0)
        {
            return i;
        }
    }
    return -1;
}

/**
*Purpose: indicate whether the led is scroll, num or caps
*@param argv[2]
*@return index of the led or -1 if wrong argument
*/
int get_led(char* argv)
{
    int i;
    for(i = 0; i < LENGTH; i++)
    {
        if(strcmp(argv, led[i]) == 0)
        {
            return i;
        }
    }
    return -1;
}

/**
*Purpose: indicate whether the state is on or off
*@param argv[3]
*@return index of on, off or -1 if wrong argument
*/
int get_state(char* argv)
{
    int i;
    for(i = 0; i < STATES; i++)
    {
        if(strcmp(argv, state[i]) == 0)
        {
            return i;
        }
    }
    return -1;
}
