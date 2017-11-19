#include "user_data.h"

void create_data(UserData *user, int pin, int balance) {
	user->pin = pin;
	user->balance = balance;
}
