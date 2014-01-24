#ifdef MODULE

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include "../trace/trace.h"

#else

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define trace_write(x,...) do { printf(__VA_ARGS__); printf("\n"); } while (0)

#endif

#include "io_crypt.h"
#include "crypt_lfsr64.h"


extern uint32_t iocrypt_trace_ctx;

/* clock the current LFSR by given number of clocks */
static void crypt_lfsr64_clock(lfsr64_ctx_t *ctx, uint32_t clocks)
{
    uint64_t lfsr = ctx->lfsr_state;
    
    for(uint32_t clock = 0; clock < clocks; clock++)
    {
        /* maximum length LFSR according to http://www.xilinx.com/support/documentation/application_notes/xapp052.pdf */
        int bit = ((lfsr >> 63) ^ (lfsr >> 62) ^ (lfsr >> 60) ^ (lfsr >> 59)) & 1;
        lfsr = (lfsr << 1) | bit;
    }
    
    ctx->lfsr_state = lfsr;
}

/* xor the buffer with 8/64 bit alignment */
static void crypt_lfsr64_xor_uint8(void *dst_in, void *src_in, lfsr64_ctx_t *ctx, uint32_t offset)
{
    uint8_t *dst = (uint8_t *)dst_in;
    uint8_t *src = (uint8_t *)src_in;
    
    *dst = *src ^ ctx->key_uint8[offset % 8];
}

static void crypt_lfsr64_xor_uint64(void *dst_in, void *src_in, lfsr64_ctx_t *ctx, uint32_t offset)
{
    uint64_t *dst = (uint64_t *)dst_in;
    uint64_t *src = (uint64_t *)src_in;
    
    *dst = *src ^ ctx->key_uint64[offset % 8];
}

static void update_key(lfsr64_ctx_t *ctx, uint32_t offset, uint32_t force)
{
    /* update the current encryption key whever reaching the next block */
    uint32_t block = offset / CRYPT_LFSR64_BLOCKSIZE;
    
    if(!force)
    {
        if(ctx->current_block == block)
        {
            return;
        }
    }
    
    ctx->current_block = block;
    
    /* first feed it with base key and block */
    ctx->lfsr_state = ctx->password ^ block;
    crypt_lfsr64_clock(ctx, 32);
    
    /* then update it with the file offset again and shift by an amount based on file offset */
    ctx->lfsr_state ^= block;
    crypt_lfsr64_clock(ctx, (11 * block) % 128);
    
    /* mask it again */
    ctx->lfsr_state ^= ctx->password;
    
    trace_write(iocrypt_trace_ctx, "update_key: offset 0x%08X, password: 0x%08X%08X, key: 0x%08X%08X", offset, (uint32_t)(ctx->password>>32), (uint32_t)ctx->password, (uint32_t)(ctx->lfsr_state>>32), (uint32_t)ctx->lfsr_state);
    
    /* build an array with key elements for every offset in uint8_t mode */
    //trace_write(iocrypt_trace_ctx, "update_key: key_uint8");
    uint64_t lfsr = ctx->lfsr_state;
    for(int pos = 0; pos < 8; pos++)
    {
        ctx->key_uint8[pos] = (uint8_t)(lfsr & 0xFF);
        lfsr >>= 8;
        //trace_write(iocrypt_trace_ctx, "update_key:     0x%02X", ctx->key_uint8[pos]);
    }
    
    /* build an array with key elements for every offset in uint64_t mode */
    //trace_write(iocrypt_trace_ctx, "update_key: key_uint32");
    for(int pos = 0; pos < 8; pos++)
    {
        uint32_t elem_addr = &ctx->key_uint64[pos];
        
        memcpy((void*)elem_addr, &ctx->key_uint8[pos], 8 - pos);
        memcpy((void*)elem_addr + (8-pos), ctx->key_uint8, pos);
        //trace_write(iocrypt_trace_ctx, "update_key:     0x%08X%08X", (uint32_t)(ctx->key_uint64[pos]>>32), (uint32_t)ctx->key_uint64[pos]);
    }
}

