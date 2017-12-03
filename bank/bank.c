#include "bank.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>
#include <limits.h>
#include "util/list.c"
#include "util/hash_table.c"
//#include "util/util_functions.c"
#include "util/user_data.c"

Bank* bank_create(FILE *fp, char *fname) {
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

	//bank->namePin = hash_table_create(10);
	//bank->nameBal = hash_table_create(10);

	bank->namePin = list_create();
	bank->nameBal =	list_create();
	bank->fname = fname;
	bank->file = fp;
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
	int i = 0;	
	char *cmds[4];

	for (i = 0; i < 4; i++) {
		cmds[i] = malloc(251);
		memset(cmds[i], '\0', 251);
	}

	sscanf(command, "%s %s %s %s", cmds[0], cmds[1], cmds[2], cmds[3]);
	
	if (strcmp(cmds[0], "create-user") == 0) {
		create_user(bank, cmds[1], cmds[2], cmds[3]);
	} else if (strcmp(cmds[0], "deposit") == 0) {
		// Checking for extra inputs to deposit.
		if (strcmp(cmds[3], "\0") != 0) {
			printf("Usage: deposit <user-name> <amt>\n");
		} else {
			deposit(bank, cmds[1], cmds[2]);
		}
	} else if (strcmp(cmds[0], "balance") == 0) {
		// Checking for extra inputs to balance.
		if (strcmp(cmds[2], "\0") != 0) {
			printf("Usage: balance <user-name>\n");
		} else {
			balance(bank, cmds[1]);
		}
	} else {
		printf("Invalid command\n");
	}
	for (i = 0; i < 4; i++) {
		free(cmds[i]);
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
void create_user(Bank *bank, char *name, char *pin, char *balance) {
	FILE *fp;
	//char personName[256];
	char filename[256];
	long long amount = strtol(balance, NULL, 10);
	int user_pin = atoi(pin);

	// Checking the formatting of inputs for validity.
	if (!reg_matches(name, "[a-zA-Z]+") ||
		!reg_matches(pin, "[0-9][0-9][0-9][0-9]") ||
		!reg_matches(balance, "[0-9]+")) {

		printf("Usage: create-user <user-name> <pin> <balance>\n");
		return;
	}

	//printf("%lld\n", amount);
	// Further checking the validity of inputs.
	if (strlen(name) > 250 || user_pin < 0 || amount < 0 || (1 + amount) > INT_MAX || amount < INT_MIN)	{
		printf("Usage: create-user <user-name> <pin> <balance>\n");
		return;
	}
	
	
	// Checking if the user is already in the bank systems.
	if (list_find(bank->nameBal, name) != NULL) {
		printf("Error: user %s already exists\n", name);
		return;
	}

	// Adding the user & data to the bank systems.
	list_add(bank->namePin, name, pin);
	list_add(bank->nameBal, name, balance);
	
	// Creating filename for user's .card file.
	strcpy(filename, name);
	strncat(filename, ".card", 5);
	// Creating the user's .card file.
	if (!(fp = fopen(filename, "w+"))) {
		printf("Error creating card file for user %s\n", name);
		list_del(bank->nameBal, name);
		list_del(bank->namePin, name);
		return;
	}

	int keylen = getKeyLen(0, 0, bank->fname);
	printf("pubkey keylen: %d\n", keylen);
	char *pubKey = malloc(keylen + 1);
	memset(pubKey, '\0', keylen + 1);
	getKey(0, 0, bank->fname, pubKey, keylen);
	
	printf("Public Key: %s\n", pubKey);
	keylen = getKeyLen(1, 0, bank->fname);
	printf("privkey keylen: %d\n", keylen);
	char *privKey = malloc(keylen + 1);
	memset(privKey, '\0', keylen + 1);
	getKey(0, 0, bank->fname, privKey, keylen);
	printf("Private Key: %s\n", privKey);

	free(pubKey);
	free(privKey);
	fprintf(fp, "%s,%s", pin, balance);
	fclose(fp);
	printf("Created user %s\n", name);
}

void deposit(Bank *bank, char *name, char *amt) {
	char *balPtr;
	int bal;
	char *balance = malloc(12);
	long long amount;
	//UserData *user;

	memset(balance, '\0', 12);
	// Checking if the amount is larger than an int can hold.
	if ((amount = strtol(amt, NULL, 10)) > INT_MAX) {
		printf("Usage: deposit <user-name> <amt>\n");
		return;
	}

	//amount = atoi(amt);
	// Checking the formatting of inputs for validity.
	if (!reg_matches(name, "[a-zA-Z]+") || strlen(name) > 250 ||
		!reg_matches(amt, "[0-9]+") || amount < 0) {

		printf("Usage: deposit <user-name> <amt>\n");
		return;
	}

	//printf("%s\n", name);
	
	// Checking if the user is already in the bank systems.
	//user = hash_table_find(bank->accounts, name);
	balPtr = (char*)list_find(bank->nameBal, name);
	if (!balPtr) {
		printf("No such user\n");
		return;
	}

	bal = atoi(balPtr);
	// Making sure that the new balance cannot exceed the maximum int value.
	//if ((user->balance + amount) > INT_MAX) {
	//printf("%d\n", bal);
	//printf("%d\n", INT_MAX);
	if ((bal + amount) > INT_MAX) {
		printf("Too rich for this program\n");
		return;
	}

	// Deleting the old entry and replacing with the new.
	bal += amount;
	sprintf(balance, "%d", bal);

	//user->balance = bal;
	//hash_table_del(bank->accounts, name);
	//hash_table_add(bank->accounts, name, user);
	list_del(bank->nameBal, name);
	list_add(bank->nameBal, name, balance);	

	free(balance);
	// Deposit successful.
	printf("$%lld added to %s's account\n", amount, name);	
}

void balance(Bank *bank, char *name) {
	//UserData *user;
	char *balPtr;
	int bal;
	//const char* username = name;
	// Checking the formatting of inputs for validity.
	if (!reg_matches(name, "[a-zA-Z]+") || strlen(name) > 250) {
		printf("Usage: balance <user-name>\n");
		return;
	}

	// Checking if the user is already in the bank systems.
	/*user = (UserData*)hash_table_find(bank->accounts, name);
	if (!user) {
		printf("No such user\n");
		return;
	}*/

	balPtr = (char*) list_find(bank->nameBal, name);
	if (!balPtr) {
		printf("No such user\n");
		return;
	}
	//bal = atoi(user->balance);
	bal = atoi(balPtr);
	// Printing user's balance.
	printf("$%d\n", bal);
}

