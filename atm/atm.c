#include "atm.h"
#include "ports.h"
#include "util/util_functions.c"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <regex.h>

static bool loggedin = false;
static char *user = "";

ATM* atm_create() {
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
    // TODO set up more, as needed

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
    cmds[0] = malloc(251);
    cmds[1] = malloc(251);

    if (bank_split_line(cmds, command) == -1) {
  		// The command is create-user.
  		if (strcmp(cmds[0], "begin-session") == 0) {
        fputs("Usage: begin-session <user-name>",stdout);
  		} else if (strcmp(cmds[0], "withdraw") == 0) {
        fputs("Usage: withdraw <amt>\n",stdout);
  		} else if (strcmp(cmds[0], "balance") == 0) {
        fputs("Usage: balance\n",stdout);
  		} else {
        fputs("Invalid command\n",stdout);
  		}
  		return;
  	}
    if (strcmp(cmds[0], "begin-session") == 0) {
  		begin_session(cmds[1], atm);
  	} else if (strcmp(cmds[0], "withdraw") == 0) {
  		withdraw(cmds[1], atm);
  	} else if (strcmp(cmds[0], "balance") == 0) {
  		balance(atm);
  	} else {
  		printf("Invalid command\n");
      return;
  	}
  	free(cmds[0]);
    free(cmds[1]);
}

int begin_session(char *data, ATM *atm){
    char *namePattern = "^[a-zA-Z]{1,250} [a-zA-Z]{1,250} [a-zA-Z]{1,250}$";
    char *pinPattern = "[0-9][0-9][0-9][0-9]";
    char recvline[10000];
    int n;
    FILE *fp;

    if(logged){
      fputs("A user is already logged in",stdout);
      return 0;
    }
    if(!reg_matches(data, namePattern)){
      fputs("Usage: begin-session <user-name>",stdout);
      return 0;
    }
    atm_send(atm, strcat("isUser ",data), strlen(command));
    n = atm_recv(atm,recvline,10000);
    if(!n){
      fputs("No such user",stdout);
      return 0;
    }

    char *name = data;
    strcat(name,".card");
    fp = fopen(name, "r");
    if(!fp){

    }
    return 1;
}

int withdraw(char *amt, ATM *atm){
  char *amtPattern = "^[0-9]+$";
  int num = atoi(amt);
  int n = 0;
  if(!logged){
    fputs("No user logged in",stdout);
    return 0;
  }
  if(num < 0 || ! reg_matches(amt,amtPattern)){
    fputs("Usage: withdraw <amt>",stdout);
    return 0;
  }
  atm_send(atm, strcat("withdraw ",amt), strlen(command));
  n = atm_recv(atm,recvline,10000);
  if(!n){
    fputs("Insufficient funds",stdout);
    return 0;
  } else {
    fputs(amt,stdout);
    fputs(" dispensed",stdout);
    return 0;
  }
  return 1;
}
/*
  char recvline[10000];
  int n;

  atm_send(atm, command, strlen(command));
  n = atm_recv(atm,recvline,10000);
  recvline[n]=0;
  fputs(recvline,stdout);
*/
int balance(ATM *atm){
  char *amtPattern = "[0-9]+";
  int num = atoi(amt);
  if(!logged){
    fputs("No user logged in",stdout);
    return 0;
  }
  if(num < 0 || ! reg_matches(amt,amtPattern)){
    fputs("Usage: balance",stdout);
    return 0;
  }
  return 1;
}

int end_session(){
  if(!logged){
    fputs("No user logged in",stdout);
    return 0;
  }
  logged = false;
  user = "";
  return 1;
}
bool reg_matches(const char *str, const char *pattern) {
    regex_t re;
    int ret;

    if (regcomp(&re, pattern, REG_EXTENDED) != 0)
        return false;

    ret = regexec(&re, str, (size_t) 0, NULL, 0);
    regfree(&re);

    if (ret == 0)
        return true;

    return false;
}
