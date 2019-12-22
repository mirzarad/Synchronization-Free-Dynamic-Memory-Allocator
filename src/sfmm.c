/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "debug.h"
#include "sfmm.h"

/*
    initialize heap_initialied to 0 on start
    if heap_init_bool == 0 then it has not been initialized --> initialize the heap and sf_free_list_heads
    if heap_init_bool != 0 then it has been initialized...
    perform a check on the sf_free_list_heads and find an appropriate free block
*/

static int is_heap_init = 0; // initially set to false
void* allocate_block(sf_block* block_list_ptr, size_t alligned_request);
void* generate_alligned_request_free_block(size_t alligned_request);
size_t total_size_needed(size_t requested_size);

void *sf_malloc(size_t size) {

    /* Check if <= 0 input size: */
    if(size <= 0) return NULL;

    /* Heap Initialization */
    if(is_heap_init == 0){

        void* check_if_null = sf_mem_grow();
        if(check_if_null == NULL){
            sf_errno = ENOMEM;
            return NULL;
        }

        /* SETTING UP THE HEAP PROLOGUE*/
        void* mem_start_ptr = sf_mem_start();
        sf_prologue heap_prologue;
        heap_prologue.header = 35; // 35 --> 100011‬ allocated(1) and previous_alloc(1) bits are set.
        heap_prologue.footer = ((heap_prologue.header)^sf_magic());
        sf_prologue* prologue_ptr = mem_start_ptr;
        // Write prologue to memory:
        *prologue_ptr = heap_prologue;

        /* SETTING UP THE HEAP EPILOGUE*/
        void* mem_end_ptr = sf_mem_end() - 8; // Go to the memory row before the end of the heap (back 8 bytes)
        sf_epilogue heap_epilogue;
        heap_epilogue.header = 2^sf_magic(); // 00000000...0010^sf_magic() -> set the allocated bit to 1 and prev bit to 0.

        sf_epilogue* epilogue_ptr = (sf_epilogue*)mem_end_ptr;
        *epilogue_ptr = heap_epilogue;

        /* SETUP THE SF_FREE_LIST_HEADS ARRAY*/
        void initialize_sf_free_list_heads();
        initialize_sf_free_list_heads();

        /* SETTING UP THE FIRST FREE BLOCK */
        //void* first_free_block_header_ptr = sf_mem_start() + 40;    // mem_row of the header of the first free block
        //void* first_free_block_footer_ptr = sf_mem_end() - 16;      // mem row of the footer of the first free block
        sf_footer* first_free_block_footer_ptr = sf_mem_end() - 16;

        /* SET LINKS FOR FREE BLOCK STRUCT */
        sf_block* first_free_block_ptr = sf_mem_start() + 32;             // Addr = footer for epilogue (prev_footer)
        first_free_block_ptr->header = 4049;                              // Write size 4048 + prev_alloc(1)
        first_free_block_ptr->body.links.next = &(sf_free_list_heads[7]); // Write next link = sf[7]
        first_free_block_ptr->body.links.prev = &(sf_free_list_heads[7]); // Write prev link = sf[7]
        *first_free_block_footer_ptr = 4049^sf_magic();                   // Write: (4048 + prev_alloc(1)^sf_magic())

        /* SET SENTINEL NODE NEXT AND PREV LINKS TO POINTER TO THIS FREE BLOCK*/
        sf_free_list_heads[7].body.links.next = (sf_block*)first_free_block_ptr;
        sf_free_list_heads[7].body.links.prev = (sf_block*)first_free_block_ptr;

        is_heap_init = 1; // Heap initialization complete
    }

    /* Satisfy the first malloc request*/

    /*
                                        SATISFYING A MALLOC REQUEST */


    /* (1) Determine the required block size: Use size_t total_size_needed(size_t requested_size); */
    size_t alligned_request = total_size_needed(size); // Search sf_free_list_heads[] using this value

    /* (2) Determine the smallest free list that would be able to satisfy a request of that size */
    int index;
    int sfListIndex(size_t total_size_needed);
    index = sfListIndex(alligned_request);

    /* (3) Search that free list from the beginning until the first sufficiently large block is found. */
    // Now that we have the index and the address of the sentinel node stored.

    int i;
    sf_block* sentinel_node_address;
    sf_block* block_list_ptr;
    // (4) If there is no such block, continue with the next larger size class. (for loop)
    for(i = index;i < NUM_FREE_LISTS; i++){
        sentinel_node_address = &sf_free_list_heads[i];
        block_list_ptr = sf_free_list_heads[i].body.links.next; // should point to the first

        if(sf_free_list_heads[i].body.links.next == &sf_free_list_heads[i] && sf_free_list_heads[i].body.links.prev == &sf_free_list_heads[i]){
            continue; // Then move onto the next class size.
        }else{
            /* (3) Search that free list from the beginning until the first sufficiently large block is found. (while loop)*/
            while(block_list_ptr != sentinel_node_address){
            // Now we have to get the block_size of the block that the pointer is currently pointing at
                size_t block_size = ((block_list_ptr->header)&(BLOCK_SIZE_MASK));

                if(block_size >= alligned_request){
                // sf_show_free_lists();
                // printf("\n");
                /*
                (5) If a big enough block is found, then after splitting it (if it will not leave a splinter),
                you should insert the remainder part back into the appropriate freelist.
                    Note: When splitting a block, the "lower part" should be used to satisfy the allocation request
                    and the "upper part" should become the remainder.
                */
                    return allocate_block(block_list_ptr, alligned_request);
                }else{
                // Set the pointer to equal the next sf_block in the same size_class sub_list
                    block_list_ptr = block_list_ptr->body.links.next;
                }
            }
        }
    }


    /* (6) If there is no block big enough in any freelist, then you must use sf_mem_grow to request more memory.
                (For requests larger than a page, more than one such call might be required)
                NOTE:After each call to sf_mem_grow, you must attempt to coalesce the newly
                        allocated page with any free block immediately preceding it,
                        in order to build blocks larger than one page.
                        Insert the new block at the beginning of the appropriate free list.
                Side-Note: Do not coalesce with the prologue or epilogue, or past the beginning
                            or end of the heap.
    */
    sf_block* new_free_block;

    /* (7) If your allocator ultimately cannot satisfy the request, your sf_malloc function must set sf_errno
        to ENOMEM and return NULL. */
    if((new_free_block = (sf_block*)generate_alligned_request_free_block(alligned_request)) == NULL){
        sf_errno = ENOMEM;
        return NULL;
    }
    return allocate_block(new_free_block, alligned_request);
} // End of malloc()



