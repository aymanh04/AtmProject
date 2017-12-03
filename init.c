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

/* 
Generates pairs of public/private keys for both the bank and atm. 


*/
int gen_keypairs(FILE *bankFile, FILE *atmFile) {
	RSA *rsa;
	BIGNUM *bn;
	BIO *bioPriv, *bioPub;
	FILE *privKeyFile, *pubKeyFile;
	int i;
	int bits = 2048;

	char *line;
	size_t len = 0;
	ssize_t lines;
	
	// Creating public/private key pairs for both the bank and atm.
	for (i = 0; i < 2; i++) {
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
		BIO_free_all(bioPub); // Done with BIO file.

		pubKeyFile = fopen("public.pem", "r");

		while ((lines = getline(&line, &len, pubKeyFile)) != -1){
			strcat(pubKeyStr, line);
		}

		fclose(pubKeyFile);
		printf("Writing public key to file.\n");
		if (i == 0) {
			// Writing to init.bank
			fputs("Public Key Size: ", bankFile);
			fprintf(bankFile, "%d\n", (int)strlen(pubKeyStr));
			//fputs("%d\n", (int)strlen(pubKeyStr));
			//fputs("\n", bankFile);
			fputs("Public: ", bankFile);
			fputs(pubKeyStr, bankFile);
			fputs("\n\n", bankFile);
		} else {
			// Writing to init.atm
			fputs("Public Key Size: ", atmFile);
			fprintf(atmFile, "%d\n", (int)strlen(pubKeyStr));
			//fputs("%d\n", (int)strlen(pubKeyStr));
			//fputs("\n", atmFile);
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
			fprintf(bankFile, "%d\n", (int)strlen(privKeyStr));
			//fputs("\n", bankFile);
			fputs("Private: ", bankFile);
			fputs(privKeyStr, bankFile);
			fputs("\n", bankFile);
		} else {
			// Writing to init.atm
			fputs("Private Key Size: ", atmFile);
			fprintf(atmFile, "%d\n", (int)strlen(privKeyStr));
			//fputs("\n", atmFile);
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
