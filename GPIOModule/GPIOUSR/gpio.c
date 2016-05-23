#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PROCFS_NAME        "/proc/my_gpio"

// Get the value from buttons.
static unsigned char readButton(unsigned char btnNr)
{
    FILE *fp = fopen(PROCFS_NAME,"r+");
    unsigned char state;

    if (btnNr >= '1' && btnNr <= '3')
    {
        fprintf(fp, "b%c", btnNr);
        fscanf(fp, "%c", &state);
    }
    else
    {
        printf ("Error: %c is not a valid button option.\n", btnNr);
        return 'n';
    }
    fclose(fp);
    return state;
}

// Change the value of LEDS.
static void writeLed(unsigned char ledNr, unsigned char ledVal)
{
    FILE *fp = fopen(PROCFS_NAME,"w+");

    if (ledNr >= '1' && ledNr <= '3')
    {
        if (ledVal == '0' || ledVal == '1')
        {
            fprintf(fp, "l%c%c", ledNr, ledVal);
        }
        else
        {
            printf ("Error: %c is not a valid LED value.\n", ledVal);
        }
    }
    else
    {
        printf ("Error: %c is not a valid LED option.\n", ledNr);
    }
    fclose(fp);
}

int main (int argc, char* argv[])
{
    // Write function turns on/off LEDs.
    unsigned char option = 0;
    unsigned char id = 0;
    unsigned char value = 0;
    unsigned char state = 0;

    if (!(argc >= 3 && argc <= 4))
    {
        printf ("Error: Wrong number of parameters.\n");
        return -1;
    }

    // Get the values from user input.
    sscanf(argv[1], "%c", &option);
    sscanf(argv[2], "%c", &id);

    if (argc == 4)  // To avoid segmentation fault.
        sscanf(argv[3], "%c", &value);

    switch (option)
    {
        case 'b':
            if ((state = readButton(id)) != 'n')
            {
                printf("State b%c is %c\n", id, state);
            }
            break;
        case 'l':
            writeLed(id, value);
            printf("Written %c to l%c\n", value, id);
            break;
        default:
            printf ("Error: Wrong option.\n");
            break;
    }
    return 0;
}