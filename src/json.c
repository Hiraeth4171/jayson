#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "./include/jayson/json.h"
#include "./include/jayson/logging.h"
#include "util.h"

#define CHUNK   16 // default number of tokens

struct Settings {
    long flags : 6;
};

struct Settings g_settings;

void print_tokens(JSONToken* tokens);
JSONObject* hashtable_init(size_t size);
JSONPair* hashtable_get(JSONObject* obj, char* key);
void hashtable_add(JSONObject* obj, JSONPair* key);
void hashtable_free(JSONObject** obj);

void
json_init(int flags) {
    g_settings.flags = flags;
    //printf("\n\tSettings :\t%u\n\n", g_settings.flags);
}


bool
add_token(JSONToken** tokens, JSONToken token, size_t *tokens_cap) {
    static size_t count = 0;
    if (count >= *tokens_cap) {
        *tokens_cap += CHUNK;
        JSONToken* tmp = (JSONToken*)realloc(*tokens, (*tokens_cap) * sizeof(JSONToken));
        if (!tmp) {
            printf("wha\n\n");
            return false;
        }
        *tokens = tmp;
        //printf("\t\tadd_token - tokens_cap: %lu, count: %lu\n\n", *tokens_cap, count);
    }
    //if(token.type != T_EOF) printf("-[ type: %d, content: { .data = '%s', \t .size = %lu} ], count: %lu\n", token.type, token.content.data, token.content.size, count);

    (*tokens)[count++] = token;
    //printf("hi\n\n");
    //printf("----- [ type: %d, content: { .data = '%s', \t .size = %lu} ], count: %lu\n", (*tokens)[count-1].type, (*tokens)[count-1].content.data, (*tokens)[count-1].content.size, count);
    return true;
}

JSONToken
create_token(JaysonTokenType type, char* str, size_t *len) {
    //printf("1: %d, 2: %s, 3: %lu", type, str, *len);
    if (str == NULL) exit(-1);
    return (JSONToken){ .type = type, .content = (String){ .size = *len, .data = str}};
}

void print_tokens(JSONToken* tokens) {
    JSONToken* ptr = tokens;
    for (;ptr->type != T_EOF;++ptr) {
#ifdef DEBUG
        printf("[ type: %s, content: { .data = '%s', \t .size = %lu} ],\n", lookup[ptr->type], ptr->content.data, ptr->content.size);
#else
        //printf("[ type: %d, content: { .data = '%s', \t .size = %lu} ],\n", ptr->type, ptr->content.data, ptr->content.size);
#endif
    }
}

int count = 0;

JSONToken*
tokenize(char* buff) {
    size_t* tokens_cap = malloc(sizeof(size_t));
    *tokens_cap = CHUNK;
    JSONToken* tokens = malloc(CHUNK * sizeof(JSONToken));
    size_t* len = calloc(1, sizeof(size_t));
    for(char* ptr = buff; *(ptr) != '\0';++ptr) {
        switch (*ptr) {
            case '"': {
                if (*(ptr-1) != ':') count++;
                add_token(&tokens, create_token(T_STRING, match_until_but_better(ptr+1, '"', len), len), tokens_cap);
                ptr += (*len+1);
            } break;
            case ':': {
                *len=1;
                add_token(&tokens, create_token(T_COLON, crt_str(":", 2), len), tokens_cap);
            } break;
            case ',': {
                *len=1;
                add_token(&tokens, create_token(T_COMMA, crt_str(",", 2), len), tokens_cap);
            } break;
            case '{': {
                *len=1;
                add_token(&tokens, create_token(T_LBRACE, crt_str("{", 2), len), tokens_cap);
            } break;
            case '}': {
                *len=1;
                add_token(&tokens, create_token(T_RBRACE, crt_str("}", 2), len), tokens_cap);
            } break;
            case '[': {
                *len=1;
                add_token(&tokens, create_token(T_LBRACKET, crt_str("[", 2), len), tokens_cap);
                //printf("$ %s", tokens[0].content.data);
            } break;
            case ']': {
                *len=1;
                add_token(&tokens, create_token(T_RBRACKET, crt_str("]", 2), len), tokens_cap);
            } break;
            case 'n': {
                if(check_match(ptr, "null", 4) == true) {
                    *len=4;
                    add_token(&tokens, create_token(T_NULL, crt_str("null", 5), len), tokens_cap);
                    ptr+=3;
                }
            } break;
        }
        //printf("-- \t%c\n\n", *ptr);
        if ('0' <= *ptr && *ptr <= '9' || *ptr == '.' || *ptr == '-') {
            add_token(&tokens, create_token(T_NUMBER, match_until_opts_but_better(ptr, ",}]", len), len), tokens_cap);
            ptr += (*len)-1;
        }
    }
    *len = 0;
    add_token(&tokens, create_token(T_EOF, crt_str("EOF", 4), len), tokens_cap);
    print_tokens(tokens);
    free(len);
    free(tokens_cap);
    return tokens;
}


