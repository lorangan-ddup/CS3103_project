/**
* Counts the words in the string in a simple manner.
* The counting is done by looking for spaces and newline 
* characters until the string terminator is reached.
*
* @param text: the string to count the words in.
* 
* @returns the total number of words in the string.
*/
int wordCount(char *text);

/**
* Checks if the <input_file> is a txt file or not
* by looking for '.txt' at the end.
*
* @param fileName: the name of the file to be checked.
* 
* @returns 1 if file is a .txt file and returns 0 if not.
*/
int validateTextFile(char *fileName);

/**
* Returns the total size (length of the file), 
* meaning the total number of chars in the file.
*
* @param file: file which length of chars will be calculated.
* 
* @returns length of file in chars (long).
*/
long fileLength(FILE *file);

/**
 * Saves a result with an int type to a given textfile.
 * 
 * @param fileName: The textfile name to save the result.
 * Please just pass the textfile name so that it can be
 * located in the same directory as the executable.
 * 
 * @param result: The value with int type to be kept.
 */

void saveResult(char *fileName, int result);