#include <stdlib.h>
#include <string.h>

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