void sf_free(void *pp) {
    /*(1) First verify that the pointer being passed to your function belongs to an allocated block. */
    int is_invalid_pointer(void *pp);
    if(is_invalid_pointer(pp) == 1){
        abort(); // (2) If an invalid pointer is passed to your function, you must call abort to exit the program.
    }
    /* (3) Now you must free the block. Save the header and footer addresses and the block size. */
    void* alloc_block_ptr = pp - 16; // Points to memory row of prev_footer

    /*  (4) Save the values of the previous footer and the next header's alloc() bit fields */
    /* (5) insert this new free block into the appropriate size class */
    void* coalesce(void* alloc_block_ptr);
    coalesce(alloc_block_ptr);
    return;
}

/*
                             IMPLEMENTATION NOTES

    When implementing your sf_realloc function, you must first verify that the
    pointer and size parameters passed to your function are valid.

    (1) If the pointer is invalid sf_errno should be set to EINVAL and function should return NULL.
    (2) If the pointer is valid but the size parameter is 0, free the block and return NULL.

    NOTE: In some cases, sf_realloc is more complicated than calling sf_malloc
            to allocate more memory, memcpy to move the old memory to the new memory, and
            sf_free to free the old memory.

    SCENARIO 1: Reallocating to a Larger Size:
    (1.1) Call sf_malloc to obtain a larger block.
    (1.2) Call memcpy to copy the data in the block given by the client to the block
            returned by sf_malloc.
    (1.3) Call sf_free on the block given by the client (coalescing if necessary).
    (1.4) Return the block given to you by sf_malloc to the client.

    SCENARIO 2: Reallocating to a Smaller Size:
    When reallocating to a smaller size, your allocator must use the block that was
        passed by the caller.  You must attempt to split the returned block. There are
        two cases for splitting:

    --------------------
    | SPLITTING CASE 1 |
    --------------------
    Splitting the returned block results in a splinter.
    In this case, do not split the block.
    Leave the splinter in the block, update the header field if necessary,
        and return the same block back to the caller.

    Example:

    b                                               b
    +----------------------+                       +------------------------+
    | allocated            |                       |   allocated.           |
    | Blocksize: 64 bytes  |   sf_realloc(b, 32)   |   Block size: 64 bytes |
    | payload: 48 bytes    |                       |   payload: 32 bytes    |
    |                      |                       |                        |
    |                      |                       |                        |
    +----------------------+                       +------------------------+
    In the example above, splitting the block would have caused a 16-byte splinter.
    Therefore, the block is not split.

    --------------------------------------------------------------------------------

    SPLITTING CASE 2:
    The block can be split without creating a splinter. In this case, split the
        block and update the block size fields in both headers.
    Free the remaining block by inserting it into the appropriate free list (after coalescing, if possible).
    Return a pointer to the payload of the now-smaller block to the caller.


If sf_malloc returns NULL, sf_realloc must also return NULL. Note that
you do not need to set sf_errno in sf_realloc because sf_malloc should
take care of this.

 * Resizes the memory pointed to by ptr to size bytes.
 *
 * @param ptr Address of the memory region to resize.
 * @param size The minimum size to resize the memory to.
 *
 * @return If successful, the pointer to a valid region of memory is
 * returned, else NULL is returned and sf_errno is set appropriately.
 *
 *   If sf_realloc is called with an invalid pointer sf_errno should be set to EINVAL.
 *   If there is no memory available sf_realloc should set sf_errno to ENOMEM.
 *
 * If sf_realloc is called with a valid pointer and a size of 0 it should free
 * the allocated block and return NULL without setting sf_errno.
 */
