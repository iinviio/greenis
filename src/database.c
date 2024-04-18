#include <stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include "greenis.h"

Node* tree = NULL;/*binary tree struct*/

void add_node(char* key, char* val, long exp_time){/*add a node to the tree with the value 'val'.*/

    tree = add_node_aux(tree, NULL, key, val, exp_time);
}

/*'prev' is the father of 'n' in the tree*/
Node* add_node_aux(Node* n, Node* prev, char* key, char* val, long exp_time){/*add a node to the tree with the value 'val'.*/

    if(n == NULL){/*create a new node*/

        n = (Node*) malloc(sizeof(Node));

        int tmp;
    
        tmp = strlen(key) + 1;/*the + 1 is for the terminating byte*/
        n->key = (char*) malloc(tmp * sizeof(char));
        strncpy(n->key, key, tmp);

        tmp = strlen(val) + 1;/*the + 1 is for the terminating byte*/
        n->value = (char*) malloc(tmp * sizeof(char));
        strncpy(n->value, val, tmp);

        n->exp_time = exp_time;
        
        n->left = NULL;
        n->right = NULL;

        /*make the father of 'n' point to 'n'*/
        if(prev != NULL){/*if prev is null then this is the first node*/

            if(strcmp(key, prev->key) < 0){prev->left = n;}
            else{prev->right = n;}
        }

        return n;
    }

    if(strcmp(key, n->key) == 0){/*restores an existing node*/

        /*realloc memory for the new value*/
        int tmp = strlen(val) + 1;
        n->value = realloc(n->value, tmp * sizeof(char));

        /*updates value*/
        strncpy(n->value, val, tmp);

        n->exp_time = exp_time;

        return n;
    }

    Node* tmp_node = strcmp(key, n->key) < 0 ? n->left : n->right;

    return add_node_aux(tmp_node, n, key, val, exp_time);
}

Node* find_node(char* key){/*retrieve the value corresponding to the key*/

    return find_node_aux(tree, key);
}

Node* find_node_aux(Node* n, char* key){/*binary search*/

    if(n == NULL){

        return NULL;
    }

    if(strncmp(key, n->key, strlen(key)) == 0){

        if(n->exp_time > 0 && (n->exp_time - (long) time(NULL)) > 0){/*i had to use a subtraction since the normal '<' operand somehow fails the operation*/

            return n;
        }

        return NULL;/*value is ignored*/
    }    

    n = strncmp(key, n->key, strlen(key)) < 0 ? n->left : n->right;
    return find_node_aux(n, key);
}

char* perform_op(OP op){/*will call the proper function to perform the operation op. Will return an empty string when set is called*/

    if(op.wop == SET){set(op.ops.set_op); return "";}
    else{return get(op.ops.get_op);} /*op.wop == GET*/
}

char* set(Set s){/*perform a set. returns an empty string on success, NULL on error*/

    long exp_time = 1;/*default value : never expiring key*/
    time_t t;

    if(s.timer_value != NULL){

        t = time(NULL);/*get the time in seconds since the epoch*/
        if(t == -1){

            puts("time error");
            return NULL;
        }

        exp_time = atoi(s.timer_value);/*when a get is performed, if the key is requested and it is expired, the key is ignored. The key can be brought back to life by call another set*/
        if(exp_time == 0){/*atoi returns 0 on error*/

            puts("atoi error");
            return NULL;
        }

        exp_time += t;/*update exp_time with the actual expiration time of the key*/
    }

    add_node(s.key, s.value, exp_time);    
    return "";
}

char* get(Get g){/*perform a get. returns NULL whether an error occurred or the key do not exists, the value otherwise*/

    if(g.key == NULL){

        return NULL;
    }

    Node* n = find_node(g.key);

    return n == NULL ? NULL : n->value;
}