typedef enum SNode_Type {
    PAIR,
    VALUE
} SNode_Type;

typedef struct SNode SNode;
struct SNode{
    SNode_Type type;
    JSONToken* parent;
    size_t size;
    size_t cap;
    union {
        JSONPair* pair_arr;
        JSONValue* val_arr;
    };
    SNode *next;
};

typedef struct Stack {
    SNode* head;
} Stack;

#define STACK_ARRAY_SIZE 16 

bool push(Stack *stack, JSONToken* parent, SNode_Type type) {
    SNode* node = malloc(sizeof(SNode));
    if (!node) return false;
    if(type == PAIR) {
        JSONPair* pair_arr = malloc(STACK_ARRAY_SIZE * sizeof(JSONPair));
        if (!pair_arr) return false;
        node->pair_arr = pair_arr;
    } else { 
        JSONValue* arr = malloc(STACK_ARRAY_SIZE * sizeof(JSONValue));
        if (!arr) return false;
        node->val_arr = arr;
    }
    node->parent = parent;
    node->size = 0;
    node->cap = STACK_ARRAY_SIZE;
    node->type = type;
    node->next = stack->head;
    stack->head = node;
    return true;
}

SNode* pop(Stack *stack) {
    if (stack->head == NULL) return NULL;
    SNode* tmp = stack->head;
    stack->head = stack->head->next;
    return tmp;
}

void add_to_stack_array(Stack *stack, void* val) {
    if (stack->head == NULL) exit(1); // syntax error
    SNode* head = stack->head; // tmp
    if (head->type == PAIR) {
        if (head->size >= head->cap) { // need to realloc
            head->cap += STACK_ARRAY_SIZE;
            JSONPair* tmp = realloc(head->pair_arr, head->cap * sizeof(JSONPair));
            if (!tmp) exit(1); // memory error
            head->pair_arr = tmp;
        }
        head->pair_arr[head->size++] = *((JSONPair*)val);
    } else {
        if (head->size >= head->cap) { // need to realloc
            head->cap += STACK_ARRAY_SIZE;
            JSONValue* tmp = realloc(head->val_arr, head->cap * sizeof(JSONValue));
            if (!tmp) exit(1); // memory error
            head->val_arr = tmp;
        }
        //printf("\n-- \t%d, %d\n", head->size, head->cap);
        head->val_arr[head->size++] = *((JSONValue*)val);
    }
}

JSONValue* get_value(JSONToken* tok) {
    JSONValue* res = malloc(sizeof(JSONValue));
    *res = (JSONValue){.type = JT_NULL, .str = (String){.data=NULL,.size=0}, .integer = 0};
    char* ptr;
    switch (tok->type) {
        case T_STRING:
            res->type = JT_STRING;
            res->str.size = tok->content.size;
            res->str.data = tok->content.data;
            break;
        case T_NULL:
            res->type = JT_NULL;
            break;
        case T_LBRACE:
            res->type = JT_OBJECT;
            break;
        case T_LBRACKET:
            res->type = JT_ARRAY;
            break;
        case T_NUMBER:
            ptr = tok->content.data;
            res->type = JT_NUMBER_LONG;
            for(;*ptr != '\0';++ptr) if (*ptr == '.') res->type = JT_NUMBER_DOUBLE;
            if (res->type == JT_NUMBER_LONG) res->integer = strtol(tok->content.data, &ptr, 10);
            else  res->floating_point = atof(tok->content.data);
            free(tok->content.data);
            break;
        default:
            printf("YOU ABSOLUTE FUCKIN BAFFOON\n");
            break;
    }
    return res;
}