#define IS_UNALIGNED(x) ((((uint32_t)dst) % (x)) || (((uint32_t)src) % (x)))
static void crypt_lfsr64_encrypt(void *ctx_in, uint8_t *dst, uint8_t *src, uint32_t length, uint32_t offset)
{
    lfsr64_ctx_t *ctx = (lfsr64_ctx_t *)ctx_in;
    
    /* initial key creation */
    update_key(ctx, offset, 0); 
    
    /* try to get the addresses aligned */
    trace_write(iocrypt_trace_ctx, "crypt_lfsr64_xor_uint8 offset 0x%08X, length 0x%08X", offset, length);
    while((IS_UNALIGNED(8) || (offset % 8)) && (length > 0))
    {
        uint32_t block_remain = offset % CRYPT_LFSR64_BLOCKSIZE;
        
        if(block_remain == 0)
        {
            update_key(ctx, offset, 0);
        }
        
        crypt_lfsr64_xor_uint8(dst, src, ctx, offset);
        dst += 1;
        src += 1;
        offset += 1;
        length -= 1;
    }
    
    trace_write(iocrypt_trace_ctx, "crypt_lfsr64_xor_uint64 offset 0x%08X, length 0x%08X", offset, length);
    if(!IS_UNALIGNED(8))
    {
        while(length >= 8)
        {
            uint32_t block_remain = offset % CRYPT_LFSR64_BLOCKSIZE;
            
            if(block_remain == 0)
            {
                update_key(ctx, offset, 0);
            }
            else if(block_remain < 8)
            {
                break;
            }
            
            crypt_lfsr64_xor_uint64(dst, src, ctx, offset);
            dst += 8;
            src += 8;
            offset += 8;
            length -= 8;
        }
    }
    
    /* do the rest */
    trace_write(iocrypt_trace_ctx, "crypt_lfsr64_xor_uint8 offset 0x%08X, length 0x%08X", offset, length);
    while(length > 0)
    {
        uint32_t block_remain = offset % CRYPT_LFSR64_BLOCKSIZE;
        
        if(block_remain == 0)
        {
            update_key(ctx, offset, 0);
        }
        
        crypt_lfsr64_xor_uint8(dst, src, ctx, offset);
        trace_write(iocrypt_trace_ctx, "crypt_lfsr64_xor_uint8 offset 0x%08X, 0x%02X <- 0x%02X", offset, *dst, *src);
        dst += 1;
        src += 1;
        offset += 1;
        length -= 1;
    }
}

/* using a symmetric cipher, both encryption and decryption are the same */
static void crypt_lfsr64_decrypt(void *ctx, uint8_t *dst, uint8_t *src, uint32_t length, uint32_t offset)
{
    crypt_lfsr64_encrypt(ctx, dst, src, length, offset);
}

static void crypt_lfsr64_deinit(void **crypt_ctx)
{
    if(*crypt_ctx)
    {
        free(*crypt_ctx);
        *crypt_ctx = NULL;
    }
}

/* allocate and initialize an LFSR64 cipher ctx and save to pointer */
void crypt_lfsr64_init(void **crypt_ctx, uint64_t password)
{
    lfsr64_ctx_t *ctx = malloc(sizeof(lfsr64_ctx_t));
    
    if(!ctx)
    {
        trace_write(iocrypt_trace_ctx, "crypt_lfsr64_init: failed to malloc");
        return;
    }
    
    /* setup cipher ctx */
    ctx->cipher.encrypt = &crypt_lfsr64_encrypt;
    ctx->cipher.decrypt = &crypt_lfsr64_decrypt;
    ctx->cipher.deinit = &crypt_lfsr64_deinit;
    ctx->password = password;
    ctx->current_block = 0xFFFFFFFF;
    ctx->lfsr_state = 0;
    
    /* setup initial cipher key */
    update_key(ctx, 0, 1); 
    
    *crypt_ctx = ctx;
    trace_write(iocrypt_trace_ctx, "crypt_lfsr64_init: initialized");
}


