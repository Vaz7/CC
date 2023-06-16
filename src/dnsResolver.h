typedef struct ST_entry { //aqui guardam-se os servidores de topo
  char *domain;
  char ip[16];
  int port;
  struct ST_entry *next;
} *STENTRY;

struct SRConfig {
    int debug;
    int port;
    char *conffilename;
    char *logfilename;
    int timeout;
};

typedef struct CNAME_entry { //estrutura para guardar os aliases
  char *name;
  char *alias;
  int ttl;
  int order;
  struct CNAME_entry *next;
} *CNAMEENTRY;

typedef struct STDbfiles{//estrutura que inicalmente guarda o nome do dominio dos STs e o ficheiro e dps guarda tudo do parse do ficheiro (ainda nao sei o que e')
  char *dbfile;
  struct STDbfiles *next;
} *STDBFILES;