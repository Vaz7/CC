
struct SPConfig {
    int debug;
    int port;
    char *conffilename;
    char *logfilename;
    int timeout;
};

typedef struct SPLogfiles {
    char *domain;
    char *logfile;
    struct SPLogfiles *next;
} *SPLOGFILES;

typedef struct NS_server {
  char *name;
  int ttl;
  int order;
  struct NS_server *next;
} *NS;

typedef struct NS_sub_domain { //subdominios, no exemplo tem o sp.smaller.example.com
  char *name;
  char *server;
  struct NS_sub_domain *next;
} *NS_sub;

typedef struct A_entry { //aqui guardam-se as equivalencias entre nomes e ips
  char *name;
  char ip[16];
  int ttl;
  int order;
  struct A_entry *next;
} *AENTRY;

typedef struct ST_entry { //aqui guardam-se os servidores de topo
  char *domain;
  char ip[16];
  int port;
  struct ST_entry *next;
} *STENTRY;

typedef struct CNAME_entry { //estrutura para guardar os aliases
  char *name;
  char *alias;
  int ttl;
  int order;
  struct CNAME_entry *next;
} *CNAMEENTRY;

typedef struct MX_server {//para guardar os servidores de mx
  char *name;
  int ttl;
  int order;
  struct MX_server *next;
} *MX;

typedef struct SPDbfiles{//estrutura que inicalmente guarda o nome do dominio e o ficheiro e dps guarda tudo do parse do ficheiro
  char *domain;//dominio
  char *dbfile;//ficheiro de dados do dominio
  char *default_domain;//dominio por defeito para acrescentar quando não tem .
  char *SOASP;
  int SOASP_TTL;
  char *SOAADMIN;
  int SOAADMIN_TTL;
  char *SOASERIAL;
  int SOASERIAL_TTL;
  int SOAREFRESH;
  int SOAREFRESH_TTL;
  int SOARETRY;
  int SOARETRY_TTL;
  int SOAEXPIRE;
  int SOAEXPIRE_TTL;
  int TTL_DEFAULT;
  int order;
  NS ns_servers;
  MX mx_servers;
  int mx_n;
  NS_sub ns_sub_domain;
  AENTRY a_entry;
  int a_n;
  CNAMEENTRY cname_entry;
  int linenumbers;
  struct SPDbfiles *next;
} *SPDBFILES;



typedef struct STDbfiles{//estrutura que inicalmente guarda o nome do dominio dos STs e o ficheiro e dps guarda tudo do parse do ficheiro (ainda nao sei o que e')
  char *dbfile;
  struct STDbfiles *next;
} *STDBFILES;


typedef struct mySPs{
    char *domain;
    char ip_address[16];
    int port_number;
    time_t last_load;
    time_t last_try;
    int ttl;
    int SOARETRY;
    SPDBFILES data;
    struct mySPs *next;
}*MYSPS;

typedef struct mySSs{
    char *domain;
    char ip_address[16];
    int port_number;
    struct mySSs *next;
}*MYSSS;

void print_configuration();
void read_conffile();
void usage();
void write_my_log(char *);
void read_SPdbfiles();
void read_SSdbfiles();
void read_queries();
void le_linha_db(SPDBFILES current, int linenumber, char *linha);
void free_db(SPDBFILES current);
void init_db(SPDBFILES current);
int connectSP(char *domain, char *ip_address, int port);