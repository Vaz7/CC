#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queries.h"


int parse_query(QUERY q, char *string){
  char *temp;

  memset(q,0,sizeof(struct query));

  if(!string)
    return -1;

  temp=strsep(&string,";"); // cabecalho da mensagem (Header fields)

  if(!temp)
    return -1;

  q->message_id=atoi(strsep(&temp,","));
  if(!temp)
    return -1;
  
  q->flags=strdup(strsep(&temp,","));
  if(!temp)
    return -1;

  q->response_code=atoi(strsep(&temp,","));
  if(!temp)
    return -1;
  
  q->n_values=atoi(strsep(&temp,","));
  if(!temp)
    return -1;
  
  q->n_authorities=atoi(strsep(&temp,","));
  if(!temp)
    return -1;

  q->n_extra_values=atoi(temp);
  
  temp=strsep(&string,";"); // segundo campo (Data fields)
  if(!temp)
    return -1; // segundo campo é obrigatorio

  q->info_name=strdup(strsep(&temp,","));
  if(!temp)
    return -1;

  q->info_type_of_name=strdup(temp); // novo ;
  
  temp=strsep(&string,";");
  if(!temp)
    return 0; // campos extra podem nao existir

  // varios response values, q->n_values deles
  q->response_values=malloc(q->n_values*sizeof(char *));
  for(int i=0;i<q->n_values;i++){
    q->response_values[i]=strsep(&temp,",");
    if(!temp)
      return -1;
  }
  
  temp=strsep(&string,";");
  if(!temp)
    return 0; // campos extra podem nao existir

  // varios authorities_values
  q->authorities_values=malloc(q->n_authorities*sizeof(char *));
  for(int i=0;i<q->n_authorities;i++){
    q->authorities_values[i]=strsep(&temp,",");
    if(!temp)
      return -1;
  }

  temp=strsep(&string,";");
  if(!temp)
    return 0; // campos extra podem nao existir

  // varios extra_values
  q->extra_values=malloc(q->n_extra_values*sizeof(char *));
  for(int i=0;i<q->n_extra_values;i++){
    q->extra_values[i]=strsep(&temp,",");
    if(!temp)
      return -1;
  }

  if(string && strlen(string)>0)
    return 1; // lixo no final da query

  return 0;
}

char *print_query(QUERY q){
  char temp[1024];
  char temp_response[256];
  char temp_auth[256];
  char temp_extra[256];
  int i;

  if(!q)
    return NULL;

  temp_response[0]=0;
  temp_auth[0]=0;
  temp_extra[0]=0;

  for(i=0;i<q->n_values;i++){
    strcat(temp_response,q->response_values[i]);
    strcat(temp_response,",");
  }

  for(i=0;i<q->n_authorities;i++){
    strcat(temp_auth,q->authorities_values[i]);
    strcat(temp_auth,",");
  }

  for(i=0;i<q->n_extra_values;i++){
    strcat(temp_extra,q->extra_values[i]);
    strcat(temp_extra,",");
  }

  sprintf(temp,"%d,%s,%d,%d,%d,%d;%s,%s;%s;%s;%s",q->message_id,q->flags,q->response_code,q->n_values,q->n_authorities,q->n_extra_values,
    q->info_name,q->info_type_of_name,temp_response,temp_auth,temp_extra);

  return strdup(temp);
}

char *print_verbose_query(QUERY q){
  char temp[4096];
  char temp_response[1024];
  char temp_auth[1024];
  char temp_extra[1024];
  int i;
  
  if(!q)
    return NULL;

  temp_response[0]=0;
  temp_auth[0]=0;
  temp_extra[0]=0;

  if(q->n_values==0){
    strcat(temp_response,"RESPONSE-VALUES = (NULL)\n");
  } else {
    for(i=0;i<q->n_values;i++){
      strcat(temp_response,"RESPONSE-VALUES = ");
      strcat(temp_response,q->response_values[i]);
      strcat(temp_response,"\n");
    }
  }

  if(q->n_authorities==0){
    strcat(temp_auth,"AUTHORITIES-VALUES = (NULL)\n");
  } else {
    for(i=0;i<q->n_authorities;i++){
      strcat(temp_auth, "AUTHORITIES-VALUES = ");
      strcat(temp_auth,q->authorities_values[i]);
      strcat(temp_auth,"\n");
    }
  }
 
  if(q->n_extra_values==0){
    strcat(temp_extra,"EXTRA-VALUES = (NULL)\n");
  } else {
    for(i=0;i<q->n_extra_values;i++){
      strcat(temp_extra,"EXTRA-VALUES = ");
      strcat(temp_extra,q->extra_values[i]);
      strcat(temp_extra,"\n");
    }
  }

  sprintf(temp,"# Header\nMESSAGE-ID = %d, FLAGS = %s, RESPONSE-CODE = %d, N-VALUES = %d, N-AUTHORITIES = %d, N-EXTRA-VALUES = %d;\n#Data :Query Info\n"
    " QUERY-INFO.NAME = %s, QUERY-INFO.TYPE = %s;\n# Data: List of Response, Authorities and Extra Values\n%s%s%s",
    q->message_id,q->flags,q->response_code,q->n_values,q->n_authorities,q->n_extra_values,
    q->info_name,q->info_type_of_name,temp_response,temp_auth,temp_extra);

  return strdup(temp);
}


void build_query(QUERY q,char *argv[]){
    char flags[3];
    sprintf(flags,"Q+%s",argv[4]);
    q->message_id=rand()%65535+1;
    q->flags=strdup(flags);
    q->response_code=0;
    q->n_values=0;
    q->n_authorities=0;
    q->n_extra_values=0;
    q->info_name=strdup(argv[2]);
    q->info_type_of_name=strdup(argv[3]);
    q->response_values=0;
    q->authorities_values=0;
    q->extra_values=0;
}
