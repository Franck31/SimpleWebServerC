#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<signal.h>
#include<fcntl.h>

#define CONNMAX 1000
#define BYTES 1024

char *ROOT;
int listenfd, clients[CONNMAX];
void error(char *);
void startServer(char *);
void respond(int);

int main()
{
  struct sockaddr_in clientaddr;
  socklen_t addrlen;
  char c;    
  char PORT[6];
  //para montar el docroot
  ROOT = getenv("PWD");
  //Puerto TCP
  strcpy(PORT,"10000");

  int slot=0;
  //Console Output Correindo
  printf("Puerot %s DocumentROOT %s \n",PORT,ROOT);

  int i;
  for (i=0; i<CONNMAX; i++)
    clients[i]=-1;
  startServer(PORT);


  while (1)
  {
    addrlen = sizeof(clientaddr);
    clients[slot] = accept (listenfd, (struct sockaddr *) &clientaddr, &addrlen);

    if (clients[slot]<0)
      error ("accept()");
    else
    {
      if ( fork()==0 )
      {
        respond(slot);
        exit(0);
      }
    }

    while (clients[slot]!=-1) slot = (slot+1)%CONNMAX;
  }

  return 0;
}

void startServer(char *port)
{
  struct addrinfo hints, *res, *p;

  memset (&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  if (getaddrinfo( NULL, port, &hints, &res) != 0)
 {
    perror ("getaddrinfo() error");
    exit(1);
 }

  for (p = res; p!=NULL; p=p->ai_next)
  {
    listenfd = socket (p->ai_family, p->ai_socktype, 0);
    if (listenfd == -1) continue;
    if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break;
  }

  freeaddrinfo(res);

  if ( listen (listenfd, 1000000) != 0 )
  {
      perror("listen() error");
       exit(1);
  }
}

//cliente
void respond(int n)
{
  char mesg[99999], *reqline[3], data_to_send[BYTES], path[99999];
  int rcvd, fd, bytes_read;

  memset( (void*)mesg, (int)'\0', 99999 );

  rcvd=recv(clients[n], mesg, 99999, 0);

  if (rcvd<0)    
    fprintf(stderr,("recv() error\n"));
  else if (rcvd==0)    
    fprintf(stderr,"Client side error 4XX.\n");
  else    
  {
    reqline[0] = strtok (mesg, " \t\n");
    if ( strncmp(reqline[0], "GET\0", 4)==0 )
    {
      //HTML Responses
      reqline[1] = strtok (NULL, " \t");
      reqline[2] = strtok (NULL, " \t\n");
      if ( strncmp( reqline[2], "HTTP/1.0", 8)!=0 && strncmp( reqline[2], "HTTP/1.1", 8)!=0 )
      {
        write(clients[n], "HTTP/1.0 400 Bad Request\n", 25);
      }
      else
      {
        if ( strncmp(reqline[1], "/\0", 2)==0 )
          reqline[1] = "/index.html";        
        strcpy(path, ROOT);
        strcpy(&path[strlen(ROOT)], reqline[1]);
        if ( (fd=open(path, O_RDONLY))!=-1 )    
        {
          send(clients[n], "HTTP/1.0 200 OK\n\n", 17, 0);
          while ( (bytes_read=read(fd, data_to_send, BYTES))>0 )
            write (clients[n], data_to_send, bytes_read);
        }
        else    write(clients[n], "HTTP/1.0 404 Not Found\n", 23); 
      }
    }
  }
//mata a los clientes
  shutdown (clients[n], SHUT_RDWR);         
  close(clients[n]);
  clients[n]=-1;
}
