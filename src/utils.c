#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

void removeEnters(char *string){//remove enters xDD
    int size = strlen(string);
    size--;

    for(int i=0;string[i];i++){
      if(string[i]=='\n' || string[i]=='\r'){
      string[i]='\0';
    }
    }
}

int get_TTL(char **linha,int ttl_default){
  char *tmp;

  if(!linha || !(*linha))
    return ttl_default;

  tmp=strsep(linha," ");

  if(!tmp)
    return ttl_default;

  if(!strcmp(tmp,"TTL")) return ttl_default;
    
  return atoi(tmp);
  
}


int get_prioridade(char *linha){

  if(!linha) return 0;

  return atoi(linha);
  
}


int IP_parser(char *ip,char *ip_field,int *ip_port, int DEFAULT_PORT){
    char *campo1, *campo2, *campo3, *campo4;
    int i;
    int ip_fields[4];

    campo1=strsep(&ip,".");
    campo2=strsep(&ip,".");
    campo3=strsep(&ip,".");

    if(!campo1 && !campo2 && !campo3) return 0; //endereço ip inválido

    ip_fields[0]=atoi(campo1);
    ip_fields[1]=atoi(campo2);
    ip_fields[2]=atoi(campo3);
    
    if(ip_port){ // estamos a' espera de um port number
      campo4=strsep(&ip,":");
      if(!campo4) return 0; // endereco ip invalido
      ip_fields[3]=atoi(campo4);
      if(ip)
	      *ip_port=atoi(ip);
      else
	      *ip_port=DEFAULT_PORT;
    } 
    else ip_fields[3]=atoi(ip);

    for(i=0;i<4;i++)
      if(ip_fields[i] > 255 || ip_fields[i] < 0) return 0;
    
    if(ip_port && (*ip_port > 65535 || *ip_port < 0)) return 0;

    sprintf(ip_field,"%d.%d.%d.%d",ip_fields[0],ip_fields[1],ip_fields[2],ip_fields[3]);

    return 1;

}

int compara_dominio(char *dominio, char *nome){
    char *temp_dominio, *temp_nome;

    // comparacao do fim para o inicio
    temp_dominio=dominio;
    while(*temp_dominio)temp_dominio++;
    temp_nome=nome;
    while(*temp_nome)temp_nome++;
    // ir para o caracter final e remover potenciais pontos finais ou @ nos subdominios
    temp_dominio--;
    temp_nome--;
    while(*temp_dominio=='.')
      temp_dominio--;
    while(*temp_nome=='.')
      temp_nome--;

    // temp no fim das strings
    while(temp_dominio>=dominio && temp_nome>=nome && *temp_dominio==*temp_nome){
        temp_dominio--;
        temp_nome--;
    }

    if(temp_dominio<dominio){
        return 0;
    } else {
        return 1;
    }
}


int remove_dominio(char *dominio, char *nome, char *resto){
    char *temp_dominio, *temp_nome;

    // comparacao do fim para o inicio
    temp_dominio=dominio;
    while(*temp_dominio)temp_dominio++;
    temp_nome=nome;
    while(*temp_nome)temp_nome++;
    // ir para o caracter final e remover potenciais pontos finais ou @ nos subdominios
    temp_dominio--;
    temp_nome--;
    while(*temp_dominio=='.' || *temp_dominio=='@')
      temp_dominio--;
    while(*temp_nome=='.' || *temp_nome=='@')
      temp_nome--;

    // temp no fim das strings
    while(temp_dominio>=dominio && temp_nome>=nome && *temp_dominio==*temp_nome){
        temp_dominio--;
        temp_nome--;
    }

    if(temp_dominio<dominio && *temp_nome=='.'){
        if(resto){// match de dominio com o nome, copiar para o resto
          int i=0;
        
          temp_nome--; // nao copiar o ponto
          temp_dominio=nome;
          while(temp_dominio<=temp_nome){
            resto[i]=nome[i];
            i++;
            temp_dominio++;
          }
          resto[i]=0;
        }
        return 0;
    } else {
        if(temp_dominio<dominio && temp_nome<nome){// match exato
          if (resto)
            *resto=0;
          return 0;
        } else {
          return 1;
        }
    }
}

void adiciona_at(char *string1, char *string2){
  char *temp_string;

  temp_string=string1;
  while(*temp_string && *temp_string!='@')
    temp_string++;

  if(temp_string)
    strcpy(temp_string,string2);

}
  
