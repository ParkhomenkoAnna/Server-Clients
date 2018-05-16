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

//---------------------------- Дефайн ----------------------------------------
#define T 2
#define RCVBUFSIZE 32
#define strtChr 92
#define cntChr 33
//--------------------------- Прототипы ---------------------------------------
void *sendMsg(void *);
int createUDPServer();
void dieWithError(char *);
//--------------------------- Главная функция ---------------------------------

int main(int argc, char* argv[])
{
    /*  Объявления переменных */
    int sockTCP, sockUDP;
    struct sockaddr_in ServAddr; /* Echo server address */
        unsigned short ServPort;     /* Echo server port */

    pthread_t tcp_client;
    int result;

    char recvString[RCVBUFSIZE+1];
    int recvStringLen;

    /* инициализация переменных */
    ServPort = 19999;

    /* Создаем сокет для соединения с TCP сервером */
    if ((sockTCP = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
       dieWithError("socket() failed");

    /* Задаем адрес TCP сервера */
    memset(&ServAddr, 0, sizeof(ServAddr));
    ServAddr.sin_family = PF_INET;
    ServAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ServAddr.sin_port = htons(ServPort);

    /* Подключаемся TCP серверу */
    if (connect(sockTCP, (struct sockaddr *) &ServAddr, sizeof(ServAddr)) < 0)
       dieWithError("connect() failed");

    /* Создаем UDP сокет */
    sockUDP = createUDPServer();

    for (;;)
    {
        /* Принимаем UDP сообщение */
        if ((recvStringLen = recvfrom(sockUDP, recvString, RCVBUFSIZE, 0, NULL, 0)) < 0)
           dieWithError("recvfrom() failed");
        recvString[recvStringLen] = '\0';
        printf("Received: %s\n", recvString);  // Print the received string

        /* Если UDP сообщение равно указанному, то начинается отправка рандомного сообщения
        TCP серверу*/
        if (strcmp(recvString, "I wait a message...\r\n\0") == 0)
            if ((result = pthread_create(&tcp_client, NULL, sendMsg, &sockTCP)) != 0)
                dieWithError("pthread_create() failed");
    }
}
//--------------------------- Функции ----------------------------------------

void dieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}

void *sendMsg(void *arg)
{
    int sock = *(int*) arg;
    char buffer[RCVBUFSIZE];
    srand(clock());

    int len = rand()%RCVBUFSIZE;
    for (int i=0; i<len; i++)
    {
        buffer[i] = strtChr + rand() % cntChr;
    }
    send(sock, buffer, strlen(buffer), 0);
    printf("CLIENT SEND MSG: %s\n", buffer);
    sleep(T);

    return (NULL);
}


int createUDPServer()
{
    int sock;
    struct sockaddr_in broadcastAddr;
        unsigned short broadcastPort;
     broadcastPort = 14444;

    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
             dieWithError("socket() failed");

    /* Задаем адрес UDP сервера */
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));
    broadcastAddr.sin_family = PF_INET;
    broadcastAddr.sin_addr.s_addr =  htonl(INADDR_ANY);
    broadcastAddr.sin_port = htons(broadcastPort);

    /* Связываем адрес с дескриптором слушающего сокета */
    if (bind(sock, (struct sockaddr *) &broadcastAddr, sizeof(broadcastAddr)) < 0)
        dieWithError("bind() failed");
    return sock;
}
