typedef struct Set{ /*contains all the datato perform a SET*/

    char* key;
    char* value;
    char* timer_type; /*currently supported types : EX (sec)*/
    char* timer_value; 

} Set;

typedef struct Get{ /*contains all the datato perform a GET*/

    char* key;

} Get;

enum WHICH_OP {SET, GET};

typedef union OP_SELECT{

    Set set_op;
    Get get_op;

} OP_SELECT;

typedef struct OP{ /*will contains the data for SET or GET operation exclusively*/

    enum WHICH_OP wop;
    OP_SELECT ops;

} OP; 

typedef struct Node{/*tree node*/

    char* key;
    char* value;
    int exp_time;/*0 means never expire*/

    struct Node* left;
    struct Node* right;

} Node;

/*implement a binary tree as database structure.
I will use binary search to retrieve values*/

/*parse funcs*/
int parse(char* str, char** response);/*parse str. Returns 0 if str is a valid redis command according to RESP2 protocol. -1 otherwise. response will contain data returned from get/set operation*/
int respstrlen(char* s);/*see description in implementation*/
void print_command(char* str);/*prints the command received by the server*/

/*database funcs*/
void add_node(char* key, char* val, int exp_time);/*add a node to the tree with the value 'val' */
void add_node_aux(Node* n, char* key, char* val, int exp_time);/*add a node to the tree with the value 'val' */
Node* find_node(char* key);/*retrieve the value corresponding to the key*/
Node* find_node_aux(Node* n, char* key);/*binary search*/

char* perform_op(OP op);/*will call the proper function to perform the operation op*/
void set(Set s);/*perform a set*/
char* get(Get g);/*perform a get. returns NULL whether an error occurred or the key do not exists, the value otherwise*/
