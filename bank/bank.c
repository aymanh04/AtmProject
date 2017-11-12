#include "bank.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>
#include "util_functions.c"
#include "user_data.h"

Bank* bank_create() {
    Bank *bank = (Bank*) malloc(sizeof(Bank));
    if(bank == NULL) {
        perror("Could not allocate Bank");
        exit(1);
    }

    // Set up the network state
    bank->sockfd=socket(AF_INET,SOCK_DGRAM,0);

    bzero(&bank->rtr_addr,sizeof(bank->rtr_addr));
    bank->rtr_addr.sin_family = AF_INET;
    bank->rtr_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    bank->rtr_addr.sin_port=htons(ROUTER_PORT);

    bzero(&bank->bank_addr, sizeof(bank->bank_addr));
    bank->bank_addr.sin_family = AF_INET;
    bank->bank_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    bank->bank_addr.sin_port = htons(BANK_PORT);
    bind(bank->sockfd,(struct sockaddr *)&bank->bank_addr,sizeof(bank->bank_addr));

    // Set up the protocol state
    // TODO set up more, as needed

	bank->accounts = hash_table_create(50);
    return bank;
}

void bank_free(Bank *bank) {
    if(bank != NULL) {
        close(bank->sockfd);
        free(bank);
    }
}

ssize_t bank_send(Bank *bank, char *data, size_t data_len) {
    // Returns the number of bytes sent; negative on error
    return sendto(bank->sockfd, data, data_len, 0,
                  (struct sockaddr*) &bank->rtr_addr, sizeof(bank->rtr_addr));
}

ssize_t bank_recv(Bank *bank, char *data, size_t max_data_len) {
    // Returns the number of bytes received; negative on error
    return recvfrom(bank->sockfd, data, max_data_len, 0, NULL, NULL);
}

void bank_process_local_command(Bank *bank, char *command, size_t len) {
    // TODO: Implement the bank's local commands
	char cmds[4][251];
	
	// An invalid command or argument has been found.
	if (bank_split_line(cmds, command) == -1) {
		if (strcmp(cmds[0], "create-user") == 0) 
			printf("Usage: create-user <user-name> <pin> <balance>\n");

		
		return;
	}
	
	switch(cmds[0]) {
		case "create-user" :
			int pin = atoi(cmds[2])
			int bal = atoi(cmds[3])
			create_user(bank, cmds[1], pin, balance);
			break;

		case "deposit" :
		
			break;

		case "balance" :

			break;
		default :
		
	}

	
}

void bank_process_remote_command(Bank *bank, char *command, size_t len) {
    // TODO: Implement the bank side of the ATM-bank protocol

	/*
	 * The following is a toy example that simply receives a
	 * string from the ATM, prepends "Bank got: " and echoes 
	 * it back to the ATM before printing it to stdout.
	 */

	/*
    char sendline[1000];
    command[len]=0;
    sprintf(sendline, "Bank got: %s", command);
    bank_send(bank, sendline, strlen(sendline));
    printf("Received the following:\n");
    fputs(command, stdout);
	*/
}

// Creates a user entry in the bank, along with a card for the user to use at the ATM.
void create_user(Bank *bank, char *name, int pin, int balance) {
	UserData *user;
	create_user(&user, pin, balance);
	hash_table_add(bank->accounts, name, user);
}
