#include "plc_io.h"

void set_iotag(IOTag_t* iotag, const char* name, Slave_PDO_t* pdo, size_t offset, int bit_index, DATA_TYPE type1, DATA_TYPE2 type2) {
    iotag->name = name;
    iotag->pdo = pdo;
    iotag->offset = offset;
    iotag->bit_index = bit_index;
    iotag->type = type1;
    iotag->type2 = type2;
}


int plc_read8(const IOTag_t* tag, uint8_t* value)
{
    if (!tag || tag->type2 == SL_INPUT || !value)
        return -1;

    uint8_t* base = (uint8_t*)tag->pdo->PDO_tx.iomap;
    uint8_t* byte = base + tag->offset;

    if (tag->type == PLC_BOOL) {
        *value = (*byte >> tag->bit_index) & 0x01;
        return 0;
    }

    return -2;
}



int plc_write8(const IOTag_t* tag, uint8_t value)
{
    if (!tag || tag->type2 == SL_INPUT)
        return -1;

    Slave_PDO_RX_t* rx = &tag->pdo->PDO_rx;

    uint8_t* base = (uint8_t*)rx->buf[2];   // buffer APP
    uint8_t* byte = base + tag->offset;

    if (tag->type == PLC_BOOL) {
        if (value)
            *byte |= (1u << tag->bit_index);
        else
            *byte &= ~(1u << tag->bit_index);

        atomic_store(&rx->dirty, true);
        return 0;
    }

    return -2;
}


int plc_write32(const IOTag_t* tag, uint32_t value)
{
    if (!tag || tag->type2 == SL_INPUT)
        return -1;

    Slave_PDO_RX_t* rx = &tag->pdo->PDO_rx;

    uint8_t* base = (uint8_t*)rx->buf[2];   // buffer APP
    uint8_t* byte = base + tag->offset;

    if (tag->type == PLC_BOOL) {
        if (value)
            *byte |= (1u << tag->bit_index);
        else
            *byte &= ~(1u << tag->bit_index);

        atomic_store(&rx->dirty, true);
        return 0;
    }
    else 

    return -2;
}