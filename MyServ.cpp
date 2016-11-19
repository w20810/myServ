// @Created time :2016年11月17日 星期四 19时09分42秒
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <ctype.h>
using namespace std;

typedef const struct sockaddr CSA;
typedef struct sockaddr SA;
const int LISTENS_SIZE = 30;

#define SERVER_STRING "Server: httpd/0.1.0\r\n"

void error_die(const char* src)
{
	perror(src);
	exit(1);
}

int getline(int client, char* buff, size_t size)
{
	char ch;
	int cnt = 0;
	while (true)
	{
		int n = read(client, &ch, 1);
			
		if (n == -1)
			error_die("read client");	
	   	if (n == 0)
			return cnt;
		buff[cnt++] = ch;
		if (ch == '\r')
		{
			n = read(client, &ch, 1);
			if (n == -1)
				error_die("read client");
			if (n == 0)
				return cnt;
			if (ch == '\n')
			{
				buff[--cnt] = '\0';
				return cnt;	
			}
			buff[cnt++] = ch;
		}
	}
	return cnt;
}

int startup(u_short *port)
{
	int httpd = 0;
	struct sockaddr_in name;
	
	httpd = socket(PF_INET, SOCK_STREAM, 0);
	if (-1 == httpd)
		error_die("socket");
	memset(&name, 0, sizeof(name));
	name.sin_family = AF_INET;
	name.sin_port   = htons(*port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(httpd, (CSA*)&name, (socklen_t )sizeof(name)) == -1)
		error_die("bind");
	if (*port == 0)
	{
		socklen_t namelen = sizeof(name);
		if (getsockname(httpd, (SA*)&name, &namelen) == -1)
			error_die("getsockname");
		*port = htons(name.sin_port);
	}
	if (listen(httpd, LISTENS_SIZE) == -1)
		error_die("listen");
	return httpd;
}

void headers(int client)
{
	char buff[1024];

	strcpy(buff, "HTTP/1.0 200 OK\r\n");
	write(client, buff, strlen(buff));
	
	strcpy(buff, SERVER_STRING);
	write(client, buff, strlen(buff));

	strcpy(buff, "Content-Type: text/html\r\n");
	write(client, buff, strlen(buff));

	strcpy(buff, "\r\n");
	write(client, buff, strlen(buff));
}

void send_homepage(int client)
{
	char buff[4096];
	read(client, buff, sizeof(buff)); //discard
	headers(client);
	int fd_html = open("index.html", O_RDONLY);
	size_t	sz = read(fd_html, buff, sizeof(buff));
	write(client, buff, sz);
	close(fd_html);
	write(1, "send ok\n", 8);
}

void* accept_request(void* client)
{
	int fd_cli =*(int*) client;
	delete (int*)client;
	char buff[1048];
	int n = getline(fd_cli, buff, sizeof(buff));
	//write(1, buff, strlen(buff));write(1, "\n", 1);
	char method[20];
	sscanf(buff, "%s", method);
	//write(1, method, strlen(method));
	if (0 == strcmp(method, "GET"))
	{
	   sscanf(buff+strlen(method), "%s", method);
	   if (0 == strcmp(method, "/"))
	   {
			send_homepage(fd_cli);	   
	   }   
	}
	close(fd_cli);
	return (void*)0;
}

int main()
{
	u_short port = 0;
	int srv_sock = startup(&port);	
	printf("httpd running on port %d\n", port);
	
	for (;;)
	{
		int* cli_sock = new int;
		*cli_sock = accept(srv_sock, NULL, NULL);
		if (-1 == *cli_sock)
			error_die("accpet");
		pthread_t newthread;
		if (pthread_create(&newthread, NULL, accept_request, (void*)cli_sock) != 0)
			error_die("pthread_create");
	}
  	return 0;
}
