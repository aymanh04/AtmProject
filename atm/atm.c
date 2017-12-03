#include "atm.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <regex.h>

static bool logged = false;
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
    for (i = 0; i < 2; i++) {
  		cmds[i] = malloc(251);
  		memset(cmds[i], '\0', 251);
  	}

    sscanf(command, "%s %s", cmds[0], cmds[1]);

  	if (strcmp(cmds[0], "begin-session") == 0) {
        begin_session(cmds[1], atm);
  		} else if (strcmp(cmds[0], "withdraw") == 0) {
        withdraw(cmds[1], atm)
  		} else if (strcmp(cmds[0], "balance") == 0) {
        if (strcmp(cmds[1], "\0") != 0) {
    			printf("Usage: balance\n");
    		} else {
    			balance(atm);
    		}
  		} else {
        fputs("Invalid command\n",stdout);
        return;
  		}
  	}

    for (i = 0; i < 2; i++) {
  		free(cmds[i]);
  	}
}

void begin_session(char *data, ATM *atm){
    char *namePattern = "^[a-zA-Z]{1,250}$";
    char *pinPattern = "^[0-9][0-9][0-9][0-9]$";
    char *pin;
    char recvline[10000];
    int n;
    FILE *fp;

    if(logged){
      printf("A user is already logged in");
      return;
    }
    if(!reg_matches(data, namePattern)){
      printf("Usage: begin-session <user-name>");
      return;
    }
    char *isuser = malloc(strlen(data)+strlen("isUser "));
    strcpy(name,"isUser ");
    strcat(name,data);

    atm_send(atm, isuser, strlen(command));
    n = atm_recv(atm,recvline,10000);
    recvline[n] =0;
    if(!recvline){
      printf("No such user");
      return;
    }

    char *name = malloc(strlen(data)+strlen(".card"));
    strcpy(name,data);
    strcat(name,".card");

    fp = fopen(name, "r");
    char line[1000];
    if(!fp){
        printf("Unable to access %s’s card",data);
        return;
    } else {
      fgets(line,sizeof(line),fp);
      printf("PIN?");
      scanf("%s", pin);
      if(!reg_matches(pin, pinPattern)){
        printf("Not authorized");
        return;
      } else {
        printf("Authorized");
        logged = true;
        user = data;
        return;
      }
    }
    fclose(fp);
    return;
}

void withdraw(char *amt, ATM *atm){
  char *amtPattern = "^[0-9]+$";
  int num = atoi(amt);
  int n = 0;
  if(!logged){
    fputs("No user logged in",stdout);
    return;
  }
  if(num < 0 || !reg_matches(amt,amtPattern)){
    printf("Usage: withdraw <amt>");
    return;
  }
  char *withdraw = malloc(strlen("withdraw ")+strlen(user)+strlen(amt)+1);
  strcpy(withdraw,"withdraw ");
  strcat(withdraw,user);
  strcat(withdraw," ");
  strcat(withdraw,amt);

  atm_send(atm, withdraw, strlen(command));
  n = atm_recv(atm,recvline,10000);
  recvline[n]=0;
  if(!recvline){
    fputs("Insufficient funds",stdout);
    return;
  } else {
    printf("$%s dispensed",recvline);
    return;
  }
}

void balance(ATM *atm){
  if(!logged){
    printf("No user logged in");
    return;
  }
  char *balance = malloc(strlen("balance ")+strlen(user));
  strcpy(balance,"balance ");
  strcat(balance,user);

  atm_send(atm, balance, strlen(command));
  n = atm_recv(atm,recvline,10000);
  recvline[n]=0;
  if(!recvline){
    return;
  } else {
    printf("$%s",recvline);
    return;
  }
}

void end_session(){
  if(!logged){
    fputs("No user logged in",stdout);
    return;
  }
  logged = false;
  user = "";
  return;
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
