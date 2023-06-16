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
#include "dnsServer.h"
#include "utils.h"
#include "cache.h"
#include "queries.h"

#define N_CACHE 300
#define DEFAULT_SOARETRY 3

#define maximo(a,b) (a<b? b:a)

//ver o que é o DD e a duvida acima
int cache_size;
struct SPConfig SPconfig={1,1234,NULL,NULL,20000};//modo debug/shy, porta por defeito, ficheiro de confg, ficheiro de log e timeout


SPLOGFILES SPlogfiles=NULL;//estrutura para guardar os logfiles dos dominios
SPDBFILES SPdbfiles=NULL;//estrutura para guardar os ficheiros db dos dominios
STDBFILES STdbfiles=NULL;//estrutura para guardar os ficheiros db dos ST
MYSPS SPlist=NULL;//estrutura para guardar os servidores que sao SPs deste
MYSSS SSlist=NULL;//estrutura para guardar os servidores que sao SSs deste
STENTRY STlist=NULL;//estrutura para guardar os dados dos servidores de topo
CACHE_RECORD *SP_cache=NULL;

void print_configuration(){
    if(SPconfig.debug)
        printf("Estou em modo debug\n");
    else
        printf("Estou em modo shy\n");

    printf("Vou usar a porta TCP %d\n",SPconfig.port);
    printf("Ficheiro log:%s\n",SPconfig.logfilename);
    printf("Ficheiro de configuracao: %s\n",SPconfig.conffilename);
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
      
    if(stat(SPconfig.logfilename, &buffer) == 0){ // log file exists
        logfile=fopen(SPconfig.logfilename,"a");
        if(!logfile){
            printf("Nao podemos prosseguir sem log\nAbortando\n");
            exit(1);
        }
        fprintf(logfile,"%02d:%02d:%04d.%02d:%02d:%02d",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec);
        fprintf(logfile," %s\n",linha);
        if(SPconfig.debug){ // modo debug imprime tambem no terminal
            printf("%02d:%02d:%04d.%02d:%02d:%02d",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec);
            printf(" %s\n",linha);
        }
    } else {
        logfile=fopen(SPconfig.logfilename,"a");
        if(!logfile){
            printf("Nao podemos prosseguir sem log\nAbortando\n");
            exit(1);
        }
        fprintf(logfile,"%02d:%02d:%04d.%02d:%02d:%02d SP log-file-created %s\n",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec,SPconfig.logfilename);
        fprintf(logfile,"%02d:%02d:%04d.%02d:%02d:%02d",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec);
        fprintf(logfile," %s\n",linha);
        if(SPconfig.debug){ // modo debug imprime tambem no terminal
            printf("%02d:%02d:%04d.%02d:%02d:%02d SP log-file-created %s\n",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec,SPconfig.logfilename);
            printf("%02d:%02d:%04d.%02d:%02d:%02d",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec);
            printf(" %s\n",linha);
        }
    }

    // é sempre possivel fechar ficheiro
    fclose(logfile);
}

void write_log(char *domain, char *linha){
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    FILE *logfile;
    struct stat buffer;
    SPLOGFILES logfiles_current, logfile_all=NULL;
    int success=0, erro=0;
    char line_buffer[1024];

    //printf("domain: %s\nlinha: %s", domain,linha);

    // remove possible last . in domain
    if(domain){
        if(domain[strlen(domain)-1]=='.')
        domain[strlen(domain)-1]=0;
    }

    for(logfiles_current=SPlogfiles;!success && !erro && logfiles_current;logfiles_current=logfiles_current->next){

        if(!strcmp(logfiles_current->domain, "all")){
	        // guardo o descritor para o default para usar mais logo
	        logfile_all=logfiles_current;
        }

        if(domain && !strcmp(domain, logfiles_current->domain)){
	        // log para este domain
	        // ver se ficheiro existe, senao cria
      
            if(stat(logfiles_current->logfile, &buffer) == 0){ // log file exists
                logfile=fopen(logfiles_current->logfile,"a");
                if(!logfile){
                    erro=1;
                }

                if(!erro){
                    success=1;
                    fprintf(logfile,"%02d:%02d:%04d.%02d:%02d:%02d",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec);
                    fprintf(logfile," %s\n",linha);
                    if(SPconfig.debug){ // modo debug imprime tambem no terminal
                        printf("%02d:%02d:%04d.%02d:%02d:%02d",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec);
                        printf(" %s\n",linha);
                    }
                }
            } else {
                logfile=fopen(logfiles_current->logfile,"a");
                if(!logfile){
                    erro=1;
                }
                if(!erro){
                    success=1;
                    fprintf(logfile,"%02d:%02d:%04d.%02d:%02d:%02d SP log-file-created %s\n",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec,SPconfig.logfilename);
                    fprintf(logfile,"%02d:%02d:%04d.%02d:%02d:%02d",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec);
                    fprintf(logfile," %s\n",linha);
                    if(SPconfig.debug){ // modo debug imprime tambem no terminal
                        printf("%02d:%02d:%04d.%02d:%02d:%02d SP log-file-created %s\n",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec,SPconfig.logfilename);
                        printf("%02d:%02d:%04d.%02d:%02d:%02d",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec);
                        printf(" %s\n",linha);
                    }
                }
            }
        }

    }

    if(!success && logfile_all){
	  // log para o domain por defeito
	  // ver se ficheiro existe, senao cria
      
	  if(stat(logfile_all->logfile, &buffer) == 0){ // log file exists
        logfile=fopen(logfile_all->logfile,"a");
        if(!logfile){
            erro=1;
        }
        
        if(!erro){
            success=1;
            fprintf(logfile,"%02d:%02d:%04d.%02d:%02d:%02d",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec);
            fprintf(logfile," %s\n",linha);
            if(SPconfig.debug){ // modo debug imprime tambem no terminal
                printf("%02d:%02d:%04d.%02d:%02d:%02d",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec);
                printf(" %s\n",linha);
            }
        }
	  } else {
	    logfile=fopen(logfile_all->logfile,"a");
	    if(!logfile){
	      erro=1;
	    }
	    if(!erro){
            success=1;
            fprintf(logfile,"%02d:%02d:%04d.%02d:%02d:%02d SP log-file-created %s\n",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec,SPconfig.logfilename);
            fprintf(logfile,"%02d:%02d:%04d.%02d:%02d:%02d",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec);
            fprintf(logfile," %s\n",linha);
            if(SPconfig.debug){ // modo debug imprime tambem no terminal
                printf("%02d:%02d:%04d.%02d:%02d:%02d SP log-file-created %s\n",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec,SPconfig.logfilename);
                printf("%02d:%02d:%04d.%02d:%02d:%02d",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec);
                printf(" %s\n",linha);
            }
	    }
	  }
    }

    if(!success){
      // registo no meu log...
      if(domain){
        sprintf(line_buffer,"EV 127.0.0.1 Unable to register in log for domain %s. Next line is the log entry\n",domain);
        write_my_log(line_buffer);
        write_my_log(linha);
      } else {
        sprintf(line_buffer,"EV 127.0.0.1 Unable to register in log for empty domain. Next line is the log entry\n");
        write_my_log(line_buffer);
        write_my_log(linha);
      }

    }

    if(success)
      fclose(logfile);
}



