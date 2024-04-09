#include<stdlib.h>
#include<string.h>
#include<time.h>
#include "greenis.h"

Node* tree = NULL;/*tree struct*/

void add_node(char* key, char* val, int exp_time){/*add a node to the tree with the value 'val'.*/

    add_node_aux(tree, key, val, exp_time);
}

void add_node_aux(Node* n, char* key, char* val, int exp_time){/*add a node to the tree with the value 'val'.*/

    if(n == NULL || strcmp(key, n->key) == 0){/*create a new node or restore an expired one*/

        n = (Node*) malloc(sizeof(Node));

        int tmp;
        
        tmp = strlen(key) + 1;/*the + 1 is for the terminating byte*/
        n->key = (char*) malloc(tmp * sizeof(char));
        strncpy(key, n->key, tmp);

        tmp = strlen(key) + 1;/*the + 1 is for the terminating byte*/
        n->value = (char*) malloc(tmp * sizeof(char));
        strncpy(val, n->value, tmp);

        n->exp_time = exp_time;

        n->left = NULL;
        n->right = NULL;

        return;
    }

    n = key < n->key ? n->left : n->right;
    add_node_aux(n, key, val, exp_time);
    return;
}

Node* find_node(char* key){/*retrieve the value corresponding to the key*/

    return find_node_aux(tree, key);
}

Node* find_node_aux(Node* n, char* key){/*binary search*/

    if(n == NULL){

        return NULL;
    }

    if(strcmp(key, n->key) == 0){

        if(n->exp_time > 0 && n->exp_time < time(NULL)){

            return n;
        }

        return NULL;/*value is ignored*/
    }    

    n = strcmp(key, n->key) < 0 ? n->left : n->right;
    return find_node_aux(n, key);
}

char* perform_op(OP op){/*will call the proper function to perform the operation op. Will return an empty string when set is called*/

    if(op.wop == SET){set(op.ops.set_op); return "";}
    else{return get(op.ops.get_op);} /*op.wop == GET*/
}

void set(Set s){/*perform a set.*/

    time_t t = time(NULL);/*get the time in seconds since the epoch*/

    int exp_time = t + atoi(s.timer_value);/*when a get is performed, if the key is requested and it is expired, the key is ignored. The key can be brought back to life by call another set*/
    add_node(s.key, s.value, exp_time);    
}
char* get(Get g){/*perform a get. returns NULL whether an error occurred or the key do not exists, the value otherwise*/

    Node* n = find_node(g.key);

    return n == NULL ? NULL : n->value;
}