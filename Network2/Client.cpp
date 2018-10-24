#include<stdio.h>	//printf
#include<string.h>	//strlen
#include<sys/socket.h>	//socket
#include<arpa/inet.h>	//inet_addr
#include <unistd.h>
#include <algorithm>
#include <sys/stat.h>
#include <iostream>
#include <fstream>

int main(int argc , char *argv[]) {
    int sock;
    struct sockaddr_in server;
    char message[4096] = "hehaheh.txt " , server_reply[2000];
    sock = socket(AF_INET , SOCK_STREAM, 0);
    if (sock == -1) {
        printf("Could not create socket");
    }
    puts("Socket created");

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(528491);

    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }

    puts("Connected\n");

         struct stat st;
        stat("heh.txt",&st);
        std::string y (message);
        auto x = std::to_string(st.st_size);
        x+=" ";
        x+="LOLKEK ";
        y+=x;

    if( send(sock , y.c_str() , y.size() , 0) < 0) {
        puts("Send failed");
        return 1;
    }
    std::fill(message,message+4096,0);

    std::cout<<"dasdad\n";

    std::cout<< recv(sock,server_reply,4096,0);
    std::cout<<server_reply;
    FILE* fin = fopen("heh.txt","r");
    size_t readed = 4096;
    for (long long i = 0; readed ==4096 ; ++i) {
        readed = fread(message, sizeof(char),4096,fin);
        send(sock,message,readed,0);
        std::cout<< readed<<std::endl;
    }
    recv(sock,server_reply,4096,0);
    puts("Server reply :");
    puts(server_reply);
    close(sock);
    return 0;
}