#ifndef JSON_H
#define JSON_H
#include <stddef.h>

// TODO:    jsonObject: hashtable, 
//          jsonArray: easier to work with array, length, item type, item type size, 
//          jsonString: easier to work with strings,
//          jsonNumber: a generic number type for floats, ints, and whatevers,

enum options {
    O_READ_FROM_FILE = 1,
    O_DYNAMIC = 1<<1,
    O_ALLOW_COMMENTS = 1<<2,
    O_ALLOW_TRAILING_COMMAS = 1<<3,
    // more options later
};
//#define DEBUG

#ifdef DEBUG

const char* lookup[] = {
    "NULL",
    "Infinity",
    "NaN",
    "LBRACE",
    "RBRACE",
    "LBRACKET",
    "RBRACKET",
    "NUMBER",
    "STRING",
    "COLON",
    "COMMA",
    "EOF"
};

#endif

typedef struct JSONArray JSONArray;
typedef struct JSONValue JSONValue;
typedef struct JSONPair JSONPair;
typedef struct Node Node;

typedef enum {
    T_NULL,
    T_LBRACE,
    T_RBRACE,
    T_LBRACKET,
    T_RBRACKET,
    T_NUMBER,
    T_STRING,
    T_COLON,
    T_COMMA,
    T_EOF, // special terminator token
} JaysonTokenType;

typedef struct {
    size_t size;
    char* data;
} String;

typedef enum {
    JT_STRING,
    JT_NUMBER_LONG,
    JT_NUMBER_DOUBLE,
    JT_OBJECT,
    JT_ARRAY,
    JT_NULL,
} JSONType;

typedef struct {
    size_t length;
    Node** items;
} JSONObject;

struct JSONArray {
    size_t length;
    struct JSONValue* items;
};

struct JSONValue {
    JSONType type;
    union {
        String str;
        union {
            long integer;
            double floating_point;
        };
        JSONObject* object;
        JSONArray* arr;
    };
};

struct JSONPair {
    String key;
    struct JSONValue* val;
};

struct Node {
    JSONPair* value;
    Node* next;
};

typedef struct {
    JaysonTokenType type;
    String content;
} JSONToken;

typedef struct JSON {
    enum {
        OBJECT,
        ARRAY
    } type;
    union {
        JSONObject* obj;
        JSONArray* arr;
    };
} JSON;

void json_init(int flags);
JSON json_load(const char* src);
JSONToken* tokenize(char* buff);
JSONValue* json_get(JSONObject* obj, char* key);
char* json_stringify(JSON obj);
void json_terminate(JSON json);

#endif
