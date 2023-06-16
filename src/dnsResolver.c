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
#include <pthread.h>
#include "utils.h"
#include "cache.h"
#include "queries.h"
#include "dnsResolver.h"

#define N_CACHE 300
#define DEFAULT_SOARETRY 3
#define TIMEOUT 1

#define maximo(a,b) (a<b? b:a)

//ver o que é o DD e a duvida acima
int cache_size;

struct SRConfig SRconfig={1,1234,NULL,NULL,20000};//modo debug/shy, porta por defeito, ficheiro de confg, ficheiro de log e timeout
STDBFILES STdbfiles=NULL;//estrutura para guardar os ficheiros db dos ST

STENTRY STlist=NULL;//estrutura para guardar os dados dos servidores de topo
CACHE_RECORD *SP_cache=NULL;

void print_configuration(){
    if(SRconfig.debug)
        printf("Estou em modo debug\n");
    else
        printf("Estou em modo shy\n");

    printf("Vou usar a porta TCP %d\n",SRconfig.port);
    printf("Ficheiro log:%s\n",SRconfig.logfilename);
    printf("Ficheiro de configuracao: %s\n",SRconfig.conffilename);
}

void usage(){
    printf("Use isto como quiser\n");
}

void write_my_log(char *linha){
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    FILE *logfile;
    struct stat buffer; 

    // ver se ficheiro existe, senao cria
      
    if(stat(SRconfig.logfilename, &buffer) == 0){ // log file exists
        logfile=fopen(SRconfig.logfilename,"a");
        if(!logfile){
            printf("Nao podemos prosseguir sem log\nAbortando\n");
            exit(1);
        }
        fprintf(logfile,"%02d:%02d:%04d.%02d:%02d:%02d",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec);
        fprintf(logfile," %s\n",linha);
        if(SRconfig.debug){ // modo debug imprime tambem no terminal
            printf("%02d:%02d:%04d.%02d:%02d:%02d",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec);
            printf(" %s\n",linha);
        }
    } else {
        logfile=fopen(SRconfig.logfilename,"a");
        if(!logfile){
            printf("Nao podemos prosseguir sem log\nAbortando\n");
            exit(1);
        }
        fprintf(logfile,"%02d:%02d:%04d.%02d:%02d:%02d SP log-file-created %s\n",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec,SRconfig.logfilename);
        fprintf(logfile,"%02d:%02d:%04d.%02d:%02d:%02d",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec);
        fprintf(logfile," %s\n",linha);
        if(SRconfig.debug){ // modo debug imprime tambem no terminal
            printf("%02d:%02d:%04d.%02d:%02d:%02d SP log-file-created %s\n",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec,SRconfig.logfilename);
            printf("%02d:%02d:%04d.%02d:%02d:%02d",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec);
            printf(" %s\n",linha);
        }
    }

    // é sempre possivel fechar ficheiro
    fclose(logfile);
}


int send_udp_query(QUERY r_query,QUERY q_query, char *dns_server_ip, int dns_server_port){
    char buffer[1024];
    char *msn;
    int udpfd, nready;
    fd_set rset;
    ssize_t n;
    socklen_t len;
    struct sockaddr_in servaddr;
    struct timeval timeout;
    QUERY n_query=malloc(sizeof (struct query));

    // iniciar geracao de numeros aleatorios
    srand(time(NULL));
    
    // socket UDP
    udpfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(udpfd<0){
        printf("Unable to create UDP socket");
        return 0;
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

    // constroi texto da query, alocando memoria para a string
    msn=print_query(r_query);

    if(sendto(udpfd, msn, (size_t)strlen(msn), 0, (struct sockaddr*)&servaddr, len)<0){
      printf("Erro no envio da query\n");
      free(n_query);
      return 0; // not found
    }

    free(msn);

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
      free(n_query);
      return 0; // not found
    }

    if (FD_ISSET(udpfd, &rset)) { // ligacao UDP
      bzero(buffer, 1024);
      n = recvfrom(udpfd, buffer, sizeof(buffer), 0,(struct sockaddr*)&servaddr, &len);//n é o numero de bytes
      if(n>0){                    // ligacao UDP para queries DNS
	      if(parse_query(n_query,buffer)>=0){
		    memcpy(q_query,n_query,sizeof(struct query));
            free(n_query);
            return 1; // found
	      } else {
            free(n_query);
		    return 0; // not found
	      }
      }
    } else {
      printf("Timeout. No response in %d seconds\n",TIMEOUT);
      free(n_query);
      return 0;
    }


}


