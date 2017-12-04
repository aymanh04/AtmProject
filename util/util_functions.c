#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <regex.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/evp.h>

/* 
	Splits a line containing a bank command and arguments. 
	Returns: 0 on success, -1 on error. 
*/
int bank_split_line(char *arr[], char *line) {
	int i = 0;
	char *str;
	char *tmp;
	while (((str = strsep(&line, " ")) != NULL) && (i < 4)) {
		// Storing the command name.
		if (i == 0) 
			tmp = str;

		if (strcmp(tmp, "create-user") != 0 && strcmp(tmp, "deposit") != 0 && strcmp(tmp, "balance") != 0) 
			return -1;
		
		// A valid username can be at most 250 characters.
		if (i == 1 && strlen(str) > 250)
			return -1;

		/*// A valid PIN can only be 4 characters long.
		if (i == 2 && strlen(str) != 4) {
			if (strcmp(tmp, "create-user") == 0)
				return -1;
		}*/

		strncpy(arr[i], str, 250 * sizeof(char));
		i++;
	}
	return 0;
}

int getKeyLen(int whichKey, int whichSys, char *path) {
	FILE *fp;
	char *type;
	char *strptr;
	char chr;
	char num[6];
	char line[256];
	//size_t len;
	int i = 0, ret = -1;


	// Determining if its the bank or ATM.
	if (whichSys == 0) {
		// 0 -> Bank
		//sysPath = malloc(strlen(path) + 6); // + ".bank" & NULL
		//strncpy(sysPath, path, strlen(path));
		fp = fopen(path, "r");
	} else if (whichSys == 1) {
		// 1 -> ATM
		//sysPath = malloc(strlen(path) + 5); // + ".atm" & NULL
		//strncpy(sysPath, path, strlen(path));
		fp = fopen(path, "r");
	} else {
		return -1;
	}

	// Determining if its the public or private key.
	if (whichKey == 0) {
		// 0 -> Public
		type = "Public Key Size:";
	} else if (whichKey == 1) {
		// 1 -> Private
		type = "Private Key Size:";
	}

	while (fgets(line, sizeof(line), fp)) {
		if ((strptr = strstr(line, type))) {
			// Getting the digits of the length.
			for (i = 0; i < 6; i++) {
				// 17 is the index after the type string.
				if (whichKey == 0) 
					chr = strptr[17 + i];
				else // 18 for Private Key
					chr = strptr[18 + i];
				
				if (chr >= '0' && chr <= '9') {
					num[i] = chr;
				} else {
					num[i] = '\0';
					break;
				}
			}
			
			break;
		}
	}
	fclose(fp);
	ret = atoi(num);
	return ret;
}

/* 
Gets a public or private key from either the .bank or .atm init file.

Params: whichKey 	- 0 or 1 for private or public key
		whichSys 	- 0 or 1 for bank or atm
		path 		- file path
		keylen 		- key length
*/
char* getKey(int whichKey, int whichSys, char *path, char *key, int keylen) {
	FILE *fp;
	//char *sysPath;
	char *type;
	char *strptr;
	//char num[6];
	char *line = malloc(512);
	//char *key = malloc(keylen);
	//size_t len;
	int cnt = 0;
	//regex_t regex;

	memset(line, '\0', 512);
	if (!key)
		return NULL;

	// Determining if its the bank or ATM.
	if (whichSys == 0) {
		// 0 -> Bank
		//sysPath = malloc(strlen(path) + 6); // + ".bank" & NULL
		//strncpy(sysPath, path, strlen(path));
		//strcat(sysPath, ".bank\0");
		fp = fopen(path, "r");
	} else if (whichSys == 1) {
		// 1 -> ATM
		//sysPath = malloc(strlen(path) + 5); // + ".atm" & NULL
		//strncpy(sysPath, path, strlen(path));
		//strcat(sysPath, ".atm\0");
		fp = fopen(path, "r");
	} else {
		return NULL;
	}

	// Determining if its the public or private key.
	if (whichKey == 0) {
		// 0 -> Public
		type = "Public:";
		/*if (regcomp(&regex, "^Public:*\n$", 0)) {
			// TODO: Error
		}*/
	} else if (whichKey == 1) {
		// 1 -> Private
		type = "Private:";
		/*if (regcomp(&regex, "^Private:*\n$", 0)) {
			// TODO: Error
		}*/
	}


	while (fgets(line, 512, fp)) {
		// Getting the starting position of the key and copying characters.
		//printf("line: %s\n", line);
		if ((strptr = strstr(line, type)) != NULL) {
			// Getting the digits of the length.	
			if (whichKey == 0) 
				strptr += 8;
			else // 9 for Private Key
				strptr += 9;
			//printf("%s\n", strptr);
			strncpy(key, strptr, strlen(line) - strlen(type));
			cnt += (int) (strlen(line) - strlen(type));
		} else {
			if (cnt > 0) {
				// Getting the rest of the key if it was too long to 
				if (cnt + (int)strlen(line) > keylen) {
					strncat(key, line, (keylen - cnt));
					cnt += keylen - cnt;
				} else {
					//printf("%s\n", line);
					strcat(key, line);
					cnt += strlen(line);
				}
			}
			
		}
		
	}
	fclose(fp);
	//free(line);
	key[cnt] = '\0';
	return key;
}

RSA* retrieveKey(int whichKey, int whichSys, char *fname) {
	int keylen;
	char *pubKey;

	keylen = getKeyLen(whichKey, whichSys, fname);
	pubKey = malloc(keylen + 1);
	memset(pubKey, '\0', keylen + 1);
	getKey(whichKey, whichSys, fname, pubKey, keylen);

	/*keylen = getKeyLen(1, 0, fname);
	printf("privkey keylen: %d\n", keylen);
	privKey = malloc(keylen + 1);
	memset(privKey, '\0', keylen + 1);
	getKey(1, 0, bank->fname, privKey, keylen);
	printf("Private Key: %s\n", privKey);
	//free(privKey);*/

	RSA *r = RSA_new();
	BIO *bioTemp = BIO_new_mem_buf((void*)pubKey, strlen(pubKey));	
	if (whichKey == 0) 
		PEM_read_bio_RSAPublicKey(bioTemp, &r, NULL, NULL);
	else 
		PEM_read_bio_RSAPrivateKey(bioTemp, &r, NULL, NULL);
	
	return r;
}


int encryptMsg(RSA *rsa, char *msg, char *enc) {
	int len = RSA_public_encrypt(strlen(msg)+1, (unsigned char*)msg,
		(unsigned char*)enc, rsa, RSA_PKCS1_OAEP_PADDING);

	return len;
}


void decryptMsg(RSA *rsa, char *enc, char *msg, int encLen) {
	RSA_private_decrypt(encLen, (unsigned char*)enc, (unsigned char*)msg,
                       rsa, RSA_PKCS1_OAEP_PADDING);
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
