#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "./include/jayson/json.h"
#include "./include/jayson/logging.h"
#include "util.h"

#define jayson_CHUNK   16 // default number of tokens

struct jayson_Settings {
    long flags : 6;
};

struct jayson_Settings g_settings;

void jayson_print_tokens(JSONToken* tokens);
JSONObject* jayson_hashtable_init(size_t size);
JSONPair* jayson_hashtable_get(JSONObject* obj, char* key);
void jayson_hashtable_add(JSONObject* obj, JSONPair* key);
void jayson_hashtable_free(JSONObject** obj);

void
json_init(int flags) {
    g_settings.flags = flags;
    //printf("\n\tSettings :\t%u\n\n", g_settings.flags);
}


bool
jayson_add_token(JSONToken** tokens, JSONToken token, size_t *tokens_cap, size_t count) {
    if (count >= *tokens_cap) {
        *tokens_cap += jayson_CHUNK;
        JSONToken* tmp = (JSONToken*)realloc(*tokens, (*tokens_cap) * sizeof(JSONToken));
        if (!tmp) {
            printf("wha\n\n");
            return false;
        }
        *tokens = tmp;
        //printf("\t\tadd_token - tokens_cap: %lu, count: %lu\n\n", *tokens_cap, count);
    }
    //if(token.type != T_EOF) printf("-[ type: %d, content: { .data = '%s', \t .size = %lu} ], count: %lu\n", token.type, token.content.data, token.content.size, count);

    (*tokens)[count] = token;
    //printf("hi\n\n");
    //printf("----- [ type: %d, content: { .data = '%s', \t .size = %lu} ], count: %lu\n", (*tokens)[count-1].type, (*tokens)[count-1].content.data, (*tokens)[count-1].content.size, count);
    return true;
}

JSONToken
jayson_create_token(jayson_TokenType type, char* str, size_t *len) {
    //printf("1: %d, 2: %s, 3: %lu", type, str, *len);
    if (str == NULL) exit(-1);
    return (JSONToken){ .type = type, .content = (jayson_String){ .size = *len, .data = str}};
}

void jayson_print_tokens(JSONToken* tokens) {
    JSONToken* ptr = tokens;
    for (;ptr->type != T_EOF;++ptr) {
#ifdef DEBUG
        printf("[ type: %s, content: { .data = '%s', \t .size = %lu} ],\n", lookup[ptr->type], ptr->content.data, ptr->content.size);
#else
        //printf("[ type: %d, content: { .data = '%s', \t .size = %lu} ],\n", ptr->type, ptr->content.data, ptr->content.size);
#endif
    }
}

int jayson_pair_count = 0;

JSONToken*
jayson_tokenize(char* buff) {
    size_t* tokens_cap = malloc(sizeof(size_t));
    *tokens_cap = jayson_CHUNK;
    JSONToken* tokens = malloc(jayson_CHUNK * sizeof(JSONToken));
    size_t* len = calloc(1, sizeof(size_t));
    size_t count = 0;
    for(char* ptr = buff; *(ptr) != '\0';++ptr) {
        switch (*ptr) {
            case '"': {
                if (*(ptr-1) != ':') jayson_pair_count++;
                jayson_add_token(&tokens, jayson_create_token(T_STRING, jayson_match_until_but_better(ptr+1, '"', len), len), tokens_cap, count);
                ptr += (*len+1);
            } break;
            case ':': {
                *len=1;
                jayson_add_token(&tokens, jayson_create_token(T_COLON, jayson_crt_str(":", 2), len), tokens_cap, count);
            } break;
            case ',': {
                *len=1;
                jayson_add_token(&tokens, jayson_create_token(T_COMMA, jayson_crt_str(",", 2), len), tokens_cap, count);
            } break;
            case '{': {
                *len=1;
                jayson_add_token(&tokens, jayson_create_token(T_LBRACE, jayson_crt_str("{", 2), len), tokens_cap, count);
            } break;
            case '}': {
                *len=1;
                jayson_add_token(&tokens, jayson_create_token(T_RBRACE, jayson_crt_str("}", 2), len), tokens_cap, count);
            } break;
            case '[': {
                *len=1;
                jayson_add_token(&tokens, jayson_create_token(T_LBRACKET, jayson_crt_str("[", 2), len), tokens_cap, count);
                //printf("$ %s", tokens[0].content.data);
            } break;
            case ']': {
                *len=1;
                jayson_add_token(&tokens, jayson_create_token(T_RBRACKET, jayson_crt_str("]", 2), len), tokens_cap, count);
            } break;
            case 'n': {
                if(jayson_check_match(ptr, "null", 4) == true) {
                    *len=4;
                    jayson_add_token(&tokens, jayson_create_token(T_NULL, jayson_crt_str("null", 5), len), tokens_cap, count);
                    ptr+=3;
                }
            } break;
        }
        //printf("-- \t%c\n\n", *ptr);
        if ('0' <= *ptr && *ptr <= '9' || *ptr == '.' || *ptr == '-') {
            jayson_add_token(&tokens, jayson_create_token(T_NUMBER, jayson_match_until_opts_but_better(ptr, ",}]", len), len), tokens_cap, count);
            ptr += (*len)-1;
        }
        count++;
    }
    *len = 0;
    jayson_add_token(&tokens, jayson_create_token(T_EOF, jayson_crt_str("EOF", 4), len), tokens_cap, count);
    jayson_print_tokens(tokens);
    free(len);
    free(tokens_cap);
    return tokens;
}


