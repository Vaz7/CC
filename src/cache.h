#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


enum TYPES {A_CACHE, MX_CACHE};

enum ORIGIN {CFILE, SP, OTHERS}; //file colidia com FILE*

enum STATUS {FREE, VALID};

typedef struct cache_record {
  char *name;
  char ip[16];
  enum TYPES type;
  enum ORIGIN origin;
  int ttl;
  int order;
  time_t timestamp;
  enum STATUS status;
} *CACHE_RECORD;

int init_cache_table(CACHE_RECORD **cache_table,int N);
void free_cache_table(CACHE_RECORD **cache_table,int N);
int push_record(char *name, char *ip, enum TYPES type, enum ORIGIN origin, int ttl, int order, CACHE_RECORD *cache_table, int N);
int pop_record(char *name, enum TYPES type, int index, CACHE_RECORD *cache_table, int N);
void check_expired_SP(CACHE_RECORD cache_table, int N);
void print_cache_table(CACHE_RECORD *cache_table, int N);


char* get_cache_ip(int index, CACHE_RECORD cache_table);
int get_cache_ttl(int index, CACHE_RECORD cache_table);
