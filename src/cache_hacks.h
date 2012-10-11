#ifndef _CACHE_HACKS_H_
#define _CACHE_HACKS_H_

#define TYPE_DCACHE 0
#define TYPE_ICACHE 1

/* we have 8 KiB of cache. that can be made dynamic depending on cache type and processor setup  */
#define CACHE_SIZE_BITS(t)          13

/* depending on cache size, INDEX has different length */
#define CACHE_INDEX_BITS(t)         (CACHE_SIZE_BITS(t)-7)
/* INDEX in tag field starts at bit 5 */
#define CACHE_INDEX_TAGOFFSET(t)    5
/* bitmask that matches the INDEX value bits */
#define CACHE_INDEX_BITMASK(t)      ((1<<CACHE_INDEX_BITS(t)) - 1)
/* bitmask to mask out the INDEX field in a tag */
#define CACHE_INDEX_ADDRMASK(t)     (CACHE_INDEX_BITMASK(t)<<CACHE_INDEX_TAGOFFSET(t))

/* depending on cache size, TAG has different length */
#define CACHE_TAG_BITS(t)           (27-CACHE_INDEX_BITS(t))
/* TAG in tag field starts at bit 5 plus INDEX size */
#define CACHE_TAG_TAGOFFSET(t)      (5+CACHE_INDEX_BITS(t))
/* bitmask that matches the TAG value bits */
#define CACHE_TAG_BITMASK(t)        ((1<<CACHE_TAG_BITS(t)) - 1)
/* bitmask to mask out the TAG field in a tag */
#define CACHE_TAG_ADDRMASK(t)       (CACHE_TAG_BITMASK(t)<<CACHE_TAG_TAGOFFSET(t))

/* the WORD field in tags is always 3 bits */
#define CACHE_WORD_BITS(t)          3
/* WORD in tag field starts at this bit position */
#define CACHE_WORD_TAGOFFSET(t)     2
/* bitmask that matches the WORD value bits */
#define CACHE_WORD_BITMASK(t)       ((1<<CACHE_WORD_BITS(t)) - 1)
/* bitmask to mask out the WORD field in a tag */
#define CACHE_WORD_ADDRMASK(t)      (CACHE_WORD_BITMASK(t)<<CACHE_WORD_TAGOFFSET(t))

/* the SEGMENT field in tags is always 2 bits */
#define CACHE_SEGMENT_BITS(t)       2
/* SEGMENT in tag field starts at this bit position */
#define CACHE_SEGMENT_TAGOFFSET(t)  30
/* bitmask that matches the SEGMENT value bits */
#define CACHE_SEGMENT_BITMASK(t)    ((1<<CACHE_SEGMENT_BITS(t)) - 1)
/* bitmask to mask out the SEGMENT field in a tag */
#define CACHE_SEGMENT_ADDRMASK(t)   (CACHE_SEGMENT_BITMASK(t)<<CACHE_SEGMENT_TAGOFFSET(t))

static void cache_patch_single_word(uint32_t address, uint32_t data, uint32_t type)
{
    uint32_t cache_seg_index_word = (address & (CACHE_INDEX_ADDRMASK(t) | CACHE_WORD_ADDRMASK(type)));
    uint32_t cache_tag_index = (address & (CACHE_TAG_ADDRMASK(type) | CACHE_INDEX_ADDRMASK(type))) | 0x10;

    if(type == TYPE_ICACHE)
    {
        asm volatile ("\
           /* write index for address to write */\
           MCR p15, 3, %0, c15, c0, 0\r\n\
           /* set TAG at given index */\
           MCR p15, 3, %1, c15, c1, 0\r\n\
           /* write instruction */\
           MCR p15, 3, %2, c15, c3, 0\r\n\
           " : : "r"(cache_seg_index_word), "r"(cache_tag_index), "r"(data));
    }
    else
    {
        asm volatile ("\
           /* write index for address to write */\
           MCR p15, 3, %0, c15, c0, 0\r\n\
           /* set TAG at given index */\
           MCR p15, 3, %1, c15, c2, 0\r\n\
           /* write data */\
           MCR p15, 3, %2, c15, c4, 0\r\n\
           " : : "r"(cache_seg_index_word), "r"(cache_tag_index), "r"(data));
    }
}

/* fetch all the instructions in that temp_cacheline the given address is in.
   this is *required* before patching a single address.
   if you omit this, only the patched instruction will be correct, the
   other instructions around will simply be crap.
   warning - this is doing a data fetch (LDR) so DCache patches may cause
   unwanted or wanted behavior. make sure you know what you do :)

   same applies to dcache routines
 */
