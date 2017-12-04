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
	RSA *rsaBank, *rsaAtm;
	BIGNUM *bnBank, *bnAtm;
	BIO *bioPrivBank, *bioPubBank, *bioPrivAtm, *bioPubAtm;
	FILE *privKeyFile, *pubKeyFile;
	//int i;
	int bits = 2048;

	char *line;
	size_t len = 0;
	ssize_t lines;
	
	// Creating public/private key pairs for both the bank and atm.

// Creating Bank keys.
	char *pubKeyStrBank = malloc(4096 * sizeof(char));
	char *privKeyStrBank = malloc(4096 * sizeof(char));

	char *pubKeyStrAtm = malloc(4096 * sizeof(char));
	char *privKeyStrAtm = malloc(4096 * sizeof(char));
	
	bnBank = BN_new();
	if (BN_set_word(bnBank, RSA_F4) != 1) {
		BN_free(bnBank);
		return -1;
	}

	// Generating the RSA struct.
	rsaBank = RSA_new();
	if (RSA_generate_key_ex(rsaBank, bits, bnBank, NULL) != 1) {
		BN_free(bnBank);
		RSA_free(rsaBank);
		return -1;
	}

	// Creating the public key file and generating the public key.
	bioPubBank = BIO_new_file("publicBank.pem", "w+");
	if (PEM_write_bio_RSAPublicKey(bioPubBank, rsaBank) != 1) {
		BIO_free_all(bioPubBank);
		BN_free(bnBank);
		RSA_free(rsaBank);
		return -1;
	}
	BIO_free_all(bioPubBank); // Done with BIO file.

	pubKeyFile = fopen("publicBank.pem", "r");
	
	// Copying the key from the .pem file into memory.
	while ((lines = getline(&line, &len, pubKeyFile)) != -1){
		strcat(pubKeyStrBank, line);
	}
	fclose(pubKeyFile);
	
	// Writing to init.atm
	fputs("Public Key Size: ", atmFile);
	fprintf(atmFile, "%d\n", (int)strlen(pubKeyStrBank));
	fputs("Public: ", atmFile);
	fputs(pubKeyStrBank, atmFile);
	fputs("\n\n", atmFile);
		
// Creating ATM pubKey
	bnAtm = BN_new();
	if (BN_set_word(bnAtm, RSA_F4) != 1) {
		BN_free(bnAtm);
		return -1;
	}

	// Generating the RSA struct.
	rsaAtm = RSA_new();
	if (RSA_generate_key_ex(rsaAtm, bits, bnAtm, NULL) != 1) {
		BN_free(bnAtm);
		RSA_free(rsaAtm);
		return -1;
	}

	// Creating the public key file and generating the public key.
	bioPubAtm = BIO_new_file("publicAtm.pem", "w+");
	if (PEM_write_bio_RSAPublicKey(bioPubAtm, rsaAtm) != 1) {
		BIO_free_all(bioPubAtm);
		BN_free(bnAtm);
		RSA_free(rsaAtm);
		return -1;
	}
	BIO_free_all(bioPubAtm); // Done with BIO file.

	pubKeyFile = fopen("publicAtm.pem", "r");

	// Copying the key from the .pem file into memory.
	while ((lines = getline(&line, &len, pubKeyFile)) != -1){
		strcat(pubKeyStrAtm, line);
	}
	fclose(pubKeyFile);

	// Writing to init.bank
	fputs("Public Key Size: ", bankFile);
	fprintf(bankFile, "%d\n", (int)strlen(pubKeyStrAtm));
	fputs("Public: ", bankFile);
	fputs(pubKeyStrAtm, bankFile);
	fputs("\n\n", bankFile);


	// Creating the private key file and generating the private key. 
	bioPrivBank = BIO_new_file("privateBank.pem", "w+");
	if (PEM_write_bio_RSAPrivateKey(bioPrivBank, rsaBank, NULL, NULL, 0, NULL, NULL) != 1) {
		BIO_free_all(bioPrivBank);
		BN_free(bnBank);
		RSA_free(rsaBank);
		return -1;
	}
	BIO_free_all(bioPrivBank);

	privKeyFile = fopen("privateBank.pem", "r");

	// Copying the key from the .pem file into memory.
	while ((lines = getline(&line, &len, privKeyFile)) != -1){
		strcat(privKeyStrBank, line);
	}

	fclose(privKeyFile);
	// Writing to init.bank
	fputs("Private Key Size: ", bankFile);
	fprintf(bankFile, "%d\n", (int)strlen(privKeyStrBank));
	fputs("Private: ", bankFile);
	fputs(privKeyStrBank, bankFile);
	fputs("\n", bankFile);

	// Freeing all structures/memory used. 
	BN_free(bnBank);
	RSA_free(rsaBank);
	free(pubKeyStrBank);
	free(privKeyStrBank);

// Creating ATM keys.

	// Creating the private key file and generating the private key. 
	bioPrivAtm = BIO_new_file("privateAtm.pem", "w+");
	if (PEM_write_bio_RSAPrivateKey(bioPrivAtm, rsaAtm, NULL, NULL, 0, NULL, NULL) != 1) {
		BIO_free_all(bioPrivAtm);
		BN_free(bnAtm);
		RSA_free(rsaAtm);
		// Add atm failure message...

		return -1;
	}
	BIO_free_all(bioPrivAtm);

	privKeyFile = fopen("privateAtm.pem", "r");

	// Copying the key from the .pem file into memory.
	while ((lines = getline(&line, &len, privKeyFile)) != -1){
		strcat(privKeyStrAtm, line);
	}

	fclose(privKeyFile);
	// Writing to init.atm
	fputs("Private Key Size: ", atmFile);
	fprintf(atmFile, "%d\n", (int)strlen(privKeyStrAtm));
	fputs("Private: ", atmFile);
	fputs(privKeyStrAtm, atmFile);
	fputs("\n", atmFile);

	// Freeing all structures/memory used. 
	BN_free(bnAtm);
	RSA_free(rsaAtm);
	free(pubKeyStrAtm);
	free(privKeyStrAtm);

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
		printf("Usage:\tinit <filename>\n");
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
		printf("Error:\tone of the files already exists\n");
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

	// Cleaning up the .pem files so attackers don't find the keys.
	system("rm privateBank.pem");
	system("rm privateAtm.pem");
	system("rm publicBank.pem");
	system("rm publicAtm.pem");

	printf("Successfully initialized bank state\n");
	return 0;
}
