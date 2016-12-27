#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <crypt.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h> 
#if defined(__APPLE__)
#  define COMMON_DIGEST_FOR_OPENSSL
#  include <CommonCrypto/CommonDigest.h>
#  define SHA1 CC_SHA1
#else
#  include <openssl/md5.h>
#endif


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

int main(int argc, char *argv[])
{
	int sockfd = 0, n = 0,clientsend = 1,i=0;
	char recvSalt[1024];
	char recvApproval[11];
	int recChar=0;
	char password[20];
	char sendUser[1024];
	char sendPass[1024];
	char sendFile[1024];
	struct sockaddr_in serv_addr; 
	char username[10];
	char *salt;
	char *fullPass;
	char pass_salt[1024];
	char search_file[20];
	char buffer[BUFSIZ];
	int file_size;
	int rf;
	int remain_data = 0;
	ssize_t len;
	int recv_count;
	ssize_t recvd_file_size,rcvd_bytes;
	char recv_str[256];

	fullPass = (char *)malloc(sizeof(char) * 34);

	if(argc != 2)
	{
		printf("\n Usage: %s <ip of server> \n",argv[0]);
		return 1;
	} 

	memset(recvSalt, '0',sizeof(recvSalt));

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\n Error : Could not create socket \n");
		return 1;
	} 

	memset(&serv_addr, '0', sizeof(serv_addr)); 

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(5000); 

	if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)
	{
		printf("\n inet_pton error occured\n");
		return 1;
	} 

	if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("\n Error : Connect Failed \n");
		return 1;
	}
	//Getting username from user
	printf("LOGIN: ");
	scanf("%s",username);

	//Sending username to Server
	snprintf(sendUser, sizeof(sendUser), "%s\n", username);
	write(sockfd, sendUser, strlen(sendUser));

	//Reading salt from server
	n = read(sockfd, recvSalt,sizeof(recvSalt) - 1);

	//Storing salt from recieved buffer dynamically
	salt = (char *)malloc(sizeof(char));
	while(recvSalt[i] != '0' && recvSalt[i] != '\n'){
		salt[i]=recvSalt[i];
		i++;
		salt = realloc(salt,(sizeof(char)));
	}

	//Getting password from user
	printf("AUTH : ");
	scanf("%s",password);

	//Meging salt and password
	snprintf(pass_salt,sizeof(pass_salt),"%s%s",salt,password);

	//MD5 encryption of salt and password
	fullPass = str2md5(pass_salt,strlen(pass_salt));

	//Sending MD5 encrypted salt and pass to server
	snprintf(sendPass, sizeof(sendPass), "%s", fullPass);

	write(sockfd, sendPass, strlen(sendPass));

	n = read(sockfd, recvApproval,sizeof(recvApproval) - 1);

	if(recvApproval[0] == 'A'){
		printf("GETFILE : ");
		scanf("%s",search_file);
		snprintf(sendFile,sizeof(sendFile),"%s",search_file);
		write(sockfd, sendFile, strlen(sendFile));

		//Recieving File from Server
		rf = open(sendFile, O_WRONLY|O_CREAT,0644);
		if (rf < 0)
		{
			fprintf(stderr, "Failed to open file foo --> %s\n", strerror(errno));

			exit(EXIT_FAILURE);
		}
		recv_count = 0;
		recvd_file_size = 0;
		while((rcvd_bytes = recv(sockfd,recv_str,256,0))>0){
			recv_count++;
			recvd_file_size += rcvd_bytes;
			if(write(rf,recv_str,rcvd_bytes) < 0){
				perror("Error writing file\n");
				return -1;
			}
		}
		close(rf);
	}else{
		printf("You are NOT allowed\n");
	}
	if(n < 0){
		printf("Error in read");
	}

	return 0;
}