JSONValue* json_get(JSONObject* obj, char* key){
    JSONObject* cur = obj; char* end_ptr = key; 
    size_t *len = malloc(sizeof(size_t)); *len = 0;
    JSONValue* v;
    while(*end_ptr != '\0') end_ptr++;

    while(key+*len < end_ptr) {
        size_t tmp = *len;
        char* k = match_until_opts_but_better(key+*len, ".[", len);
        (*len)+=tmp+1;
        if (*(key+*len-1) == '[') {
            v = hashtable_get(cur, k)->val;
            char* end = key+*len;
            while(*end++ != ']');
            long index = strtol(key+*len, &end, 10);
            v = v->arr->items+index;
            (*len)+=(end - (key+*len)+2);
        } else {
            v = hashtable_get(cur, k)->val;
        }
        if (key+*len < end_ptr) cur = v->object;
        free(k);
    }
    if (v == NULL) exit(1);
    free(len);

    return v;
}

void json_terminate(JSON json) {
    if (json.type == OBJECT) hashtable_free(&json.obj);
}

void value_stringify(JSONValue val, char** out);

void pair_stringify(JSONPair* pair, char** out) {
    **out = '"';
    (*out)++;
    str_cpy(*out, pair->key.data);
    *out += pair->key.size;
    **out = '"';
    (*out)++;
    **out = ':';
    (*out)++;
    value_stringify(*pair->val, out);
    **out = ',';
    (*out)++;
}

void obj_stringify(JSONObject* obj, char** out) {
    **out = '{';
    (*out)++;
    for (size_t i = 0; i < obj->length; ++i) {
        if (obj->items[i] != NULL) {
            Node* cur = obj->items[i];
            while (cur->next != NULL) {
                pair_stringify(cur->value, out);
                cur = cur->next;
            }
            pair_stringify(cur->value, out);
        }
    }
    *((*out)-1) = '}';
}

void arr_stringify(JSONArray* arr, char** out) {
    **out = '[';
    (*out)++;
    for(size_t i = 0; i < arr->length; ++i) {
        value_stringify(arr->items[i], out);
        **out = ',';
        (*out)++;
    }
    *((*out)-1) = ']';
}

void value_stringify(JSONValue val, char** out) {
    char _out[33] = {'\0'};
    size_t _len;
    switch (val.type) {
        case JT_STRING:
            _len = val.str.size;
            **out = '"'; (*out)++;
            str_cpy(*out, val.str.data);
            (*out) += _len;
            **out = '"'; (*out)++;
            break;
        case JT_NULL:
            str_cpy(*out, "null");
            (*out) += 4;
            break;
        case JT_NUMBER_LONG:
            sprintf(_out, "%lu", val.integer);
            str_cpy(*out, _out);
            _len = str_len(_out);
            (*out) += _len;
            break;
        case JT_NUMBER_DOUBLE:
            sprintf(_out, "%f", val.floating_point);
            str_cpy(*out, _out);
            _len = str_len(_out);
            (*out) += _len;
            break;
        case JT_OBJECT:
            obj_stringify(val.object, out);
            break;
        case JT_ARRAY:
            arr_stringify(val.arr, out);
            break;
    }
}

void count_obj(JSONObject* obj, size_t* count);
void count_arr(JSONArray* arr, size_t* count);

void count_value(JSONValue val, size_t* count) {
    char _out[33] = {'\0'};
    switch (val.type) {
        case JT_OBJECT:
            count_obj(val.object, count);
            break;
        case JT_ARRAY:
            count_arr(val.arr, count);
            break;
        case JT_STRING:
            (*count) += val.str.size+3;
            break;
        case JT_NUMBER_LONG:
            sprintf(_out, "%lu", val.integer);
            (*count) += str_len(_out)+1;
            break;
        case JT_NUMBER_DOUBLE:
            sprintf(_out, "%f", val.floating_point);
            (*count) += str_len(_out)+1;
            break;
        case JT_NULL:
            (*count) += 5;
    }
}

void count_pair(JSONPair* pair, size_t* count) {
    (*count) += pair->key.size+3;
    count_value(*pair->val, count);
}

void count_obj(JSONObject* obj, size_t* count) {
    for (size_t i = 0; i < obj->length; ++i) {
        if (obj->items[i] != NULL) {
            Node* cur = obj->items[i];
            while (cur->next != NULL) {
                count_pair(cur->value, count);
                cur = cur->next;
            }
            count_pair(cur->value, count);
        }
    }
    (*count)+=2;
}

void count_arr(JSONArray* arr, size_t* count) {
    for(size_t i = 0; i < arr->length; ++i) {
        count_value(arr->items[i], count);
    }
    (*count)+=1;
}

