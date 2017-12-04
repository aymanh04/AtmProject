static bool logged = false;
static char *user = "";
char* deblank(char* input);
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
        withdraw(cmds[1], atm);
    } else if (strcmp(cmds[0], "balance") == 0) {
        if (strcmp(cmds[1], "\0") != 0) {
    	    printf("Usage: balance\n");
        } else {
	    balance(atm);
        }
    } else if (strcmp(cmds[0], "end-session") == 0) {
        if (strcmp(cmds[1], "\0") != 0) {
    	    printf("Usage: end-session\n");

        } else {
	    end_session();
        }
    } else {
        printf("Invalid command\n");
    }

    for (i = 0; i < 2; i++) {
        free(cmds[i]);
    }
}

void begin_session(char *data, ATM *atm){
    char *namePattern = "^[a-zA-Z]{1,250}$";
    char *pinPattern = "^[0-9][0-9][0-9][0-9]$";
    char recvline[10000];
    int n;
    FILE *fp;

    if(logged){
      printf("A user is already logged in\n");
      return;
    }
    if(!reg_matches(data, namePattern)){
      printf("Usage: begin-session <user-name>\n");
      return;
    }
    char *isuser = malloc(strlen(data)+strlen("isUser "));
    strcpy(isuser,"isUser ");
    strcat(isuser,data);

    //atm_send(atm, isuser, strlen(isuser));
    //n = atm_recv(atm,recvline,10000);
    //recvline[n] =0;
    //if(!recvline){
    //  printf("No such user");
    //  return;
    //}

    char *name = malloc(strlen(data)+strlen(".card"));
    strcpy(name,data);
    strcat(name,".card");

    fp = fopen(name, "r");
    char *line = malloc(5);
    memset(line, '\0', 5);
    char pin[5];
    
    if(!fp){
        printf("Unable to access %s’s card\n",data);
        return;
    } else {
      fgets(line,5,fp);
      printf("PIN? ");
      fgets(pin, 5,stdin);
      //sscanf("%s", pin);
      printf("%s\n",line);
      printf("%s\n",pin);
      if(!reg_matches(pin, pinPattern)) {
        printf("Not authorized pattern\n");
	if(!strcmp(pin,line)){
printf("Not authorized str\n");
}
      } else {
        printf("Authorized\n");
        logged = true;
        user = data;
      }
    }
    free(line);
    free(name);
    fclose(fp);
}

void withdraw(char *amt, ATM *atm){
    char *amtPattern = "^[0-9]+$";
    char recvline[10000];
    int num = atoi(amt);
    int n = 0;
    if(!logged){
      printf("No user logged in\n");
      return;
    }
    if(num < 0 || !reg_matches(amt,amtPattern)){
      printf("Usage: withdraw <amt>\n");
      return;
    }
    char *withdraw = malloc(strlen("withdraw ")+strlen(user)+strlen(amt)+1);
    strcpy(withdraw,"withdraw ");
    strcat(withdraw,user);
    strcat(withdraw," ");
    strcat(withdraw,amt);

    //atm_send(atm, withdraw, strlen(withdraw));
    //n = atm_recv(atm,recvline,10000);
    //recvline[n]=0;
    //if(!recvline){
    //  printf("Insufficient funds\n");
    //} else {
    //  printf("$%s dispensed\n",recvline);
    //}
    free(withdraw);
}

void balance(ATM *atm){
  char recvline[10000];
  int n =0;
  if(!logged){
    printf("No user logged in\n");
    return;
  }
  char *balance = malloc(strlen("balance ")+strlen(user));
  strcpy(balance,"balance ");
  strcat(balance,user);

  //atm_send(atm, balance, strlen(balance));
  //n = atm_recv(atm,recvline,10000);
  //recvline[n]=0;
  //if(!recvline){
  //  printf("$%s\n",recvline);
  //}
  free(balance);
}

void end_session(){
    if(!logged){
        printf("No user logged in\n");
    } else {
        printf("User logged out\n");
        logged = false;
        user = "";
    }
}