void *sf_realloc(void *pp, size_t rsize) {

    /* Implementation Details */

    /*(1) If the pointer is invalid sf_errno should be set to EINVAL and function should return NULL. */
    int is_invalid_pointer(void *pp);
    if(is_invalid_pointer(pp) == 1){
        sf_errno = EINVAL;
        return NULL;
    }

    /* (2) If the pointer is valid but the size parameter is 0, free the block and return NULL. */
    if(rsize == 0){
        // free the block:
        sf_free(pp);
        return NULL;
    }

    /* Determine if rsize with alligned_padding means we need a greater or smaller block*/
    void* pp_block_header_ptr = pp - 8;
    size_t pp_block_size = ((*(sf_header*)pp_block_header_ptr)&BLOCK_SIZE_MASK);
    size_t total_size_needed(size_t requested_size);
    size_t alligned_reallocate_request = total_size_needed(rsize);

    /*  SCENARIO 1: Reallocating to a Larger Size */
    if(alligned_reallocate_request > pp_block_size){

        /* (1.1) Call sf_malloc to obtain a larger block. */
        void* larger_block = sf_malloc(rsize);
        /* (1.2) Call memcpy to copy the data in the block given
                by the client to the block returned by sf_malloc.   */
        size_t bytes_to_copy = pp_block_size - 16; // subtract header and footer amount
        memcpy(larger_block, pp, bytes_to_copy);
        /* (1.3) Call sf_free on the block given by the client (coalescing if necessary). */
        sf_free(pp);
        /* (1.4) Return the block given to you by sf_malloc to the client. */
        return larger_block;

    }else if(alligned_reallocate_request < pp_block_size){
        /*
            SCENARIO 2: Reallocating to a Smaller Size:
            When reallocating to a smaller size, your allocator must use the block that was
            passed by the caller.  You must attempt to split the returned block. There are
            two cases for splitting:
        */
        size_t original_block_size = pp_block_size;
        size_t payload_alligned = total_size_needed(rsize);
        size_t difference = original_block_size - payload_alligned;

        if(difference < 32){
        /*
                --------------------
                | SPLITTING CASE 1 |
                --------------------
                Splitting the returned block results in a splinter.
                In this case, do not split the block.
                Leave the splinter in the block, update the header field if necessary,
                    and return the same block back to the caller.
        */

        return pp;

        }else if (difference >= 32){
        /*
                --------------------
                | SPLITTING CASE 2 |
                --------------------
                The block can be split without creating a splinter. In this case, split the
                block and update the block size fields in both headers.
                Free the remaining block by inserting it into the appropriate free list (after coalescing, if possible).
                Return a pointer to the payload of the now-smaller block to the caller.
        */

            void* new_alloc_header_address;
            size_t new_alloc_block_size;
            void* new_alloc_footer_address;
            // payload pointer will be the same
            void* new_free_header_address;
            size_t new_free_block_size;
            void* new_free_footer_address;

            // we have payload_alligned --> includes the + 16 right?
            new_alloc_block_size = payload_alligned;
            new_alloc_header_address = pp - 8;
            new_alloc_footer_address = new_alloc_header_address + new_alloc_block_size - 8;
            new_free_header_address = new_alloc_footer_address + 8;
            new_free_block_size = original_block_size - new_alloc_block_size;
            new_free_footer_address = new_alloc_footer_address + new_free_block_size;
            void* epilogue_address = sf_mem_end() - 8;

            sf_block* new_free_block_ptr;
            new_free_block_ptr = new_alloc_footer_address;

            // determine if prev_block is allocated
            void* prev_block_footer_address = new_alloc_header_address - 8;
            size_t is_prev_block_alloc_bit = (((*(sf_header*)prev_block_footer_address)^sf_magic())&2);
            is_prev_block_alloc_bit = is_prev_block_alloc_bit >> 1;

            // Set header and footer contents for alloc block
            *(sf_header*)new_alloc_header_address = (new_alloc_block_size|2)|is_prev_block_alloc_bit;
            *(sf_header*)new_alloc_footer_address = (*(sf_header*)new_alloc_header_address)^sf_magic();

            // NOW WE MUST CHECK: IS THE NEXT BLOCK AFTER THE FREE_BLOCK ALSO A FREE BLOCK
            void* next_block_header = new_free_footer_address + 8;
            size_t next_block_size = ((*(sf_header*)next_block_header)&BLOCK_SIZE_MASK);
            void* next_block_footer = new_free_footer_address + next_block_size;
            int sfListIndex(size_t new_free_block_size);
            sf_block* free_block_ptr;
            sf_block* next_block_ptr;

            if(next_block_header == epilogue_address){
                // Simply insert the free block we currently have into the free list

                // Set header and footer contents for free block
                *(sf_header*)new_free_header_address = (new_free_block_size|1);
                *(sf_header*)new_free_footer_address = (*(sf_header*)new_free_header_address)^sf_magic();

                // Find out the new index of the sf_free_list to insert into:
                int new_free_block_index = sfListIndex(new_free_block_size);

                // Set new address of free block to new_alloc_footer_address
                free_block_ptr = new_alloc_footer_address;

                // Set the links for new free block:
                free_block_ptr->body.links.next = sf_free_list_heads[new_free_block_index].body.links.next;
                free_block_ptr->body.links.prev = &sf_free_list_heads[new_free_block_index];

                // Insert into the list correctly:
                sf_free_list_heads[new_free_block_index].body.links.next = free_block_ptr;
                (free_block_ptr->body.links.next)->body.links.prev = free_block_ptr;


            }else{
                // continue the check:
                size_t is_next_block_allocated = (*(sf_header*)next_block_header)&2;
                is_next_block_allocated = is_next_block_allocated >> 1;
                if(is_next_block_allocated == 1){
                    // Simply insert the free block we currently have into the free list

                    // Set header and footer contents for free block
                    *(sf_header*)new_free_header_address = (new_free_block_size|1);
                    *(sf_header*)new_free_footer_address = (*(sf_header*)new_free_header_address)^sf_magic();

                    // Find out the new index of the sf_free_list to insert into:
                    int new_free_block_index = sfListIndex(new_free_block_size);

                    // Set new address of free block to new_alloc_footer_address
                    free_block_ptr = new_alloc_footer_address;

                    // Set the links for new free block:
                    free_block_ptr->body.links.next = sf_free_list_heads[new_free_block_index].body.links.next;
                    free_block_ptr->body.links.prev = &sf_free_list_heads[new_free_block_index];

                    // Insert into the list correctly:
                    sf_free_list_heads[new_free_block_index].body.links.next = free_block_ptr;
                    (free_block_ptr->body.links.next)->body.links.prev = free_block_ptr;
                }else{
                    // combine the two free blocks AND THEN insert into free list

                    // we have next_block header and footer addresses and next_block size

                    size_t new_new_free_block_size = new_free_block_size + next_block_size;

                    // remove free_footer and next_header contents
                    *(sf_header*)new_free_footer_address = 0;
                    *(sf_header*)next_block_header = 0;

                    // Remove next_block links from the list
                    next_block_ptr = new_free_footer_address;

                    (next_block_ptr->body.links.next)->body.links.prev = next_block_ptr->body.links.prev;
                    (next_block_ptr->body.links.prev)->body.links.next = next_block_ptr->body.links.next;

                    // Set header and footer contents for free block
                    *(sf_header*)new_free_header_address = (new_new_free_block_size|1);
                    *(sf_header*)next_block_footer = (*(sf_header*)new_free_header_address)^sf_magic();

                    // Find out the new index of the sf_free_list to insert into:
                    int new_free_block_index = sfListIndex(new_new_free_block_size);

                    // Set new address of free block to new_alloc_footer_address
                    new_free_block_ptr = new_alloc_footer_address;

                    // Set the links for new free block:
                    new_free_block_ptr->body.links.next = sf_free_list_heads[new_free_block_index].body.links.next;
                    new_free_block_ptr->body.links.prev = &sf_free_list_heads[new_free_block_index];

                    // Insert into the list correctly:
                    sf_free_list_heads[new_free_block_index].body.links.next = new_free_block_ptr;
                    (new_free_block_ptr->body.links.next)->body.links.prev = new_free_block_ptr;
                }
            }
        }
    }
    return pp;
}

/*
    +-----------------------------------------------------------------------------------------+
    |                                    HELPER FUNCTIONS                                     |
    +-----------------------------------------------------------------------------------------+
*/