size_t count_chars(JSON json) {
    size_t count = 0;
    if(json.type == OBJECT) count_obj(json.obj, &count);
    else count_arr(json.arr, &count);
    //printf("\n\t--- %lu\n", count);
    return count;
}

char* json_stringify(JSON json) {
    size_t count = count_chars(json)+1;
    char* out = calloc(1, count); char* ptr = out;
    if(json.type == OBJECT) obj_stringify(json.obj, &ptr);
    else arr_stringify(json.arr, &ptr);
    out[count] = '\0';
    //printf("\n\t--- %lu\n", str_len(out));
    return out;
}

JSON // change to JSONObject
ast(JSONToken *tokens) {
    //printf("--- AST ---\n");
    // inital error handling here tbh
    size_t i = 0;
    Stack tracker = (Stack){.head = NULL}; 
    SNode* tmp = NULL, *check = NULL;
    JSON result;
    JSONValue storage;
    for (JSONToken *ptr = tokens; ptr->type != T_EOF; ++ptr){
        switch (ptr->type) {
            case T_LBRACE:
                // JSONObject
                push(&tracker, ptr, PAIR);
                if (ptr == tokens) check = tracker.head;
                //printf("$ %s\n\n", (ptr+1)->content.data);
                break;
            case T_RBRACE: // populate from tmp->pair_arr
                tmp = pop(&tracker);
                JSONObject* obj = hashtable_init(tmp->size);
                for(size_t i = 0; i < tmp->size; ++i) hashtable_add(obj, tmp->pair_arr+i);
                if (tmp == check) {
                    result.type = OBJECT;
                    result.obj = obj;
                } else if (((tmp->parent)-1)->type == T_COLON) {
                    JSONValue* val = malloc(sizeof(JSONValue));
                    *val = (JSONValue){.type = JT_OBJECT, .object = obj};
                    add_to_stack_array(&tracker, &(JSONPair){.key = ((tmp->parent)-2)->content, .val = val});
                } else {
                    JSONValue* val = malloc(sizeof(JSONValue));
                    *val = (JSONValue){JT_OBJECT, .object = obj};
                    add_to_stack_array(&tracker, val);
                }
                free(tmp->pair_arr);
                free(tmp);
                //hashtable_free(obj);
                break;
            case T_LBRACKET:
                push(&tracker, ptr, VALUE);
                if (ptr == tokens) check = tracker.head;
                break;
            case T_RBRACKET: // populate from tmp->val_arr
                tmp = pop(&tracker);
                JSONArray* arr = malloc(sizeof(JSONArray));
                arr->length = tmp->size;
                arr->items = tmp->val_arr;
                if (tmp == check) {
                    result.type = ARRAY;
                    result.arr = arr;
                } else if (((tmp->parent)-1)->type == T_COLON) {
                    JSONValue* val = malloc(sizeof(JSONValue));
                    *val = (JSONValue){.type=JT_ARRAY, .arr = arr};
                    add_to_stack_array(&tracker, &(JSONPair){.key = ((tmp->parent)-2)->content, .val = val});
                } else {
                    JSONValue* val = malloc(sizeof(JSONValue));
                    *val = (JSONValue){JT_ARRAY,.arr = arr};
                    add_to_stack_array(&tracker, val);
                }
                free(tmp);
                break;
            case T_COLON: // JSONPair; handle prev and next
                if (tracker.head->type != PAIR) printf("PROBLEM.");
                if((ptr+1)->type != T_LBRACKET && (ptr+1)->type != T_LBRACE) {
                    add_to_stack_array(&tracker, (void*)(&(JSONPair){.key = (ptr-1)->content, .val = get_value((ptr+1))}));
                    ptr+=1;
                } else {
                
                }
                break;
            case T_NUMBER: // if prev = , or prev = [ add_to_stack_array, else ignore
                if ((ptr-1)->type == T_COMMA || (ptr-1)->type == T_LBRACKET) add_to_stack_array(&tracker, get_value(ptr));
                break;
            case T_STRING: // if prev = , or prev = [ add_to_stack_array, else ignore
                if ((ptr+1)->type != T_COLON) {
                    JSONValue* str = get_value(ptr);
                    add_to_stack_array(&tracker, str);
                    free(str);
                }
                break;
            case T_COMMA: // mostly ignore for now
                break;
            case T_NULL: // if prev = , or prev = [ add_to_stack_array, else ignore
                if ((ptr-1)->type == T_COMMA || (ptr-1)->type == T_LBRACKET) add_to_stack_array(&tracker, get_value(ptr));
                break;
            case T_EOF:
                break;
        }
    }
    return result;
}

