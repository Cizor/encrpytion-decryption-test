#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <time.h> 
#include <crypt.h> 
#include <openssl/sha.h> 
#include <openssl/md5.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#if defined(__APPLE__)
#  define COMMON_DIGEST_FOR_OPENSSL
#  include <CommonCrypto/CommonDigest.h>
#  define SHA1 CC_SHA1
#else
#  include <openssl/md5.h>
#endif

#define MAX_DIR_PATH 2048	/* maximal full path we support.      */

char *cwd;	/* current working directory.        */
void findfile(char* pattern)
{
	DIR* dir;			/* pointer to the scanned directory. */
	struct dirent* entry;	/* pointer to one directory entry.   */
	struct stat dir_stat;       /* used by stat().                   */
	char *buf;
	cwd=(char *)malloc(sizeof(char)*(MAX_DIR_PATH+1));
	buf=(char*)malloc(sizeof(char)*100000);
	static int flag;

	/* first, save path of current working directory */
	if (!getcwd(cwd, MAX_DIR_PATH+1)) {
		perror("getcwd:");
		return;
	}


	/* open the directory for reading */
	dir = opendir(".");
	if (!dir) {
		fprintf(stderr, "Cannot read directory '%s': ", cwd);
		perror("");
		return;
	}

	/* scan the directory, traversing each sub-directory, and */
	/* matching the pattern for each file name.               */
	while ((entry = readdir(dir)) && flag == 0) {
		/* check if the pattern matchs. */
		if (entry->d_name && strstr(entry->d_name, pattern)) {
			flag = 1;
			printf("Found at %s\n",cwd);
			return;
		}
		/* check if the given entry is a directory. */
		if (stat(entry->d_name, &dir_stat) == -1) {
			perror("stat:");
			continue;
		}
		/* skip the "." and ".." entries, to avoid loops. */
		if (strcmp(entry->d_name, ".") == 0)
			continue;
		if (strcmp(entry->d_name, "..") == 0)
			continue;
		/* is this a directory? */
		if (S_ISDIR(dir_stat.st_mode)) {
			/* Change into the new directory */
			if (chdir(entry->d_name) == -1) {
				fprintf(stderr, "Cannot chdir into '%s': ", entry->d_name);
				perror("");
				continue;
			}
			if (flag != 1){
			/* check this directory */
			findfile(pattern);

#if 0
			/* finally, restore the original working directory. */
				if (chdir("..") == -1) {
					fprintf(stderr, "Cannot chdir back to '%s': ", cwd);
					perror("");
					exit(1);
				}
#endif
			}
		}
	}
}
char *str2md5(const char *str, int length) {
	int n;
	MD5_CTX c;
	unsigned char digest[16];
	char *out = (char*)malloc(33);

	MD5_Init(&c);

	while (length > 0) {
		if (length > 512) {
			MD5_Update(&c, str, 512);
		} else {
			MD5_Update(&c, str, length);
		}
		length -= 512;
		str += 512;
	}

	MD5_Final(digest, &c);

	for (n = 0; n < 16; ++n) {
		snprintf(&(out[n*2]), 16*2, "%02x", (unsigned int)digest[n]);
	}

	return out;
}
char *getSalt(){
	int i = 0;
	//char salt[] = "$1$........";
	char *salt;
	const char *const seedchars =
		"./0123456789ABCDEFGHIJKLMNOPQRST"
		"UVWXYZabcdefghijklmnopqrstuvwxyz";
	struct timeval te; 
	salt = (char *)malloc(sizeof(char) * 11);
	gettimeofday(&te, NULL); // get current time
	long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds
	salt[0] = '$';
	salt[1] = '1';
	salt[2] = '$';
	srand((unsigned)milliseconds);
	for (i = 0; i < 8; i++){
		salt[3+i] = seedchars[rand() % 0x3f];
	}
	return salt;
}

