#include "ethercat_slaves.h"


typedef enum DATA_TYPE {
	PLC_BOOL, PLC_BYTE, PLC_WORD, PLC_DWORD, PLC_REAL, PLC_STRING
};

typedef enum DATA_TYPE2 {
	SL_INPUT, SL_OUTPUT, SL_INTERN, SL_PV //process value
};

typedef struct IOTag {
	const char* name;
	Slave_PDO_t* pdo;
	size_t offset;
	int bit_index;
	DATA_TYPE type;
	DATA_TYPE2 type2;
	void* ptr;
} IOTag_t;



void set_iotag(IOTag_t* iotag, const char* name, Slave_PDO_t* pdo, size_t offset, int bit_index, DATA_TYPE type1, DATA_TYPE2 type2);

// Macro interne pour bool
#define PLC_WRITE_BOOL(tag, val) do {                        \
    Slave_PDO_RX_t* rx = &((tag)->pdo->PDO_rx);             \
    uint8_t* base = (uint8_t*)rx->buf[2];                  \
    uint8_t* byte = base + (tag)->offset;                  \
    if (val)                                               \
        *byte |= (1u << (tag)->bit_index);                \
    else                                                   \
        *byte &= ~(1u << (tag)->bit_index);               \
    atomic_store(&rx->dirty, true);                        \
} while(0)

// Macro interne pour uint8, uint16, uint32, float
#define PLC_WRITE_RAW(tag, val, TYPE) do {                  \
    Slave_PDO_RX_t* rx = &((tag)->pdo->PDO_rx);             \
    TYPE* ptr = (TYPE*)((uint8_t*)rx->buf[2] + (tag)->offset); \
    *ptr = val;                                            \
    atomic_store(&rx->dirty, true);                        \
} while(0)

// Fonction générique
static inline void plc_write(const IOTag_t* tag, auto value) {
    if (!tag || tag->type2 == SL_INPUT) return;

    switch (tag->type) {
    case PLC_BOOL:   PLC_WRITE_BOOL(tag, value); break;
    case PLC_BYTE:  PLC_WRITE_RAW(tag, value, uint8_t); break;
    case PLC_WORD: PLC_WRITE_RAW(tag, value, uint16_t); break;
    case PLC_DWORD: PLC_WRITE_RAW(tag, value, uint32_t); break;
    case PLC_REAL:   PLC_WRITE_RAW(tag, value, float); break;
    default: break;
    }
}