/*
    @param total_size_needed
    @return the index number of the appropriate size class for sf_free_list_heads

    Key: M = 32 Bytes
    index 0: [M]          :   [32]
    index 1: (M,2M]       :   [33,64]
    index 2: (2M,4M]      :   [65,128]
    index 3: (4M,8M]      :   [129,256]
    index 4: (8M,16M]     :   [257,512]
    index 5: (16M,32M]    :   [513,1024]
    index 6: (32M,64M]    :   [1025,2048]
    index 7: (64M,128M]   :   [2049,4096]
    index 8: (128M,inf]   :   [>=4097]
*/
int sfListIndex(size_t total_size_needed){
    if(total_size_needed == 32){
        return 0;
    }else if(total_size_needed >= 33 && total_size_needed <= 64){
        return 1;
    }else if(total_size_needed >= 65 && total_size_needed <= 128){
        return 2;
    }else if(total_size_needed >= 129 && total_size_needed <= 256){
        return 3;
    }else if(total_size_needed >= 257 && total_size_needed <= 512){
        return 4;
    }else if(total_size_needed >= 513 && total_size_needed <= 1024){
        return 5;
    }else if(total_size_needed >= 1025 && total_size_needed <= 2048){
        return 6;
    }else if(total_size_needed >= 2049 && total_size_needed <= 4096){
        return 7;
    }else{
        return 8;
    }
}


/*
    @param total_size_needed is
    @return the address of the sentinel node of the appropriate size class

    Note: 8 Bytes header, 8 Bytes footer and

    Key: M = 32 Bytes
    index 0: [M]          :   [32]
    index 1: (M,2M]       :   [33,64]
    index 2: (2M,4M]      :   [65,128]
    index 3: (4M,8M]      :   [129,256]
    index 4: (8M,16M]     :   [257,512]
    index 5: (16M,32M]    :   [513,1024]
    index 6: (32M,64M]    :   [1025,2048]
    index 7: (64M,128M]   :   [2049,4096]
    index 8: (128M,inf]   :   [>=4097]
*/

sf_block* sentinel_node_address(size_t total_size_needed){
    if(total_size_needed == 32){
        return &sf_free_list_heads[0]; // index 0: [M]          :   [32]
    }else if(total_size_needed >= 33 && total_size_needed <= 64){
        return &sf_free_list_heads[1]; // index 1: (M,2M]       :   [33,64]
    }else if(total_size_needed >= 65 && total_size_needed <= 128){
        return &sf_free_list_heads[2]; // index 2: (2M,4M]      :   [65,128]
    }else if(total_size_needed >= 129 && total_size_needed <= 256){
        return &sf_free_list_heads[3]; // index 3: (4M,8M]      :   [129,256]
    }else if(total_size_needed >= 257 && total_size_needed <= 512){
        return &sf_free_list_heads[4]; // index 4: (8M,16M]     :   [257,512]
    }else if(total_size_needed >= 513 && total_size_needed <= 1024){
        return &sf_free_list_heads[5]; // index 5: (16M,32M]    :   [513,1024]
    }else if(total_size_needed >= 1025 && total_size_needed <= 2048){
        return &sf_free_list_heads[6]; // index 6: (32M,64M]    :   [1025,2048]
    }else if(total_size_needed >= 2049 && total_size_needed <= 4096){
        return &sf_free_list_heads[7]; // index 7: (64M,128M]   :   [2049,4096]
    }else{
        return &sf_free_list_heads[8]; // index 8: (128M,inf]   :   [>=4097]
    }
}

/*
    This function sets the links each index of the param sf_free_list_heads[] to be self-referential.
    @param sf_free_list_heads is an array of sf_block structs (of size 9).
    @return void after initialization is complete.
*/
void initialize_sf_free_list_heads(){
    int i;
    for(i = 0; i < NUM_FREE_LISTS; i++){
        sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
        sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
    }
}


/*
    This function returns the total size needed to complete the request for malloc.
    This computes : total_size_needed = requested_size + header + footer + padding required.
    This should be used to set the header of a free or allocated block.
    Formula for computing padding: 16 - (requested_size % 16)

    Short-hand: return requested_size + padding_needed + size_of_header_and_footer

    An example:
        requested_size = 44
        padding = 16 - (44 % 16) == 16 - (12) == 4
        requested_size_plus_padding = 44 + 4 = 48
        total_size_needed = requested_size_plus_padding + 16 (16 Bytes for header and footer)
                            48 + 16 = 64 Byte Block needed
        returning 64.

    Should use this function as sentinel_node_address( total_size_needed(size));

    @param requested_size: the size the user is requesting.
    @return total_size_needed
*/

