#ifndef JSON_H
#define JSON_H
#include <stddef.h>

// TODO:    jsonObject: hashtable, 
//          jsonArray: easier to work with array, length, item type, item type size, 
//          jsonString: easier to work with strings,
//          jsonNumber: a generic number type for floats, ints, and whatevers,

enum jayson_options {
    O_READ_FROM_FILE = 1,
    O_DYNAMIC = 1<<1,
    O_ALLOW_COMMENTS = 1<<2,
    O_ALLOW_TRAILING_COMMAS = 1<<3,
    // more options later
};
//#define jayson_DEBUG

#ifdef jayson_DEBUG

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
typedef struct jayson_Node jayson_Node;

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
} jayson_TokenType;

typedef struct {
    size_t size;
    char* data;
} jayson_String;

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
    jayson_Node** items;
} JSONObject;

struct JSONArray {
    size_t length;
    struct JSONValue* items;
};

struct JSONValue {
    JSONType type;
    union {
        jayson_String str;
        union {
            long integer;
            double floating_point;
        };
        JSONObject* object;
        JSONArray* arr;
    };
};

struct JSONPair {
    jayson_String key;
    struct JSONValue* val;
};

struct jayson_Node {
    JSONPair* value;
    jayson_Node* next;
};

typedef struct {
    jayson_TokenType type;
    jayson_String content;
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

__attribute__((depricated("Use json_load_from_file instead")))
JSON json_load(const char* src);
char json_load_to(const char* src, JSON* json);
JSONValue* json_get(JSONObject* obj, char* key);
char* json_stringify(JSON obj);
void json_terminate(JSON json);

#endif