// funcao que responde a uma query por TCP. Atencao que estamos num thread. Qualquer modificacao nas variaveis globais tem consequencias.
void *reply_tcp(void *c){
    FILE *to_send;//File pointer, will be used to open the db file we want to send
    char *ip;//used to store the ip adress of the client asking for zone transfer
    char buffer[1024];//buffer to recieve message
    char str_temp[1024];//string used to get line from file to_send
    int tcpfd=*((int*)c); // descritor do ficheiro com a ligacao TCP, fechar 'a saida
    char log_msg[4096], domain[512];
    int nentries;

    struct sockaddr_in cliaddr;
    
    socklen_t len = sizeof(cliaddr);
    
    MYSSS temp=SSlist;//aux para percorrer a lista dos SS
    
    SPDBFILES db_temp=SPdbfiles;//aux para percorrer a lista dos db files
    int autorizado=0; //se autorizado passar a zero então o servidor pode pedir transferencia de zona
    
    int connfd = accept(tcpfd, (struct sockaddr*)&cliaddr, &len);
    
    ip = inet_ntoa(cliaddr.sin_addr); //get client IP adress

    bzero(buffer, sizeof(buffer));
    bzero(domain, sizeof(domain));
    
    read(connfd, buffer, sizeof(buffer));//read to buffer
    sscanf(buffer,"domain: %s",domain);

    //percorrer a lista dos SS para verificar se quem está a pedir tem autorização para o dominio recebido no buffer
    for(;temp && autorizado==0;temp=temp->next){
        removeEnters(domain);//no telnet fica la um \r
        removeEnters(temp->domain);

        if((!strcmp(temp->domain,domain)) && (!strcmp(temp->ip_address,ip))) autorizado=1;
    }

    if(autorizado){//if client has autorization...
        for(;db_temp;db_temp=db_temp->next){
            if(!strcmp(db_temp->domain,domain)) break;//found the asked domain on my db files
        }
        if(db_temp){//open file and send everything
	    bzero(buffer, sizeof(buffer));
	    sprintf(buffer,"soaserial: %s",db_temp->SOASERIAL);
	    write(connfd,buffer,strlen(buffer));
            bzero(buffer, sizeof(buffer));
	    read(connfd,buffer,1024);
            if(sscanf(buffer,"soaserial: %s", db_temp->SOASERIAL)>0){
	      bzero(buffer, sizeof(buffer));
	      sprintf(buffer,"entries: %d",db_temp->linenumbers);
	      write(connfd,buffer,strlen(buffer));
	      bzero(buffer, sizeof(buffer));
	      read(connfd,buffer,1024);
	      sscanf(buffer,"ok: %d", &nentries);
	      printf("Enviar %d entradas\n",nentries);
	      if(nentries==db_temp->linenumbers){
                to_send = fopen(db_temp->dbfile,"r");
                while(fgets(str_temp, sizeof(str_temp), to_send)){
		  if(str_temp[0]!='#' && str_temp[0]!='\n'){
                    write(connfd,str_temp,strlen(str_temp));
		    fsync(connfd);
		  }
		  
                }
                sprintf(log_msg,"ZT 127.0.0.1 Domain %s transfer to %s", domain,ip);
                write_my_log(log_msg);
	      }
	    } else {
	      sprintf(log_msg,"ZT 127.0.0.1 Domain %s transfer to %s not made due to version %s", domain,ip, db_temp->SOASERIAL);
	      write_my_log(log_msg);
	    }
	} else {
                sprintf(log_msg,"ZT 127.0.0.1 Failed domain %s transfer to %s", domain,ip);
                write_my_log(log_msg);
	}
    } else {
        sprintf(log_msg,"EZ 127.0.0.1 Failed domain %s transfer to %s, domain does not exist or client is not authorized",ip,buffer);
        write_my_log(log_msg);
    }

    close(connfd);
    pthread_exit(NULL);
    return NULL;
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
  CNAMEENTRY current_cname=NULL;
  SPDBFILES current;
  MYSPS current_SS;
  STENTRY current_ST;
  char nome[128],nome_subdominio[128];

  char *response_value[10],temp_response[1024],temp_int[20];
  char *authorities_value[10], *extra_value[10];
  int index=0, n_response_value, n_authorities_value, n_extra_value;
  char *tmp_response, *domain=NULL;
  int found;

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


    if(!strcmp(r_query->info_type_of_name,"A")){
        // percorrer todos os dominios em que sou authority
        for(current=SPdbfiles;!found && current;current=current->next){
            // verificar se dominio faz match (procura inversa)
            if(!remove_dominio(current->domain,r_query->info_name,nome)){
	            strcpy(nome_subdominio,nome); // guardar porque pode ser sub dominio

                // fez match do dominio, no nome esta' o restante para procura (pode ser CNAME, A ou NS)
                for(current_cname=current->cname_entry;current_cname && strcmp(nome,current_cname->alias);current_cname=current_cname->next);
                
                if(current_cname){// temos um alias
                    // adicionar um response value
                    n_response_value++;
                    strcpy(temp_response,current_cname->name);
                    strcat(temp_response,".");
                    strcat(temp_response,current->domain);
                    strcat(temp_response,". CNAME ");
                    strcat(temp_response,nome);
                    strcat(temp_response,".");
                    strcat(temp_response,current->domain);
                    strcat(temp_response,".");
                    
                    response_value[n_response_value]=strdup(temp_response);

                    strcpy(nome,current_cname->name);
                }

                // add domain with dot
                strcat(nome,".");
                strcat(nome,current->domain);
                strcat(nome,".");

                index=0;
                while(index>=0){
                    index=pop_record(nome,A_CACHE,index,SP_cache,cache_size);           
                    if(index>=0){

                        n_response_value++;
                        strcpy(temp_response,nome);
                        strcat(temp_response," A ");
                        strcat(temp_response,SP_cache[index]->ip);
                        strcat(temp_response," ");
                        sprintf(temp_int,"%d",SP_cache[index]->ttl);
                        strcat(temp_response,temp_int);
                        response_value[n_response_value]=strdup(temp_response);
                        
                        index++; // search for next
                    }
                }

                // ver se e' subdominio
                for(NS_sub nsd=current->ns_sub_domain;nsd;nsd=nsd->next){
                    if(!remove_dominio(nsd->name,nome_subdominio,nome)){
                        n_authorities_value++;
                        strcpy(nome,nsd->name);
                        adiciona_at(nome,current->domain);
                        strcpy(temp_response,nome);
                        strcat(temp_response,". NS ");
                        strcat(temp_response,nsd->server);
                        //strcat(temp_response," ");
                        //sprintf(temp_int,"%d",nsd->ttl); // subdominos sem TTL
                        //strcat(temp_response,temp_int);
                        authorities_value[n_authorities_value]=strdup(temp_response);
                        // adicionar os extra...
                        index=0;
                        while(index>=0){
                            index=pop_record(nsd->server,A_CACHE,index,SP_cache,cache_size);           
                            if(index>=0){
                                n_extra_value++;
                                strcpy(temp_response,nsd->server);
                                strcat(temp_response," A ");
                                strcat(temp_response,SP_cache[index]->ip);
                                strcat(temp_response," ");
                                sprintf(temp_int,"%d",SP_cache[index]->ttl);
                                strcat(temp_response,temp_int);
                                extra_value[n_extra_value]=strdup(temp_response);
                                
                                index++; // search for next
                            }
                        }
                        
                    }
                }

                // sou autoridade sobre esse nome, por isso devolvo os NS nos AUTHORITIES, se nao for subdominio
                if(n_authorities_value<0){
                    for(NS ns=current->ns_servers;ns;ns=ns->next){
                        n_authorities_value++;
                        strcpy(temp_response,current->domain);
                        strcat(temp_response,". NS ");
                        strcat(temp_response,ns->name);
                        strcat(temp_response," ");
                        sprintf(temp_int,"%d",ns->ttl);
                        strcat(temp_response,temp_int);
                        authorities_value[n_authorities_value]=strdup(temp_response);
                        // adicionar os extra...
                        index=0;
                        while(index>=0){
                            index=pop_record(ns->name,A_CACHE,index,SP_cache,cache_size);           
                            if(index>=0){
                                n_extra_value++;
                                strcpy(temp_response,ns->name);
                                strcat(temp_response," A ");
                                strcat(temp_response,SP_cache[index]->ip);
                                strcat(temp_response," ");
                                sprintf(temp_int,"%d",SP_cache[index]->ttl);
                                strcat(temp_response,temp_int);
                                extra_value[n_extra_value]=strdup(temp_response);
                                
                                index++; // search for next
                            }
                        }
                    }
                }

                // devolver resposta do encontrado (construir resposta autoritaria)
                q_query->n_values=n_response_value+1;
                q_query->n_authorities=n_authorities_value+1;
                q_query->n_extra_values=n_extra_value+1;
                q_query->response_values=response_value;
                q_query->authorities_values=authorities_value;
                q_query->extra_values=extra_value;

                found = 1; // encontrei e nao vale a pena continuar
                domain=current->domain; // para o log

            } // else nao fez match com este dominio e por isso nao ha nada a devolver, continuar noutro dominio

        } // terminei na procura dos dominios authorities

        if(!found){ // verificar nos dominios em que sou SS
            for(current_SS=SPlist;!found && current_SS;current_SS=current_SS->next){
                if(current_SS->data){
                    current=current_SS->data;
                    // verificar se dominio faz match (procura inversa)
                    if(!remove_dominio(current->domain,r_query->info_name,nome)){
                        strcpy(nome_subdominio,nome); // guardar porque pode ser sub dominio
                    
                        // fez match do dominio, no nome esta' o restante para procura (pode ser CNAME, A ou NS)
                        for(current_cname=current->cname_entry;current_cname && strcmp(nome,current_cname->alias);current_cname=current_cname->next);

                        if(current_cname){// temos um alias
                            // adicionar um response value
                            n_response_value++;
                            strcpy(temp_response,current_cname->name);
                            strcat(temp_response,".");
                            strcat(temp_response,current->domain);
                            strcat(temp_response,". CNAME ");
                            strcat(temp_response,nome);
                            strcat(temp_response,".");
                            strcat(temp_response,current->domain);
                            strcat(temp_response,".");
                                    
                            response_value[n_response_value]=strdup(temp_response);
                            
                            strcpy(nome,current_cname->name);
                        }
                
                        // add domain with dot
                        strcat(nome,".");
                        strcat(nome,current->domain);
                        strcat(nome,".");
                
                        index=0;
                        while(index>=0){
                        index=pop_record(nome,A_CACHE,index,SP_cache,cache_size);           
                        if(index>=0){
                            
                            n_response_value++;
                            strcpy(temp_response,nome);
                            strcat(temp_response," A ");
                            strcat(temp_response,SP_cache[index]->ip);
                            strcat(temp_response," ");
                            sprintf(temp_int,"%d",SP_cache[index]->ttl);
                            strcat(temp_response,temp_int);
                            response_value[n_response_value]=strdup(temp_response);
                                    
                            index++; // search for next
                        }
                    }
            
                    // ver se e' subdominio
                    for(NS_sub nsd=current->ns_sub_domain;nsd;nsd=nsd->next){
                        if(!remove_dominio(nsd->name,nome_subdominio,nome)){
                            n_authorities_value++;
                            strcpy(nome,nsd->name);
                            adiciona_at(nome,current->domain);
                            strcpy(temp_response,nome);
                            strcat(temp_response,". NS ");
                            strcat(temp_response,nsd->server);
                            //strcat(temp_response," ");
                            //sprintf(temp_int,"%d",nsd->ttl); // subdominos sem TTL
                            //strcat(temp_response,temp_int);
                            authorities_value[n_authorities_value]=strdup(temp_response);
                            // adicionar os extra...
                            index=0;
                            while(index>=0){
                                index=pop_record(nsd->server,A_CACHE,index,SP_cache,cache_size);           
                                if(index>=0){
                                    n_extra_value++;
                                    strcpy(temp_response,nsd->server);
                                    strcat(temp_response," A ");
                                    strcat(temp_response,SP_cache[index]->ip);
                                    strcat(temp_response," ");
                                    sprintf(temp_int,"%d",SP_cache[index]->ttl);
                                    strcat(temp_response,temp_int);
                                    extra_value[n_extra_value]=strdup(temp_response);
                                    
                                    index++; // search for next
                                }
                            }                            
                        }
                    }
            
                    // sou autoridade sobre esse nome, por isso devolvo os NS nos AUTHORITIES, se nao for subdominio
                    if(n_authorities_value<0){
                        for(NS ns=current->ns_servers;ns;ns=ns->next){
                            n_authorities_value++;
                            strcpy(temp_response,current->domain);
                            strcat(temp_response,". NS ");
                            strcat(temp_response,ns->name);
                            strcat(temp_response," ");
                            sprintf(temp_int,"%d",ns->ttl);
                            strcat(temp_response,temp_int);
                            authorities_value[n_authorities_value]=strdup(temp_response);
                            // adicionar os extra...
                            index=0;
                            while(index>=0){
                                index=pop_record(ns->name,A_CACHE,index,SP_cache,cache_size);           
                                if(index>=0){
                                    n_extra_value++;
                                    strcpy(temp_response,ns->name);
                                    strcat(temp_response," A ");
                                    strcat(temp_response,SP_cache[index]->ip);
                                    strcat(temp_response," ");
                                    sprintf(temp_int,"%d",SP_cache[index]->ttl);
                                    strcat(temp_response,temp_int);
                                    extra_value[n_extra_value]=strdup(temp_response);
                                    
                                    index++; // search for next
                                }
                            }
                        }
                    }
            
                    // devolver resposta do encontrado (construir resposta autoritaria)
                    q_query->n_values=n_response_value+1;
                    q_query->n_authorities=n_authorities_value+1;
                    q_query->n_extra_values=n_extra_value+1;
                    q_query->response_values=response_value;
                    q_query->authorities_values=authorities_value;
                    q_query->extra_values=extra_value;
                    
                    found = 1; // encontrei e nao vale a pena continuar
                    domain=current_SS->domain;
		
                } // else nao fez match com este dominio e por isso nao ha nada a devolver, continuar noutro dominio
        
            } // fim do current_SS data
        
        } // terminei na procura dos dominios secundarios
    
    } // else encontrou e e' para terminar

        if(!found){ // nem dominio auth nem SS
            // simplesmente procurar na cache, mas nao fazemos ideia de dominios auth
            index=0;
            while(index>=0){
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

            // devolver resposta do encontrado (construir resposta autoritaria)
            if(found){
                q_query->n_values=n_response_value+1;
                q_query->n_authorities=n_authorities_value+1;
                q_query->n_extra_values=n_extra_value+1;
                q_query->response_values=response_value;
                q_query->authorities_values=authorities_value;
                q_query->extra_values=extra_value;
            }
        }
        
        if(!found) { // devolver dominios de topo ou perguntar a alguem!
            // se for recursivo deviamos enviar a r_query para um servidor de topo, caso exista
            // Devolver a lista de servidores de topo
            printf("Passei aqui\n");
            for(current_ST=STlist;current_ST;current_ST=current_ST->next){
                if(!compara_dominio(current_ST->domain,r_query->info_name)){
                    n_extra_value++;
                    strcpy(temp_response,current_ST->domain);
                    strcat(temp_response," A ");
                    strcat(temp_response,current_ST->ip);
                    strcat(temp_response,":");
                    sprintf(temp_int,"%d",current_ST->port);
                    strcat(temp_response,temp_int);
                    extra_value[n_extra_value]=strdup(temp_response);
                    found=1;
                }
            }

            if(found){
                q_query->n_values=n_response_value+1;
                q_query->n_authorities=n_authorities_value+1;
                q_query->n_extra_values=n_extra_value+1;
                q_query->response_values=response_value;
                q_query->authorities_values=authorities_value;
                q_query->extra_values=extra_value;
            }
        }

    } // fim da procura do tipo A

    if(!strcmp(r_query->info_type_of_name,"MX")){
        // procura por MX
        // percorrer todos os dominios em que sou authority
        for(current=SPdbfiles;current;current=current->next){
            if(!remove_dominio(current->domain,r_query->info_name,nome)){
	            strcpy(nome_subdominio,nome); // guardar porque pode ser sub dominio
            
                if(!(*nome)){// match do dominio, adicionar todos os dominios
                    domain=r_query->info_name; // para o log
                    for(MX current_MX=current->mx_servers;current_MX;current_MX=current_MX->next){
                        n_response_value++;
                        strcpy(temp_response,current->domain);
                        strcat(temp_response,". MX ");
                        strcat(temp_response,current_MX->name);
                        strcat(temp_response," ");
                        sprintf(temp_int,"%d",current_MX->ttl);
                        strcat(temp_response,temp_int);
                        strcat(temp_response," ");
                        sprintf(temp_int,"%d",current_MX->order);
                        strcat(temp_response,temp_int);
                        response_value[n_response_value]=strdup(temp_response);
                        // adicionar os extra...
                        strcpy(nome,current_MX->name);
                        if(nome[strlen(nome)-1]!='.'){ // adiciona ponto
                            nome[strlen(nome)+1]=0;
                            nome[strlen(nome)]='.';
                        }
                        index=0;
                        while(index>=0){
                            index=pop_record(nome,A_CACHE,index,SP_cache,cache_size);           
                            if(index>=0){
                                n_extra_value++;
                                strcpy(temp_response,current_MX->name);
                                strcat(temp_response," A ");
                                strcat(temp_response,SP_cache[index]->ip);
                                strcat(temp_response," ");
                                sprintf(temp_int,"%d",SP_cache[index]->ttl);
                                strcat(temp_response,temp_int);
                                extra_value[n_extra_value]=strdup(temp_response);
                                index++; // search for next
                            }
                        }
                    }
                } else {
                    // ver se e' subdominio
                    for(NS_sub nsd=current->ns_sub_domain;nsd;nsd=nsd->next){
                        if(!remove_dominio(nsd->name,nome_subdominio,nome)){
                        n_authorities_value++;
                        strcpy(nome,nsd->name);
                        adiciona_at(nome,current->domain);
                        strcpy(temp_response,nome);
                        strcat(temp_response,". NS ");
                        strcat(temp_response,nsd->server);
                        //strcat(temp_response," ");
                        //sprintf(temp_int,"%d",nsd->ttl); // subdominos sem TTL
                        //strcat(temp_response,temp_int);
                        authorities_value[n_authorities_value]=strdup(temp_response);
                        // adicionar os extra...
                        index=0;
                        while(index>=0){
                            index=pop_record(nsd->server,A_CACHE,index,SP_cache,cache_size);           
                            if(index>=0){
                                n_extra_value++;
                                strcpy(temp_response,nsd->server);
                                strcat(temp_response," A ");
                                strcat(temp_response,SP_cache[index]->ip);
                                strcat(temp_response," ");
                                sprintf(temp_int,"%d",SP_cache[index]->ttl);
                                strcat(temp_response,temp_int);
                                extra_value[n_extra_value]=strdup(temp_response);
                                
                                index++; // search for next
                            }
                        }
                        
                    }
                    }
                }

                // tb sou a autoridade
                for(NS ns=current->ns_servers;ns;ns=ns->next){
                    n_authorities_value++;
                    strcpy(temp_response,current->domain);
                    strcat(temp_response,". NS ");
                    strcat(temp_response,ns->name);
                    strcat(temp_response," ");
                    sprintf(temp_int,"%d",ns->ttl);
                    strcat(temp_response,temp_int);
                    authorities_value[n_authorities_value]=strdup(temp_response);
                    // adicionar os extra...
                    strcpy(nome,ns->name);
                    if(nome[strlen(nome)-1]!='.'){ // adiciona ponto
                        nome[strlen(nome)+1]=0;
                        nome[strlen(nome)]='.';
                    }
                    index=0;
                    while(index>=0){
                        index=pop_record(nome,A_CACHE,index,SP_cache,cache_size);           
                        if(index>=0){
                            n_extra_value++;
                            strcpy(temp_response,ns->name);
                            strcat(temp_response," A ");
                            strcat(temp_response,SP_cache[index]->ip);
                            strcat(temp_response," ");
                            sprintf(temp_int,"%d",SP_cache[index]->ttl);
                            strcat(temp_response,temp_int);
                            extra_value[n_extra_value]=strdup(temp_response);
                                    
                            index++; // search for next
                        }
                    }
                }
            }
        } // terminei dominios auth

        // ver dominios SS
        for(current_SS=SPlist;!found && current_SS;current_SS=current_SS->next){
            if(current_SS->data){
                current=current_SS->data;
                strcpy(nome,current->domain);
                if(nome[strlen(nome)-1]!='.'){ // adiciona ponto
                    nome[strlen(nome)+1]=0;
                    nome[strlen(nome)]='.';
                }
                if(!strcmp(nome,r_query->info_name)){ // match do dominio, adicionar todos os dominios
                    domain=r_query->info_name; // para o log
                    for(MX current_MX=current->mx_servers;current_MX;current_MX=current_MX->next){
                        n_response_value++;
                        strcpy(temp_response,current->domain);
                        strcat(temp_response,". MX ");
                        strcat(temp_response,current_MX->name);
                        strcat(temp_response," ");
                        sprintf(temp_int,"%d",current_MX->ttl);
                        strcat(temp_response,temp_int);
                        strcat(temp_response," ");
                        sprintf(temp_int,"%d",current_MX->order);
                        strcat(temp_response,temp_int);
                        response_value[n_response_value]=strdup(temp_response);
                        // adicionar os extra...
                        strcpy(nome,current_MX->name);
                        if(nome[strlen(nome)-1]!='.'){ // adiciona ponto
                            nome[strlen(nome)+1]=0;
                            nome[strlen(nome)]='.';
                        }
                        index=0;
                        while(index>=0){
                            index=pop_record(nome,A_CACHE,index,SP_cache,cache_size);           
                            if(index>=0){
                                n_extra_value++;
                                strcpy(temp_response,SP_cache[index]->name);
                                strcat(temp_response," A ");
                                strcat(temp_response,SP_cache[index]->ip);
                                strcat(temp_response," ");
                                sprintf(temp_int,"%d",SP_cache[index]->ttl);
                                strcat(temp_response,temp_int);
                                extra_value[n_extra_value]=strdup(temp_response);
                                index++; // search for next
                            }
                        }
                    }
                    // tb sou a autoridade
                    for(NS ns=current->ns_servers;ns;ns=ns->next){
                        n_authorities_value++;
                        strcpy(temp_response,current->domain);
                        strcat(temp_response,". NS ");
                        strcat(temp_response,ns->name);
                        strcat(temp_response," ");
                        sprintf(temp_int,"%d",ns->ttl);
                        strcat(temp_response,temp_int);
                        authorities_value[n_authorities_value]=strdup(temp_response);
                        // adicionar os extra...
                        strcpy(nome,ns->name);
                        if(nome[strlen(nome)-1]!='.'){ // adiciona ponto
                            nome[strlen(nome)+1]=0;
                            nome[strlen(nome)]='.';
                        }
                        index=0;
                        while(index>=0){
                            index=pop_record(nome,A_CACHE,index,SP_cache,cache_size);           
                            if(index>=0){
                                n_extra_value++;
                                strcpy(temp_response,SP_cache[index]->name);
                                strcat(temp_response," A ");
                                strcat(temp_response,SP_cache[index]->ip);
                                strcat(temp_response," ");
                                sprintf(temp_int,"%d",SP_cache[index]->ttl);
                                strcat(temp_response,temp_int);
                                extra_value[n_extra_value]=strdup(temp_response);
                                        
                                index++; // search for next
                            }
                        }
                    }
                }
            }
        }

        if(n_response_value+n_authorities_value+n_extra_value==-3) { // -3, porque cada é -1, devolver dominios de topo ou perguntar a alguem!
            // se for recursivo deviamos enviar a r_query para um servidor de topo, caso exista
            // Devolver a lista de servidores de topo
            //printf("E passei aqui\n");
            for(current_ST=STlist;current_ST;current_ST=current_ST->next){
                if(!compara_dominio(current_ST->domain,r_query->info_name)){
                    n_extra_value++;
                    strcpy(temp_response,current_ST->domain);
                    strcat(temp_response," A ");
                    strcat(temp_response,current_ST->ip);
                    strcat(temp_response,":");
                    sprintf(temp_int,"%d",current_ST->port);
                    strcat(temp_response,temp_int);
                    extra_value[n_extra_value]=strdup(temp_response);
                }
            }
        }
      
      
        // devolver resposta do encontrado (construir resposta autoritaria)
        q_query->n_values=n_response_value+1;
        q_query->n_authorities=n_authorities_value+1;
        q_query->n_extra_values=n_extra_value+1;
        q_query->response_values=response_value;
        q_query->authorities_values=authorities_value;
        q_query->extra_values=extra_value;

    }  // fim de tipo MX

    tmp_response=print_query(q_query);

    sendto(udpfd, tmp_response, (size_t)strlen(tmp_response), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));

    sprintf(log_msg,"RR %s: %s", inet_ntoa(cliaddr.sin_addr), print_query(r_query));
    write_log(domain,log_msg);
    sprintf(log_msg,"RP 127.0.0.1: %s", print_query(q_query));
    write_log(domain,log_msg);
  }

  //pthread_exit(NULL);
  return NULL;
}