size_t total_size_needed(size_t requested_size){
    if((requested_size % 16) == 0){
        return requested_size + 16;
    }else{
        return requested_size + (16 - (requested_size % 16)) + 16;
    }
}
/*
    Upon completion of this function, a free block should have either been allocated entirely or have been split
    If split a new free block should be inserted into appropriate size class and the next block's "prev alloc" field should be updated.
    @param block_ptr is the free block we found from searching the free_list
           alligned_request: the size of the user request + 16 + optional padding
    @return payload pointer
*/
void* allocate_block(sf_block* block_ptr, size_t alligned_request){
    // Pointer Declarations
    void* block_ptr_address = block_ptr;            // ADDR = prev_footer
    void* header_address = block_ptr_address + 8;   // ADDR = free block header

    size_t free_block_size = ((*(sf_header*)header_address) & BLOCK_SIZE_MASK); // free block size

    void* footer_address = block_ptr_address + free_block_size; // ADDR = free block footer

    /* Check if we must split the block or allocate the whole block */
    size_t remainder = free_block_size - alligned_request;
    /* Determine if the prev_block is allocated(1) (which it should be) */
    //void* prev_block_footer_address = block_ptr_address;
    //size_t prev_alloc_bit;
    //prev_alloc_bit = (((*((sf_header*)prev_block_footer_address))^sf_magic())&2); // prev_alloc bit mask

    char* payload_address;

    if(remainder >= 0 && remainder <= 31){
        // Then allocate the entire free block:

        // Set the header and footer information
        *((sf_header*)header_address) = ((*((sf_header*)header_address)|3)); // OR header contents: alloc(1) | prev_alloc(1)
        *((sf_header*)footer_address) = (*((sf_header*)header_address))^sf_magic();

        // Remove the old free block from the list:
        (block_ptr->body.links.next)->body.links.prev = block_ptr->body.links.prev;
        (block_ptr->body.links.prev)->body.links.next = block_ptr->body.links.next;

        // Remove the link completely from the free block (possibly unecessary)
        block_ptr->body.links.prev = NULL;
        block_ptr->body.links.next = NULL;

        /* Setting the next block's "Prev_alloc" Bit */
        void* next_header_address = header_address + free_block_size;
        void* epilogue_address = sf_mem_end() - 8;

        // Since we'll be allocating we need to set the prev_alloc bit to 1 in the next block
        if(next_header_address == epilogue_address){
            // Just Set prev_alloc bit:
            *((sf_header*)epilogue_address) = 3^sf_magic(); // epilogue contents: size(0) alloc(1) and prev_alloc(1) ^sf_magic()
        }else{
            // Then set the next header and the next footer's prev_alloc bits to 1
            size_t next_block_size = ((*(sf_header*)next_header_address)&BLOCK_SIZE_MASK);
            void* next_footer_address = footer_address + next_block_size;

            // set the next_header and next_footer prev_alloc(1)
            *(sf_header*)next_header_address = ((*(sf_header*)next_header_address)|1);
            *(sf_footer*)next_footer_address = ((*(sf_header*)next_header_address)^sf_magic());
        }

        // Get payload address
        payload_address = header_address + 8;
    }else{
    // Then split the block
        void* new_free_header_address;
        void* new_free_footer_address;
        void* new_alloc_header_address;
        void* new_alloc_footer_address;
        size_t new_alloc_block_size;
        size_t new_free_block_size;

        // Remove the old free block from the list of original size_class:
        (block_ptr->body.links.next)->body.links.prev = block_ptr->body.links.prev;
        (block_ptr->body.links.prev)->body.links.next = block_ptr->body.links.next;

        // Get sizes for new free block and new alloc block:
        new_free_block_size = free_block_size - alligned_request; // aka remainder from before
        new_alloc_block_size = alligned_request;

        // Get new free header Addr and new alloc footer addr (easy)
        new_alloc_header_address = header_address;
        new_free_footer_address = footer_address;

        // Calculate Addresses for new_alloc header and new_free_footer
        new_alloc_footer_address = block_ptr_address + new_alloc_block_size;
        new_free_header_address = new_alloc_footer_address + 8;

        // Set free block header / footer information:
        *((sf_header*)new_free_header_address) = ((new_free_block_size)|1); // alloc(0)| prev_alloc(1)
        *((sf_header*)new_free_footer_address) = ((*(sf_header*)new_free_header_address)^sf_magic());

        // Set alloc block header / footer information:
        *((sf_header*)new_alloc_header_address) = ((new_alloc_block_size|3)); // alloc(1) | prev_alloc(1)
        *((sf_header*)new_alloc_footer_address) = ((*(sf_header*)new_alloc_header_address))^sf_magic();

        // Find out the new index of the sf_free_list to insert into:
        int new_free_block_index = sfListIndex(new_free_block_size);

        // Set new address of free block to new_alloc_footer_address
        sf_block* new_free_block = new_alloc_footer_address;

        // Set the links for new free block:
        new_free_block->body.links.next = sf_free_list_heads[new_free_block_index].body.links.next;
        new_free_block->body.links.prev = &sf_free_list_heads[new_free_block_index];

        // Insert into the list correctly:
        sf_free_list_heads[new_free_block_index].body.links.next = new_free_block;
        (new_free_block->body.links.next)->body.links.prev = new_free_block;

        payload_address = new_alloc_header_address + 8;
    }
    return (void*)payload_address;
}

/*
    Upon completion of this function, a new free block will be generated that shall satisfy a malloc request.
    This function first checks if the free block before the epilogue is a free or allocated one
    If it is free block we save that block size, store it in a variable called new_free_block_size
    and call += 4096 in a while loop with sf_mem_grow() until we have a size that fits the malloc request.
    The function also inserts this new free block that satisfies the request into the respective free list
    and updates all header / footer information respectively
    This function also zero's out the previous footer of the free block (if there was one there).

    @param alligned_request: the size of the user request + 16 + optional padding
    @return pointer to the new free block that can satisfy the request
*/
void* generate_alligned_request_free_block(size_t alligned_request){

    void* epilogue_address = sf_mem_end() - 8;
    void* prev_block_footer_address = sf_mem_end() - 16;
    void* starting_header_address;
    void* new_free_block_footer_address;
    void* new_epilogue_address;

    sf_block* return_free_block;
    size_t new_free_block_size;

    if((((*(sf_header*)prev_block_footer_address)^sf_magic())|2) == 2){
        // Then the previous block is allocated so we start from the epilo with inital size 0
        new_free_block_size = 0;
        starting_header_address = epilogue_address;

    }else{
        // Then the previous block is free and we need to find out the block_size and starting header address
        new_free_block_size = ((*((sf_header*)prev_block_footer_address))^sf_magic())&BLOCK_SIZE_MASK;
        starting_header_address = epilogue_address - new_free_block_size;

        // Remove original prev_footer and epilogue by zeroing out
        *(sf_header*)prev_block_footer_address = 0;
        *(sf_header*)epilogue_address = 0;
    }

    while(alligned_request >= new_free_block_size){

        if(sf_mem_grow() != NULL){
            new_free_block_size+= 4096;
        }else{
            /* Use the current new_free_block_size and make a free block with it because
               as of right now we've got a messed up heap (no free block and no epilogue) */

            new_epilogue_address = sf_mem_end() - 8;
            new_free_block_footer_address = sf_mem_end() - 16;

            if(new_epilogue_address == epilogue_address){
                sf_errno = ENOMEM;
                return NULL;
            }else{
                /* Set new free block header and footer information */
                *(sf_header*)starting_header_address = new_free_block_size|1;
                *(sf_header*)new_free_block_footer_address = ((*(sf_header*)starting_header_address)^sf_magic());
                *(sf_header*)new_epilogue_address = 2^sf_magic();

                sf_block* new_free_block_ptr = starting_header_address - 8;

                // Remove the contents of :
                if(starting_header_address != epilogue_address){
                    *(sf_header*)epilogue_address = 0;
                    *(sf_header*)prev_block_footer_address = 0;
                }

                // Remove the old free block from the list of original size_class:
                if(starting_header_address != (epilogue_address - 8)){
                    (new_free_block_ptr->body.links.next)->body.links.prev = new_free_block_ptr->body.links.prev;
                    (new_free_block_ptr->body.links.prev)->body.links.next = new_free_block_ptr->body.links.next;
                }

                // Find out the new index of the sf_free_list to insert into:
                size_t new_free_block_index = sfListIndex(new_free_block_size);

                // Set the links for new free block:
                new_free_block_ptr->body.links.next = sf_free_list_heads[new_free_block_index].body.links.next;
                new_free_block_ptr->body.links.prev = &sf_free_list_heads[new_free_block_index];

                // Insert into the list correctly:
                sf_free_list_heads[new_free_block_index].body.links.next = new_free_block_ptr;
                (new_free_block_ptr->body.links.next)->body.links.prev = new_free_block_ptr;

                sf_show_heap();

                sf_errno = ENOMEM;
                return NULL;
            }
        }
    }

    // Create new free block of size: new_free_block_size
    new_epilogue_address = sf_mem_end() - 8;
    new_free_block_footer_address = sf_mem_end() - 16;

    // Set the new header and footer and epilogue
    *(sf_header*)starting_header_address = new_free_block_size|1;
    *(sf_header*)new_free_block_footer_address = ((*(sf_header*)starting_header_address)^sf_magic());
    *(sf_header*)new_epilogue_address = 2^sf_magic();

    // Create new free_block_pointer
    return_free_block = starting_header_address - 8;

    // Remove the old free block from the old free list (Case 2: free block before first sf_mem_grow())
    // Remove the free block from the size class list in the case where the free block starts before the old epilogue
    if(starting_header_address != epilogue_address){
        (return_free_block->body.links.next)->body.links.prev = return_free_block->body.links.prev;
        (return_free_block->body.links.prev)->body.links.next = return_free_block->body.links.next;
    }

    // Insert the free block into the appropriate size class
    size_t new_free_block_index = sfListIndex(new_free_block_size);

    // Set the links for new free block:
    return_free_block->body.links.next = sf_free_list_heads[new_free_block_index].body.links.next;
    return_free_block->body.links.prev = &sf_free_list_heads[new_free_block_index];

    // Insert into the list correctly:
    (sf_free_list_heads[new_free_block_index].body.links.next)->body.links.prev = return_free_block;
    sf_free_list_heads[new_free_block_index].body.links.next = return_free_block;

    return return_free_block;
}

