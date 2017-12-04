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
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/evp.h>

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
	bank->pubAtm = retrieveKey(0, 0, fname);
	bank->privBank = retrieveKey(1, 0, fname);
    return bank;
}

void bank_free(Bank *bank) {
    if(bank != NULL) {
        close(bank->sockfd);
		list_free(bank->namePin);
		list_free(bank->nameBal);
		RSA_free(bank->pubAtm);
		RSA_free(bank->privBank);
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
			printf("Usage:\tdeposit <user-name> <amt>\n");
		} else {
			deposit(bank, cmds[1], cmds[2]);
		}
	} else if (strcmp(cmds[0], "balance") == 0) {
		// Checking for extra inputs to balance.
		if (strcmp(cmds[2], "\0") != 0) {
			printf("Usage:\tbalance <user-name>\n");
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
  	char sendline[1000];
  	int i = 0;
	char *cmds[3];
  	int status = 0;
	for (i = 0; i < 3; i++) {
		cmds[i] = malloc(251);
		memset(cmds[i], '\0', 251);
	}

	sscanf(command, "%s %s %s", cmds[0], cmds[1], cmds[2]);
	memset(sendline, '\0', 1000);
	if (strcmp(cmds[0], "isUser") == 0) {
		printf("Got : %s %s\n",cmds[1],cmds[2]);
		status =  exists(bank, cmds[1]);
    	sprintf(sendline, "%d",status);
    	bank_send(bank, sendline, strlen(sendline));
	} else if (strcmp(cmds[0], "withdraw") == 0) {
		status = withdraw(bank, cmds[1], cmds[2]);
    	sprintf(sendline, "%d",status);
    	bank_send(bank, sendline, strlen(sendline));
	} else if (strcmp(cmds[0], "balance") == 0) {
		printf("Got : %s %s\n",cmds[1],cmds[2]);
		status = atmBalance(bank, cmds[1]);
    	sprintf(sendline, "%d",status);
    	bank_send(bank, sendline, strlen(sendline));
	} else {
    	sprintf(sendline, "Error");
    	bank_send(bank, sendline, strlen(sendline));
	}
	memset(sendline, '\0', 1000);
	for (i = 0; i < 3; i++) {
		free(cmds[i]);
	}
}

int exists(Bank* bank, char *name){
  	if(list_find(bank->nameBal, name) == NULL){
   		return 0;
  	}
	return 1;
}

int withdraw(Bank *bank,char *name,char* amt){
    char *balPtr;
	int bal;
	char *balance = malloc(12);
	long long amount;

	memset(balance, '\0', 12);
    amount = strtol(amt, NULL, 10);
	
	if(list_find(bank->nameBal, name) == NULL){
		free(balance);
   		return 0;
  	}

	balPtr = (char*)list_find(bank->nameBal, name);
	bal = atoi(balPtr);

	// Making sure that the new balance cannot be zero.
	if ((bal - amount) < 0) {
		free(balance);
		return 0;
	}

	// Deleting the old entry and replacing with the new.
	bal -= amount;
	sprintf(balance, "%d", bal);

	list_del(bank->nameBal, name);
	list_add(bank->nameBal, name, balance);

	free(balance);
	// Deposit successful.
	return 1;
}

int atmBalance(Bank *bank, char *name) {
	char *balPtr;
	printf("%s",name);
	balPtr = (char*) list_find(bank->nameBal, name);
	if (!balPtr) {
		printf("No such user\n");
		return -1;
	}

	return atoi(balPtr);
}

// Creates a user entry in the bank, along with a card for the user to use at the ATM.
void create_user(Bank *bank, char *name, char *pin, char *balance) {
	FILE *fp;
	char filename[256];
	long long amount = strtol(balance, NULL, 10);
	int user_pin = atoi(pin);

	// Checking the formatting of inputs for validity.
	if (!reg_matches(name, "[a-zA-Z]+") ||
		!reg_matches(pin, "[0-9][0-9][0-9][0-9]") ||
		!reg_matches(balance, "[0-9]+")) {

		printf("Usage:\tcreate-user <user-name> <pin> <balance>\n");
		return;
	}

	// Further checking the validity of inputs.
	if (strlen(name) > 250 || user_pin < 0 || amount < 0 || (1 + amount) > INT_MAX || amount < INT_MIN)	{
		printf("Usage:\tcreate-user <user-name> <pin> <balance>\n");
		return;
	}
	
	
	// Checking if the user is already in the bank systems.
	if (list_find(bank->nameBal, name) != NULL) {
		printf("Error:\tuser %s already exists\n", name);
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

	unsigned char *tmp = malloc(RSA_size(bank->pubAtm));
	int length = encryptMsg(bank->pubAtm, pin, tmp);
	/*RSA *r2 = retrieveKey(1, 1, "./test1.atm");
	RSA *r = retrieveKey(0, 1, "./test1.atm");
	unsigned char *tmp2 = malloc(RSA_size(r2));*/
	fwrite(tmp, 1, 256, fp);
	fclose(fp);
	

//Use this to decrypt in ATM
	/*char *tmp3 = malloc(RSA_size(r2));
	//memset(tmp3, '\0', 256);
	fp = fopen(filename, "r");
	fread(tmp3, 1, 256, fp);
	decryptMsg(r2, tmp3, tmp2, 256);
	
	printf("decrypted: %s\n", tmp2);*/

	
	printf("Created user %s\n", name);
	//free(tmp3);
	//free(tmp2);
	free(tmp);
}

void deposit(Bank *bank, char *name, char *amt) {
	char *balPtr;
	int bal;
	char *balance = malloc(12);
	long long amount;

	memset(balance, '\0', 12);
	// Checking if the amount is larger than an int can hold.
	if ((amount = strtol(amt, NULL, 10)) > INT_MAX) {
		printf("Usage:\tdeposit <user-name> <amt>\n");
		return;
	}

	// Checking the formatting of inputs for validity.
	if (!reg_matches(name, "[a-zA-Z]+") || strlen(name) > 250 ||
		!reg_matches(amt, "[0-9]+") || amount < 0) {

		printf("Usage:\tdeposit <user-name> <amt>\n");
		return;
	}
	
	// Checking if the user is already in the bank systems.
	balPtr = (char*)list_find(bank->nameBal, name);
	if (!balPtr) {
		printf("No such user\n");
		return;
	}

	bal = atoi(balPtr);
	// Making sure that the new balance cannot exceed the maximum int value.
	if ((bal + amount) > INT_MAX) {
		printf("Too rich for this program\n");
		return;
	}

	// Deleting the old entry and replacing with the new.
	bal += amount;
	sprintf(balance, "%d", bal);
	list_del(bank->nameBal, name);
	list_add(bank->nameBal, name, balance);	

	free(balance);
	// Deposit successful.
	printf("$%lld added to %s's account\n", amount, name);	
}

void balance(Bank *bank, char *name) {
	char *balPtr;
	int bal;

	// Checking the formatting of inputs for validity.
	if (!reg_matches(name, "[a-zA-Z]+") || strlen(name) > 250) {
		printf("Usage:\tbalance <user-name>\n");
		return;
	}

	// Checking if the user is already in the bank systems.
	balPtr = (char*) list_find(bank->nameBal, name);
	if (!balPtr) {
		printf("No such user\n");
		return;
	}

	bal = atoi(balPtr);
	// Printing user's balance.
	printf("$%d\n", bal);
}

