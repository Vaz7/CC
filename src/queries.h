typedef struct query{
    int message_id;
    char *flags;
    int response_code;
    int n_values;
    int n_authorities;
    int n_extra_values;
    char* info_name;
    char* info_type_of_name;
    char** response_values;
    char** authorities_values;
    char** extra_values;
}*QUERY;

int parse_query(QUERY q, char *string);
void build_query(QUERY q,char *argv[]);
char *print_verbose_query(QUERY q);
char *print_query(QUERY q);