/*
    First verifies that the pointer being passed belongs to an allocated block.
    This can be done by examining the fields in the block header and footer.
    In this assignment, we will consider the following cases to be invalid pointers:
        ~ The pointer is NULL.
        ~ The allocated bit in the header is 0.
        ~ The header of the block is before the end of the prologue, or the footer of the block is
            after the beginning of the epilogue.
        ~ The block_size field is less than the minimum block size of 32 bytes.
        ~ The prev_alloc field is 0, indicating that the previous block is free,
            but the alloc field of the previous block header is not 0.
        ~ The bitwise XOR of the footer contents and the value returned by sf_magic()
            does not equal the header contents.

    @param *pp a pointer to the first byte of the payload from a malloc() call
    @return 1 on true (is invalid) and 0 on false (is not invalid)
*/
int is_invalid_pointer(void *pp){
    // The pointer is NULL.
    if(pp == NULL){
        return 1;
    }

    sf_block* block_ptr = (sf_block*)(pp - 16); // Addr = prev footer
    // The allocated bit in the header is 0.
    if(((block_ptr->header)&2) == 0){
        return 1;
    }

    // The header of the block is before the end of the prologue,
    //      or the footer of the block is after the beginning of the epilogue.
    void* prologue_footer_address = sf_mem_start() + 32;
    void* epilogue_address = sf_mem_end() - 8;
    void* header_address = pp - 8;
    if((pp < prologue_footer_address) || (pp > epilogue_address)){
        return 1;
    }

    // The block_size field is less than the minimum block size of 32 bytes.
    size_t block_size = (block_ptr->header)&BLOCK_SIZE_MASK;
    if(block_size < 32){
        return 1;
    }

    // The prev_alloc field is 0, indicating that the previous block is free,
    //      but the alloc field of the previous block header is not 0.
    size_t previous_block_alloc_bit;
    size_t current_block_prev_alloc_bit;
    void* previous_footer_address = pp - 16;
    previous_block_alloc_bit = ((*(sf_header*)previous_footer_address)^sf_magic())&2;
    current_block_prev_alloc_bit = (*(sf_header*)header_address)&1;
    if((current_block_prev_alloc_bit == 0) && (previous_block_alloc_bit == 1)){
        return 1;
    }

    // The bitwise XOR of the footer contents and the value returned by sf_magic()
    //        does not equal the header contents.
    // In otherwords check if the header contents equals the footer contents^sf_magic()
    void* footer_address = previous_footer_address + block_size;
    if((*(sf_header*)header_address) != ((*(sf_footer*)footer_address)^sf_magic())){
        return 1;
    }

    return 0;
}

