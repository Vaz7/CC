#include "cache.h"


int init_cache_table(CACHE_RECORD **cache_table, int N){
    
    *cache_table=malloc(N*sizeof(struct cache_record*));

    if(!(*cache_table))
        return -1; // error

    for(int i=0;i<N;i++){
      (*cache_table)[i]=malloc(sizeof(struct cache_record));
      if(!((*cache_table)[i]))
            return -1; // error
        (*cache_table)[i]->status=FREE; // a free struct
        (*cache_table)[i]->name=NULL;
    }
    
    return 0;
}

void print_cache_table(CACHE_RECORD *cache_table, int N){
    for (int i=0;i<N;i++){
        printf("Record %d is ",i+1);
        if(cache_table[i]->status==VALID){
            printf("valid: name is %s, ip is %s, TTL=%d,Prioridade:%d\n",cache_table[i]->name,cache_table[i]->ip,cache_table[i]->ttl,cache_table[i]->order); // TODO podia imprimir mais coisas!
        } else {
            printf("free\n");
        }
    }
}


void free_cache_table(CACHE_RECORD **cache_table,int N){

    for(int i=0;i<N;i++)
      free((*cache_table)[i]);

    free(*cache_table);

}

// push do registo no primeiro free, retorna negative se erro
int push_record(char *name, char *ip, enum TYPES type, enum ORIGIN origin, int ttl, int order, CACHE_RECORD *cache_table, int N){
    time_t now;
    int success=-1;
    int primeiro_livre=N+1;

    now = time(NULL);
    
    if(origin==CFILE){ // || origin==SP){ // inserir no primeiro livre
        for(int i=0;success<0 && i<N;i++){
            if(cache_table[i]->status==FREE){ // found
                cache_table[i]->name=strdup(name);
                strcpy(cache_table[i]->ip,ip);
                cache_table[i]->type=type;
                cache_table[i]->origin=origin;
                cache_table[i]->ttl=ttl;
                cache_table[i]->timestamp=now;
                cache_table[i]->ttl=ttl;
                cache_table[i]->order=order;
                cache_table[i]->status=VALID;
                success=0; // stop, success
            }

        }
    } else { // verificar se há uma atualizacao (se colide com algum ja existente na cache), guarda-se o primeiro livre que aparecer, é neste que se vai inserir caso seja necessario
        for(int i=0;success<0 && i<N;i++){
            if(cache_table[i]->status==FREE){ // encontrou um livre, vamos guardar para mais tarde inserir num livre se for caso disso
                if(primeiro_livre>i)
                    primeiro_livre=i;
            } else { // nao e' livre, verificar um hit na cache
                if(!strcmp(cache_table[i]->name,name) && cache_table[i]->type==type && cache_table[i]->order==order && !memcmp(cache_table[i]->ip,ip, 4*sizeof(int))){
                    // temos um match, atualizamos o timestamp e o ttl, paramos o ciclo com o success
                    cache_table[i]->timestamp=now;
                    cache_table[i]->ttl=ttl;
                    success=0;
                }
            }
        }

        if(success && primeiro_livre<N){ // inserir no primeiro livre, se existir
            cache_table[primeiro_livre]->name=strdup(name);
            strcpy(cache_table[primeiro_livre]->ip,ip);
            cache_table[primeiro_livre]->type=type;
            cache_table[primeiro_livre]->origin=origin;
            cache_table[primeiro_livre]->ttl=ttl;
            cache_table[primeiro_livre]->timestamp=now;
            cache_table[primeiro_livre]->order=order;
            cache_table[primeiro_livre]->status=VALID;
            success=0;
        }
    }

    // success=-1 se nao ha espaco para mais registos
    return success;
}

void check_expired(CACHE_RECORD registo){
    time_t now;

    now=time(NULL);

    if((registo->origin==SP || registo->origin==OTHERS)  && now-registo->timestamp>registo->ttl){// ultrapassou ttl, remover
        registo->status=FREE;
        free(registo->name); // desalocamos para ser novamente alocado no futuro
    }
}

// TODO as entradas dos SPs também podem expirar, mas aqui tem de se recarregar a partir do SP
void check_expired_SP(CACHE_RECORD cache_table, int N){

};

// procura a entrada, retorna negativo se nao encontrado ou o numero no array se encontrado
int pop_record(char *name, enum TYPES type, int index, CACHE_RECORD *cache_table, int N){
    int idx=-1; // indice to registo que faz match

    for(int i=index;idx<0 && i<N;i++){
        check_expired(cache_table[i]); // se estiver expirado e' removido
        if(cache_table[i]->status==VALID && !strcmp(cache_table[i]->name,name) && cache_table[i]->type==type){ // encontrou
            idx=i;
        }
    }

    return idx;
}

// implementar as funcoes necessarias para que o codigo se mantenha encapsulado
char* get_cache_ip(int index, CACHE_RECORD cache_table){
    return cache_table[index].ip;
}

int get_cache_ttl(int index, CACHE_RECORD cache_table){
    return cache_table[index].ttl;
}
