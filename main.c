#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/poll.h>


#include <arpa/inet.h>
#include <fcntl.h>
//---------------------------- Дефайны ----------------------------------------
#define MAXPENDING 5   /* Размер  очереди */
#define RCVBUFSIZE 32   /* Размер буфера */
#define K 3
#define L 1
#define T 1

#define CHK(eval) if(eval < 0){perror("eval"); exit(-1);}
#define EPOLL_SIZE 100
//--------------------------- Структуры и глобальные переменные ----------------
struct ThreadArgs
{
    int clntSock;                      /* Socket descriptor for client */
};

typedef struct ThreadArgs Args;

char buffer[MAXPENDING][RCVBUFSIZE];
int queue;
//--------------------------- Прототипы ----------------------------------------
int createTCPServerSocket(unsigned short);
void *UDPNotification(void *);
int acceptTCPConnection(int);
void dieWithError(char *);
int setnonblocking(int);
//--------------------------- Главная функция ----------------------------------

int main(int argc, char* argv[])
{
    /*  Объявления переменных */
    int servSockTCP, sockBroadcast;
    int clntSock;
    struct sockaddr_in ClntAddr;
    unsigned short ServPortTCP, Client1PortUDP;
    ServPortTCP = 19999;
    Client1PortUDP = 14444;
    unsigned int clntLen;

    pthread_t thread1, thread2, udp_client;
    Args *threadArgs;
    int result;


     queue = 0;

    char message[RCVBUFSIZE];
    clock_t tStart;
    int numfd=0;
    int recvMsgSize;
   //немножко poll в массы / на чтение / я хз что тут творится
    struct pollfd fds[EPOLL_SIZE];

    servSockTCP = createTCPServerSocket(ServPortTCP);

    //     set listener to event template
    fds[0].fd = servSockTCP;
    fds[0].events = POLLIN;
    numfd++;

    if ((pthread_create(&udp_client, NULL, UDPNotification, &Client1PortUDP)) != 0)
               dieWithError("pthread_create() failed");

    if ((threadArgs = ( Args *) malloc(sizeof( Args)))  == NULL)
    dieWithError("malloc() failed");
    for (;;) /* Бесконечный цикл */
    {
           poll(fds,numfd, 1000);
           for(int i = 0; i < numfd ; i++)
           {
               if( fds[i].revents & POLLIN )
               {
                    if(fds[i].fd == servSockTCP)
                    {
                        clntSock = acceptTCPConnection(servSockTCP);

                        fds[numfd].fd = clntSock;
                        // set new client to event template
                        if (queue < MAXPENDING)
                        {
//                            fds[numfd].fd = clntSock ;
                            fds[numfd].events = POLLIN;
                            numfd++;
                        }
                        else
                        {
                            while (queue >0)
                            {
                                if ((send(fds[numfd].fd, buffer[--queue], RCVBUFSIZE,0)) <0)
                                    dieWithError("send() failed");
                            }
                            numfd++;
                        }
                   }
                   else
                   {
                        if ((recvMsgSize = recv(fds[i].fd, buffer[queue], RCVBUFSIZE, 0)) < 0)
                            dieWithError("recv() failed");
                        else if (recvMsgSize == 0)
                        {
                            printf("Connection close\n");
                            fds[i].fd = -1;
                            fds[i].events = 0;
//                            numfd--;
//                            close(fds[i].fd);
                        }
                        else
                        {
                            printf("SERVER received MSG: %s\n ", buffer[queue]);
                            queue++;
                        }
                   }
               }
          }

      }
}
//--------------------------- Функции ----------------------------------------------------

void dieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}

int setnonblocking(int sockfd)
{
   CHK(fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)|O_NONBLOCK));
   return 0;
}

void *UDPNotification(void *arg)
{
    unsigned short port = *(unsigned short*) arg;
    int sock;
    int broadcastPermission;
    struct sockaddr_in addr;
    int bytes ;
    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
            dieWithError("socket() failed");

    broadcastPermission = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void *) &broadcastPermission,
          sizeof(broadcastPermission)) < 0)
        dieWithError("setsockopt() failed");

      memset(&addr, 0, sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = inet_addr("192.168.0.255");
      addr.sin_port = htons(port);
      char message[RCVBUFSIZE], message1[RCVBUFSIZE];
      strcpy(message,"I wait a message...\r\n\0");
      strcpy(message1,"I have a message...\r\n\0");
      for (;;)
      {
         printf("<Queue msg: %d>\n", queue);
         if (queue < MAXPENDING)
         {
             sleep(5);
             bytes = sendto(sock, message, strlen(message), 0, (struct sockaddr*)&addr, sizeof addr);
             sleep(K);
         }
         else
         {
             bytes = sendto(sock, message1, strlen(message1), 0, (struct sockaddr*)&addr, sizeof addr);
             sleep(L);
         }
      }
     pthread_exit(0);
}

int createTCPServerSocket(unsigned short port)
{
    /*  Объявления переменных */
    int sock;
    struct sockaddr_in ServAddr;

    /* Создаем сокет для входящих соединений */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        dieWithError("socket() failed");

    setnonblocking(sock);

    /* Задаем адрес TCP сервера */
    memset(&ServAddr, 0, sizeof(ServAddr));
    ServAddr.sin_family = PF_INET;
    ServAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ServAddr.sin_port = htons(port);

    /* Связываем адрес с дескриптором слушающего сокета */
    if (bind(sock, (struct sockaddr *) &ServAddr, sizeof(ServAddr)) < 0)
        dieWithError("bind() failed");

    /* Слушаем сокет */
    if (listen(sock, MAXPENDING+10) < 0)
        dieWithError("listen() failed");
    return sock;
}

int acceptTCPConnection(int servSock)
{
     /*  Объявления переменных */
    int clntSock;
    struct sockaddr_in ClntAddr;
    unsigned int clntLen;

    /* Set the size of the in-out parameter */
    clntLen = sizeof(ClntAddr);

    /* Wait for a client to connect */
    if ((clntSock = accept(servSock, (struct sockaddr *) &ClntAddr,
           &clntLen)) < 0)
        dieWithError("accept() failed");

    setnonblocking(clntSock);
    printf("Handling client %s: \n", inet_ntoa(ClntAddr.sin_addr));

    return clntSock;
}
