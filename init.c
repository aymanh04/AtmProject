#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <math.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/evp.h>


/*int gen_keypairs(FILE *bankFile, FILE *atmFile) {
	RSA *rsa;
	BIGNUM *bn;
	BIO *bioPriv, *bioPub;
	const EVP_CIPHER *cipher;
	FILE *privKeyFile, *pubKeyFile;
	int keylen, i, tmp;
	char *pemKey;
	int bits = 2048;

	char *line;
	size_t len = 0;
	ssize_t lines;
	char *pubKeyStr = malloc(4096 * sizeof(char));
	char *privKeyStr = malloc(4096 * sizeof(char));


	privKeyFile = fopen("private.pem", "r");
	pubKeyFile = fopen("public.pem", "r");
	
	while ((lines = getline(&line, &len, pubKeyFile)) != -1){
		strcat(pubKeyStr, line);
	}

	while ((lines = getline(&line, &len, privKeyFile)) != -1){
		strcat(privKeyStr, line);
	}
	
	fputs("Public Key Size: ", bankFile);
	fputs("\n", bankFile);
	fputs("Public: ", bankFile);
	fputs(pubKeyStr, bankFile);
	fputs("\n", bankFile);
	fputs("\n", bankFile);

	fputs("Private Key Size: ", bankFile);
	fputs("\n", bankFile);
	fputs("Private: ", bankFile);
	fputs(privKeyStr, bankFile);
	fputs("\n", bankFile);
	
	fclose(pubKeyFile);
	fclose(privKeyFile);
	return 1;
}*/
/* 
Generates pairs of public/private keys for both the bank and atm. 


*/
int gen_keypairs(FILE *bankFile, FILE *atmFile) {
	RSA *rsa;
	BIGNUM *bn;
	BIO *bioPriv, *bioPub;
	const EVP_CIPHER *cipher;
	FILE *fp;
	FILE *privKeyFile, *pubKeyFile;
	int keylen, i, tmp;
	char *pemKey;
	int bits = 2048;

	char *line;
	size_t len = 0;
	ssize_t lines;
	
	cipher = EVP_aes_256_cbc();
	// Creating public/private key pairs for both the bank and atm.
	for (i = 0; i < 2; i++) {
		EVP_PKEY *pPriv;
		EVP_PKEY *pPub;
		RSA *rsaKey;

		char *pubKeyStr = malloc(4096 * sizeof(char));
		char *privKeyStr = malloc(4096 * sizeof(char));

		bn = BN_new();
		if (BN_set_word(bn, RSA_F4) != 1) {
			BIO_free_all(bioPriv);
			BIO_free_all(bioPub);
			BN_free(bn);
			RSA_free(rsa);
			// Add atm failure message...
			
			return -1;
		}

		// Generating the RSA struct.
		rsa = RSA_new();
		printf("Making RSA struct\n");
		if (RSA_generate_key_ex(rsa, bits, bn, NULL) != 1) {
			BIO_free_all(bioPriv);
			BIO_free_all(bioPub);
			BN_free(bn);
			RSA_free(rsa);
			// Add atm failure message...
			
			return -1;
		}

		// Creating the public key file and generating the public key.
		printf("Making pubkey file\n");
		bioPub = BIO_new_file("public.pem", "w+");
		if (PEM_write_bio_RSAPublicKey(bioPub, rsa) != 1) {
			BIO_free_all(bioPriv);
			BIO_free_all(bioPub);
			BN_free(bn);
			RSA_free(rsa);
			// Add atm failure message...

			return -1;
		}
		BIO_free_all(bioPub);

		pubKeyFile = fopen("public.pem", "r");

		while ((lines = getline(&line, &len, pubKeyFile)) != -1){
			strcat(pubKeyStr, line);
		}

		fclose(pubKeyFile);
		printf("Writing public key to file.\n");
		if (i == 0) {
			// Writing to init.bank
			fputs("Public Key Size: ", bankFile);
			fputs("\n", bankFile);
			fputs("Public: ", bankFile);
			fputs(pubKeyStr, bankFile);
			fputs("\n\n", bankFile);
		} else {
			// Writing to init.atm
			fputs("Public Key Size: ", atmFile);
			fputs("\n", atmFile);
			fputs("Public: ", atmFile);
			fputs(pubKeyStr, atmFile);
			fputs("\n\n", atmFile);
		}

		// Creating the private key file and generating the private key. 
		printf("Creating private key file  & key.\n");
		bioPriv = BIO_new_file("private.pem", "w+");
		if (PEM_write_bio_RSAPrivateKey(bioPriv, rsa, NULL, NULL, 0, NULL, NULL) != 1) {
			BIO_free_all(bioPriv);
			BIO_free_all(bioPub);
			BN_free(bn);
			RSA_free(rsa);
			// Add atm failure message...

			return -1;
		}
		BIO_free_all(bioPriv);

		privKeyFile = fopen("private.pem", "r");

		while ((lines = getline(&line, &len, privKeyFile)) != -1){
			strcat(privKeyStr, line);
		}

		fclose(privKeyFile);
		printf("Writing private key to file.\n");
		if (i == 0) {
			// Writing to init.bank
			fputs("Private Key Size: ", bankFile);
			fputs("\n", bankFile);
			fputs("Private: ", bankFile);
			fputs(privKeyStr, bankFile);
			fputs("\n", bankFile);
		} else {
			// Writing to init.atm
			fputs("Private Key Size: ", atmFile);
			fputs("\n", atmFile);
			fputs("Private: ", atmFile);
			fputs(privKeyStr, atmFile);
			fputs("\n", atmFile);
		}

		// Freeing all structures/memory used. 
		BN_free(bn);
		RSA_free(rsa);
		free(pubKeyStr);
		free(privKeyStr);
	}

	// Do rest of init down here...
	return 1;
}