// funcao que responde a uma query por UDP
void *reply_udp(void *c){
  char buffer[1024],log_msg[2048];
  struct sockaddr_in cliaddr;
  int udpfd=*((int*)c); // descritor de ficheiro com a ligacao UDP, nao fechar 'a saida
  socklen_t len = sizeof(cliaddr);
  ssize_t n;
  QUERY r_query=malloc(sizeof(struct query));
  QUERY q_query=malloc(sizeof(struct query));
  QUERY q2_query=malloc(sizeof(struct query));
  QUERY q3_query=malloc(sizeof(struct query));
  CNAMEENTRY current_cname=NULL;
  STENTRY current_ST;
  char nome[128],nome_subdominio[128],nome_server[128];
  char ip[20],ip_field[20];
  int ttl,ordem,ip_port;

  char *response_value[10],temp_response[1024],temp_int[20];
  char *authorities_value[10], *extra_value[10];
  int index=0, n_response_value, n_authorities_value, n_extra_value;
  char *tmp_response, *domain=NULL;
  int found,i,j,k;

  bzero(buffer, sizeof(buffer));
  n = recvfrom(udpfd, buffer, sizeof(buffer), 0,(struct sockaddr*)&cliaddr, &len);//n é o numero de bytes
  if(n>0){                    // ligacao UDP para queries DNS
   
    if(parse_query(r_query,buffer)<0){
        snprintf(log_msg,2048,"EV 127.0.0.1 Invalid query %s",buffer);
        write_my_log(log_msg);
        //pthread_exit(NULL);
        return NULL;
    };


    found=0;
    
    n_response_value=-1;
    n_authorities_value=-1;
    n_extra_value=-1;

    memset(q_query,0,sizeof(struct query));
    q_query->message_id=r_query->message_id;
    q_query->flags="R+A";
    q_query->response_code=0;
    q_query->n_values=n_response_value+1;
    q_query->n_authorities=n_authorities_value+1;
    q_query->n_extra_values=n_extra_value+1;
    q_query->info_name=r_query->info_name;
    q_query->info_type_of_name=r_query->info_type_of_name;
    q_query->response_values=NULL;//array de response values
    q_query->authorities_values=NULL;//array de authorities values
    q_query->extra_values=NULL; //array de extra values


    //fazer funcao de procura na cache
    if(!strcmp(r_query->info_type_of_name,"A")){
        // simplesmente procurar na cache, mas nao fazemos ideia de dominios auth
        index=0;
        while(index>=0 && SP_cache){
            index=pop_record(r_query->info_name,A_CACHE,index,SP_cache,cache_size);           
            if(index>=0){
                found=1;
                n_response_value++;
                strcpy(temp_response,SP_cache[index]->name);
                strcat(temp_response," A ");
                strcat(temp_response,SP_cache[index]->ip);
                strcat(temp_response," ");
                sprintf(temp_int,"%d",SP_cache[index]->ttl);
                strcat(temp_response,temp_int);
                response_value[n_response_value]=strdup(temp_response);
                
                index++; // search for next
            }
        }

        // devolver resposta do encontrado
        if(found){
            q_query->n_values=n_response_value+1;
            q_query->n_authorities=n_authorities_value+1;
            q_query->n_extra_values=n_extra_value+1;
            q_query->response_values=response_value;
            q_query->authorities_values=authorities_value;
            q_query->extra_values=extra_value;
        }

        
        if(!found) { // perguntar a dominio de topo

            for(current_ST=STlist;!found && current_ST;current_ST=current_ST->next){
                if(!remove_dominio(current_ST->domain,r_query->info_name,nome)){

                    // temos match de um dominio, enviamos a pergunta que nos foi
                    // feita e esperamos a resposta na resposta que iremos dar
                    found=send_udp_query(r_query,q_query,current_ST->ip,current_ST->port);
                    // resolver, por isso so' uma resposta com nome e' valida (pode ser uma resposta sem informacao)
                    if(found && q_query->n_values>0){
                        for(i=0;i<q_query->n_values;i++){
                            if(sscanf(q_query->response_values[i],"%s A %s %d",nome,ip,&ttl)==3){
                                push_record(nome,ip,A_CACHE,OTHERS,ttl,0,SP_cache,N_CACHE);
                            };
                        }
                    } else {
                        if(found && q_query->n_authorities>0){
                            for(i=0;i<q_query->n_authorities;i++){
                                if(sscanf(q_query->authorities_values[i],"%s NS %s",nome_subdominio,nome)==2){
                                    if(!remove_dominio(nome_subdominio,r_query->info_name,NULL)){ // encontrei a quem perguntar, procurar o ip nos extras
                                        for(j=0;j<q_query->n_extra_values;j++){
                                            if(sscanf(q_query->extra_values[j],"%s A %s %d",nome_server,ip,&ttl)==3){
                                                if(!strcmp(nome,nome_server)){
                                                    if(IP_parser(ip,ip_field,&ip_port, SRconfig.port)){
                                                        found=send_udp_query(r_query,q2_query,ip_field,ip_port);
                                                        if(found && q2_query->n_values>0){
                                                            for(k=0;k<q2_query->n_values;k++){
                                                                if(sscanf(q2_query->response_values[k],"%s A %s %d",nome,ip,&ttl)==3){
                                                                    push_record(nome,ip,A_CACHE,OTHERS,ttl,0,SP_cache,cache_size);
                                                                }
                                                            }
                                                            memcpy(q_query,q2_query,sizeof(struct query));
                                                        } else {
                                                            if(found && q2_query->n_authorities>0){
                                                                for(i=0;i<q2_query->n_authorities;i++){
                                                                    if(sscanf(q2_query->authorities_values[i],"%s NS %s",nome_subdominio,nome)==2){
                                                                        if(!remove_dominio(nome_subdominio,r_query->info_name,NULL)){ // encontrei a quem perguntar, procurar o ip nos extras
                                                                            for(j=0;j<q2_query->n_extra_values;j++){
                                                                                if(sscanf(q2_query->extra_values[j],"%s A %s %d",nome_server,ip,&ttl)==3){
                                                                                    if(!strcmp(nome,nome_server)){
                                                                                        if(IP_parser(ip,ip_field,&ip_port, SRconfig.port)){
                                                                                            found=send_udp_query(r_query,q3_query,ip_field,ip_port);
                                                                                            if(found && q3_query->n_values>0){
                                                                                                for(k=0;k<q3_query->n_values;k++){
                                                                                                    if(sscanf(q3_query->response_values[k],"%s A %s %d",nome,ip,&ttl)==3){
                                                                                                        push_record(nome,ip,A_CACHE,OTHERS,ttl,0,SP_cache,cache_size);
                                                                                                    }
                                                                                                }
                                                                                                memcpy(q_query,q3_query,sizeof(struct query));
                                                                                            } else {
                                                                                                found=0;
                                                                                            }
                                                                                        }
                                                                                    }
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                            } else {
                                                                found=0;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            found=0;
                        }
                    }
                }
            }

        }


    } // fim da procura do tipo A

    if(!strcmp(r_query->info_type_of_name,"MX")){
        // simplesmente procurar na cache, mas nao fazemos ideia de dominios auth
        index=0;
        while(index>=0 && SP_cache){
            index=pop_record(r_query->info_name,MX_CACHE,index,SP_cache,cache_size);           
            if(index>=0){
                found=1;
                n_response_value++;
                strcpy(temp_response,SP_cache[index]->name);
                strcat(temp_response," MX ");
                strcat(temp_response,SP_cache[index]->ip);
                strcat(temp_response," ");
                sprintf(temp_int,"%d",SP_cache[index]->ttl);
                strcat(temp_response,temp_int);
                strcat(temp_response," ");
                sprintf(temp_int,"%d",SP_cache[index]->order);
                strcat(temp_response,temp_int);
                response_value[n_response_value]=strdup(temp_response);
                
                index++; // search for next
            }
        }

        // devolver resposta do encontrado
        if(found){
            q_query->n_values=n_response_value+1;
            q_query->n_authorities=n_authorities_value+1;
            q_query->n_extra_values=n_extra_value+1;
            q_query->response_values=response_value;
            q_query->authorities_values=authorities_value;
            q_query->extra_values=extra_value;
        }
        
        if(!found){// procura por MX
            for(current_ST=STlist;!found && current_ST;current_ST=current_ST->next){
                if(!remove_dominio(current_ST->domain,r_query->info_name,nome)){

                    // temos match de um dominio, enviamos a pergunta que nos foi
                    // feita e esperamos a resposta na resposta que iremos dar
                    found=send_udp_query(r_query,q_query,current_ST->ip,current_ST->port);
                    // resolver, por isso so' uma resposta com nome e' valida (pode ser uma resposta sem informacao)
                    if(found && q_query->n_values>0){
                        for(i=0;i<q_query->n_values;i++){
                            if(sscanf(q_query->response_values[i],"%s MX %s %d %d",nome,ip,&ttl,&ordem)==4){
                                push_record(nome,ip,MX_CACHE,OTHERS,ttl,ordem,SP_cache,cache_size);
                            }
                        }
                    } else {
                        if(found && q_query->n_authorities>0){
                            for(i=0;i<q_query->n_authorities;i++){
                                if(sscanf(q_query->authorities_values[i],"%s NS %s",nome_subdominio,nome)==2){
                                    if(!strcmp(nome_subdominio,r_query->info_name)){ // encontrei a quem perguntar, procurar o ip nos extras
                                        for(j=0;j<q_query->n_extra_values;j++){
                                            if(sscanf(q_query->extra_values[j],"%s A %s %d",nome_server,ip,&ttl)==3){
                                                if(!strcmp(nome,nome_server)){
                                                    if(IP_parser(ip,ip_field,&ip_port, SRconfig.port)){
                                                        found=send_udp_query(r_query,q2_query,ip_field,ip_port);
                                                        if(found && q2_query->n_values>0){
                                                            for(k=0;k<q2_query->n_values;k++){
                                                                if(sscanf(q2_query->response_values[k],"%s MX %s %d %d",nome,ip,&ttl,&ordem)==4){
                                                                    push_record(nome,ip,MX_CACHE,OTHERS,ttl,ordem,SP_cache,cache_size);
                                                                }
                                                            }
                                                            memcpy(q_query,q2_query,sizeof(struct query));
                                                        } else {
                                                            found=0;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            found=0;
                        }
                    }
                }
            }
        }
    }  // fim de tipo MX

    // vericar numero da reposta
    tmp_response=print_query(q_query);

    sendto(udpfd, tmp_response, (size_t)strlen(tmp_response), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));

    sprintf(log_msg,"RR %s: %s", inet_ntoa(cliaddr.sin_addr), print_query(r_query));
    write_my_log(log_msg);
    sprintf(log_msg,"RP 127.0.0.1: %s", print_query(q_query));
    write_my_log(log_msg);
                

  }

  //pthread_exit(NULL);
  return NULL;
}


// le as queries em TCP e UDP
void read_queries(){
    int udpfd, nready, maxfdp, wakeup;
    char buffer[1024];
    fd_set rset;
    struct sockaddr_in servaddr;
    struct timeval timeout;
    pthread_t thread_id;
    time_t now;
    

    // socket UDP
    udpfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(udpfd<0){
        sprintf(buffer,"SP 127.0.0.1 unable to create UDP socket");
        write_my_log(buffer);
        exit(1);
    }

    sprintf(buffer,"EV 127.0.0.1 UDP socket");
    write_my_log(buffer);

    bzero(&servaddr, sizeof(servaddr));//inicializar a estrutura a zeros
    servaddr.sin_family = AF_INET; // IP
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // aceita connecoes de todos
    servaddr.sin_port = htons(SRconfig.port); // no port default


    // binding server addr structure to udp sockfd
    bind(udpfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
 
    sprintf(buffer,"EV 127.0.0.1 UDP bind on port %d",SRconfig.port);
    write_my_log(buffer);

    // clear the descriptor set
    FD_ZERO(&rset);
 
    

    while(1) { // ciclo para estar sempre a ler queries DNS

        FD_SET(udpfd, &rset);


        // select the ready descriptor
        nready = select(udpfd+1, &rset, NULL, NULL, NULL); // leitura sem timeout, o select monitora todas as alteraçoes nos file descriptors abaixo do numero dado
        if(nready<0){
            sprintf(buffer,"SP 127.0.0.1 error on TCP/UDP select");
            write_my_log(buffer);
            //exit(1);
        }

        if(nready>0){

            if (FD_ISSET(udpfd, &rset)) { // ligacao UDP
            //pthread_create(&thread_id, NULL, reply_udp, &udpfd);
                reply_udp(&udpfd); // para puder usar o gbd
            }
        }
    }

}
// nc -t localhost 1234 para testar conexao tcp
//nc -u localhost 1234 para testar conexao udp



// le os servidores de topo
void read_STdbfiles(){
  FILE *dbfile;
  char linha[1024], *linha_tmp;
  char *campo1;
  STDBFILES current=STdbfiles;
  STENTRY newST;
  char comment[1024];
  int linenumber, valid_ip;

  if(!current){// sou um resolver e nao conheco nada?!, entao sair
    sprintf(comment,"SP 127.0.0.1 resolver without root servers to ask queries");
    write_my_log(comment);
    exit(1);
  }
  
  while(current){

    dbfile=fopen(current->dbfile,"r");
    if(!dbfile){
      sprintf(comment,"SP 127.0.0.1 unable to open ST DB file %s",current->dbfile);
      write_my_log(comment);
      exit(1);
    }

    // ler ficheiro
    linenumber=0;
    while(fgets(linha, sizeof(linha), dbfile)){
        linenumber++;
        
        if(linha[0]!='#' && linha[0]!='\n'){
            linha_tmp=linha;
            linha[strlen(linha)-1]=0; // remover o enter final
            
            campo1=strsep(&linha_tmp," ");

            newST=malloc(sizeof(struct ST_entry));

            if(!newST){
                sprintf(comment,"SP 127.0.0.1 unable to allocate memory for ST");
		        write_my_log(comment);
		        exit(1);
            }
	        
            valid_ip=IP_parser(campo1, newST->ip, &(newST->port), SRconfig.port);

            if(!valid_ip){
                sprintf(comment,"SP 127.0.0.1 line %d in ST file %s has an invalid IP address", linenumber,current->dbfile);
                write_my_log(comment);//escrever o erro no log do sp
	            exit(1);
            }

            newST->domain=strdup(linha_tmp);

            newST->next=STlist;
            STlist=newST;
        }
    }


    sprintf(comment,"EV 127.0.0.1 ST db-file-read %s", current->dbfile);
    write_my_log(comment);
    current=current->next;
  }

}



void read_conffile(){
    FILE *conffile;
    char linha[1024], *linha_tmp;
    char *campo1, *campo2, *campo3;
    STDBFILES newSTdbfile;
    int linenumber;
    char comment[1024];
    int valid_ip=0;
    

    conffile=fopen(SRconfig.conffilename,"r");

    if(!conffile){//se nao consegue abrir o ficheiro de configuração escreve no log e aborta
        sprintf(comment,"SP 127.0.0.1 unable to open conf file %s", SRconfig.conffilename);
        write_my_log(comment);
        printf("Cannot open conf file\n");
        printf("Aborting\n");
        exit(1);
    }

    linenumber=0;
    while(fgets(linha, sizeof(linha), conffile)){//percorrer todas as linhas do ficheiro
        linenumber++;
        if(linha[0]!='#' && linha[0]!='\n'){
            linha_tmp=linha;
            campo1=strsep(&linha_tmp," ");
            campo2=strsep(&linha_tmp," ");
	        if(linha_tmp[strlen(linha_tmp)-1]=='\n')
                linha_tmp[strlen(linha_tmp)-1]=0;//para tirar o /n
            campo3=linha_tmp;

            if(!campo3){//se nao existe o campo 3
                sprintf(comment,"SP 127.0.0.1 line %d in conf file is invalid", linenumber);
                write_my_log(comment);//escrever o erro no log do sp
                exit(1);
            }

	        else if(!strcmp(campo2,"ST")){
                if(strcmp(campo1,"root")){
                    sprintf(comment,"SP 127.0.0.1 line %d in conf file has an invalid ST entry", linenumber);
                    write_my_log(comment);//escrever o erro no log do sp
		            exit(1);
                }

                newSTdbfile=malloc(sizeof(struct STDbfiles));
                newSTdbfile->dbfile=strdup(campo3);//o terceiro campo é o path para o ficheiro db do dominio dado
		        newSTdbfile->next=STdbfiles;//adicionar na cabeça da ligada
                STdbfiles=newSTdbfile;//o inicio da lista passa a ser este nodo
            }

	    else {
	      sprintf(comment,"SP 127.0.0.1 line %d in conf file has invalid type", linenumber);
	      write_my_log(comment);
	      exit(1);
            }
        }
    }

    sprintf(comment,"EV 127.0.0.1 config-file-read %s", SRconfig.conffilename);
    write_my_log(comment);
    
}


int main(int argc, char **argv){
    int current_arg;
    char comment[1024];

    current_arg = 1;
     
    while (argv[current_arg]) {
        if (!strncmp(argv[current_arg], "--debug", 7)) {
            SRconfig.debug = 1;
            current_arg++;
        } else if (!strncmp(argv[current_arg], "--shy", 5)) {
            SRconfig.debug = 0;
            current_arg++;
        } else if (!strncmp(argv[current_arg], "--port", 6)) {
            if (!argv[current_arg + 1]) {
                printf("\nMissing port number\n\n");
                usage(argv[0]);
                return 1;
            }
            SRconfig.port = atoi(argv[current_arg + 1]);
            current_arg+=2;
        } else if (!strncmp(argv[current_arg], "--timeout", 9)) {
            if (!argv[current_arg + 1]) {
                printf("\nMissing timeout number\n\n");
                usage(argv[0]);
                return 1;
            }
            SRconfig.timeout = atoi(argv[current_arg + 1]);
            current_arg+=2;
         } else if (!strncmp(argv[current_arg], "--conf", 6)) {
             if (!argv[current_arg + 1]) {
                printf("\nMissing configuration filename\n\n");
                usage(argv[0]);
                return 1;
            }
            SRconfig.conffilename = argv[current_arg+1];
            current_arg+=2;
        } else if (!strncmp(argv[current_arg], "--log", 5)) {
             if (!argv[current_arg + 1]) {
                printf("\nMissing log filename\n\n");
                usage(argv[0]);
                return 1;
            }
            SRconfig.logfilename = argv[current_arg+1];
            current_arg+=2;
        } else {
            printf("Invalid option: %s\n",argv[current_arg]);
            current_arg++;
        }
    }

    if(!SRconfig.logfilename){// utilizador nao definiu um ficheiro de log
        SRconfig.logfilename="logs/SP.log"; // definimos o log por defeito
    }

    if(!SRconfig.conffilename){// utilizador nao definiu um ficheiro de configuracao
        SRconfig.conffilename="confs/SP.conf"; // definimos o ficheiro por defeito
    }

    // registar o arranque
    if(SRconfig.debug)
        sprintf(comment,"ST %d %d debug",SRconfig.port, SRconfig.timeout);
    else
        sprintf(comment,"ST %d %d shy",SRconfig.port, SRconfig.timeout);
    write_my_log(comment);//escrever no log do Resolver que iniciou

    //print_configuration();

    // ler ficheiro de configuracao
    read_conffile();

    // ler ficheiros da base de dados, com servidores de topo
    read_STdbfiles();

    // inicializar cache, calculando espaco
    cache_size=N_CACHE;

    if(init_cache_table(&SP_cache,cache_size)<0){
      sprintf(comment,"EV 127.0.0.1 SP unable to initialize cache with size %d",cache_size);
      write_my_log(comment);
      SP_cache=NULL;
    }



    //print_cache_table(SP_cache, cache_size);
    //free_cache_table(&SP_cache, cache_size);
    
    // abre sockets e espera por queries TCP e UDP
    read_queries();

    // termina, nao deveria acontecer porque o programa nao termina quando estiver em funcionamento
    write_my_log("EV 127.0.0.1 terminating - should not happen!");
    return 0;
}