typedef enum jayson_SNode_Type {
    PAIR,
    VALUE
} jayson_SNode_Type;

typedef struct jayson_SNode jayson_SNode;
struct jayson_SNode{
    jayson_SNode_Type type;
    JSONToken* parent;
    size_t size;
    size_t cap;
    union {
        JSONPair* pair_arr;
        JSONValue* val_arr;
    };
    jayson_SNode *next;
};

typedef struct jayson_Stack {
    jayson_SNode* head;
} jayson_Stack;

#define STACK_ARRAY_SIZE 16 

bool jayson_push(jayson_Stack *stack, JSONToken* parent, jayson_SNode_Type type) {
    jayson_SNode* node = malloc(sizeof(jayson_SNode));
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

jayson_SNode* jayson_pop(jayson_Stack *stack) {
    if (stack->head == NULL) return NULL;
    jayson_SNode* tmp = stack->head;
    stack->head = stack->head->next;
    return tmp;
}

void jayson_add_to_stack_array(jayson_Stack *stack, void* val) {
    if (stack->head == NULL) exit(1); // syntax error
    jayson_SNode* head = stack->head; // tmp
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

JSONValue* jayson_get_value(JSONToken* tok) {
    JSONValue* res = malloc(sizeof(JSONValue));
    *res = (JSONValue){.type = JT_NULL, .str = (jayson_String){.data=NULL,.size=0}, .integer = 0};
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
        char* k = jayson_match_until_opts_but_better(key+*len, ".[", len);
        (*len)+=tmp+1;
        if (*(key+*len-1) == '[') {
            v = jayson_hashtable_get(cur, k)->val;
            char* end = key+*len;
            while(*end++ != ']');
            long index = strtol(key+*len, &end, 10);
            v = v->arr->items+index;
            (*len)+=(end - (key+*len)+2);
        } else {
            v = jayson_hashtable_get(cur, k)->val;
        }
        if (key+*len < end_ptr) cur = v->object;
        free(k);
    }
    if (v == NULL) exit(1);
    free(len);

    return v;
}

void json_terminate(JSON json) {
    if (json.type == OBJECT) jayson_hashtable_free(&json.obj);
}

void jayson_value_stringify(JSONValue val, char** out);

void jayson_pair_stringify(JSONPair* pair, char** out) {
    **out = '"';
    (*out)++;
    jayson_str_cpy(*out, pair->key.data);
    *out += pair->key.size;
    **out = '"';
    (*out)++;
    **out = ':';
    (*out)++;
    jayson_value_stringify(*pair->val, out);
    **out = ',';
    (*out)++;
}

void jayson_obj_stringify(JSONObject* obj, char** out) {
    **out = '{';
    (*out)++;
    for (size_t i = 0; i < obj->length; ++i) {
        if (obj->items[i] != NULL) {
            jayson_Node* cur = obj->items[i];
            while (cur->next != NULL) {
                jayson_pair_stringify(cur->value, out);
                cur = cur->next;
            }
            jayson_pair_stringify(cur->value, out);
        }
    }
    *((*out)-1) = '}';
}

void jayson_arr_stringify(JSONArray* arr, char** out) {
    **out = '[';
    (*out)++;
    for(size_t i = 0; i < arr->length; ++i) {
        jayson_value_stringify(arr->items[i], out);
        **out = ',';
        (*out)++;
    }
    *((*out)-1) = ']';
}

void jayson_value_stringify(JSONValue val, char** out) {
    char _out[33] = {'\0'};
    size_t _len;
    switch (val.type) {
        case JT_STRING:
            _len = val.str.size;
            **out = '"'; (*out)++;
            jayson_str_cpy(*out, val.str.data);
            (*out) += _len;
            **out = '"'; (*out)++;
            break;
        case JT_NULL:
            jayson_str_cpy(*out, "null");
            (*out) += 4;
            break;
        case JT_NUMBER_LONG:
            sprintf(_out, "%lu", val.integer);
            jayson_str_cpy(*out, _out);
            _len = jayson_str_len(_out);
            (*out) += _len;
            break;
        case JT_NUMBER_DOUBLE:
            sprintf(_out, "%f", val.floating_point);
            jayson_str_cpy(*out, _out);
            _len = jayson_str_len(_out);
            (*out) += _len;
            break;
        case JT_OBJECT:
            jayson_obj_stringify(val.object, out);
            break;
        case JT_ARRAY:
            jayson_arr_stringify(val.arr, out);
            break;
    }
}

void jayson_count_obj(JSONObject* obj, size_t* count);
void jayson_count_arr(JSONArray* arr, size_t* count);

void jayson_count_value(JSONValue val, size_t* count) {
    char _out[33] = {'\0'};
    switch (val.type) {
        case JT_OBJECT:
            jayson_count_obj(val.object, count);
            break;
        case JT_ARRAY:
            jayson_count_arr(val.arr, count);
            break;
        case JT_STRING:
            (*count) += val.str.size+3;
            break;
        case JT_NUMBER_LONG:
            sprintf(_out, "%lu", val.integer);
            (*count) += jayson_str_len(_out)+1;
            break;
        case JT_NUMBER_DOUBLE:
            sprintf(_out, "%f", val.floating_point);
            (*count) += jayson_str_len(_out)+1;
            break;
        case JT_NULL:
            (*count) += 5;
    }
}

void jayson_count_pair(JSONPair* pair, size_t* count) {
    (*count) += pair->key.size+3;
    jayson_count_value(*pair->val, count);
}

void jayson_count_obj(JSONObject* obj, size_t* count) {
    for (size_t i = 0; i < obj->length; ++i) {
        if (obj->items[i] != NULL) {
            jayson_Node* cur = obj->items[i];
            while (cur->next != NULL) {
                jayson_count_pair(cur->value, count);
                cur = cur->next;
            }
            jayson_count_pair(cur->value, count);
        }
    }
    (*count)+=2;
}

void jayson_count_arr(JSONArray* arr, size_t* count) {
    for(size_t i = 0; i < arr->length; ++i) {
        jayson_count_value(arr->items[i], count);
    }
    (*count)+=1;
}

size_t jayson_count_chars(JSON json) {
    size_t count = 0;
    if(json.type == OBJECT) jayson_count_obj(json.obj, &count);
    else jayson_count_arr(json.arr, &count);
    //printf("\n\t--- %lu\n", count);
    return count;
}

char* json_stringify(JSON json) {
    size_t count = jayson_count_chars(json)+1;
    char* out = calloc(1, count); char* ptr = out;
    if(json.type == OBJECT) jayson_obj_stringify(json.obj, &ptr);
    else jayson_arr_stringify(json.arr, &ptr);
    out[count] = '\0';
    //printf("\n\t--- %lu\n", str_len(out));
    return out;
}

JSON // change to JSONObject
jayson_ast(JSONToken *tokens) {
    //printf("--- AST ---\n");
    // inital error handling here tbh
    size_t i = 0;
    jayson_Stack tracker = (jayson_Stack){.head = NULL}; 
    jayson_SNode* tmp = NULL, *check = NULL;
    JSON result;
    JSONValue storage;
    for (JSONToken *ptr = tokens; ptr->type != T_EOF; ++ptr){
        switch (ptr->type) {
            case T_LBRACE:
                // JSONObject
                jayson_push(&tracker, ptr, PAIR);
                if (ptr == tokens) check = tracker.head;
                //printf("$ %s\n\n", (ptr+1)->content.data);
                break;
            case T_RBRACE: // populate from tmp->pair_arr
                tmp = jayson_pop(&tracker);
                JSONObject* obj = jayson_hashtable_init(tmp->size);
                for(size_t i = 0; i < tmp->size; ++i) jayson_hashtable_add(obj, tmp->pair_arr+i);
                if (tmp == check) {
                    result.type = OBJECT;
                    result.obj = obj;
                } else if (((tmp->parent)-1)->type == T_COLON) {
                    JSONValue* val = malloc(sizeof(JSONValue));
                    *val = (JSONValue){.type = JT_OBJECT, .object = obj};
                    jayson_add_to_stack_array(&tracker, &(JSONPair){.key = ((tmp->parent)-2)->content, .val = val});
                } else {
                    JSONValue* val = malloc(sizeof(JSONValue));
                    *val = (JSONValue){JT_OBJECT, .object = obj};
                    jayson_add_to_stack_array(&tracker, val);
                }
                free(tmp->pair_arr);
                free(tmp);
                break;
            case T_LBRACKET:
                jayson_push(&tracker, ptr, VALUE);
                if (ptr == tokens) check = tracker.head;
                break;
            case T_RBRACKET: // populate from tmp->val_arr
                tmp = jayson_pop(&tracker);
                JSONArray* arr = malloc(sizeof(JSONArray));
                arr->length = tmp->size;
                arr->items = tmp->val_arr;
                if (tmp == check) {
                    result.type = ARRAY;
                    result.arr = arr;
                } else if (((tmp->parent)-1)->type == T_COLON) {
                    JSONValue* val = malloc(sizeof(JSONValue));
                    *val = (JSONValue){.type=JT_ARRAY, .arr = arr};
                    jayson_add_to_stack_array(&tracker, &(JSONPair){.key = ((tmp->parent)-2)->content, .val = val});
                } else {
                    JSONValue* val = malloc(sizeof(JSONValue));
                    *val = (JSONValue){JT_ARRAY,.arr = arr};
                    jayson_add_to_stack_array(&tracker, val);
                }
                free(tmp);
                break;
            case T_COLON: // JSONPair; handle prev and next
                if (tracker.head->type != PAIR) printf("PROBLEM.");
                if((ptr+1)->type != T_LBRACKET && (ptr+1)->type != T_LBRACE) {
                    jayson_add_to_stack_array(&tracker, (void*)(&(JSONPair){.key = (ptr-1)->content, .val = jayson_get_value((ptr+1))}));
                    ptr+=1;
                } else {
                
                }
                break;
            case T_NUMBER: // if prev = , or prev = [ add_to_stack_array, else ignore
                if ((ptr-1)->type == T_COMMA || (ptr-1)->type == T_LBRACKET) jayson_add_to_stack_array(&tracker, jayson_get_value(ptr));
                break;
            case T_STRING: // if prev = , or prev = [ add_to_stack_array, else ignore
                if ((ptr+1)->type != T_COLON) {
                    JSONValue* str = jayson_get_value(ptr);
                    jayson_add_to_stack_array(&tracker, str);
                    free(str);
                }
                break;
            case T_COMMA: // mostly ignore for now
                break;
            case T_NULL: // if prev = , or prev = [ add_to_stack_array, else ignore
                if ((ptr-1)->type == T_COMMA || (ptr-1)->type == T_LBRACKET) jayson_add_to_stack_array(&tracker, jayson_get_value(ptr));
                break;
            case T_EOF:
                break;
        }
    }
    return result;
}

void jayson_free_tokens(JSONToken** tokens) {
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
    if (g_settings.flags & O_READ_FROM_FILE) {
        int result = access(src, R_OK);
        if (result == -1) { 
            jayson_log_err("json_load failed"); 
        }
    }
    char* buff = g_settings.flags & O_DYNAMIC ? (char*)src : jayson_read_file(src, length);
    //printf("%s\n", buff);
    buff = jayson_lex(buff);
    //printf("%s\n", buff);
    JSONToken* tokens = jayson_tokenize(buff);
    if (g_settings.flags & O_READ_FROM_FILE) free(buff);
    JSON res = jayson_ast(tokens);
    free(length);
    jayson_free_tokens(&tokens);
    return res;
}

char json_load_to(const char* src, JSON* json) {
    long* length = malloc(sizeof(long));
    if (g_settings.flags & O_READ_FROM_FILE) {
        int result = access(src, R_OK);

        if (result == -1) { 
            jayson_log_err("json_load failed"); 
            return 0;
        }
        
    }
    char* buff = g_settings.flags & O_DYNAMIC ? (char*)src : jayson_read_file(src, length);
    buff = jayson_lex(buff);
    JSONToken* tokens = jayson_tokenize(buff);
    if (g_settings.flags & O_READ_FROM_FILE) free(buff);
    *json = jayson_ast(tokens);
    free(length);
    jayson_free_tokens(&tokens);
    return 1;
}

// http://www.cse.yorku.ca/~oz/hash.html#sdbm
size_t jayson_hash_function(char* key, size_t size) {
    unsigned long hash = 0;
    int c;
    while((c = *key++))
        hash = c + (hash << 6) + (hash << 16) - hash;

    return hash % size;
}
// http://www.cse.yorku.ca/~oz/hash.html#sdbm


JSONObject* jayson_hashtable_init(size_t size) {
    JSONObject* obj = malloc(sizeof(JSONObject));
    obj->items = malloc(size * sizeof(jayson_Node*));
    for(size_t i=0; i < size; ++i) {
        obj->items[i] = NULL;
    }
    obj->length = size;
    return obj;
}

JSONPair* jayson_hashtable_get(JSONObject* obj, char* key) {
    size_t i = jayson_hash_function(key, obj->length);
    if (obj->items[i] != NULL) {
        jayson_Node* cur = obj->items[i];
        while (cur != NULL) {
            if (cur->value == NULL) return NULL;
            if(jayson_str_cmp(cur->value->key.data, key)) {
                cur = cur->next;
                continue;
            }
            return cur->value;
        }
    }
    return NULL;
}

void jayson_hashtable_add(JSONObject* obj, JSONPair* pair) {
    size_t i = jayson_hash_function(pair->key.data, obj->length);
    if (obj->items[i] != NULL) {
        jayson_Node* cur = obj->items[i];
        while (cur->next != NULL) cur = cur->next;
        cur->next = malloc(sizeof(jayson_Node));
        JSONPair* _pair = malloc(sizeof(JSONPair));
        *_pair = *pair;
        *(cur->next) = (jayson_Node){.value = _pair, .next = NULL};
        return;
    }
    JSONPair* _pair = malloc(sizeof(JSONPair));
    *_pair = *pair;
    obj->items[i] = (jayson_Node*)malloc(sizeof(jayson_Node));
    *obj->items[i] = (jayson_Node){.value = _pair, .next = NULL};
    return;
}

void jayson_array_free(JSONArray** arr) {
    for(size_t i = 0; i < (*arr)->length; ++i) {
        switch ((*arr)->items[i].type) {
            case JT_STRING:
                free((*arr)->items[i].str.data);
                break;
            case JT_OBJECT:
                jayson_hashtable_free(&((*arr)->items[i].object));
                break;
            case JT_ARRAY:
                jayson_array_free(&((*arr)->items[i].arr));
                break;
            default:
                break;
        }
    }
    free((*arr)->items);
    free(*arr);
}

void jayson_value_free(JSONValue** val) {
    switch ((*val)->type) {
        case JT_STRING:
            free((*val)->str.data);
            break;
        case JT_OBJECT:
            jayson_hashtable_free(&((*val)->object));
            break;
        case JT_ARRAY:
            jayson_array_free(&((*val)->arr));
            break;
        default:
            break;
    }
    free(*val);
}

void jayson_pair_free(JSONPair** pair) {
    free((*pair)->key.data);
    jayson_value_free(&((*pair)->val));
    free(*pair);
}

void jayson_hashtable_free(JSONObject** obj) {
    if ((*obj)->items == NULL) return;
    for (size_t i = 0; i < (*obj)->length; ++i) {
        //printf("\n\t---  %lu, %lu\n", i, (*obj)->length);
        jayson_Node* cur = (*obj)->items[i];
        if (cur == NULL) continue;
        while (cur->next != NULL) {
            jayson_pair_free(&(cur->value));
            jayson_Node* tmp = cur;
            cur = cur->next;
            free(tmp);
        }
        jayson_pair_free(&cur->value);
        free(cur);
    }
    free((*obj)->items);
    free(*obj);
}