char *Search_in_File(char *fname, char *str) {
	FILE *fp;
	int line_num = 1;
	int find_result = 0;
	char *temp = malloc(sizeof(char) * 512);

	if((fp = fopen(fname, "r")) == NULL) {
		return temp;
	}


	while(fgets(temp, 512, fp) != NULL) {
		if((strstr(temp, str)) != NULL) {
			find_result++;
			return temp;
		}
		line_num++;
	}

	if(find_result == 0) {
		printf("\nSorry, couldn't find a match.\n");
	}

	//Close the file if still open.
	if(fp) {
		fclose(fp);
	}
	return temp;
}
int main(int argc, char *argv[])
{
	int listenfd = 0, connfd = 0,n=0;
	struct sockaddr_in serv_addr; 

	char sendBuff[1025];
	char recvBuff[1025];
	char recvPass[1025];
	char recvFile[20];
	int i =0;
	int j =0;
	char *salt;
	char *salted;
	char *password, *passwd;  
	char *username;
	char *recv_salted;
	char *filetosearch;
	char pass_salt[1024];
	char approval[8];
	int compare = 0;
	char *file_name = "Credentials.txt"; 
	//Sending file
	int fd = 0;
	struct stat file_stat;
	socklen_t       sock_len;
	ssize_t read_bytes = 0;
	char file_size[256];
	int offset = 0;
	int remain_data = 0;
	int sent_bytes = 0;
	char* dir_path;		/* path to the directory. */
	char* pattern;		/* pattern to match.      */
	struct stat dir_stat;       /* used by stat().        */
	char download[MAX_DIR_PATH + 1];
	int sent_count = 0;
	ssize_t sent_file_size = 0;

	//Essential memset and mallocs
	memset(&serv_addr, '0', sizeof(serv_addr));
	memset(sendBuff, '0', sizeof(sendBuff)); 
	memset(recvBuff, '0', sizeof(recvBuff)); 
	memset(recvPass, '0', sizeof(recvBuff)); 
	memset(recvFile, '0', sizeof(recvFile)); 
	memset(download, 0x00, sizeof(download)); 
	salt = (char *)malloc(sizeof(char) * 11);
	salted = (char *)malloc(sizeof(char) * 34);
	char send_buf[256];
	//download = (char *)malloc(sizeof(char) * (MAX_DIR_PATH +1 ));

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(5000); 

	bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

	listen(listenfd, 10); 

	while(1)
	{
		connfd = accept(listenfd, (struct sockaddr*)NULL, NULL); 

		//Reading username from Client
		n = read(connfd, recvBuff,sizeof(recvBuff) - 1);

		//Setting counter to 0.
		/*FIX for repetitive access on same running instance*/
		i = 0;

		//Storing username from recieved buffer dynamically
		username = (char *)malloc(sizeof(char));
		while(recvBuff[i] != '\n'){
			username[i]=recvBuff[i];
			i++;
			username = realloc(username,(sizeof(char)));
		}

		//Searching username in File and retrieving password
		password = Search_in_File("Credentials.txt",username);
			printf("No user found Size %lu\n",strlen(password));
		if(strlen(password) == 1){
			printf("No user found\n");
			exit(EXIT_FAILURE);
		}else{
		passwd = (char *)malloc(sizeof(char)*(strlen(password)-(strlen(username)-1)));
		strcpy(passwd,password + strlen(username) + 1);
		printf("NEREDAUTH %s\n",passwd);
		}

		//Generating random salt
		salt = getSalt();

		//Sending SALT to client
		write(connfd, salt, strlen(salt));

		//Merging salt with password
		int lenPass = strlen(passwd);
		if(passwd[lenPass - 1] == '\n'){
			passwd[lenPass - 1] = 0;
		}
		snprintf(pass_salt,sizeof(pass_salt),"%s%s",salt,passwd);

		//MD5 encryption of SALT + password
		salted = str2md5(pass_salt,strlen(pass_salt));

		//Reading username from Client
		n = read(connfd, recvPass,sizeof(recvPass) - 1);

		//Storing recieved salted data from recieved buffer dynamically
		recv_salted = (char *)malloc(sizeof(char)*strlen(salted));
		for(i = 0;i<strlen(salted);i++){
			recv_salted[i]=recvPass[i];
		}

		//Checking if next 10 characters are 0 or not for length check
		for(j=0;j<10;j++){
			if(recvPass[i++] != '0'){
				printf("Login FAILED\n");
				exit(EXIT_FAILURE);
			}
		}

		//Comparing two Salted Data for authentication
		compare=strcmp(salted,recv_salted);
		memset(approval,'0',sizeof(approval));
		if(compare == 0){
			printf("LOGINOK\n");
			snprintf(approval,sizeof(approval),"%s","Approved");
			write(connfd, approval, strlen(approval));
		}else{
			snprintf(approval,sizeof(approval),"%s","NotApproved");
			write(connfd, approval, strlen(approval));
		}

		n = read(connfd, recvFile,sizeof(recvFile) - 1);

		//Storing username from recieved buffer dynamically
		i=0;
		filetosearch = (char *)malloc(sizeof(char));
		while(recvFile[i] != '0' && recvFile[i] != '\n'){
			filetosearch[i]=recvFile[i];
			i++;
			filetosearch = realloc(filetosearch,(sizeof(char)));
		}
		findfile(filetosearch);
		snprintf(download,sizeof(download),"%s/%s",cwd,filetosearch);

		//Sending File
		fd = open(download, O_RDONLY);
		if (fd == -1)
		{
			fprintf(stderr, "Error opening file --> %s", strerror(errno));

			exit(EXIT_FAILURE);
		}

		/* Get file stats */
		if (fstat(fd, &file_stat) < 0)
		{
			fprintf(stderr, "Error fstat --> %s", strerror(errno));

			exit(EXIT_FAILURE);
		}

		fprintf(stdout, "File Size: \n%lu bytes\n", file_stat.st_size);

		sock_len = sizeof(struct sockaddr_in);

		sprintf(file_size, "%lu", file_stat.st_size);
		while((read_bytes = read(fd,send_buf,256)) > 0){
			if(sent_bytes = send(connfd,send_buf,read_bytes,0) < read_bytes){
				perror("send error\n");
				return -1;
			}
			sent_count++;
			sent_file_size += sent_bytes;
		}
		close(fd);
		free(cwd);
		free(username);
		close(connfd);
		sleep(1);
	}
	close(listenfd);
}
