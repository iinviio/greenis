#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include "greenis.h"

/*the purpose of this file is to contain all the functions needed to parse the redis client request

    +-----------------------------------------------------------+
    |                                                           |
    |   PLEASE NOTE THAT THIS SERVER IS ONLY RESP2 COMPLIANT.   |
    |            RESP3 PROTOCOL IS NOT SUPPORTED!!!             |
    |                                                           |
    +-----------------------------------------------------------+
*/

/*resp2 symbols*/
enum symbols {
                SIMPLE_STRING = '+',
                SIMPLE_ERROR = '-',
                INTEGER = ':',
                BULK_STRING = '$',
                ARRAY = '*',
                CR = '\r',
                LF = '\n',
};

/*parse return values : 
0 : success
-1 : error
1 : get returned NULL
2 : op returned empty string (SET op completed, +OKCRLF response)*/
int parse(char* str, char** response){/*parse str according to RESP2 protocol.*/

    /*If str contains SET or GET execute the command*/

    bool OPERATION_REQUESTED = false; /*flags that tells wheter a get or a set has been received. op.wop will store the operation type*/
    OP op;/*Contains the data needed to perform SET or GET operation*/

    char* tokens_to_retrieve[4] = {NULL, NULL, NULL, NULL};/*will contain all the values needed for get/set operation. in order, i expect : KEY, VALUE, TIMER_TYPE, TIMER_VALUE. Unused values will be set to null and/or ignored*/
    int ttri = 0; /*tokens to retrieve index*/

    int len = -1;/*will contain the payload length. -1 means that len was not overwritten*/

    int iterations = 1;/*number of iterations that are needed to parse the whole content of str.*/
    while (iterations){

        switch(*str){

            case SIMPLE_STRING: /*composed by 5 pieces. " +<string>CRLF " . <string> must not contain CR or LF*/

                /*print the data*/
                print_command(str);

                len = respstrlen(str);
                if(OPERATION_REQUESTED){


                    tokens_to_retrieve[ttri] = (char*) malloc( (len + 1) * sizeof(char));
                    strncpy(tokens_to_retrieve[ttri], str, len);
                    *(tokens_to_retrieve[ttri] + len) = '\0';

                    ttri++;
                }

                str += len;
                if(*str != CR || *(str + 1) != LF){/*after a CR a LF is expected.*/

                    puts("SIMPLE_STRING ERR");
                    return -1;
                }
                str += 2;

                iterations--;
                break;

            case SIMPLE_ERROR: /*same as SIMPLE_STRING, but the first character is a '-', not a '+'*/

                print_command(str);

                len = respstrlen(str);
                
                str += len;
                if(*str != CR || *(str + 1) != LF){/*after a CR a LF is expected.*/

                    puts("SIMPLE_ERROR ERR");
                    return -1;
                }
                str += 2;

                iterations--;
                break;

            case INTEGER: /*base 10, 64-bit integer. it is composed as follow : " :[<+|->]<value>CRLF ". the [<+|->] means that + or - can be put before the number (signed int)*/

                str++;
                if(*str != '+' || *str != '-'){

                    puts("INTEGER ERR1");
                    return -1;
                }

                putchar(*str++);
                print_command(str);

                int len = respstrlen(str);
                if(OPERATION_REQUESTED){


                    tokens_to_retrieve[ttri] = (char*) malloc( (len + 1) * sizeof(char));
                    strncpy(tokens_to_retrieve[ttri], str, len);
                    *(tokens_to_retrieve[ttri] + len) = '\0';

                    ttri++;
                }

                str += len;
                if(*str != CR || *(str + 1) != LF){/*after a CR a LF is expected.*/

                    puts("INTEGER ERR2");
                    return -1;
                }
                str += 2;

                iterations--;
                break;

            case BULK_STRING: /*represents a single binary string. RESP2 encodes bulk strings as follows : " $<length>CRLF<data>CRLF ", where length is the string's length. 
                                ($0CRLFCRLF is the empty string, $-1CRLF is the NULL string)*/

                /*if the content of the string is SET or GET do something. else just validate the string*/
                /*parse for set/get. If set check for timer parameter*/

                len = atoi(++str);

                str += 3; /*skip CRLF characters. this way str will point to the payload*/
                print_command(str);

                char *data = (char*) malloc( (len + 1) * sizeof(char)); 
                
                /*payload of str.*/
                strncpy(data, str, len);
                data[len] = '\0';

                /*extract the data from str*/
                if(OPERATION_REQUESTED){

                    /*special case : NULL string ($-1CRLF)*/
                    if(len == -1){

                        tokens_to_retrieve[ttri] = NULL;
                        ttri++;

                        free(data);
                        iterations--;
                        break;
                    }

                    tokens_to_retrieve[ttri] = data;

                    /*this if statement cover the special case : EMPTY string ($0CRLF)*/
                    if(len > 0){strncpy(tokens_to_retrieve[ttri], data, len);}

                    *(tokens_to_retrieve[ttri] + len) = '\0';/*in case of len == 0 the terminating character will still be put in token_to_retrieve*/
                    ttri++;
                }

                str += len;
                if(*str != CR || *(str + 1) != LF){/*after a CR a LF is expected.*/

                    puts("BULK_STRING ERR1");
                    return -1;
                }
                str += 2;

                if(len == 3 && strncmp(data, "SET", len) == 0){

                    if(OPERATION_REQUESTED){puts("BULK_STRING ERR2"); return -1;}
                    
                    puts("\nSET REQUESTED");
                    OPERATION_REQUESTED = true;
                    op.wop = SET;
                }

                if(len == 3 && strncmp(data, "GET", len) == 0){

                    if(OPERATION_REQUESTED){puts("BULK_STRING ERR3"); return -1;}

                    puts("\nGET REQUESTED");
                    OPERATION_REQUESTED = true;
                    op.wop = GET;
                }

                iterations--;
                break;

            case ARRAY: /*array of items. encoded as follow : " *<number-of-items>CRLF<item-1>CRLF<item-2>CRLF ... <item-n>CRLF ".
                          Every item of the array might contains mixed RESP data type (e.g. *4 CRLF :2 CRLF $4TEST CRLF +HELLO CRLF -WORLD CRLF).
                          The empty array is *0CRLF. Null array is *-1CRLF*/

                iterations = atoi(++str);/*i need <number-of-items> iterations to parse str*/
                if(!iterations){

                    puts("ARRAY ERR1");
                    return -1;
                }

                str++;
                if(*str != CR || *(str + 1) != LF){/*after a CR a LF is expected.*/

                    puts("ARRAY ERR2");
                    return -1;
                }
                str += 2;

                break;

            default:

                puts("DEFAULT ERR");
                return -1;
                break;
        }

        len = -1; /*len reset*/
    }
    
    /*fills the field of op*/
    if(OPERATION_REQUESTED){

        op.ops.set_op.key = tokens_to_retrieve[0];/*get only need the key*/

        if(op.wop == SET){

            op.ops.set_op.value = tokens_to_retrieve[1];
            op.ops.set_op.timer_type = tokens_to_retrieve[2];
            op.ops.set_op.timer_value = tokens_to_retrieve[3];
        }

        *response = perform_op(op);
        if(*response == NULL){/*get returned NULL*/

            return 1;
        }
        else if(strcmp(*response, "") == 0){/*SET performed*/

            return 2;
        }
    }
    

    /*free the memory and return*/
    for(int i = 0; i < 4; i++){

        if(tokens_to_retrieve[i] != NULL){
            
            free(tokens_to_retrieve[i]);
        }
    }
    return 0;
}

/*returns the amount of bytes of the payload. uses \r\l as terminating character instead of '\0'. 
If \r\l is missing the string will end with the classic terminating character.
\r\l characters are NOT considered in the final length*/
int respstrlen(char* s){

    int len = 0;
    while(*s){

        if(*s == CR && *(s + 1) == LF){

            break;
        }

        s++;
        len++;
    }

    return len;
}

void print_command(char* str){/*prints the command received by the server*/

    while(*str){

        if(*str == CR && *(str + 1) == LF){str += 2; break;}
        putchar(*str++);
    }
}