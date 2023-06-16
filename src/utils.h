int IP_parser(char *ip, char *ip_field, int *ip_port, int default_port);
int get_TTL(char **linha,int ttl_default);
void removeEnters(char *string);
int get_prioridade(char *linha);
int remove_dominio(char *dominio, char *nome, char *resto);
int compara_dominio(char *dominio, char *nome);
void adiciona_at(char *string1, char *string2);
