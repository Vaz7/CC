#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "queries.h"

#include "utils.h"

#define DEFAULT_PORT 1234
// 5 seconds timeout to select
#define TIMEOUT 5

void usage(){
  
    printf("Use isto como quiser\n");
}


int main(int argc, char **argv){
    char buffer[1024];
    char *msn;
    char dns_server_ip[16];
    int dns_server_port;
    int udpfd, nready;
    fd_set rset;
    ssize_t n;
    socklen_t len;
    struct sockaddr_in servaddr;
    struct timeval timeout;
    QUERY n_query=malloc(sizeof (struct query));

    if(argc!=5){
      printf("Numero de argumentos invalido\n");
      usage();
      exit(1);
    }

    if (!IP_parser(argv[1],dns_server_ip, &dns_server_port, DEFAULT_PORT)){
      printf("Numero IP do servidor DNS invalido\n); //%s\n", argv[1]);
    }
    // iniciar geracao de numeros aleatorios
    srand(time(NULL));
    
    // socket UDP
    udpfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(udpfd<0){
        printf("Unable to create UDP socket");
        exit(1);
    }

    // minha informacao
    bzero(&servaddr, sizeof(servaddr));//inicializar a estrutura a zeros
    servaddr.sin_family = AF_INET; // IP
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // aceita connecoes de todos, mas podia ser apenas do servidor
    servaddr.sin_port = htons(0); // porta aleatoria para mim (primeira porta disponivel). Nao pode ser a mesma porta do servidor, senao o cliente "ouve-se a si mesmo".


    // binding server addr structure to udp sockfd
    bind(udpfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
 
    len = sizeof(servaddr);

    // informacao do servidor
    inet_aton(dns_server_ip,&(servaddr.sin_addr));
    servaddr.sin_port = htons(dns_server_port); // porta do servidor para onde enviar 

    
    // constroi query a partir dos argumentos de entrada
    build_query(n_query,argv);

    // constroi texto da query, alocando memoria para a string
    msn=print_query(n_query);

    if(sendto(udpfd, msn, (size_t)strlen(msn), 0, (struct sockaddr*)&servaddr, len)<0){
      printf("Erro no envio da query\n");
      exit(1);
    }

    // descriptor set
    FD_ZERO(&rset);
    FD_SET(udpfd, &rset);
    buffer[0]=0;

    timeout.tv_sec=TIMEOUT;
    timeout.tv_usec=0;
    // select the ready descriptor
    nready = select(udpfd+1, &rset, NULL, NULL, &timeout);

    if(nready<0){
      printf("Error on UDP select");
      exit(1);
    }

    if (FD_ISSET(udpfd, &rset)) { // ligacao UDP
      bzero(buffer, 1024);
      n = recvfrom(udpfd, buffer, sizeof(buffer), 0,(struct sockaddr*)&servaddr, &len);//n é o numero de bytes
      if(n>0){                    // ligacao UDP para queries DNS
	      if(parse_query(n_query,buffer)>=0){
		puts(print_verbose_query(n_query));
	      } else {
		printf("Invalid response from server\n");
	      }
      }
    } else {
      printf("Timeout. No response in %d seconds\n",TIMEOUT);
    }

      free(msn);
    return 0;
}