void* coalesce(void* alloc_block_ptr){

    sf_block* prev_block_ptr;
    sf_block* curr_block_ptr;
    sf_block* next_block_ptr;

    void* prev_block_header_address;
    void* prev_block_footer_address;

    void* curr_block_header_address;
    void* curr_block_footer_address;

    void* next_block_header_address;
    void* next_block_footer_address;

    void* prologue_footer_address;
    void* epilogue_address;

    size_t prev_alloc_bit, next_alloc_bit;
    size_t prev_block_size, curr_block_size, next_block_size, new_free_block_size;

    int new_free_block_index;

    sf_block* new_free_block_ptr; // return value

    // Set prologue footer and epilogue pointer address values:
    prologue_footer_address = sf_mem_start() + 32;
    epilogue_address = sf_mem_end() - 8;

    // get the prev_alloc bit and next alloc bits
    // We have void* alloc_block_ptr --> is basically prev_block_address
    void* curr_bit_header = alloc_block_ptr + 8;
    size_t curr_bit_block_size = ((*(sf_header*)curr_bit_header)&BLOCK_SIZE_MASK);
    void* curr_bit_footer = alloc_block_ptr + curr_bit_block_size;
    void* next_bit_header = curr_bit_footer + 8;

    if((alloc_block_ptr == prologue_footer_address) && (next_bit_header == epilogue_address)){
        prev_alloc_bit = 1;
        next_alloc_bit = 1;
    }else if((alloc_block_ptr == prologue_footer_address) && (next_bit_header != epilogue_address)){
        prev_alloc_bit = 1;
        next_alloc_bit = (*(sf_header*)next_bit_header)&2;
        next_alloc_bit = next_alloc_bit >> 1;
    }else if((alloc_block_ptr != prologue_footer_address) && (next_bit_header == epilogue_address)){
        next_alloc_bit = 1;
        prev_alloc_bit = (*(sf_header*)(curr_bit_header))&1;
    }else{
        // get the prev_alloc_bit and next_alloc_bit normally
        prev_alloc_bit = (*(sf_header*)(curr_bit_header))&1; // for checking prev_alloc set --> use 1
        next_alloc_bit = (*(sf_header*)next_bit_header)&2;
        next_alloc_bit = next_alloc_bit >> 1;
    }

    if((prev_alloc_bit == 0) && (next_alloc_bit == 0)){
        // Case 1: New Free Block = prev block + current free'd one + next block

        /* Set all appropriate pointer values */
        void* prev_prev_block_footer_address;

        prev_block_footer_address = alloc_block_ptr;
        prev_block_size = (((*(sf_header*)prev_block_footer_address)^sf_magic())&BLOCK_SIZE_MASK);
        curr_block_header_address = prev_block_footer_address + 8;
        prev_block_header_address = curr_block_header_address - prev_block_size;
        prev_prev_block_footer_address = prev_block_header_address - 8;
        prev_block_ptr = prev_prev_block_footer_address;
        curr_block_size = ((*(sf_header*)curr_block_header_address)&BLOCK_SIZE_MASK);
        curr_block_footer_address = prev_block_footer_address + curr_block_size;
        next_block_ptr = curr_block_footer_address;
        next_block_header_address = curr_block_footer_address + 8;
        next_block_size = ((*(sf_header*)next_block_header_address)&BLOCK_SIZE_MASK);
        next_block_footer_address = curr_block_footer_address + next_block_size;

        /* Remove all free blocks from list */

        /* Current block does not have links to remove! Remove from prev free block and next free block */

        /* Remove the prev free block from the list */
        (prev_block_ptr->body.links.next)->body.links.prev = prev_block_ptr->body.links.prev;
        (prev_block_ptr->body.links.prev)->body.links.next = prev_block_ptr->body.links.next;

        /* Remove the next free block from the list */
        (next_block_ptr->body.links.next)->body.links.prev = next_block_ptr->body.links.prev;
        (next_block_ptr->body.links.prev)->body.links.next = next_block_ptr->body.links.next;

        /* Compute new block_size */
        new_free_block_size = prev_block_size + curr_block_size + next_block_size;

        /* Set new header and footer information */
        *(sf_header*)prev_block_header_address = new_free_block_size|1;
        *(sf_header*)next_block_footer_address = ((*(sf_header*)prev_block_header_address)^sf_magic());

        /* Remove prev_block_footer, curr_block_header, curr_block_footer, next_block_header information */
        *(sf_header*)prev_block_footer_address = 0;
        *(sf_header*)curr_block_header_address = 0;
        *(sf_header*)curr_block_footer_address = 0;
        *(sf_header*)next_block_header_address = 0;

        // Find out the new index of the sf_free_list to insert into:
        new_free_block_index = sfListIndex(new_free_block_size);

        // Set the links for new free block:
        prev_block_ptr->body.links.next = sf_free_list_heads[new_free_block_index].body.links.next;
        prev_block_ptr->body.links.prev = &sf_free_list_heads[new_free_block_index];

        // Insert into the list correctly:
        sf_free_list_heads[new_free_block_index].body.links.next = prev_block_ptr;
        (prev_block_ptr->body.links.next)->body.links.prev = prev_block_ptr;


        // Now We must go into the block below and check the prev_alloc bit to 0!
        /*
            We have the addr of next_header and next_footer, toggle those prev_alloc bits!
            Edge case: check if the next_header addr is epilogue address
        */

        /*
        void* next_next_block_header_address;
        size_t next_next_block_size;
        void* next_next_block_footer_address;

        next_next_block_header_address = next_block_footer_address + 8;

        if(next_next_block_header_address == epilogue_address){
            // Only set the prev_alloc bit of the epilogue
            *(sf_header*)epilogue_address = ((((*(sf_header*)epilogue_address)^sf_magic())&0xFFFFFFFE)^sf_magic());
        }else{
            next_next_block_size = *(sf_header*)next_next_block_header_address;
            next_next_block_footer_address = next_block_footer_address + next_next_block_size;
            // Set the next_header_addr and next_footer
            *(sf_header*)next_next_block_header_address = ((*(sf_header*)next_next_block_header_address)&0xFFFFFFFE);
            *(sf_header*)next_next_block_footer_address = (*(sf_header*)next_next_block_header_address)^sf_magic();
        }
        */

        new_free_block_ptr = prev_block_ptr;

    }else if((prev_alloc_bit == 0)&&(next_alloc_bit == 1)){
        // case 2: New Free Block == prev block + current free'd one

        prev_block_footer_address = alloc_block_ptr;
        curr_block_header_address = prev_block_footer_address + 8;
        curr_block_size = ((*(sf_header*)curr_block_header_address)&BLOCK_SIZE_MASK);
        prev_block_header_address = curr_block_header_address - curr_block_size;
        prev_block_size = ((*(sf_header*)prev_block_header_address)&BLOCK_SIZE_MASK);
        prev_block_ptr = prev_block_header_address - 8;
        curr_block_footer_address = prev_block_footer_address + curr_block_size;

        (prev_block_ptr->body.links.next)->body.links.prev = prev_block_ptr->body.links.prev;
        (prev_block_ptr->body.links.prev)->body.links.next = prev_block_ptr->body.links.next;

        /* Current block is currently allocated so there are no links to remove! */

        /* Compute new block_size */
        new_free_block_size = prev_block_size + curr_block_size;

        /* Set new header and footer information */
        *(sf_header*)prev_block_header_address = new_free_block_size|1;
        *(sf_header*)curr_block_footer_address = (new_free_block_size|1)^sf_magic();

        /* Remove prev_block_footer, curr_block_header, curr_block_footer, next_block_header information */
        *(sf_header*)prev_block_footer_address = 0;
        *(sf_header*)curr_block_header_address = 0;

        // Find out the new index of the sf_free_list to insert into:
        new_free_block_index = sfListIndex(new_free_block_size);

        // Set the links for new free block:
        prev_block_ptr->body.links.next = sf_free_list_heads[new_free_block_index].body.links.next;
        prev_block_ptr->body.links.prev = &sf_free_list_heads[new_free_block_index];

        // Insert into the list correctly:
        sf_free_list_heads[new_free_block_index].body.links.next = prev_block_ptr;
        (prev_block_ptr->body.links.next)->body.links.prev = prev_block_ptr;

        // Set the next_block's prev_alloc to 0!
        /*
            We have the addr of next_header and next_footer, toggle those prev_alloc bits!
            Edge case: check if the next_header addr is epilogue address
        */

        next_block_header_address = curr_block_footer_address + 8;
        next_block_size = ((*(sf_header*)next_block_header_address)&BLOCK_SIZE_MASK);
        next_block_footer_address = curr_block_footer_address + next_block_size;

        if(next_block_header_address == epilogue_address){
            // Only set the prev_alloc bit of the epilogue
            *(sf_header*)epilogue_address = ((((*(sf_header*)epilogue_address)^sf_magic())&0xFFFFFFFE)^sf_magic());
        }else{
            // Set the next_header_addr and next_footer
            *(sf_header*)next_block_header_address = ((*(sf_header*)next_block_header_address)&0xFFFFFFFE);
            *(sf_header*)next_block_footer_address = (*(sf_header*)next_block_header_address)^sf_magic();
        }

        new_free_block_ptr = prev_block_ptr;

    }else if((prev_alloc_bit == 1)&&(next_alloc_bit == 0)){
        // case 3: New Free Block == current free'd + next free block

        // Get all the appropriate values:
        curr_block_ptr = alloc_block_ptr;
        curr_block_header_address = alloc_block_ptr + 8;
        curr_block_size = ((*(sf_header*)curr_block_header_address)&BLOCK_SIZE_MASK);
        curr_block_footer_address = alloc_block_ptr + curr_block_size;
        next_block_header_address = curr_block_footer_address + 8;
        next_block_size = ((*(sf_header*)next_block_header_address)&BLOCK_SIZE_MASK);
        next_block_footer_address = curr_block_footer_address + next_block_size;
        next_block_ptr = curr_block_footer_address;

        /* Remove all free blocks from list */

        /* Current block does not have links to be removed! Remove only from the next block: */

        /* Remove the next free block from the list */
        (next_block_ptr->body.links.next)->body.links.prev = next_block_ptr->body.links.prev;
        (next_block_ptr->body.links.prev)->body.links.next = next_block_ptr->body.links.next;

        /* Compute new block_size */
        new_free_block_size = curr_block_size + next_block_size;

        /* Set new header and footer information */
        *(sf_header*)curr_block_header_address = new_free_block_size|1;
        *(sf_header*)next_block_footer_address = ((*(sf_header*)curr_block_header_address)^sf_magic());

        /* Remove prev_block_footer, curr_block_header, curr_block_footer, next_block_header information */
        *(sf_header*)curr_block_footer_address = 0;
        *(sf_header*)next_block_header_address = 0;

        // Find out the new index of the sf_free_list to insert into:
        new_free_block_index = sfListIndex(new_free_block_size);

        // Set the links for new free block:
        curr_block_ptr->body.links.next = sf_free_list_heads[new_free_block_index].body.links.next;
        curr_block_ptr->body.links.prev = &sf_free_list_heads[new_free_block_index];

        // Insert into the list correctly:
        sf_free_list_heads[new_free_block_index].body.links.next = curr_block_ptr;
        (curr_block_ptr->body.links.next)->body.links.prev = curr_block_ptr;

        new_free_block_ptr = curr_block_ptr;

    }else{
        // Case 4: Normal free: just update header / footer and insert this block into appropriate free list

        curr_block_ptr = alloc_block_ptr; // Use to set links
        curr_block_header_address = alloc_block_ptr + 8;
        curr_block_size = ((*(sf_header*)curr_block_header_address)&BLOCK_SIZE_MASK);
        curr_block_footer_address = alloc_block_ptr + curr_block_size;

        *(sf_header*)curr_block_header_address = ((*(sf_header*)curr_block_header_address)&0xFFFFFFFD);
        *(sf_header*)curr_block_footer_address = (*(sf_header*)curr_block_header_address)^sf_magic();

        // Find out the new index of the sf_free_list to insert into:
        new_free_block_index = sfListIndex(curr_block_size);

        // Set the links for new free block:
        curr_block_ptr->body.links.next = sf_free_list_heads[new_free_block_index].body.links.next;
        curr_block_ptr->body.links.prev = &sf_free_list_heads[new_free_block_index];

        // Insert into the list correctly:
        sf_free_list_heads[new_free_block_index].body.links.next = curr_block_ptr;
        (curr_block_ptr->body.links.next)->body.links.prev = curr_block_ptr;

        // Now We must go into the block below and check the prev_alloc bit to 0!
        /*
            We have the addr of next_header and next_footer, toggle those prev_alloc bits!
            Edge case: check if the next_header addr is epilogue address
        */

        void* next_block_header_address;
        size_t next_block_size;
        void* next_block_footer_address;

        next_block_header_address = curr_block_footer_address + 8;

        if(next_block_header_address == epilogue_address){
            // Only set the prev_alloc bit of the epilogue
            *(sf_header*)epilogue_address = ((((*(sf_header*)epilogue_address)^sf_magic())&0xFFFFFFFE)^sf_magic());
        }else{
            next_block_size = ((*(sf_header*)next_block_header_address)&BLOCK_SIZE_MASK);
            next_block_footer_address = curr_block_footer_address + next_block_size;
            // Set the next_header_addr and next_footer
            *(sf_header*)next_block_header_address = ((*(sf_header*)next_block_header_address)&0xFFFFFFFE);
            *(sf_header*)next_block_footer_address = ((*(sf_header*)next_block_header_address)^sf_magic());
        }
        new_free_block_ptr = curr_block_ptr;
    }
    return new_free_block_ptr;
}