void free_tokens(JSONToken** tokens) {
    JSONToken* ptr = *tokens;
    for(;ptr->type != T_EOF; ++ptr) {
        switch (ptr->type) {
            case T_STRING: case T_NUMBER:
                break;
            default:
                free(ptr->content.data);
        }
    }
    free(ptr->content.data);
    free(*tokens);
}

JSON json_load(const char* src) {
    long* length = malloc(sizeof(long));
    int result = access(src, R_OK);
    if (result == -1) { 
        log_err("json_load failed"); 
    }
    char* buff = read_file(src, length);
    //printf("%s\n", buff);
    buff = lex(buff);
    //printf("%s\n", buff);
    JSONToken* tokens = tokenize(buff);
    free(buff);
    JSON res = ast(tokens);
    free(length);
    free_tokens(&tokens);
    return res;
}

// http://www.cse.yorku.ca/~oz/hash.html#sdbm
size_t hash_function(char* key, size_t size) {
    unsigned long hash = 0;
    int c;
    while((c = *key++))
        hash = c + (hash << 6) + (hash << 16) - hash;

    return hash % size;
}
// http://www.cse.yorku.ca/~oz/hash.html#sdbm


JSONObject* hashtable_init(size_t size) {
    JSONObject* obj = malloc(sizeof(JSONObject));
    obj->items = malloc(size * sizeof(Node*));
    for(size_t i=0; i < size; ++i) {
        obj->items[i] = NULL;
    }
    obj->length = size;
    return obj;
}

JSONPair* hashtable_get(JSONObject* obj, char* key) {
    size_t i = hash_function(key, obj->length);
    if (obj->items[i] != NULL) {
        Node* cur = obj->items[i];
        while (cur != NULL) {
            if (cur->value == NULL) return NULL;
            if(str_cmp(cur->value->key.data, key)) {
                cur = cur->next;
                continue;
            }
            return cur->value;
        }
    }
    return NULL;
}

void hashtable_add(JSONObject* obj, JSONPair* pair) {
    size_t i = hash_function(pair->key.data, obj->length);
    if (obj->items[i] != NULL) {
        Node* cur = obj->items[i];
        while (cur->next != NULL) cur = cur->next;
        cur->next = malloc(sizeof(Node));
        JSONPair* _pair = malloc(sizeof(JSONPair));
        *_pair = *pair;
        *(cur->next) = (Node){.value = _pair, .next = NULL};
        return;
    }
    JSONPair* _pair = malloc(sizeof(JSONPair));
    *_pair = *pair;
    obj->items[i] = (Node*)malloc(sizeof(Node));
    *obj->items[i] = (Node){.value = _pair, .next = NULL};
    return;
}

void array_free(JSONArray** arr) {
    for(size_t i = 0; i < (*arr)->length; ++i) {
        switch ((*arr)->items[i].type) {
            case JT_STRING:
                free((*arr)->items[i].str.data);
                break;
            case JT_OBJECT:
                hashtable_free(&((*arr)->items[i].object));
                break;
            case JT_ARRAY:
                array_free(&((*arr)->items[i].arr));
                break;
            default:
                break;
        }
    }
    free((*arr)->items);
    free(*arr);
}

void value_free(JSONValue** val) {
    switch ((*val)->type) {
        case JT_STRING:
            free((*val)->str.data);
            break;
        case JT_OBJECT:
            hashtable_free(&((*val)->object));
            break;
        case JT_ARRAY:
            array_free(&((*val)->arr));
            break;
        default:
            break;
    }
    free(*val);
}

void pair_free(JSONPair** pair) {
    free((*pair)->key.data);
    value_free(&((*pair)->val));
    free(*pair);
}

void hashtable_free(JSONObject** obj) {
    if ((*obj)->items == NULL) return;
    for (size_t i = 0; i < (*obj)->length; ++i) {
        //printf("\n\t---  %lu, %lu\n", i, (*obj)->length);
        Node* cur = (*obj)->items[i];
        if (cur == NULL) continue;
        while (cur->next != NULL) {
            pair_free(&(cur->value));
            Node* tmp = cur;
            cur = cur->next;
            free(tmp);
        }
        pair_free(&cur->value);
        free(cur);
    }
    free((*obj)->items);
    free(*obj);
}