static void cache_fetch_line(uint32_t address, uint32_t type)
{
    uint32_t base = (address & ~0x1F);
    uint32_t temp_cacheline[8];
    
    /* our ARM946 has 0x20 byte temp_cachelines. fetch the current line
       thanks to unified memories, we can do LDR on instructions.
    */
    for(int pos = 0; pos < 8; pos++)
    {
        temp_cacheline[pos] = ((uint32_t *)base)[pos];
    }

    /* and nail it into locked cache */
    for(int pos = 0; pos < 8; pos++)
    {
        cache_patch_single_word(base + pos * 4, temp_cacheline[pos], type);
    }
}


/* return the tag and content at given index (segment+index+word) */
static void cache_get_content(uint32_t segment, uint32_t index, uint32_t word, uint32_t type, uint32_t *tag, uint32_t *data)
{
    uint32_t cache_seg_index_word = 0;
    uint32_t stored_tag_index = 0;
    uint32_t stored_data = 0;

    cache_seg_index_word |= ((segment & CACHE_SEGMENT_BITMASK(type)) << CACHE_SEGMENT_TAGOFFSET(type));
    cache_seg_index_word |= ((index & CACHE_INDEX_BITMASK(type)) << CACHE_INDEX_TAGOFFSET(type));
    cache_seg_index_word |= ((word & CACHE_WORD_BITMASK(type)) << CACHE_WORD_TAGOFFSET(type));

    if(type == TYPE_ICACHE)
    {
        asm volatile ("\
           /* write index for address to write */\
           MCR p15, 3, %2, c15, c0, 0\r\n\
           /* get TAG at given index */\
           MRC p15, 3, %0, c15, c1, 0\r\n\
           /* get DATA at given index */\
           MRC p15, 3, %1, c15, c3, 0\r\n\
           " : "=r"(stored_tag_index), "=r"(stored_data) : "r"(cache_seg_index_word));
    }
    else
    {
        asm volatile ("\
           /* write index for address to write */\
           MCR p15, 3, %2, c15, c0, 0\r\n\
           /* get TAG at given index */\
           MRC p15, 3, %0, c15, c2, 0\r\n\
           /* get DATA at given index */\
           MRC p15, 3, %1, c15, c4, 0\r\n\
           " : "=r"(stored_tag_index), "=r"(stored_data) : "r"(cache_seg_index_word));
    }

    *tag = stored_tag_index;
    *data = stored_data;
}

/* check if given address is already patched in cache */
static uint32_t cache_get_cached(uint32_t address, uint32_t type)
{
    uint32_t cache_seg_index_word = (address & (CACHE_TAG_ADDRMASK(t) | CACHE_WORD_ADDRMASK(type)));
    uint32_t stored_tag_index = 0;

    if(type == TYPE_ICACHE)
    {
        asm volatile ("\
           /* write index for address to write */\
           MCR p15, 3, %1, c15, c0, 0\r\n\
           /* get TAG at given index */\
           MRC p15, 3, %0, c15, c1, 0\r\n\
           " : "=r"(stored_tag_index) : "r"(cache_seg_index_word));
    }
    else
    {
        asm volatile ("\
           /* write index for address to write */\
           MCR p15, 3, %1, c15, c0, 0\r\n\
           /* get TAG at given index */\
           MRC p15, 3, %0, c15, c2, 0\r\n\
           " : "=r"(stored_tag_index) : "r"(cache_seg_index_word));
    }

    /* now check if the TAG RAM content matches with what we expect and valid bit is set */
    uint32_t tag_index_valid_mask = CACHE_TAG_ADDRMASK(type) | CACHE_INDEX_ADDRMASK(type) | 0x10;
    uint32_t cache_tag_index = (address & tag_index_valid_mask) | 0x10;

    if((stored_tag_index & tag_index_valid_mask) == cache_tag_index)
    {
        return 1;
    }

    return 0;
}

static void icache_unlock()
{
    uint32_t old_int = cli();

    /* make sure all entries are set to invalid */
    for(int index = 0; index < (1<<CACHE_INDEX_BITS(TYPE_ICACHE)); index++)
    {
        asm volatile ("\
           /* write index for address to write */\
           MOV R0, %0, LSL #5\r\n\
           MCR p15, 3, R0, c15, c0, 0\r\n\
           /* set TAG at given index */\
           MCR p15, 3, R0, c15, c1, 0\r\n\
           " : : "r"(index) : "r0");
    }

    /* disable cache lockdown */
    asm volatile ("\
       MOV R0, #0\r\n\
       MCR p15, 0, R0, c9, c0, 1\r\n\
       " : : : "r0");

    /* and flush cache */
    asm volatile ("\
        MOV R0, #0\r\n\
        MCR p15, 0, R0, c7, c5, 0\r\n\
        MCR p15, 0, R0, c7, c10, 4\r\n\
        " : : : "r0"
    );
    sei(old_int);
}