// le as queries em TCP e UDP
void read_queries(){
    int tcpfd, udpfd, nready, maxfdp, wakeup;
    char buffer[1024];
    fd_set rset;
    struct sockaddr_in servaddr;
    struct timeval timeout;
    pthread_t thread_id;
    MYSPS current;
    time_t now;
    
    // socket TCP
    tcpfd = socket(AF_INET, SOCK_STREAM, 0);
    if(tcpfd<0){
        sprintf(buffer,"SP 127.0.0.1 unable to create TCP socket");
        write_my_log(buffer);
        exit(1);
    }

    sprintf(buffer,"EV 127.0.0.1 TCP socket");
    write_my_log(buffer);

    bzero(&servaddr, sizeof(servaddr));//inicializar a estrutura a zeros
    servaddr.sin_family = AF_INET; // IP
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // aceita connecoes de todos
    servaddr.sin_port = htons(SPconfig.port); // no port default
 
    // binding server addr structure to tcpfd
    bind(tcpfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

    sprintf(buffer,"EV 127.0.0.1 TCP bind on port %d",SPconfig.port);
    write_my_log(buffer);
 
    sprintf(buffer,"EV 127.0.0.1 TCP listening");
    write_my_log(buffer);

    // socket UDP
    udpfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(udpfd<0){
        sprintf(buffer,"SP 127.0.0.1 unable to create UDP socket");
        write_my_log(buffer);
        exit(1);
    }

    sprintf(buffer,"EV 127.0.0.1 UDP socket");
    write_my_log(buffer);

    // binding server addr structure to udp sockfd
    bind(udpfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
 
    sprintf(buffer,"EV 127.0.0.1 UDP bind on port %d",SPconfig.port);
    write_my_log(buffer);

    // clear the descriptor set
    FD_ZERO(&rset);
 
    // get maxfd
    if(tcpfd>udpfd)//ver qual o maior file descriptor para usar no select
        maxfdp = tcpfd+1;
    else
        maxfdp = udpfd + 1;
    

    while(1) { // ciclo para estar sempre a ler queries DNS

        // verificar e atualizar os SSs
        read_SSdbfiles();
        
        // inserir todos os registos A dos SSs na cache
        if(SP_cache){
            for(MYSPS current_SS=SPlist;current_SS;current_SS=current_SS->next){
                if(current_SS->data){
                    for(AENTRY current_a=current_SS->data->a_entry;current_a;current_a=current_a->next){
                        sprintf(buffer,"%s.%s.",current_a->name,current_SS->domain);
                        push_record(buffer,current_a->ip,A_CACHE,SP,current_a->ttl,current_a->order, SP_cache, cache_size);
                    }
                }
            }
        }

        now=time(NULL);
        wakeup=3600; // +infty
        for(current=SPlist;current;current=current->next){
            if(maximo((current->last_load+current->ttl)-now,(current->last_try+current->SOARETRY)-now)<wakeup)
                wakeup=maximo((current->last_load+current->ttl)-now,(current->last_try+current->SOARETRY)-now);
        }
        wakeup++; // add one second

        if(wakeup<=0)
            wakeup=1; // espera pelo menos um segundo....

        printf("wakeup=%d\n",wakeup);

        timeout.tv_sec=wakeup;
        timeout.tv_usec=0;

        // set tcpfd and udpfd in readset
        FD_SET(tcpfd, &rset);
        FD_SET(udpfd, &rset);


        // select the ready descriptor
        nready = select(maxfdp, &rset, NULL, NULL, &timeout); // leitura sem timeout, o select monitora todas as alteraçoes nos file descriptors abaixo do numero dado
        if(nready<0){
            sprintf(buffer,"SP 127.0.0.1 error on TCP/UDP select");
            write_my_log(buffer);
            //exit(1);
        }

        if(nready>0){
            //testar o return
            listen(tcpfd, 10); // aceitamos no maximo 10 ligacoes em simultaneo (serao de apenas SSs)
            
            if (FD_ISSET(tcpfd, &rset)) { // ligacao TCP
                pthread_create(&thread_id, NULL, reply_tcp, &tcpfd); // o thread fecha o descritor de ficheiro
                //reply_tcp(&tcpfd); //para fazer debug
            }

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
	        
            valid_ip=IP_parser(campo1, newST->ip, &(newST->port), SPconfig.port);

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

int connectSP(char *domain, char *ip_address, int port){
    int tcpfd;
    char buffer[1024];
    struct sockaddr_in servaddr;
    socklen_t len=sizeof(servaddr);
    
    // socket TCP
    tcpfd = socket(AF_INET, SOCK_STREAM, 0);
    if(tcpfd<0){
        sprintf(buffer,"SP 127.0.0.1 unable to create TCP socket for SS %s",ip_address);
        write_my_log(buffer);
        return -1;
    }

    bzero(&servaddr, len);//inicializar a estrutura a zeros
    servaddr.sin_family = AF_INET; // IP
    if(!inet_aton(ip_address, &servaddr.sin_addr))
        return -1; // aceita connecoes de todos
    servaddr.sin_port = htons(port); // no port default
 
    // connect the client socket to server socket
    if (connect(tcpfd, (struct sockaddr *)&servaddr, len)!= 0) {
        printf("connection with the server failed...\n");
        tcpfd=-1;  
    }

    return tcpfd;
 }

// sou um servidor SS e leio os ficheiros DB do SP respetivo
void read_SSdbfiles(){
    MYSPS current;
    SPDBFILES current_db;
    time_t now;
    int tcp_fd; // descritor de ficheiro para ler
    char linha[1024],comment[1024];
    int linenumber,nentries;
    FILE *tcp_fp;

    now=time(NULL);

    // fazer ciclo e verificar o que nao está up-to-date. Esta rotina pode ser chamada varias vezes
    for(current=SPlist;current;current=current->next){
        if(now-current->last_load>current->ttl && now-current->last_try>current->SOARETRY){ // após ttl dos dados e após tempo até retry
            // fora de prazo e vamos atualizar a memoria
            current->last_try=now;
            tcp_fd=connectSP(current->domain,current->ip_address,current->port_number);
            if(tcp_fd>0){
                // sucesso na ligacao TCP.... comunicar e obter as linhas
	            bzero(linha,sizeof(linha));
	            sprintf(linha,"domain: %s",current->domain);
                write(tcp_fd,linha,strlen(linha));
		        bzero(linha,sizeof(linha));
		        read(tcp_fd,linha,1024);
		        bzero(comment,sizeof(comment));
                sscanf(linha,"soaserial: %s",comment);

		if(current->data==NULL || strcmp(comment,current->data->SOASERIAL)){ // se nunca foi carregado ou e' uma versao diferente
		  // uso o proprio texto recebido
		  write(tcp_fd,linha,strlen(linha));
		  bzero(linha,sizeof(linha));
		  read(tcp_fd,linha,1024);
		  sscanf(linha,"entries: %d",&nentries);
		  // deveriamos verificar!
		  bzero(linha,sizeof(linha));
		  sprintf(linha,"ok: %d",nentries);
		  write(tcp_fd,linha,strlen(linha));
		  
		  if(current->data)
                    free_db(current->data);
		  
		  current->data=malloc(sizeof(struct SPDbfiles));
		  bzero(current->data,sizeof(struct SPDbfiles));
		  current_db=current->data;
		  current_db->domain=strdup((const char*)current->domain);
		  
		  if(current_db){
		    
                    // ler descritor ficheiro TCP
                    linenumber=0;
		    tcp_fp=fdopen(tcp_fd,"r");
                    while(fgets(linha,sizeof(linha),tcp_fp)){
		      linenumber++;
		      le_linha_db(current_db,linenumber,linha);
		      //bzero(linha,sizeof(linha));
                    }

                    current_db->linenumbers=linenumber;
		    
                    sprintf(comment,"EV 127.0.0.1 SS db-TCP-read for domain %s with %d lines", current->domain,linenumber);
                    write_my_log(comment);
                    
                    now=time(NULL);
                    current->last_load=now;
                    current->last_try=0;
                    // update SOARETRY, if set up
                    if(current_db->SOARETRY>0)
		      current->SOARETRY=current_db->SOARETRY;
                    if(current_db->TTL_DEFAULT>0)
		      current->ttl=current_db->TTL_DEFAULT;
		  }
                } else { // ja tinha lido e a versao e' igual, entao registar
                    now=time(NULL);
                    current->last_load=now;
                    current->last_try=0;
		    sprintf(comment,"EV 127.0.0.1 SS not db-TCP-read for domain %s due to SOASERIAL", current->domain);
		    write_my_log(comment);
		    
		}
            } else {
	      sprintf(comment,"EV 127.0.0.1 SS failed db-TCP-read for domain %s", current->domain);
	      write_my_log(comment);
	    }
            // close TCP connection
            close(tcp_fd);
        }
    }
}

void init_db(SPDBFILES current){
    // inicializa estrutura
    current->a_n=0;
    current->default_domain=NULL;
    current->a_entry=NULL;
    current->mx_servers=NULL;
    current->ns_servers=NULL;
    current->ns_sub_domain=NULL;
    current->cname_entry=NULL;
    }

// libertar db
void free_db(SPDBFILES current){
    NS ns_servers;
    MX mx_servers;
    NS_sub ns_sub_domain;
    AENTRY a_entry;
    CNAMEENTRY cname_entry;

    if(!current)
        return;

    if(current->domain)
        free(current->domain);
    if(current->dbfile)
        free(current->dbfile);
    if(current->default_domain)
        free(current->default_domain);
    if(current->SOASP)
        free(current->SOASP);
    if(current->SOAADMIN)
        free(current->SOAADMIN);
    if(current->SOASERIAL)
        free(current->SOASERIAL);

    if(current->ns_servers)
        while(current->ns_servers){
            ns_servers=current->ns_servers->next;
            free(current->ns_servers);
            current->ns_servers=ns_servers;
        }
    if(current->ns_sub_domain)
        while(current->ns_sub_domain){
            ns_sub_domain=current->ns_sub_domain->next;
            free(current->ns_sub_domain);
            current->ns_sub_domain=ns_sub_domain;
        }

    if(current->mx_servers)
        while(current->mx_servers){
            mx_servers=current->mx_servers->next;
            free(current->mx_servers);
            current->mx_servers=mx_servers;
        }

    if(current->a_entry)
        while(current->a_entry){
            a_entry=current->a_entry->next;
            free(current->a_entry);
            current->a_entry=a_entry;
        }

    if(current->cname_entry)
        while(current->cname_entry){
            cname_entry=current->cname_entry->next;
            free(current->cname_entry);
            current->cname_entry=cname_entry;
        }

    free(current);
}

// sou um servidor SP e leio os ficheiros DB
void read_SPdbfiles(){
  FILE *dbfile;
  char linha[1024];
  SPDBFILES current=SPdbfiles;
  char comment[1024];
  int linenumber,validlines;

  while(current){

    dbfile=fopen(current->dbfile,"r");
    if(!dbfile){
      sprintf(comment,"SP 127.0.0.1 unable to open SP DB file %s",current->dbfile);
      write_my_log(comment);
      exit(1);
    }
    
    //inicializar a estrutura vazia
    init_db(current);

    // ler ficheiro
    linenumber=0;
    validlines=0;
    while(fgets(linha, sizeof(linha), dbfile)){
        linenumber++;
	if(linha[0]=='#' || linha[0]=='\n')
	  validlines++;
        le_linha_db(current,linenumber,linha);
    }

    current->linenumbers=validlines;
    
    sprintf(comment,"EV 127.0.0.1 SP db-file-read %s", current->dbfile);
    write_my_log(comment);
    current=current->next;
  }

}


// le uma linha de texto
void le_linha_db(SPDBFILES current, int linenumber, char * linha){
  char *linha_tmp;
  char *campo1, *campo2, *campo3, *campo4;
  char comment[1024];
  int valid_ip;
  NS ns_tmp;
  MX mx_tmp;
  NS_sub ns_sub_tmp;
  AENTRY a_entry_tmp;
  CNAMEENTRY cname_entry_tmp;


    if(linha[0]!='#' && linha[0]!='\n'){
                linha_tmp=linha;
                if(linha[strlen(linha)-1]=='\n')
                    linha[strlen(linha)-1]=0; // remover o enter final
                
                campo1=strsep(&linha_tmp," ");

                if(!strcmp(campo1,"@")){ // linha comecando com @
                    campo2=strsep(&linha_tmp," ");
                
                    if(!campo2){
                        sprintf(comment,"SP 127.0.0.1 line %d in DB file %s is invalid",linenumber,current->dbfile);
                        write_my_log(comment);
                        exit(1);
                    }

                    if(!strcmp(campo2,"DEFAULT")){
                        // default domain
                        current->default_domain=strdup(linha_tmp); // devia verificar-se o campo
                    
                    } else if (!strcmp(campo2,"SOASP")) {
                        campo3=strsep(&linha_tmp," ");
                        if(!campo3){
                            sprintf(comment,"SP 127.0.0.1 line %d in DB file %s has invalid SOASP",linenumber,current->dbfile);
                            write_my_log(comment);
                            exit(1);
                        }

                        current->SOASP=strdup(campo3);
                        current->SOASP_TTL=get_TTL(&linha_tmp,current->TTL_DEFAULT);
                        current->order=get_prioridade(linha_tmp);

                        if(current->SOASP_TTL<0){
                            sprintf(comment,"SP 127.0.0.1 line %d in DB file %s has invalid SOASP TTL field",linenumber,current->dbfile);
                            write_my_log(comment);
                            exit(1);
                        }
                    
                    } else if (!strcmp(campo2,"SOAADMIN")) {
                        campo3=strsep(&linha_tmp," ");
                        if(!campo3){
                            sprintf(comment,"SP 127.0.0.1 line %d in DB file %s has invalid SOAADMIN",linenumber,current->dbfile);
                            write_my_log(comment);
                            exit(1);
                        }

                        current->SOAADMIN=strdup(campo3); // deveria verificar que e' um email e substituir o \. por @
                        current->SOAADMIN_TTL=get_TTL(&linha_tmp,current->TTL_DEFAULT);
                        current->order=get_prioridade(linha_tmp);

                        if(current->SOAADMIN_TTL<0){
                            sprintf(comment,"SP 127.0.0.1 line %d in DB file %s has invalid SOAADMIN TTL field",linenumber,current->dbfile);
                            write_my_log(comment);
                            exit(1);
                        }
            
                    } else if (!strcmp(campo2,"SOASERIAL")) {
                        campo3=strsep(&linha_tmp," ");
                        if(!campo3){
                            sprintf(comment,"SP 127.0.0.1 line %d in DB file %s has invalid SOASERIAL",linenumber,current->dbfile);
                            write_my_log(comment);
                            exit(1);
                        }

                        current->SOASERIAL=strdup(campo3);
                        current->SOASERIAL_TTL=get_TTL(&linha_tmp,current->TTL_DEFAULT);
                        current->order=get_prioridade(linha_tmp);

                        if(current->SOASERIAL_TTL<0){
                            sprintf(comment,"SP 127.0.0.1 line %d in DB file %s has invalid SOASERIAL TTL field",linenumber,current->dbfile);
                            write_my_log(comment);
                            exit(1);
                        }
            
                    } else if (!strcmp(campo2,"SOAREFRESH")) {
                        campo3=strsep(&linha_tmp," ");
                        if(!campo3){
                            sprintf(comment,"SP 127.0.0.1 line %d in DB file %s has invalid SOAREFRESH",linenumber,current->dbfile);
                            write_my_log(comment);
                            exit(1);
                        }

                        current->SOAREFRESH=atoi(campo3);
                        current->SOAREFRESH_TTL=get_TTL(&linha_tmp,current->TTL_DEFAULT);
                        current->order=get_prioridade(linha_tmp);

                        if(current->SOAREFRESH_TTL<0){
                            sprintf(comment,"SP 127.0.0.1 line %d in DB file %s has invalid SOAREFRESH TTL field",linenumber,current->dbfile);
                            write_my_log(comment);
                            exit(1);
                        }
                    } else if (!strcmp(campo2,"SOARETRY")) {
                        campo3=strsep(&linha_tmp," ");
                        if(!campo3){
                            sprintf(comment,"SP 127.0.0.1 line %d in DB file %s has invalid SOARETRY",linenumber,current->dbfile);
                            write_my_log(comment);
                            exit(1);
                        }

                        current->SOARETRY=atoi(campo3);
                        current->SOARETRY_TTL=get_TTL(&linha_tmp,current->TTL_DEFAULT);
                        current->order=get_prioridade(linha_tmp);

                        if(current->SOARETRY_TTL<0){
                            sprintf(comment,"SP 127.0.0.1 line %d in DB file %s has invalid SOARETRY TTL field",linenumber,current->dbfile);
                            write_my_log(comment);
                            exit(1);
                        }
                    } else if (!strcmp(campo2,"SOAEXPIRE")) {
                        campo3=strsep(&linha_tmp," ");
                        if(!campo3){
                            sprintf(comment,"SP 127.0.0.1 line %d in DB file %s has invalid SOAEXPIRE",linenumber,current->dbfile);
                            write_my_log(comment);
                            exit(1);
                        }

                        current->SOAEXPIRE=atoi(campo3);
                        current->SOAEXPIRE_TTL=get_TTL(&linha_tmp,current->TTL_DEFAULT);
                        current->order=get_prioridade(linha_tmp);

                        if(current->SOAEXPIRE_TTL<0){
                            sprintf(comment,"SP 127.0.0.1 line %d in DB file %s has invalid SOAEXPIRE TTL field",linenumber,current->dbfile);
                            write_my_log(comment);
                            exit(1);
                        }
                    } else if (!strcmp(campo2,"NS")) {
                        campo3=strsep(&linha_tmp," ");
                        if(!campo3){
                            sprintf(comment,"SP 127.0.0.1 line %d in DB file %s has invalid NS",linenumber,current->dbfile);
                            write_my_log(comment);
                            exit(1);
                        }

                        ns_tmp=malloc(sizeof(struct NS_server));
                        if(!ns_tmp){
                            sprintf(comment,"SP 127.0.0.1 unable to allocate memory for NS");
                            write_my_log(comment);
                            exit(1);
                        }

                        ns_tmp->name=strdup(campo3); // deriamos verificar se e' um endereco valido
                        ns_tmp->ttl=get_TTL(&linha_tmp,current->TTL_DEFAULT);
                        ns_tmp->order=get_prioridade(linha_tmp);

                        if(ns_tmp->ttl<0){
                            sprintf(comment,"SP 127.0.0.1 line %d in DB file %s has invalid NS TTL field",linenumber,current->dbfile);
                            write_my_log(comment);
                            exit(1);
                        }

                        ns_tmp->next=current->ns_servers;
                        current->ns_servers=ns_tmp;

                    } else if (!strcmp(campo2,"MX")) {
                        campo3=strsep(&linha_tmp," ");
                        if(!campo3){
                            sprintf(comment,"SP 127.0.0.1 line %d in DB file %s has invalid MX",linenumber,current->dbfile);
                            write_my_log(comment);
                            exit(1);
                        }

                        mx_tmp=malloc(sizeof(struct MX_server));
                        if(!mx_tmp){
                            sprintf(comment,"SP 127.0.0.1 unable to allocate memory for MX");
                            write_my_log(comment);
                            exit(1);
                        }

                        mx_tmp->name=strdup(campo3); // deviamos verificar se e' um endereco valido
                        mx_tmp->ttl=get_TTL(&linha_tmp,current->TTL_DEFAULT);
                        mx_tmp->order=get_prioridade(linha_tmp);

                        if(mx_tmp->ttl<0){
                            sprintf(comment,"SP 127.0.0.1 line %d in DB file %s has invalid MX TTL field",linenumber,current->dbfile);
                            write_my_log(comment);
                            exit(1);
                        }

                        mx_tmp->next=current->mx_servers;
                        current->mx_servers=mx_tmp;

                    } else {
                        sprintf(comment,"SP 127.0.0.1 line %d in DB file %s has an invalid field %s",linenumber,current->dbfile,campo2);
                        write_my_log(comment);
                        exit(1);
                    }
            
            
                } else { // linha nao comecando com @
                    campo2=strsep(&linha_tmp," ");

                    if(!strcmp(campo1,"TTL") && !strcmp(campo2,"DEFAULT")){ // Ler o default TTL
                        current->TTL_DEFAULT=atoi(linha_tmp); // deviamos verificar se é um inteiro ou se o resto da linha tem lixo

                    } else if (!strcmp(campo2,"NS")){ // servidores de sub dominios
                        campo3=strsep(&linha_tmp," ");
                        if(!campo3){
                            sprintf(comment,"SP 127.0.0.1 line %d in DB file %s has invalid NS subdomain record",linenumber,current->dbfile);
                            write_my_log(comment);
                            exit(1);
                        }

                        ns_sub_tmp=malloc(sizeof(struct NS_server));
                        if(!ns_sub_tmp){
                            sprintf(comment,"SP 127.0.0.1 unable to allocate memory for NS sub domain");
                            write_my_log(comment);
                            exit(1);
                        }

                        ns_sub_tmp->name=strdup(campo1);
                        ns_sub_tmp->server=strdup(campo3);

                        ns_sub_tmp->next=current->ns_sub_domain;
                        current->ns_sub_domain=ns_sub_tmp;
            
                    } else if (!strcmp(campo2,"A")){
                        campo3=strsep(&linha_tmp," ");
                        if(!campo3){
                            sprintf(comment,"SP 127.0.0.1 line %d in DB file %s has invalid A record",linenumber,current->dbfile);
                            write_my_log(comment);
                            exit(1);
                        }

                        a_entry_tmp=malloc(sizeof(struct A_entry));
                        if(!a_entry_tmp){
                            sprintf(comment,"SP 127.0.0.1 unable to allocate memory for A entry");
                            write_my_log(comment);
                            exit(1);
                        }

                        a_entry_tmp->name=strdup(campo1);
                        
                        campo4=strsep(&linha_tmp," ");
                        /*if(!campo4){
                            sprintf(comment,"SP 127.0.0.1 line %d in DB file %s has invalid A record",linenumber,current->dbfile);
                            write_my_log(comment);
                            exit(1);
                        }*/

                        valid_ip=IP_parser(campo3,a_entry_tmp->ip,NULL, 0);
                    
                        if(!valid_ip){
                            sprintf(comment,"SP 127.0.0.1 line %d in conf file has an invalid IP address", linenumber);
                            write_my_log(comment);//escrever o erro no log do sp
                            exit(1);
                        }

                        a_entry_tmp->ttl=get_TTL(&campo4,current->TTL_DEFAULT);
                        a_entry_tmp->order=get_prioridade(linha_tmp);

                        if(a_entry_tmp->ttl<0){
                            sprintf(comment,"SP 127.0.0.1 line %d in DB file %s has invalid A TTL field",linenumber,current->dbfile);
                            write_my_log(comment);
                            exit(1);
                        }

                        a_entry_tmp->next=current->a_entry;
                        current->a_n++;
                        current->a_entry=a_entry_tmp;
            
                    } else if (!strcmp(campo2,"CNAME")){
                        campo3=strsep(&linha_tmp," ");
                        if(!campo3){
                            sprintf(comment,"SP 127.0.0.1 line %d in DB file %s has invalid cname record",linenumber,current->dbfile);
                            write_my_log(comment);
                            exit(1);
                        }

                        cname_entry_tmp=malloc(sizeof(struct A_entry));
                        if(!cname_entry_tmp){
                            sprintf(comment,"SP 127.0.0.1 unable to allocate memory for cname entry");
                            write_my_log(comment);
                            exit(1);
                        }

                        cname_entry_tmp->alias=strdup(campo1);
                        
                        campo4=strsep(&linha_tmp," ");
                        /*if(!campo4){
                            sprintf(comment,"SP 127.0.0.1 line %d in DB file %s has invalid cname record",linenumber,current->dbfile);
                            write_my_log(comment);
                            exit(1);
                        }
			*/

                        cname_entry_tmp->name=strdup(campo3);

                        cname_entry_tmp->ttl=get_TTL(&campo4,current->TTL_DEFAULT);
                        cname_entry_tmp->order=get_prioridade(linha_tmp);
                        if(cname_entry_tmp->ttl<0){
                            sprintf(comment,"SP 127.0.0.1 line %d in DB file %s has invalid cname TTL field",linenumber,current->dbfile);
                            write_my_log(comment);
                            exit(1);
                        }

                        cname_entry_tmp->next=current->cname_entry;
                        current->cname_entry=cname_entry_tmp;
                    } else {
                        sprintf(comment,"SP 127.0.0.1 line %d in DB file %s has an invalid field %s",linenumber,current->dbfile,campo2);
                        write_my_log(comment);
                        exit(1);
                    }
            
                }
	    }
}


void read_conffile(){
    FILE *conffile;
    char linha[1024], *linha_tmp;
    char *campo1, *campo2, *campo3;
    SPLOGFILES newSPlogfile;
    SPDBFILES newSPdbfile;
    STDBFILES newSTdbfile;
    MYSPS newSP;
    MYSSS newSS;
    int linenumber;
    char comment[1024];
    int valid_ip=0;
    

    conffile=fopen(SPconfig.conffilename,"r");

    if(!conffile){//se nao consegue abrir o ficheiro de configuração escreve no log e aborta
        sprintf(comment,"SP 127.0.0.1 unable to open conf file %s", SPconfig.conffilename);
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

            if(!strcmp(campo2,"LG")){// log file para o dominio dado
                newSPlogfile=malloc(sizeof(struct SPLogfiles));
                newSPlogfile->domain=strdup(campo1);//neste caso o primeiro campo é o dominio
                newSPlogfile->logfile=strdup(campo3);//o terceiro campo é o path para o ficheiro log do dominio dado
		        newSPlogfile->next=SPlogfiles;//adicionar na cabeça da lligada
                SPlogfiles=newSPlogfile;//o inicio da lista passa a ser este nodo

            } 
            else if(!strcmp(campo2,"DB")){//ficheiro db para o dominio dado
                newSPdbfile=malloc(sizeof(struct SPDbfiles));
                newSPdbfile->domain=strdup(campo1);//neste caso o primeiro campo é o dominio
                newSPdbfile->dbfile=strdup(campo3);//o terceiro campo é o path para o ficheiro db do dominio dado
		        newSPdbfile->next=SPdbfiles;//adicionar na cabeça da lligada
                SPdbfiles=newSPdbfile;//o inicio da lista passa a ser este nodo
            }

            else if(!strcmp(campo2,"SP")){//neste caso é indicado um servidor que é sp deste servidor
                newSP=malloc(sizeof(struct mySPs));
                newSP->domain=strdup(campo1);
                newSP->last_load=0; // time_t da ultima vez que fizemos load da BD por TCP
                newSP->last_try=0; // time_t da ultima tentativa de ligação
                newSP->SOARETRY=DEFAULT_SOARETRY; // SOARETRY por omissão, uma vez que até à leitura não se sabe
                newSP->data=NULL;
                newSP->ttl=SPconfig.timeout;
                
                valid_ip=IP_parser(campo3,newSP->ip_address,&(newSP->port_number),SPconfig.port);
                
                if(!valid_ip){
                    sprintf(comment,"SP 127.0.0.1 line %d in conf file has an invalid IP address", linenumber);
                    write_my_log(comment);//escrever o erro no log do sp
		            exit(1);
                }

		        newSP->next=SPlist;
                SPlist=newSP;
            }

            else if(!strcmp(campo2,"DD")){
                //printf("Ainda nao percebi bem o que isto é, perguntar ao stor\n");
            }

	        else if(!strcmp(campo2,"SS")){
                newSS=malloc(sizeof(struct mySSs));
                newSS->domain=strdup(campo1);//neste caso o primeiro campo é o dominio
                valid_ip=IP_parser(campo3,newSS->ip_address,&(newSS->port_number),SPconfig.port); //o terceiro campo é o IP:port

            if(!valid_ip){
                    sprintf(comment,"SP 127.0.0.1 line %d in conf file has an invalid IP address", linenumber);
                    write_my_log(comment);//escrever o erro no log do sp
		            exit(1);
                }

		        newSS->next=SSlist;//adicionar na cabeça da estrutura ligada
                SSlist=newSS;//o inicio da lista passa a ser este nodo
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

    sprintf(comment,"EV 127.0.0.1 config-file-read %s", SPconfig.conffilename);
    write_my_log(comment);
    
}


int main(int argc, char **argv){
    int current_arg;
    char comment[1024];
    SPDBFILES current;

    current_arg = 1;
     
    while (argv[current_arg]) {
        if (!strncmp(argv[current_arg], "--debug", 7)) {
            SPconfig.debug = 1;
            current_arg++;
        } else if (!strncmp(argv[current_arg], "--shy", 5)) {
            SPconfig.debug = 0;
            current_arg++;
        } else if (!strncmp(argv[current_arg], "--port", 6)) {
            if (!argv[current_arg + 1]) {
                printf("\nMissing port number\n\n");
                usage(argv[0]);
                return 1;
            }
            SPconfig.port = atoi(argv[current_arg + 1]);
            current_arg+=2;
        } else if (!strncmp(argv[current_arg], "--timeout", 9)) {
            if (!argv[current_arg + 1]) {
                printf("\nMissing timeout number\n\n");
                usage(argv[0]);
                return 1;
            }
            SPconfig.timeout = atoi(argv[current_arg + 1]);
            current_arg+=2;
         } else if (!strncmp(argv[current_arg], "--conf", 6)) {
             if (!argv[current_arg + 1]) {
                printf("\nMissing configuration filename\n\n");
                usage(argv[0]);
                return 1;
            }
            SPconfig.conffilename = argv[current_arg+1];
            current_arg+=2;
        } else if (!strncmp(argv[current_arg], "--log", 5)) {
             if (!argv[current_arg + 1]) {
                printf("\nMissing log filename\n\n");
                usage(argv[0]);
                return 1;
            }
            SPconfig.logfilename = argv[current_arg+1];
            current_arg+=2;
        } else {
            printf("Invalid option: %s\n",argv[current_arg]);
            current_arg++;
        }
    }

    if(!SPconfig.logfilename){// utilizador nao definiu um ficheiro de log
        SPconfig.logfilename="logs/SP.log"; // definimos o log por defeito
    }

    if(!SPconfig.conffilename){// utilizador nao definiu um ficheiro de configuracao
        SPconfig.conffilename="confs/SP.conf"; // definimos o ficheiro por defeito
    }

    // registar o arranque
    if(SPconfig.debug)
        sprintf(comment,"ST %d %d debug",SPconfig.port, SPconfig.timeout);
    else
        sprintf(comment,"ST %d %d shy",SPconfig.port, SPconfig.timeout);
    write_my_log(comment);//escrever no log do SP que iniciou

    //print_configuration();

    // ler ficheiro de configuracao
    read_conffile();

    // ler ficheiros da base de dados
    read_SPdbfiles();
    read_SSdbfiles();
    read_STdbfiles();

    // inicializar cache, calculando espaco
    cache_size=N_CACHE;
    for(current=SPdbfiles;current;current=current->next)cache_size+=current->a_n;

    if(init_cache_table(&SP_cache,cache_size)<0){
      sprintf(comment,"EV 127.0.0.1 SP unable to initialize cache with size %d",cache_size);
      write_my_log(comment);
    }

    // inserir todos os registos A dos dbs
    for(current=SPdbfiles;current;current=current->next){
      for(AENTRY current_a=current->a_entry;current_a;current_a=current_a->next){ //percorrer a lista ligada de Aentrys que está dentro da struct spdbfiles
	    // entrada com o nome e dominio (senao teriamos uma cache para cada dominio)
	    sprintf(comment,"%s.%s.",current_a->name,current->domain);
	    push_record(comment,current_a->ip,A_CACHE,CFILE,current_a->ttl,current_a->order, SP_cache, cache_size);
      }
    }

    // inserir todos os registos A dos SSs
    for(MYSPS current_SS=SPlist;current_SS;current_SS=current_SS->next){
        if(current_SS->data){
            for(AENTRY current_a=current_SS->data->a_entry;current_a;current_a=current_a->next){
                sprintf(comment,"%s.%s.",current_a->name,current_SS->domain);
                push_record(comment,current_a->ip,A_CACHE,SP,current_a->ttl,current_a->order, SP_cache, cache_size);
            }
        }
    }


    //print_cache_table(SP_cache, cache_size);
    //free_cache_table(&SP_cache, cache_size);
    
    // abre sockets e espera por queries TCP e UDP
    read_queries();

    // termina, nao deveria acontecer porque o programa nao termina quando estiver em funcionamento
    write_my_log("EV 127.0.0.1 terminating - should not happen!");
    return 0;
}
