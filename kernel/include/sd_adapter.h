#ifndef _SD_ADAPTER_H_
#define _SD_ADAPTER_H_

#include "block_io.h"
#include "physmem.h"
#include "printf.h"
#include "sd.h"

/*
    Think about this class as the equivalent of the IDE class in the 439 code, except the ide class 
    also had the sd driver logic in it. This is just layers of abstraction. When you call a fs op it'll
    call these implementations of the blockIO virtual class

*/

class SDAdapter : public BlockIO {
public:
    SDAdapter(uint32_t block_size);

    virtual ~SDAdapter() {}

    uint32_t size_in_bytes() override;

    void read_block(uint32_t block_number, char* buffer) override;

    void write_block(uint32_t block_number, char* buffer, uint32_t offset, uint32_t n) override;

    int64_t write(uint32_t offset, uint32_t n, char* buffer) override;

    int64_t write_all(uint32_t offset, uint32_t n, char* buffer) override;

private:
    uint32_t local_min(uint32_t a, uint32_t b);
};

#endif // _SD_ADAPTER_H_