int getKeyLen(int whichKey, int whichSys, char *path) {
	FILE *fp;
	char *sysPath;
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
		sysPath = malloc(strlen(path) + 6); // + ".bank" & NULL
		strncpy(sysPath, path, strlen(path));
		fp = fopen(sysPath, "r");
	} else if (whichSys == 1) {
		// 1 -> ATM
		sysPath = malloc(strlen(path) + 5); // + ".atm" & NULL
		strncpy(sysPath, path, strlen(path));
		fp = fopen(sysPath, "r");
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
char* getKey(int whichKey, int whichSys, char *path, int keylen) {
	FILE *fp;
	char *sysPath;
	char *type;
	char *strptr;
	char chr;
	//char num[6];
	char line[512];
	char *key = malloc(2048);
	//size_t len;
	int i = 0, cnt = 0;
	regex_t regex;

	if (!key)
		return NULL;

	// Determining if its the bank or ATM.
	if (whichSys == 0) {
		// 0 -> Bank
		sysPath = malloc(strlen(path) + 6); // + ".bank" & NULL
		strncpy(sysPath, path, strlen(path));
		strcat(sysPath, ".bank\0");
		fp = fopen(sysPath, "r");
	} else if (whichSys == 1) {
		// 1 -> ATM
		sysPath = malloc(strlen(path) + 5); // + ".atm" & NULL
		strncpy(sysPath, path, strlen(path));
		strcat(sysPath, ".atm\0");
		fp = fopen(sysPath, "r");
	} else {
		return NULL;
	}

	// Determining if its the public or private key.
	if (whichKey == 0) {
		// 0 -> Public
		type = "Public:";
		if (regcomp(&regex, "^Public:*\n$", 0)) {
			// TODO: Error
		}
	} else if (whichKey == 1) {
		// 1 -> Private
		type = "Private:";
		if (regcomp(&regex, "^Private:*\n$", 0)) {
			// TODO: Error
		}
	}

	while (fgets(line, sizeof(line), fp)) {
		// Getting the starting position of the key and copying characters.
		if ((cnt == 0) && (strptr = strstr(line, type)) != NULL) {
			// Getting the digits of the length.
			for (i = 0; i < (sizeof(line) - sizeof(type)) && i < keylen; i++) {
				// 8 is the index after the type string.
				if (whichKey == 0) 
					chr = strptr[8 + i];
				else // 9 for Private Key
					chr = strptr[9 + i];
				
				key[i] = chr;
				cnt++;
			}
		} else {
			// Getting the rest of the key if it was too long to 
			for (i = 0; i < sizeof(line); i++) {
				if (cnt < keylen) {
					key[cnt] = line[i];
					cnt++;
				}
			}
		}
	}
	key[cnt] = '\0';
	return key;
}


int main(int argc, char *argv[]) {
	FILE *bankFile;
	FILE *atmFile;
	char *filename = argv[1];
	char bankName[256];
	char atmName[255];
	// Only a single argument is allowed for this function.
	if (argc != 2) {
		printf("Usage: init <filename>\n");
		return 62;
	}

	strncpy(bankName, filename, 250);
	strncpy(atmName, filename, 250);
	strncat(bankName, ".bank", 5);
	strncat(atmName, ".atm", 4);
	bankFile = fopen(bankName, "r+");
	atmFile = fopen(atmName, "r+");
	// If either of the files exists, return.
	if (bankFile || atmFile) {
		printf("Error: one of the files already exists\n");
		return 63;
	}

	bankFile = fopen(bankName, "w+");
	atmFile = fopen(atmName, "w+");
	// Checking if any issues occured while opening.
	if (!bankFile || !atmFile) {
		printf("Error creating initialization files\n");
		return 64;
	}

	// Generating keys - if anything goes wrong, return.
	if (!gen_keypairs(bankFile, atmFile)) {
		printf("Error creating initialization files\n");
		return 64;
	}
	
	fclose(bankFile);
	fclose(atmFile);
	printf("Successfully initialized bank state\n");
	return 0;
}