static void dcache_unlock()
{
    uint32_t old_int = cli();

    /* make sure all entries are set to invalid */
    for(int index = 0; index < (1<<CACHE_INDEX_BITS(TYPE_ICACHE)); index++)
    {
        asm volatile ("\
           /* write index for address to write */\
           MOV R0, %0, LSL #5\r\n\
           MCR p15, 3, R0, c15, c0, 0\r\n\
           /* set TAG at given index */\
           MCR p15, 3, R0, c15, c2, 0\r\n\
           " : : "r"(index) : "r0");
    }

    /* disable cache lockdown */
    asm volatile ("\
       MOV R0, #0\r\n\
       MCR p15, 0, R0, c9, c0, 0\r\n\
       " : : : "r0");

    /* and flush cache */
    asm volatile ("\
        MOV R0, #0\r\n\
        MCR p15, 0, R0, c7, c6, 0\r\n\
        MCR p15, 0, R0, c7, c10, 4\r\n\
        " : : : "r0"
    );
    sei(old_int);
}

static void icache_lock()
{
    uint32_t old_int = cli();

    /* no need to clean entries, directly lock cache */
    asm volatile ("\
       /* enable cache lockdown for segment 0 (of 4) */\
       MOV R0, #0x80000000\r\n\
       MCR p15, 0, R0, c9, c0, 1\r\n\
       \
       /* finalize lockdown */\
       MOV R0, #1\r\n\
       MCR p15, 0, R0, c9, c0, 1\r\n\
       " : : : "r0");

    /* make sure all entries are set to invalid */
    for(int index = 0; index < (1<<CACHE_INDEX_BITS(TYPE_ICACHE)); index++)
    {
        asm volatile ("\
           /* write index for address to write */\
           MOV R0, %0, LSL #5\r\n\
           MCR p15, 3, R0, c15, c0, 0\r\n\
           /* set TAG at given index */\
           MCR p15, 3, R0, c15, c1, 0\r\n\
           " : : "r"(index) : "r0");
    }
    sei(old_int);
}

static void dcache_lock()
{
    uint32_t old_int = cli();

    /* first clean and flush dcache entries */
    for(uint32_t segment = 0; segment < (1<<CACHE_SEGMENT_BITS(TYPE_DCACHE)); segment++ )
    {
        for(uint32_t index = 0; index < (1<<CACHE_INDEX_BITS(TYPE_DCACHE)); index++)
        {
            uint32_t seg_index = (segment << CACHE_SEGMENT_TAGOFFSET(t)) | (index << CACHE_INDEX_TAGOFFSET(t));
            asm volatile ("\
                mcr p15, 0, %0, c7, c14, 2\r\n\
                " : : "r"(seg_index)
            );
        }
    }

    /* then lockdown data cache */
    asm volatile ("\
       /* enable cache lockdown for segment 0 (of 4) */\
       MOV R0, #0x80000000\r\n\
       MCR p15, 0, R0, c9, c0, 0\r\n\
       \
       /* finalize lockdown */\
       MOV R0, #1\r\n\
       MCR p15, 0, R0, c9, c0, 0\r\n\
       " : : : "r0");

    /* make sure all entries are set to invalid */
    for(int index = 0; index < (1<<CACHE_INDEX_BITS(TYPE_DCACHE)); index++)
    {
        asm volatile ("\
           /* write index for address to write */\
           MOV R0, %0, LSL #5\r\n\
           MCR p15, 3, R0, c15, c0, 0\r\n\
           /* set TAG at given index */\
           MCR p15, 3, R0, c15, c2, 0\r\n\
           " : : "r"(index) : "r0");
    }
    sei(old_int);
}

/* these are the "public" functions. please use only these if you are not sure what the others are for */
static void cache_lock()
{
    icache_lock();
    dcache_lock();
}

static void cache_unlock()
{
    icache_lock();
    dcache_lock();
}

static void cache_fake(uint32_t address, uint32_t data, uint32_t type)
{
    /* is that line not in cache yet? */
    if(!cache_get_cached(address, type))
    {
        /* no, then fetch it */
        cache_fetch_line(address, type);
    }

    cache_patch_single_word(address, data, type);
}

#endif
