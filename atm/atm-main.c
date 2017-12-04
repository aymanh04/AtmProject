/* 
 * The main program for the ATM.
 *
 * You are free to change this as necessary.
 */

#include "atm.h"
#include <stdio.h>
#include <stdlib.h>

static const char prompt[] = "ATM: ";

int main(int argc, char**argv)
{
	FILE *fp;
	char *fname = argv[1];
	char user_input[1000];
	fp = fopen(fname, "r");
	if (!fp) {
		printf("Error opening ATM initialization file\n");
		return 64;
	}

    ATM *atm = atm_create(fp,fname);

    printf("%s", prompt);
    fflush(stdout);

    while (fgets(user_input, 10000,stdin) != NULL)
    {
        atm_process_command(atm, user_input);
	if(atm->logged){
	    printf("ATM (%s): ", atm->user);
        } else {
            printf("%s", prompt);
        }
        fflush(stdout);
    }
	return EXIT_SUCCESS;
}
