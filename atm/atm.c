#include "atm.h"
#include "ports.h"
#include "util/util_functions.c"
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <openssl/rsa.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <regex.h>

ATM* atm_create(FILE *fp, char *fname) {
    ATM *atm = (ATM*) malloc(sizeof(ATM));
    if(atm == NULL) {
        perror("Could not allocate ATM");
        exit(1);
    }

    // Set up the network state
    atm->sockfd=socket(AF_INET,SOCK_DGRAM,0);

    bzero(&atm->rtr_addr,sizeof(atm->rtr_addr));
    atm->rtr_addr.sin_family = AF_INET;
    atm->rtr_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    atm->rtr_addr.sin_port=htons(ROUTER_PORT);

    bzero(&atm->atm_addr, sizeof(atm->atm_addr));
    atm->atm_addr.sin_family = AF_INET;
    atm->atm_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    atm->atm_addr.sin_port = htons(ATM_PORT);
    bind(atm->sockfd,(struct sockaddr *)&atm->atm_addr,sizeof(atm->atm_addr));

    // Set up the protocol state
    atm->user = malloc(251);
    memset(atm->user,'\0',251);
    atm->logged = false;
	atm->fname = fname;
	atm->file = fp;
	atm->pubBank = retrieveKey(0, 1, fname);
	atm->privATM = retrieveKey(1, 1, fname);

    return atm;
}

void atm_free(ATM *atm) {
    if(atm != NULL) {
        close(atm->sockfd);
        free(atm);
    }
}

ssize_t atm_send(ATM *atm, char *data, size_t data_len) {
    // Returns the number of bytes sent; negative on error
    return sendto(atm->sockfd, data, data_len, 0,
                  (struct sockaddr*) &atm->rtr_addr, sizeof(atm->rtr_addr));
}

ssize_t atm_recv(ATM *atm, char *data, size_t max_data_len) {
    // Returns the number of bytes received; negative on error
    return recvfrom(atm->sockfd, data, max_data_len, 0, NULL, NULL);
}

void atm_process_command(ATM *atm, char *command) {
    // TODO: Implement the ATM's side of the ATM-bank protocol

    int i =0;
    char *cmds[2];
    for (i = 0; i < 2; i++) {
		cmds[i] = malloc(251);
		memset(cmds[i], '\0', 251);
    }

    sscanf(command, "%s %s", cmds[0], cmds[1]);

    if (strcmp(cmds[0], "begin-session") == 0) {
        begin_session(cmds[1], atm);
    } else if (strcmp(cmds[0], "withdraw") == 0) {
        withdraw(cmds[1], atm);
    } else if (strcmp(cmds[0], "balance") == 0) {
        if (strcmp(cmds[1], "\0") != 0) {
    	    printf("Usage:\tbalance\n");
        } else {
	    balance(atm);
        }
    } else if (strcmp(cmds[0], "end-session") == 0) {
	    end_session(atm);
    } else {
        printf("Invalid command\n");
    }

    for (i = 0; i < 2; i++) {
        free(cmds[i]);
    }
}

void begin_session(char *data, ATM *atm){
    char recvline[10000];
    int n;
    FILE *fp;

    if(atm->logged){
      printf("A user is already logged in\n");
      return;
    }
    if(!reg_matches(data, "^[a-zA-Z]{1,250}$")){
      printf("Usage:\tbegin-session <user-name>\n");
      return;
    }
    char *isuser = malloc(strlen(data)+8);
    strcpy(isuser,"isUser ");
    strcat(isuser,data);

	printf("%s %d",isuser,strlen(isuser));
    atm_send(atm, isuser, strlen(isuser));
    n = atm_recv(atm,recvline,10000);
    recvline[n] =0;
    if(strcmp(recvline,"0") ==0){
    	printf("No such user\n");
		free(isuser);
		memset(recvline, '\0', 10000);
    	return;
    }
	memset(recvline, '\0', 10000);
    char *name = malloc(strlen(data)+6);
    strcpy(name,data);
    strcat(name,".card");

    fp = fopen(name, "r");

	char *inputLine = malloc(256);
    memset(inputLine, '\0', 256);

    char *pin = malloc(256);
    memset(pin, '\0', 256);

    if(!fp){
        printf("Unable to access %s\'s card\n",data);
    } else {
		fgets(inputLine,256,fp);
		printf("Here\n");
		char *tmp2 = malloc(RSA_size(atm->privATM));
		printf(inputLine);
		char *tmp3 = malloc(RSA_size(atm->privATM));

    	//fread(tmp3, sizeof(tmp3), RSA_size(atm->privATM), fp);
    	//decryptMsg(atm->privATM, tmp3, tmp2, 256);
    	//printf("decrypted: %s\n", tmp2);

		printf("PIN? ");
		fgets(pin,256,stdin);

		if(reg_matches(pin, "[0-9][0-9][0-9][0-9]") && strcmp(pin,inputLine) == 0) {
			printf("Authorized\n");
		    strncpy(atm->user,data,strlen(data));
			atm->logged = true;
		} else {
		    printf("Not authorized\n");
		}
		free(tmp2);
		free(tmp3);
    }
	free(inputLine);
    free(isuser);
    free(name);
	free(pin);
    fclose(fp);
}

void withdraw(char *amt, ATM *atm){
    char recvline[10000];
    int num = atoi(amt);
    int n = 0;
    if(!atm->logged){
    	printf("No user logged in\n");
    	return;
    }
    if(num < 0 || !reg_matches(amt,"[0-9]+") || num > INT_MAX){
    	printf("Usage:\twithdraw <amt>\n");
    	return;
    }
    char *withdraw = malloc(strlen(atm->user)+strlen(amt)+11);
	char *amount = malloc(1+strlen(amt));
	strcpy(amount,"0");
	strcat(amount,amt);
    strcpy(withdraw,"withdraw ");
    strcat(withdraw,atm->user);
    strcat(withdraw," ");
    strcat(withdraw,amount);

    atm_send(atm, withdraw, strlen(withdraw));
    n = atm_recv(atm,recvline,10000);
    recvline[n]=0;
    if(strcmp(recvline,"0") ==0){
    	printf("Insufficient funds\n");
    } else {
    	printf("$%s dispensed\n",recvline);
    }
    memset(recvline, '\0', 10000);
    free(withdraw);
}

void balance(ATM *atm){
  	char recvline[10000];
  	int n =0;
  	if(!atm->logged){
  	  printf("No user logged in\n");
  	  return;
  	}
  	char *balance = malloc(strlen("balance ")+strlen(atm->user));
 	strcpy(balance,"balance ");
 	strcat(balance,atm->user);
	printf("%s : %s",atm->user,balance);

  	atm_send(atm, balance, strlen(balance));
 	n = atm_recv(atm,recvline,10000);
  	recvline[n]=0;
  	printf("$%s\n",recvline);
	memset(recvline, '\0', 10000);
  	free(balance);
}

void end_session(ATM* atm){
    if(!atm->logged){
        printf("No user logged in\n");
    } else {
        printf("User logged out\n");
        atm->logged = false;
		memset(atm->user,'\0',251);
    }
}
