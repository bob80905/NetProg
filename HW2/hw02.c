#include <stdbool.h> 
#include <stdlib.h> 
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include "unp.h"

int MAXIMUMLINE = 1024;

//information on a word and letter frequency
typedef struct {
	int freq;
	char letter;
}letter_info;

//user information
typedef struct{
	int connection_fd;
	char* username;
	int usernameLength;
}user;

//true if a character is not within the letterinfo struct secret_word_info
bool notWithin(char c, letter_info* secret_word_info, int total_length){
	for(int i = 0; i < total_length; i++){
		if(secret_word_info[i].letter == c){
			return false;
		}
	}
	return true;
}

//adds a letter to the letter_info struct arry, c
void addLetter(char c, letter_info** secret_word_info, int total_length){
	letter_info* temp = calloc(total_length+1, sizeof(letter_info));
	//copy data over
	for(int i = 0; i < total_length; i++){
		temp[i] = (*secret_word_info)[i];
	}
	//add letter to the end.
	letter_info t;
	t.freq = 1; t.letter = c;
	temp[total_length] = t;
	free(*secret_word_info);
	*secret_word_info = temp;
}

//increments the frequency of letter_info with character c
void incrementFreq(char c, letter_info** secret_word_info, int total_length){
	for(int i = 0; i < total_length; i++){
		if(c == (*secret_word_info)[i].letter){
			(*secret_word_info)[i].freq += 1;
			return;
		}
	}
	perror("ERROR: could not find letter to increment frequency\n");
	return;
}

//n is length of secret word, total_length is length of produced letter_info array
//return letter info struct array given word secret
letter_info* loadLetterInfo(char* secret, int n, int* total_length){
	letter_info* secret_word_info = calloc(1, sizeof(letter_info));
	letter_info base;
	base.freq = 1;
	base.letter = '\n';
	secret_word_info[0] = base;

	*total_length = 1;
	//load data on secret word
	for(int i = 0; i < n; i++){
		char c = secret[i];
		//if the character we are reading is a new char, then add to letter_info array
		if(notWithin(c, secret_word_info, *total_length)){
			addLetter(c, &secret_word_info, *total_length);
			*total_length += 1;
		}
		else{
			//otherwise, find where the letter is in secret_word_info,
			//and increment associated freq
			incrementFreq(c, &secret_word_info, *total_length);
		}
	}

	return secret_word_info;
}


//guess is the user's word guess, secret is the actual word
//Function returns via reference,
//correct will be set to number of letters correct, same with correctly_placed
//n is the length of the secret word
//This function assumes that guess length and secret length are equal
void getCorrectAndCorrectlyPlaced(char* guess, char* secret, int guess_length, int* correct, int* correctly_placed){
	int* info_length = calloc(1, sizeof(int));
	letter_info* info = loadLetterInfo(secret, guess_length, info_length);
	int num_correct = 0;
	int num_correctly_placed = 0;
	for (int i = 0; i < guess_length; i++){
		char c = guess[i];

		//for each letter, try to find it within our loaded letter_info data
		//if not found, do nothing, otherwise, increment num_correct
		for(int j = 0; j < *info_length; j++){
			//only count a guessed letter as "correct"
			//if it accounts for a distinct identical letter in the secret word
			if(toupper(info[j].letter) == c && info[j].freq > 0){
				info[j].freq -= 1;
				num_correct += 1;
				break;
			}
		}
	}

	//now compute num_correctly_placed
	for(int i = 0; i < guess_length; i++){
		char c = guess[i];
		if (c == toupper(secret[i])){
			num_correctly_placed += 1;
		}
	}

	*correct = num_correct;
	*correctly_placed = num_correctly_placed;
	free(info);
	free(info_length);
	return;
}

//make an entire string upper case
void uppercaseify(char word[], char** newWord, int length){
	*newWord = calloc(length, sizeof(char));
	for(int i = 0; i < length; i++){
		char upper = (char) toupper((int)(word)[i]);
		(*newWord)[i] = upper;
	}
	return;
}

//process the entire dictionary file and produce a 2D char array for each word in the dictionary
void loadDictionary(char*** allWords, FILE* wordFile, int* dictLength){
	int allWordsSize = 0;
	while(1){

		char* word = calloc(MAXIMUMLINE, sizeof(char)); //all words will be loaded into the dictionary
		int result = fscanf(wordFile, "%s", word);

		if(result == EOF){
			break; //done reading the file
		}
		allWordsSize += 1;
		if(allWordsSize == 1){
			(*allWords) = calloc(1, sizeof(char*));
			(*allWords)[0] = word;
		}
		else{
			(*allWords) = realloc(*allWords, allWordsSize*sizeof(char*));
			(*allWords)[allWordsSize-1] = word;	
		}
	}
	*dictLength = allWordsSize;

}

