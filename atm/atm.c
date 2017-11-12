#include "atm.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
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

	/*
	 * The following is a toy example that simply sends the
	 * user's command to the bank, receives a message from the
	 * bank, and then prints it to stdout.
	 */

	/*
    char recvline[10000];
    int n;

    atm_send(atm, command, strlen(command));
    n = atm_recv(atm,recvline,10000);
    recvline[n]=0;
    fputs(recvline,stdout);
	*/
}

int begin_session(char *data){
    char *namePattern = "[a-zA-Z]+";
    char *pinPattern = "[0-9][0-9][0-9][0-9]";
    FILE *fp;

    if(logged){
      fputs("A user is already logged in",stdout);
      return 0;
    }
    if(!reg_matches(data, namePattern)){
      fputs(strcat("Usage: begin-session ",data),stdout);
      return 0;
    }

    char *name = data;
    strcat(name,".card");
    fp = fopen(name, "r");
    if(!fp){

    }
    return 1;
}

int withdraw(char *amt){
  char *amtPattern = "[0-9]+";
  int num = atoi(amt);
  if(!logged){
    fputs("No user logged in",stdout);
    return 0;
  }
  if(num < 0 || ! reg_matches(amt,amtPattern)){
    fputs(strcat("Usage: begin-session ",data),stdout);
    return 0;
  }
  return 1;
}
int balance(){
  char *amtPattern = "[0-9]+";
  int num = atoi(amt);
  if(!logged){
    fputs("No user logged in",stdout);
    return 0;
  }
  if(num < 0 || ! reg_matches(amt,amtPattern)){
    fputs(strcat("Usage: begin-session ",data),stdout);
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
