#include "bank.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>
#include <limits.h>
#include "util/list.c"
#include "util/hash_table.c"
#include "util/util_functions.c"
#include "util/user_data.c"

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
	int i = 0;
	char *cmds[4];

	for (i = 0; i < 4; i++) {
		cmds[i] = malloc(251);
	}

	// An invalid command or argument has been found.
	if (bank_split_line(cmds, command) == -1) {
		// The command is create-user.
		if (strcmp(cmds[0], "create-user") == 0) {
			printf("Usage: create-user <user-name> <pin> <balance>\n");
		} else if (strcmp(cmds[0], "deposit") == 0) {
			printf("Usage: deposit <user-name> <amt>\n");
		} else if (strcmp(cmds[0], "balance") == 0) {
			printf("Usage: balance <user-name>\n");
		} else {
			printf("Invalid command\n");
		}
		return;
	}


	if (strcmp(cmds[0], "create-user") == 0) {
		create_user(bank, cmds[1], cmds[2], cmds[3]);
	} else if (strcmp(cmds[0], "deposit") == 0) {
		deposit(bank, cmds[1], cmds[2]);
	} else if (strcmp(cmds[0], "balance") == 0) {
		balance(bank, cmds[1]);
	} else {
		printf("Invalid command\n");
	}

	for (i = 0; i < 4; i++) {
		free(cmds[i]);
	}
}

void bank_process_remote_command(Bank *bank, char *command, size_t len) {
int i = 0;
char *cmds[3];

for (i = 0; i < 3; i++) {
  cmds[i] = malloc(251);
}

// An invalid command or argument has been found.
if (atm_bank_split_line(cmds, command) == -1) {
  // The command is create-user.
  if (strcmp(cmds[0], "create-user") == 0) {
    printf("Usage: create-user <user-name> <pin> <balance>\n");
  } else if (strcmp(cmds[0], "deposit") == 0) {
    printf("Usage: deposit <user-name> <amt>\n");
  } else if (strcmp(cmds[0], "balance") == 0) {
    printf("Usage: balance <user-name>\n");
  } else {
    printf("Invalid command\n");
  }
  return;
}


if (strcmp(cmds[0], "isUser") == 0) {
  isUser(cmds[1]);
} else if (strcmp(cmds[0], "withdraw") == 0) {
  atmWithdraw(cmds[1], cmds[2]);
} else if (strcmp(cmds[0], "balance") == 0) {
  atmBalance(cmds[1],cmds[2]);
} else {
  printf("Invalid command\n");
}

for (i = 0; i < 4; i++) {
  free(cmds[i]);
}
}

void isUser(char *name){
  
}
// Creates a user entry in the bank, along with a card for the user to use at the ATM.
void create_user(Bank *bank, char *name, char *pin, char *balance) {
	FILE *fp;
	char filename[256];
	int user_pin = atoi(pin);
	int user_bal = atoi(balance);

	// Checking the formatting of inputs for validity.
	if (!reg_matches(name, "[a-zA-Z]+") ||
		!reg_matches(pin, "[0-9][0-9][0-9][0-9]") ||
		!reg_matches(balance, "[0-9]+")) {

		printf("Usage: create-user <user-name> <pin> <balance>\n");
		return;
	}

	// Further checking the validity of inputs.
	if (strlen(name) > 250 || user_pin < 0 || user_bal < 0 || user_bal > INT_MAX)	{
		printf("Usage: create-user <user-name> <pin> <balance>\n");
		return;
	}

	// Checking if the user is already in the bank systems.
	if (hash_table_find(bank->accounts, name) != NULL) {
		printf("Error: user %s already exists\n", name);
		return;
	}

	// Adding the user & data to the bank systems.
	UserData *user = malloc(sizeof(UserData));
	create_data(user, user_pin, user_bal);
	hash_table_add(bank->accounts, name, user);

	// Creating filename for user's .card file.
	strcpy(filename, name);
	strncat(filename, ".card", 5);
	// Creating the user's .card file.
	if (!(fp = fopen(filename, "w+"))) {
		printf("Error creating card file for user %s\n", name);
		hash_table_del(bank->accounts, name);
		free(user);
		return;
	}

	fprintf(fp, "%s,%s", pin, balance);
	fclose(fp);
}

void deposit(Bank *bank, char *name, char *amt) {
	int amount;
	UserData *user;

	// Checking if the amount is larger than an int can hold.
	if (atoi(amt) > INT_MAX) {
		printf("Usage: deposit <user-name> <amt>\n");
		return;
	}

	amount = atoi(amt);
	// Checking the formatting of inputs for validity.
	if (!reg_matches(name, "[a-zA-Z]+") || strlen(name) > 250 ||
		!reg_matches(amt, "[0-9]+") || amount < 0) {

		printf("Usage: deposit <user-name> <amt>\n");
		return;
	}

	// Checking if the user is already in the bank systems.
	user = hash_table_find(bank->accounts, name);
	if (!user) {
		printf("No such user\n");
		return;
	}

	// Making sure that the new balance cannot exceed the maximum int value.
	if ((user->balance + amount) > INT_MAX) {
		printf("Too rich for this program\n");
		return;
	}

	// Deleting the old entry and replacing with the new.
	user->balance += amount;
	hash_table_del(bank->accounts, name);
	hash_table_add(bank->accounts, name, user);

	// Deposit successful.
	printf("$%d added to %s's account\n", amount, name);
}

void balance(Bank *bank, char *name) {
	UserData *user;
	// Checking the formatting of inputs for validity.
	if (!reg_matches(name, "[a-zA-Z]+") || strlen(name) > 250) {
		printf("Usage: balance <user-name>\n");
		return;
	}

	// Checking if the user is already in the bank systems.
	user = hash_table_find(bank->accounts, name);
	if (!user) {
		printf("No such user\n");
		return;
	}

	// Printing user's balance.
	printf("$%d\n", user->balance);
}