//return the number of digits inside num
int numDigits(int num){
	double numCopy = (double)num;
	int ret = 1;
	while(numCopy >= 10.0){
		numCopy = numCopy / 10.0;
		ret += 1;
	}
	return ret;
}

int main(int argc, char* argv[]){

	if(argc != 5){
		perror("Expected 4 arguments: [seed] [port] [dictionary_file] [longest_word_length]\n");
		exit(1);
	}
	//load user args
	int seed = atoi(argv[1]);
	int PORT = atoi(argv[2]);
	char* file = argv[3];
	int longest_length = atoi(argv[4]);

	longest_length = longest_length < MAXIMUMLINE ? longest_length : MAXIMUMLINE;

	//Load dictionary contents
	char** allWords;

	FILE* wordFile = fopen(file, "r");
	if(wordFile == NULL){
      printf("ERROR: Could not open %s\n", file);   
      exit(1);             
   	}

   	int dictLength = 0;
	
	//load the dictionary
   	loadDictionary(&allWords, wordFile, &dictLength);

   	srand(seed);
   	int random_num = rand() % dictLength;
   	char* secret_word = allWords[random_num];
   	int secret_word_length = strlen(secret_word);

	// start a server, make listener socket
	int listenfd, connfd;
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;

	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	//clear sockaddr_in struct, and set it
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT); 

	Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

	Listen(listenfd, 1);

	user users[5];
	int number_connected = 0; //number of users connected
	int maxfd = listenfd; 
	//clear all user entries at first
	for(int i = 0; i < 5; i++){
		users[i].connection_fd = -1; //-1 means this slot is empty
		users[i].username = NULL;
	}



	while(1){
		
		fd_set readfds;
		FD_ZERO( &readfds );
		FD_SET(listenfd, &readfds);
		maxfd = listenfd;
		for(int i = 0; i < 5; i++){
			if(users[i].connection_fd != -1){
				FD_SET(users[i].connection_fd, &readfds);
				maxfd = maxfd > users[i].connection_fd ? maxfd : users[i].connection_fd;
				//reset maxfd if necessary
			}
		}
		
		//listen to listening socket and all other clients
		int select_result = select( maxfd+1, &readfds, NULL, NULL, NULL);
		
		clilen = sizeof(cliaddr);
		if(select_result == -1){
			perror("ERROR: Select failed\n");
		}
		//if a new client wants to connect
		if(FD_ISSET(listenfd, &readfds)){
			if(number_connected == 5){
				continue; //max client capacity is 5, can't add any more.
			}
			connfd = Accept(listenfd, (SA *) &cliaddr, &clilen);
			Send(connfd, "Welcome to Guess the Word, please enter your username.\n", 55 , 0);
			
			//find first available empty slot, add client
			for(int i = 0; i < 5; i++){
				if(users[i].connection_fd == -1){
					users[i].connection_fd = connfd;
					users[i].username = NULL;
					break;
				}
			}

			number_connected += 1; //new client, more clients connected
			
		}
		//check to see if any other connected user's fds are set
		for(int i = 0; i < 5; i++){
			if(users[i].connection_fd == -1){
				continue;
			}
			//if a client has sent something to us
			if(FD_ISSET(users[i].connection_fd, &readfds)){
				char* recvmsg = calloc(MAXIMUMLINE, sizeof(char));
				int recvbytes = recv(users[i].connection_fd, recvmsg, MAXIMUMLINE, 0);
				if(recvbytes < 0){
					perror("ERROR: recv failed.\n");
					exit(1);
				}
				//if the client disconnected
				if(recvbytes == 0){
					//handle when a client disconnects from game
					close(users[i].connection_fd);
					number_connected -= 1;
					users[i].connection_fd = -1;
					users[i].username = NULL;
					users[i].usernameLength = 0;

					continue;
				}
				recvbytes-=1;
				recvmsg[recvbytes] = '\0';
				//if the user already has an associated username,
				//then this is a guess we've just received
				if(users[i].username){
					char* capitalGuess = NULL;
					uppercaseify(recvmsg, &capitalGuess, recvbytes );
					
					if(recvbytes != secret_word_length){
						int secret_word_length_length = numDigits(secret_word_length);
						
						int msglen = 41 + secret_word_length_length + 11;
						char response[msglen];
						
						sprintf(response, "Invalid guess length. The secret word is %d letter(s).", secret_word_length);
						Send(users[i].connection_fd, response, msglen, 0);
					}
					else{
						int correct;
						int correctly_placed;
						getCorrectAndCorrectlyPlaced(capitalGuess, secret_word, recvbytes, &correct, &correctly_placed);
						//if user guessed secret word correctly
						if(correctly_placed == secret_word_length){
							int response_length = users[i].usernameLength + secret_word_length + 33;
							char response[response_length];
							sprintf(response, "%s has correctly guessed the word %s\n", users[i].username, secret_word);
							
							//the announcement should be sent to all active users
							//close all connections to active users and clean up
							for(int j= 0; j < 5; j++){
								if(users[j].connection_fd != -1 && j != i){
									Send(users[j].connection_fd, response, response_length, 0);
									close(users[j].connection_fd);
									free(users[j].username);
									users[j].username = NULL;
									users[j].connection_fd = -1;
									users[j].usernameLength = 0;
								}
							}
							Send(users[i].connection_fd, response, response_length, 0);
							close(users[i].connection_fd);
							free(users[i].username);
							users[i].username = NULL;
							users[i].connection_fd = -1;
							users[i].usernameLength = 0;
							
							number_connected = 0; //no more users connected

							//continuously select a new random word until its less than the max limit
							random_num = rand() % dictLength;
							secret_word = allWords[random_num];
							secret_word_length = strlen(secret_word);
							int m = (MAXIMUMLINE < longest_length) ? MAXIMUMLINE : longest_length;
							while(secret_word_length > m){
								random_num = rand() & dictLength;
								secret_word = allWords[random_num];
								secret_word_length = strlen(secret_word);
							}

							break; //go back to select and listen
							
						}
						//if the guess was incorrect
						else{
							int numCorrectLength = numDigits(correct);
							int numCorrectlyPlacedLength = numDigits(correctly_placed);

							recvmsg[recvbytes] = '\0'; //replace newline with null terminating char
							int sendlen = 73 + numCorrectLength + numCorrectlyPlacedLength + recvbytes + users[i].usernameLength;
							char response[sendlen];
							
							sprintf(
							response, "%s guessed %s: %d letter(s) were correct and %d letter(s) were correctly placed.\n",
							users[i].username, recvmsg, correct, correctly_placed);


							//send numcorrect and correctly placed to all users.
							for(int j= 0; j < 5; j++){
								if(users[j].connection_fd != -1){
									
									Send(users[j].connection_fd, response, sendlen, 0);
								}
							}
						}
					}
				}
				//otherwise, its a username to be assigned
				else{
					//First check to see if username is taken
					char* temp_user = NULL;
					bool same = false; //assume at first that the username is not taken.
					uppercaseify(recvmsg, &temp_user, recvbytes);
					//search all other usernames to see if the username is taken.
					for(int j = 0; j < 5; j++){
						if(users[j].username){
							if(recvbytes == users[j].usernameLength){
								char* temp_user2 = NULL;
								uppercaseify(users[j].username, &temp_user2, users[j].usernameLength);
								int k = 0;
								for(; k < recvbytes; k++){
									if(temp_user[k] != temp_user2[k]){

										break;
									}
								}
								if(k == recvbytes){
									same = true;
									break;
								}
							}
						}
					}
					
					if(same){
						//username is taken, send an error msg
						int msgLength = 62 + recvbytes;
						char response[msgLength];
						char first[9] = "Username ";
						char rest[53] = " is already taken, please enter a different username\n";
						int j = 0;
						for(; j < 9; j++){
							response[j] = first[j];
						}
						for(; j < 9 + recvbytes; j++ ){
							response[j] = recvmsg[j-9];
						}
						for(; j < msgLength; j++){
							response[j] = rest[j-(9+recvbytes)];
						}
						Send(users[i].connection_fd, response, msgLength, 0);
					}
					else{
						//assign the username, store to correct client slot
						users[i].username = calloc(recvbytes+1, sizeof(char));
						users[i].usernameLength = recvbytes;
						for(int j = 0; j < recvbytes; j++){
							users[i].username[j] = recvmsg[j];
						}
						int secret_word_length_length = numDigits(secret_word_length);
						int msglen = 21 + recvbytes + 11 + 1 + 39 + secret_word_length_length + 11;
						char response[msglen];
						char numPlayers[2]; //include the terminating null
						sprintf(numPlayers, "%d", number_connected);
						numPlayers[1] = '\0';

						char strSecretWordLength[secret_word_length_length];
						sprintf(strSecretWordLength, "%d", secret_word_length);

						//send info message
						strncpy(response, "Let's start playing, ", 21);
						strncpy(&response[21], users[i].username, recvbytes );
						strncpy(&response[21 + recvbytes], "\nThere are ", 11);
						strncpy(&response[21 + recvbytes + 11], numPlayers, 1);
						strncpy(&response[21 + recvbytes + 11 + 1], " player(s) playing. The secret word is " , 39);
						strncpy(&response[21 + recvbytes + 11 + 1 + 39], strSecretWordLength, secret_word_length_length);
						strncpy(&response[21 + recvbytes + 11 + 1 + 39 + secret_word_length_length]," letter(s).", 11);
						Send(users[i].connection_fd, response, msglen, 0);
						
					}
					
				}
			}
			
		}	
	}	
} 