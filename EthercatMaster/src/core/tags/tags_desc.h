#pragma once
#include <stdint.h>
#include <stdbool.h>

#define SIZE_OF_BOOL	1
#define SIZE_OF_INT		2
#define SIZE_OF_DINT	4
#define SIZE_OF_WORD	2
#define SIZE_OF_DWORD	4
#define SIZE_OF_REAL	4

typedef enum {
	PLC_BOOL,
	PLC_INT, //16bits signé
	PLC_DINT, //32bits  signé
	PLC_WORD, //16bits non signé
	PLC_DWORD, //32bits non signé
	PLC_REAL,
	PLC_STRING,
	PLC_ARRAY
} PLC_DataType_t;

typedef enum {
	PLC_IN,
	PLC_OUT,
	PLC_HMI,
	PLC_PV
} PLC_VarType_t;


typedef union {
    bool bool_value;          // Pour les booléens
    int int_value;            // Pour les entiers
    int32_t dint_value;       // Pour les entiers de 32 bits
    uint16_t word_value;      // Pour les entiers non signés
    uint32_t dword_value;     // Pour les entiers non signés de 32 bits
    float real_value;         // Pour les réels
    char* string_value;       // Pour les chaînes de caractères
    void* array_value;        // Pour les tableaux (pointeur générique)
} PLC_TagValue_t;




