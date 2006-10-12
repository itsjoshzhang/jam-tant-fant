/************************************************************************
**
** NAME:    bpdb.c
**
** DESCRIPTION:    Accessor functions for the bit-perfect database.
**
** AUTHOR:    Ken Elkabany
**        GamesCrafters Research Group, UC Berkeley
**        Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
**
** DATE:    2006-05-01
**
** LICENSE:    This file is part of GAMESMAN,
**        The Finite, Two-person Perfect-Information Game Generator
**        Released under the GPL:
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program, in COPYING; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
**************************************************************************/

// TODO

// SUPPORT tiers
// hardcode VALUE as slot? maybe...
// fix bit widths from shrinks and grows

// NOTE
// supports up to 256 slices each 64-bits in size
// this support is violated in the shrink and grow (should fix soon)

#include "bpdb.h"
#include "gamesman.h"
#include "bpdb_bitlib.h"
#include "bpdb_schemes.h"


//
// Global Variables
//

//
// stores the format of a slice; in particular the
// names, sizes, and maxvalues of its slots
//

SLICE       bpdb_write_slice = NULL;
SLICE       bpdb_nowrite_slice = NULL;

// in memory database
BYTE        *bpdb_write_array = NULL;
BYTE        *bpdb_nowrite_array = NULL;

// numbers of slices
UINT64      bpdb_slices = 0;

// pointer to list of schemes
SLIST       bpdb_schemes = NULL;
SCHEME      bpdb_headerScheme = NULL;

// legacy slots
UINT32 BPDB_VALUESLOT = 0;
UINT32 BPDB_MEXSLOT = 0;
UINT32 BPDB_REMSLOT = 0;
UINT32 BPDB_VISITEDSLOT = 0;

// graphical purposes
BOOLEAN bpdb_have_printed = FALSE;

UINT32 bpdb_buffer_length = 10000;
//
// bpdb_init
//
void
bpdb_init(
                DB_Table *new_db
                )
{
    GMSTATUS status = STATUS_SUCCESS;

    if(NULL == new_db) {
        status = STATUS_INVALID_INPUT_PARAMETER;
        BPDB_TRACE("bpdb_init()", "Input parameter new_db is null", status);
        goto _bailout;
    }

    //
    // initialize global variables
    //
    bpdb_slices = gNumberOfPositions;

    bpdb_write_slice = (SLICE) calloc( 1, sizeof(struct sliceformat) );
    if(NULL == bpdb_write_slice) {
        status = STATUS_NOT_ENOUGH_MEMORY;
        BPDB_TRACE("bpdb_init()", "Could not allocate bpdb_write_slice in memory", status);
        goto _bailout;
    }

    bpdb_nowrite_slice = (SLICE) calloc( 1, sizeof(struct sliceformat) );
    if(NULL == bpdb_nowrite_slice) {
        status = STATUS_NOT_ENOUGH_MEMORY;
        BPDB_TRACE("bpdb_init()", "Could not allocate bpdb_nowrite_slice in memory", status);
        goto _bailout;
    }

    if(!gBitPerfectDBSolver) {

        status = bpdb_add_slot( 2, "VALUE", TRUE, FALSE, &BPDB_VALUESLOT );        //slot 0
        if(!GMSUCCESS(status)) {
            BPDB_TRACE("bpdb_init()", "Could not add value slot", status);
            goto _bailout;
        }
    
        status = bpdb_add_slot(5, "MEX", TRUE, TRUE, &BPDB_MEXSLOT );            //slot 2
        if(!GMSUCCESS(status)) {
            BPDB_TRACE("bpdb_init()", "Could not add mex slot", status);
            goto _bailout;
        }
    
        status = bpdb_add_slot( 8, "REMOTENESS", TRUE, TRUE, &BPDB_REMSLOT );     //slot 4
        if(!GMSUCCESS(status)) {
            BPDB_TRACE("bpdb_init()", "Could not add remoteness slot", status);
            goto _bailout;
        }

        status = bpdb_add_slot( 1, "VISITED", FALSE, FALSE, &BPDB_VISITEDSLOT );   //slot 1
        if(!GMSUCCESS(status)) {
            BPDB_TRACE("bpdb_init()", "Could not add visited slot", status);
            goto _bailout;
        }

        status = bpdb_allocate();
        if(!GMSUCCESS(status)) {
            BPDB_TRACE("bpdb_init()", "Failed call to bpdb_allocate() to allocate bpdb arrays", status);
            goto _bailout;
        }
    }

    new_db->get_value = bpdb_get_value;
    new_db->put_value = bpdb_set_value;
    new_db->get_remoteness = bpdb_get_remoteness;
    new_db->put_remoteness = bpdb_set_remoteness;
    new_db->check_visited = bpdb_check_visited;
    new_db->mark_visited = bpdb_mark_visited;
    new_db->unmark_visited = bpdb_unmark_visited;
    new_db->get_mex = bpdb_get_mex;
    new_db->put_mex = bpdb_set_mex;
    new_db->save_database = bpdb_save_database;
    new_db->load_database = bpdb_load_database;
    new_db->allocate = bpdb_allocate;
    new_db->add_slot = bpdb_add_slot;
    new_db->get_slice_slot = bpdb_get_slice_slot;
    new_db->set_slice_slot = bpdb_set_slice_slot;    
    new_db->free_db = bpdb_free;

    bpdb_schemes = slist_new();

    SCHEME scheme0 = scheme_new( 0, NULL, NULL, NULL, FALSE );
    SCHEME scheme1 = scheme_new( 1, bpdb_generic_varnum_gap_bits, bpdb_generic_varnum_size_bits, bpdb_generic_varnum_implicit_amt, TRUE );
    SCHEME scheme2 = scheme_new( 2, bpdb_ken_varnum_gap_bits, bpdb_ken_varnum_size_bits, bpdb_ken_varnum_implicit_amt, TRUE );

    bpdb_headerScheme = scheme1;

    bpdb_schemes = slist_add( bpdb_schemes, scheme0 );

    if( gBitPerfectDBSchemes ) {
        bpdb_schemes = slist_add( bpdb_schemes, scheme1 );
    }

    if( gBitPerfectDBAllSchemes) {
        bpdb_schemes = slist_add( bpdb_schemes, scheme2 );
    }

_bailout:
    return;
}

GMSTATUS
bpdb_allocate( )
{
    GMSTATUS status = STATUS_SUCCESS;
    UINT64 i = 0;

    // fixed heap corruption =p
    bpdb_write_array = (BYTE *) calloc( (size_t)ceil(((double)bpdb_slices/(double)BITSINBYTE) * (size_t)(bpdb_write_slice->bits) ), sizeof(BYTE));
    if(NULL == bpdb_write_array) {
        status = STATUS_NOT_ENOUGH_MEMORY;
        BPDB_TRACE("bpdb_init()", "Could not allocate bpdb_write_array in memory", status);
        goto _bailout;
    }

    bpdb_nowrite_array = (BYTE *) calloc( (size_t)ceil(((double)bpdb_slices/(double)BITSINBYTE) * (size_t)(bpdb_nowrite_slice->bits) ), sizeof(BYTE));
    if(NULL == bpdb_write_array) {
        status = STATUS_NOT_ENOUGH_MEMORY;
        BPDB_TRACE("bpdb_init()", "Could not allocate bpdb_nowrite_array in memory", status);
        goto _bailout;
    }

    // cannot put in init since bpdbsolver does not have any slots inputted
    // at the time when init is called
    if(!gBitPerfectDBAdjust) {
        for(i = 0; i < (bpdb_write_slice->slots); i++) {
            bpdb_write_slice->adjust[i] = FALSE;
        }
        for(i = 0; i < (bpdb_nowrite_slice->slots); i++) {
            bpdb_nowrite_slice->adjust[i] = FALSE;
        }
    }

    // everything will work without this for loop,
    // until someone changes the value of the undecided enum
    // which is at the time of writing this 0.
    //for(i = 0; i < bpdb_slices; i++) {
    //    bpdb_set_slice_slot( i, BPDB_VALUESLOT, undecided );
    //}

_bailout:
    return status;
}

void
bpdb_free_slice( SLICE sl )
{
    int i = 0;

    SAFE_FREE( sl->size );
    SAFE_FREE( sl->offset );
    SAFE_FREE( sl->maxvalue );
    SAFE_FREE( sl->maxseen );

    for(i = 0; i < (sl->slots); i++) {
        SAFE_FREE( sl->name[i] );
    }

    SAFE_FREE( sl->name );
    SAFE_FREE( sl->overflowed );
    SAFE_FREE( sl->adjust );
}

void
bpdb_free( )
{
    // TODO: must free schemes list
    bpdb_free_slice( bpdb_write_slice );
    bpdb_free_slice( bpdb_nowrite_slice );
    SAFE_FREE( bpdb_nowrite_array );
    SAFE_FREE( bpdb_write_array );
}


VALUE
bpdb_set_value(
                POSITION pos,
                VALUE val
                )
{
    bpdb_set_slice_slot( (UINT64)pos, BPDB_VALUESLOT, (UINT64) val );
    return val;
}

VALUE
bpdb_get_value(
                POSITION pos
                )
{
    return (VALUE) bpdb_get_slice_slot( (UINT64)pos, BPDB_VALUESLOT );
}

REMOTENESS
bpdb_get_remoteness(
                    POSITION pos
                    )
{
    return (REMOTENESS) bpdb_get_slice_slot( (UINT64)pos, BPDB_REMSLOT );
}

void
bpdb_set_remoteness(
                    POSITION pos,
                    REMOTENESS val
                    )
{
    bpdb_set_slice_slot( (UINT64)pos, BPDB_REMSLOT, (REMOTENESS) val );
}

BOOLEAN
bpdb_check_visited(
                POSITION pos
                )
{
    return (BOOLEAN) bpdb_get_slice_slot( (UINT64)pos, BPDB_VISITEDSLOT );
}

void
bpdb_mark_visited(
                POSITION pos
                )
{
    bpdb_set_slice_slot( (UINT64)pos, BPDB_VISITEDSLOT, (UINT64)1 );
}

void
bpdb_unmark_visited(
                POSITION pos
                )
{
    bpdb_set_slice_slot( (UINT64)pos, BPDB_VISITEDSLOT, (UINT64)0 );
}

void
bpdb_set_mex(
                POSITION pos,
                MEX mex
                )
{
    bpdb_set_slice_slot( (UINT64)pos, BPDB_MEXSLOT, (UINT64)mex );
}

MEX
bpdb_get_mex(
                POSITION pos
                )
{
    return (MEX) bpdb_get_slice_slot( (UINT64)pos, BPDB_MEXSLOT );
}
inline
UINT64
bpdb_set_slice_slot(
                UINT64 position,
                UINT8 index,
                UINT64 value
                )
{
    UINT64 byteOffset = 0;
    UINT8 bitOffset = 0;
    BYTE *bpdb_array = NULL;
    SLICE bpdb_slice = NULL;
    BOOLEAN write = TRUE;

    if(index % 2) {
        bpdb_array = bpdb_nowrite_array;
        bpdb_slice = bpdb_nowrite_slice;
        write = FALSE;
    } else {
        bpdb_array = bpdb_write_array;
        bpdb_slice = bpdb_write_slice;
    }
    index /= 2;

    if(value > bpdb_slice->maxseen[index]) {
        bpdb_slice->maxseen[index] = value;
    }

    if(value > bpdb_slice->maxvalue[index]) {
        if(bpdb_slice->adjust[index]) {
            bpdb_grow_slice(bpdb_array, bpdb_slice, index, value);
            if(write) {
                bpdb_array = bpdb_write_array;
            } else {
                bpdb_array = bpdb_nowrite_array;
            }
        } else {
            if(!bpdb_slice->overflowed[index]) {
                if(!bpdb_have_printed) {
                    bpdb_have_printed = TRUE;
                    printf("\n");
                }
                printf("Warning: Slot %s with bit size %u had to be rounded from %llu to its maxvalue %llu.\n",
                                                                            bpdb_slice->name[index],
                                                                            bpdb_slice->size[index],
                                                                            value,
                                                                            bpdb_slice->maxvalue[index]
                                                                            );
                bpdb_slice->overflowed[index] = TRUE;
            }
            value = bpdb_slice->maxvalue[index];
        }
    }

    byteOffset = (bpdb_slice->bits * position)/BITSINBYTE;
    bitOffset = ((UINT8)(bpdb_slice->bits % BITSINBYTE) * (UINT8)(position % BITSINBYTE)) % BITSINBYTE;
    bitOffset += bpdb_slice->offset[index];
    
    byteOffset += bitOffset / BITSINBYTE;
    bitOffset %= BITSINBYTE;

    //printf("byteoff: %d bitoff: %d value: %llu length: %d\n", byteOffset, bitOffset, value, length);
    //printf("value: %llu\n", value);
    bitlib_insert_bits( bpdb_array + byteOffset, bitOffset, value, bpdb_slice->size[index] );

    return value;
}

GMSTATUS
bpdb_grow_slice(
                BYTE *bpdb_array,
                SLICE bpdb_slice,
                UINT8 index,
                UINT64 value
                )
{
    GMSTATUS status = STATUS_SUCCESS;
    BYTE *bpdb_new_array = NULL;

    // information about conversion
    UINT32 newSlotSize = 0;
    UINT32 oldSlotSize = 0;
    UINT32 bitsToAddToSlot = 0;

    UINT32 oldSliceSize = 0;
    UINT32 newSliceSize = 0; 

    // counters
    UINT64 currentSlot = 0;
    UINT64 currentSlice = 0;

    // used to count the number
    // of bits a new value needs
    UINT64 temp = value;

    // offsets of old slices
    UINT32 fbyteOffset = 0;
    UINT32 fbitOffset = 0;

    // offsets of new slices
    UINT32 tbyteOffset = 0;
    UINT32 tbitOffset = 0;

    // offsets of old slices + slot
    UINT32 mfbyteOffset = 0;
    UINT32 mfbitOffset = 0;

    // offsets of new slices + slot
    UINT32 mtbyteOffset = 0;
    UINT32 mtbitOffset = 0;

    // size of right and left portions of the slice
    // **left**|**middle/expanding***|**right**
    UINT32 rightSize = 0;
    UINT32 leftSize = 0;

    // save the data of a slice before copying it
    UINT64 data = 0;

    //
    // main code
    //

    // debug-temp
    //bpdb_dump_database(1);

    // determine size of new largest value
    while(0 != temp) {
        bitsToAddToSlot++;
        temp = temp >> 1;
    }

    //
    // WE WANT TO DO THESE CONVERSIONS FIRST B/C
    // WE DO NOT WANT TO CHANGE ANY OF THE MEMBER VARS
    // IF WE DO, AND THE ALLOC FAILS, THE DB WILL BE DAMAGED
    // THIS MAINTAINS INTEGRITY AFTER AN ALLOC FAIL
    //

    // find difference between old size, and the new
    // size required for the slot
    bitsToAddToSlot -= bpdb_slice->size[index];

    // save old slice size
    oldSlotSize = bpdb_slice->size[index];

    // number of additional bits
    newSlotSize = oldSlotSize + bitsToAddToSlot;

    // bits
    oldSliceSize = bpdb_slice->bits;
    newSliceSize = bpdb_slice->bits + bitsToAddToSlot;
    if(!bpdb_have_printed) {
        bpdb_have_printed = TRUE;
        printf("\n");
    }
    printf("Expanding database (Slot %s %u bits->%u bits)... ", bpdb_slice->name[index], oldSlotSize, newSlotSize);

    // allocate new space needed for the larger database
    bpdb_new_array = (BYTE *) realloc( bpdb_array, (size_t)ceil(((double)bpdb_slices/(double)BITSINBYTE) * (size_t)(newSliceSize) ) * sizeof(BYTE));
    if(NULL == bpdb_new_array) {
        status = STATUS_NOT_ENOUGH_MEMORY;
        BPDB_TRACE("bpdb_grow_slice()", "Could not allocate new database", status);
        goto _bailout;
    }

    // determine whether new array replaces the write
    // array or the nowrite array
    if(bpdb_array == bpdb_write_array) {
        bpdb_write_array = bpdb_new_array;
    } else {
        bpdb_nowrite_array = bpdb_new_array;
    }

    // debug-temp
    //bpdb_dump_database(2);

    // set slot to new size
    bpdb_slice->size[index] += bitsToAddToSlot;
    // set slice to new size
    bpdb_slice->bits += bitsToAddToSlot;

    // set offsets to new values
    for(currentSlot = 0; currentSlot < bpdb_slice->slots; currentSlot++) {
        if(currentSlot > index) {
            bpdb_slice->offset[currentSlot] += bitsToAddToSlot;
        }
    }

    // set new max value
    bpdb_slice->maxvalue[index] = (UINT64)pow(2, newSlotSize) - 1;

    leftSize = bpdb_slice->offset[index];
    rightSize = bpdb_slice->bits - bpdb_slice->size[index] - leftSize;

    //printf( "start while (value %llu, add %d)\n", value, bitsToAddToSlot );

    // for each record starting from the max record
    // copy from the old db format to the new db format
    currentSlice = bpdb_slices - 1;

    // could not use for loop since currentSlice will wrap from 0 to its max value
    while(TRUE) {
        fbyteOffset = (oldSliceSize * currentSlice)/BITSINBYTE;
        fbitOffset = ((UINT8)(oldSliceSize % BITSINBYTE) * (UINT8)(currentSlice % BITSINBYTE)) % BITSINBYTE;
        fbyteOffset += fbitOffset / BITSINBYTE;
        fbitOffset %= BITSINBYTE;

        tbyteOffset = (newSliceSize * currentSlice)/BITSINBYTE;
        tbitOffset = ((UINT8)(newSliceSize % BITSINBYTE) * (UINT8)(currentSlice % BITSINBYTE)) % BITSINBYTE;
        tbyteOffset += tbitOffset / BITSINBYTE;
        tbitOffset %= BITSINBYTE;

        //printf("Slice %llu (%u, %u) (%u, %u) ", currentSlice, fbyteOffset, fbitOffset, tbyteOffset, tbitOffset);

        // right
        mfbyteOffset = fbyteOffset; mfbitOffset = fbitOffset;
        mtbyteOffset = tbyteOffset; mtbitOffset = tbitOffset;
        mfbitOffset += leftSize + oldSlotSize;
        mtbitOffset += leftSize + newSlotSize;
        mfbyteOffset += mfbitOffset / BITSINBYTE;
        mfbitOffset %= BITSINBYTE;
        mtbyteOffset += mtbitOffset / BITSINBYTE;
        mtbitOffset %= BITSINBYTE;

        data = bitlib_read_bits( bpdb_new_array + mfbyteOffset, mfbitOffset, rightSize );
        //printf("%llu(%u) ", data, rightSize);
        bitlib_insert_bits( bpdb_new_array + mtbyteOffset, mtbitOffset, data, rightSize );


        // middle
        mfbyteOffset = fbyteOffset; mfbitOffset = fbitOffset;
        mtbyteOffset = tbyteOffset; mtbitOffset = tbitOffset;
        mfbitOffset += leftSize;
        mtbitOffset += leftSize;
        mfbyteOffset += mfbitOffset / BITSINBYTE;
        mfbitOffset %= BITSINBYTE;
        mtbyteOffset += mtbitOffset / BITSINBYTE;
        mtbitOffset %= BITSINBYTE;

        data = bitlib_read_bits( bpdb_new_array + mfbyteOffset, mfbitOffset, oldSlotSize );
        //printf("%llu(%u %u) ", data, oldSlotSize, newSlotSize);
        bitlib_insert_bits( bpdb_new_array + mtbyteOffset, mtbitOffset, data, newSlotSize );

        // left
        mfbyteOffset = fbyteOffset; mfbitOffset = fbitOffset;
        mtbyteOffset = tbyteOffset; mtbitOffset = tbitOffset;

        data = bitlib_read_bits( bpdb_new_array + mfbyteOffset, mfbitOffset, leftSize );
        //printf("%llu(%u) \n", data, leftSize);
        bitlib_insert_bits( bpdb_new_array + mtbyteOffset, mtbitOffset, data, leftSize );


        if(currentSlice == 0) break;
        currentSlice--;
    }

    printf("done\n");

    /*
    // debugging information
    printf("Offsets: ");
    for(currentSlot = 0; currentSlot < bpdb_slice->slots; currentSlot++) {
        printf(" %u(%llu)", bpdb_slice->offset[currentSlot], currentSlot);
    }
    printf("\n");

    printf("OldSlice: %u NewSlice: %u OldSlot: %u NewSlot: %u Totalbits %u\n", oldSliceSize, newSliceSize, oldSlotSize, newSlotSize, bpdb_slice->bits);
    printf("Leftsize: %u Rightsize: %u\n", leftSize, rightSize);
    */
    
    // debug-temp
    //bpdb_dump_database(3);

_bailout:
    return status;
}



GMSTATUS
bpdb_shrink_slice(
                BYTE *bpdb_array,
                SLICE bpdb_slice,
                UINT8 index
                )
{
    GMSTATUS status = STATUS_SUCCESS;
    BYTE *bpdb_new_array = NULL;

    // information about conversion
    UINT32 newSlotSize = 0;
    UINT32 oldSlotSize = 0;
    UINT32 bitsToShrink = 0;

    UINT32 oldSliceSize = 0;
    UINT32 newSliceSize = 0; 

    // counters
    UINT64 currentSlot = 0;
    UINT64 currentSlice = 0;

    // used to count the number
    // of bits a new value needs
    UINT64 temp = bpdb_slice->maxseen[index];

    // offsets of old slices
    UINT32 fbyteOffset = 0;
    UINT32 fbitOffset = 0;

    // offsets of new slices
    UINT32 tbyteOffset = 0;
    UINT32 tbitOffset = 0;

    // offsets of old slices + slot
    UINT32 mfbyteOffset = 0;
    UINT32 mfbitOffset = 0;

    // offsets of new slices + slot
    UINT32 mtbyteOffset = 0;
    UINT32 mtbitOffset = 0;

    // size of right and left portions of the slice
    // **left**|**middle/expanding***|**right**
    UINT32 rightSize = 0;
    UINT32 leftSize = 0;

    // save the data of a slice before copying it
    UINT64 data = 0;

    //
    // main code
    //

    // debug-temp
    //bpdb_dump_database(1);

    // determine size of new largest value
    while(0 != temp) {
        bitsToShrink++;
        temp = temp >> 1;
    }

    //
    // WE WANT TO DO THESE CONVERSIONS FIRST B/C
    // WE DO NOT WANT TO CHANGE ANY OF THE MEMBER VARS
    // IF WE DO, AND THE ALLOC FAILS, THE DB WILL BE DAMAGED
    // THIS MAINTAINS INTEGRITY AFTER AN ALLOC FAIL
    //

    // find difference between old size, and the new
    // size required for the slot
    //if(bitsToShrink == 0) bitsToShrink = 1;
    bitsToShrink = bpdb_slice->size[index] - bitsToShrink;

    // save old slice size
    oldSlotSize = bpdb_slice->size[index];

    // number of additional bits
    newSlotSize = oldSlotSize - bitsToShrink;

    // bits
    oldSliceSize = bpdb_slice->bits;
    newSliceSize = bpdb_slice->bits - bitsToShrink;
    if(!bpdb_have_printed) {
        bpdb_have_printed = TRUE;
        printf("\n");
    }
    printf("Shrinking (Slot %s %u bits->%u bits)... ", bpdb_slice->name[index], oldSlotSize, newSlotSize);



    // debug-temp
    //bpdb_dump_database(2);

    // set slot to new size
    bpdb_slice->size[index] -= bitsToShrink;
    // set slice to new size
    bpdb_slice->bits -= bitsToShrink;

    // set offsets to new values
    for(currentSlot = 0; currentSlot < bpdb_slice->slots; currentSlot++) {
        if(currentSlot > index) {
            bpdb_slice->offset[currentSlot] -= bitsToShrink;
        }
    }

    // set new max value
    bpdb_slice->maxvalue[index] = (UINT64)pow(2, newSlotSize) - 1;

    leftSize = bpdb_slice->offset[index];
    rightSize = bpdb_slice->bits - bpdb_slice->size[index] - leftSize;

    //printf( "start while (value %llu, add %d)\n", value, bitsToAddToSlot );

    // for each record
    // copy from the old db format to the new db format

    for(currentSlice = 0; currentSlice < bpdb_slices; currentSlice++) {
        fbyteOffset = (oldSliceSize * currentSlice)/BITSINBYTE;
        fbitOffset = ((UINT8)(oldSliceSize % BITSINBYTE) * (UINT8)(currentSlice % BITSINBYTE)) % BITSINBYTE;
        fbyteOffset += fbitOffset / BITSINBYTE;
        fbitOffset %= BITSINBYTE;

        tbyteOffset = (newSliceSize * currentSlice)/BITSINBYTE;
        tbitOffset = ((UINT8)(newSliceSize % BITSINBYTE) * (UINT8)(currentSlice % BITSINBYTE)) % BITSINBYTE;
        tbyteOffset += tbitOffset / BITSINBYTE;
        tbitOffset %= BITSINBYTE;

        //printf("Slice %llu (%u, %u) (%u, %u) ", currentSlice, fbyteOffset, fbitOffset, tbyteOffset, tbitOffset);
   
         // left
        mfbyteOffset = fbyteOffset; mfbitOffset = fbitOffset;
        mtbyteOffset = tbyteOffset; mtbitOffset = tbitOffset;

        data = bitlib_read_bits( bpdb_array + mfbyteOffset, mfbitOffset, leftSize );
        //printf("%llu(%u) \n", data, leftSize);
        bitlib_insert_bits( bpdb_array + mtbyteOffset, mtbitOffset, data, leftSize );

        // middle
        mfbyteOffset = fbyteOffset; mfbitOffset = fbitOffset;
        mtbyteOffset = tbyteOffset; mtbitOffset = tbitOffset;
        mfbitOffset += leftSize;
        mtbitOffset += leftSize;
        mfbyteOffset += mfbitOffset / BITSINBYTE;
        mfbitOffset %= BITSINBYTE;
        mtbyteOffset += mtbitOffset / BITSINBYTE;
        mtbitOffset %= BITSINBYTE;

        data = bitlib_read_bits( bpdb_array + mfbyteOffset, mfbitOffset, oldSlotSize );
        //printf("%llu(%u %u) ", data, oldSlotSize, newSlotSize);
        bitlib_insert_bits( bpdb_array + mtbyteOffset, mtbitOffset, data, newSlotSize );

        // right
        mfbyteOffset = fbyteOffset; mfbitOffset = fbitOffset;
        mtbyteOffset = tbyteOffset; mtbitOffset = tbitOffset;
        mfbitOffset += leftSize + oldSlotSize;
        mtbitOffset += leftSize + newSlotSize;
        mfbyteOffset += mfbitOffset / BITSINBYTE;
        mfbitOffset %= BITSINBYTE;
        mtbyteOffset += mtbitOffset / BITSINBYTE;
        mtbitOffset %= BITSINBYTE;

        data = bitlib_read_bits( bpdb_array + mfbyteOffset, mfbitOffset, rightSize );
        //printf("%llu(%u) ", data, rightSize);
        bitlib_insert_bits( bpdb_array + mtbyteOffset, mtbitOffset, data, rightSize );


    }

    printf("done\n");


    // allocate new space needed for the larger database
    bpdb_new_array = (BYTE *) realloc( bpdb_array, (size_t)ceil(((double)bpdb_slices/(double)BITSINBYTE) * (size_t)(newSliceSize) ) * sizeof(BYTE));
    if(NULL == bpdb_new_array) {
        status = STATUS_NOT_ENOUGH_MEMORY;
        BPDB_TRACE("bpdb_shrink_slice()", "Could not allocate new database", status);
        goto _bailout;
    }

    // determine whether new array replaces the write
    // array or the nowrite array
    if(bpdb_array == bpdb_write_array) {
        bpdb_write_array = bpdb_new_array;
    } else {
        bpdb_nowrite_array = bpdb_new_array;
    }

    /*
    // debugging information
    printf("Offsets: ");
    for(currentSlot = 0; currentSlot < bpdb_slice->slots; currentSlot++) {
        printf(" %u(%llu)", bpdb_slice->offset[currentSlot], currentSlot);
    }
    printf("\n");

    printf("OldSlice: %u NewSlice: %u OldSlot: %u NewSlot: %u Totalbits %u\n", oldSliceSize, newSliceSize, oldSlotSize, newSlotSize, bpdb_slice->bits);
    printf("Leftsize: %u Rightsize: %u\n", leftSize, rightSize);
    */
    
    // debug-temp
    //bpdb_dump_database(3);

_bailout:
    return status;
}





inline
UINT64
bpdb_get_slice_slot(
                UINT64 position,
                UINT8 index
                )
{
    UINT64 byteOffset = 0;
    UINT8 bitOffset = 0;
    BYTE *bpdb_array = NULL;
    SLICE bpdb_slice = NULL;

    if(index % 2) {
        bpdb_array = bpdb_nowrite_array;
        bpdb_slice = bpdb_nowrite_slice;
    } else {
        bpdb_array = bpdb_write_array;
        bpdb_slice = bpdb_write_slice;
    }
    index /= 2;

    byteOffset = (bpdb_slice->bits * position)/BITSINBYTE;
    bitOffset = ((UINT8)(bpdb_slice->bits % BITSINBYTE) * (UINT8)(position % BITSINBYTE)) % BITSINBYTE;
    bitOffset += bpdb_slice->offset[index];
    
    byteOffset += bitOffset / BITSINBYTE;
    bitOffset %= BITSINBYTE;

    return bitlib_read_bits( bpdb_array + byteOffset, bitOffset, bpdb_slice->size[index] );
}

// need to return the index of the slot
// slotindex is an output
GMSTATUS
bpdb_add_slot(
                UINT8 size,
                char *name,
                BOOLEAN write,
                BOOLEAN adjust,
                UINT32 *slotindex
                )
{
    GMSTATUS status = STATUS_SUCCESS;
    SLICE bpdb_slice = NULL;

    if(NULL == name) {
        status = STATUS_INVALID_INPUT_PARAMETER;
        BPDB_TRACE("bpdb_add_slot()", "name is passed in as null", status);
        goto _bailout;
    }

    if(NULL == bpdb_write_slice) {
        status = STATUS_INVALID_INPUT_PARAMETER;
        BPDB_TRACE("bpdb_add_slot()", "bpdb_write_slice must be initialized first. call bpdb_init()", status);
        goto _bailout;
    }

    if(NULL == bpdb_nowrite_slice) {
        status = STATUS_INVALID_INPUT_PARAMETER;
        BPDB_TRACE("bpdb_add_slot()", "bpdb_nowrite_slice must be initialized first. call bpdb_init()", status);
        goto _bailout;
    }

    if(write) {
        bpdb_slice = bpdb_write_slice;
        *slotindex = 0;
    } else {
        bpdb_slice = bpdb_nowrite_slice;
        *slotindex = 1;
    }

    *slotindex += bpdb_slice->slots*2;

    // for backwards compatibility
    if(strcmp(name, "VALUE") == 0) BPDB_VALUESLOT = *slotindex;
    else if(strcmp(name, "MEX") == 0) BPDB_MEXSLOT = *slotindex;
    else if(strcmp(name, "REMOTENESS") == 0) BPDB_REMSLOT = *slotindex;
    else if(strcmp(name, "VISITED") == 0) BPDB_VISITEDSLOT = *slotindex;

    bpdb_slice->slots++;
    if(bpdb_slice->slots == 1) {
        bpdb_slice->size = (UINT8 *) calloc( 1, sizeof(UINT8) );
        bpdb_slice->offset = (UINT32 *) calloc( 1, sizeof(UINT32) );
        bpdb_slice->maxvalue = (UINT64 *) calloc( 1, sizeof(UINT64) );
        bpdb_slice->maxseen = (UINT64 *) calloc( 1, sizeof(UINT64) );
        bpdb_slice->name = (char **) calloc( 1, sizeof(char **) );
        bpdb_slice->overflowed = (BOOLEAN *) calloc( 1, sizeof(BOOLEAN) );
        bpdb_slice->adjust = (BOOLEAN *) calloc( 1, sizeof(BOOLEAN) );
    } else {
        bpdb_slice->size = (UINT8 *) realloc( bpdb_slice->size, bpdb_slice->slots*sizeof(UINT8) );
        bpdb_slice->offset = (UINT32 *) realloc( bpdb_slice->offset, bpdb_slice->slots*sizeof(UINT32) );
        bpdb_slice->maxvalue = (UINT64 *) realloc( bpdb_slice->maxvalue, bpdb_slice->slots*sizeof(UINT64) );
        bpdb_slice->maxseen = (UINT64 *) realloc( bpdb_slice->maxseen, bpdb_slice->slots*sizeof(UINT64) );
        bpdb_slice->name = (char **) realloc( bpdb_slice->name, bpdb_slice->slots*sizeof(char **) );
        bpdb_slice->overflowed = (BOOLEAN *) realloc( bpdb_slice->overflowed, bpdb_slice->slots*sizeof(BOOLEAN) );
        bpdb_slice->adjust = (BOOLEAN *) realloc( bpdb_slice->adjust, bpdb_slice->slots*sizeof(BOOLEAN) );
    }

    bpdb_slice->bits += size;
    bpdb_slice->size[bpdb_slice->slots-1] = size;
    if(bpdb_slice->slots == 1) {
        bpdb_slice->offset[bpdb_slice->slots-1] = 0;
    } else {
        bpdb_slice->offset[bpdb_slice->slots-1] = bpdb_slice->offset[bpdb_slice->slots - 2] + bpdb_slice->size[bpdb_slice->slots - 2];
    }
    bpdb_slice->maxvalue[bpdb_slice->slots-1] = (UINT64)pow(2, size) - 1;
    bpdb_slice->maxseen[bpdb_slice->slots-1] = 0;
    bpdb_slice->name[bpdb_slice->slots-1] = (char *) malloc( strlen(name) + 1 );
    strcpy( bpdb_slice->name[bpdb_slice->slots-1], name );
    bpdb_slice->overflowed[bpdb_slice->slots-1] = FALSE;
    bpdb_slice->adjust[bpdb_slice->slots-1] = adjust;

_bailout:
    return status;
}


//
//

void
bpdb_print_database()
{
    UINT64 i = 0;
    UINT8 j = 0;

    printf("\n\nDatabase Printout\n");

    for(i = 0; i < bpdb_slices; i++) {
        printf("Slice %llu  (write) ", i);
        for(j=0; j < (bpdb_write_slice->slots); j++) {
            printf("%s: %llu  ", bpdb_write_slice->name[j], bpdb_get_slice_slot(i, j*2));
        }
        printf("(no write) ");
        for(j=0; j < (bpdb_nowrite_slice->slots); j++) {
            printf("%s: %llu  ", bpdb_nowrite_slice->name[j], bpdb_get_slice_slot(i, j*2+1));
        }
        printf("\n");
    }
}

void
bpdb_dump_database( int num )
{
    char filename[256];
    UINT64 i = 0;
    UINT8 j = 0;
    UINT32 currentSlot = 0;

    mkdir("dumps", 0755) ;

    sprintf(filename, "./dumps/m%s_%d_bpdb_%d.dump", kDBName, getOption(), num);

    FILE *f = fopen( filename, "wb");

    fprintf(f, "\nDatabase Printout\n");

    fprintf(f, "Offsets: ");
    for(currentSlot = 0; currentSlot < bpdb_write_slice->slots; currentSlot++) {
        fprintf(f, " %u(%u)", bpdb_write_slice->offset[currentSlot], currentSlot);
    }
    fprintf(f, "\n");

    for(i = 0; i < bpdb_slices; i++) {
        fprintf(f, "Slice %llu  (write) ", i);
        for(j=0; j < (bpdb_write_slice->slots); j++) {
            fprintf(f, "%s: %llu  ", bpdb_write_slice->name[j], bpdb_get_slice_slot(i, j*2));
        }
        fprintf(f, "(no write) ");
        for(j=0; j < (bpdb_nowrite_slice->slots); j++) {
            fprintf(f, "%s: %llu  ", bpdb_nowrite_slice->name[j], bpdb_get_slice_slot(i, j*2+1));
        }
        fprintf(f, "\n");
    }

    fclose(f);
}


BOOLEAN
bpdb_save_database()
{
    // debug-temp
    //bpdb_print_database();

    GMSTATUS status = STATUS_SUCCESS;

    // counter
    int i;
    SLIST cur;

    // file names of saved files
    char **outfilenames;

    // final file name
    char outfilename[256];

    // track smallest file
    int smallestscheme = 0;
    int smallestsize = -1;

    // struct for fileinfo
    struct stat fileinfo;

    // set counters
    i = 0;
    cur = bpdb_schemes;

    // must free this
    for(i = 0; i<slist_size(bpdb_schemes); i++) {
        outfilenames = (char **) malloc( slist_size(bpdb_schemes)*sizeof(char*) );
    }
    for(i = 0; i<slist_size(bpdb_schemes); i++) {
        outfilenames[i] = (char *) malloc( 256*sizeof(char) );
    }

/*
    // debug-temp
    // print out the status of the slots after solving
    printf("\nNum slots: %u\n", bpdb_write_slice->slots);

    for(i = 0; i<bpdb_write_slice->slots; i++) {
        printf("%s: %llu\n", bpdb_write_slice->name[i], bpdb_write_slice->maxseen[i]);
    }
*/

    bpdb_have_printed = FALSE;

    UINT64 temp = 0;
    UINT8 bitsNeeded = 0;
    for(i=0; i < (bpdb_write_slice->slots); i++) {
        temp = bpdb_write_slice->maxseen[i];
        bitsNeeded = 0;
        while(0 != temp) {
            bitsNeeded++;
            temp = temp >> 1;
        }

        if(bitsNeeded < bpdb_write_slice->size[i] && bpdb_write_slice->adjust[i]) {
            bpdb_shrink_slice( bpdb_write_array, bpdb_write_slice, i );
        }
    }

    // determine size of new largest value
    i = 0;

    printf("\n");

    // save file under each encoding scheme
    while(cur != NULL) {
        // saves with encoding scheme and returns filename
        //sprintf(outfilenames[i], "./data/m%s_%d_bpdb_%d.dat.gz", "test", 1, cur->scheme);
        sprintf(outfilenames[i], "./data/m%s_%d_bpdb_%d.dat.gz", kDBName, getOption(), ((SCHEME)cur->obj)->id);

        status = bpdb_generic_save_database( (SCHEME) cur->obj, outfilenames[i] );
        if(!GMSUCCESS(status)) {
            BPDB_TRACE("bpdb_save_database()", "call to bpdb_generic_save_database with scheme failed", status);
        } else {
            // get size of file
            stat(outfilenames[i], &fileinfo);

            printf("Scheme: %d. Wrote %s with size of %d\n", ((SCHEME)cur->obj)->id, outfilenames[i], (int)fileinfo.st_size);

            // if file is a smaller size, set min
            if(smallestsize == -1 || fileinfo.st_size < smallestsize) {
                smallestscheme = i;
                smallestsize  = fileinfo.st_size;
            }
        }
        cur = cur->next;
        i++;
    }

    printf("Choosing scheme: %d\n", smallestscheme);
    
    // for each file, delete if not the smallest encoding,
    // and if it is, rename it to the final file name.
    for(i = 0; i < slist_size(bpdb_schemes); i++) {
        if(i == smallestscheme) {

            // rename smallest file to final file name
            sprintf(outfilename, "./data/m%s_%d_bpdb.dat.gz", kDBName, getOption());
            //sprintf(outfilename, "./data/m%s_%d_bpdb.dat.gz", "test", 1);
            printf("Renaming %s to %s\n", outfilenames[i], outfilename);
            rename(outfilenames[i], outfilename);
        } else {

            // delete files that are not the smallest
            printf("Removing %s\n", outfilenames[i]);
            remove(outfilenames[i]);
        }
    }

    if(!GMSUCCESS(status)) {
        return TRUE;
    } else {
        return FALSE;
    }
}


BOOLEAN
bpdb_generic_write_varnum(
                dbFILE *outFile,
                SCHEME scheme,
                BYTE **curBuffer,
                BYTE *outputBuffer,
                UINT32 bufferLength,
                UINT8 *offset,
                UINT64 consecutiveSkips
                )
{
    UINT8 leftBits, rightBits;
    
    //leftBits = bpdb_generic_varnum_gap_bits( consecutiveSkips );
    leftBits = scheme->varnum_gap_bits( consecutiveSkips );
    //rightBits = leftBits;
    rightBits = scheme->varnum_size_bits(leftBits);

    bitlib_value_to_buffer( outFile, curBuffer, outputBuffer, bufferLength, offset, bitlib_right_mask64( leftBits), leftBits );
    bitlib_value_to_buffer( outFile, curBuffer, outputBuffer, bufferLength, offset, 0, 1 );

    //consecutiveSkips -= bpdb_generic_varnum_implicit_amt( leftBits );
    consecutiveSkips -= scheme->varnum_implicit_amt( leftBits );

    bitlib_value_to_buffer( outFile, curBuffer, outputBuffer, bufferLength, offset, consecutiveSkips, rightBits );

    return TRUE;
}

UINT8
bpdb_generic_varnum_gap_bits(
                UINT64 consecutiveSkips
                )
{
    UINT8 leftBits = 1;
    UINT8 powerTo = 2;
    UINT64 skipsRepresented = 2;
    
    while(skipsRepresented < consecutiveSkips)
    {
        skipsRepresented += (UINT64)pow(2, powerTo);
        leftBits++;
        powerTo++;
    }

    return leftBits;
}

UINT64
bpdb_generic_varnum_implicit_amt(
                UINT8 leftBits
                )
{
    // for completeness we should have the 64 == leftBits
    // check, but for all practical purposes, there is no need
    // for it

    //if(64 == leftBits) {
    //    return UINT64_MAX;
    //} else {
        return (1 << leftBits) - 1;
    //}
}

GMSTATUS
bpdb_generic_save_database(
                SCHEME scheme,
                char *outfilename
                )
{
    GMSTATUS status = STATUS_SUCCESS;

    UINT64 consecutiveSkips = 0;
    UINT8 offset = 0;
    UINT8 i, j;

    // gzip file ptr
    dbFILE *outFile = NULL;

    UINT64 slice;
    UINT8 slot;

    BYTE *outputBuffer = NULL;
    BYTE *curBuffer = NULL;

    outputBuffer = alloca( bpdb_buffer_length * sizeof(BYTE));
    memset(outputBuffer, 0, bpdb_buffer_length);
    curBuffer = outputBuffer;

    mkdir("data", 0755);

    status = bitlib_file_open(outfilename, "wb", &outFile);
    if(!GMSUCCESS(status)) {
        BPDB_TRACE("bpdb_generic_save_database()", "call to bitlib to open file failed", status);
        goto _bailout;
    }

    bitlib_value_to_buffer ( outFile, &curBuffer, outputBuffer, bpdb_buffer_length, &offset, scheme->id, 8 );

    bpdb_generic_write_varnum( outFile, bpdb_headerScheme, &curBuffer, outputBuffer, bpdb_buffer_length, &offset, bpdb_slices );
    bpdb_generic_write_varnum( outFile, bpdb_headerScheme, &curBuffer, outputBuffer, bpdb_buffer_length, &offset, bpdb_write_slice->bits );

    bpdb_generic_write_varnum( outFile, bpdb_headerScheme, &curBuffer, outputBuffer, bpdb_buffer_length, &offset, bpdb_write_slice->slots );

    for(i = 0; i < (bpdb_write_slice->slots); i++) {
        bpdb_generic_write_varnum( outFile, bpdb_headerScheme, &curBuffer, outputBuffer, bpdb_buffer_length, &offset, bpdb_write_slice->size[i]+1 );
        bpdb_generic_write_varnum( outFile, bpdb_headerScheme, &curBuffer, outputBuffer, bpdb_buffer_length, &offset, strlen(bpdb_write_slice->name[i]) );
        
        for(j = 0; j<strlen(bpdb_write_slice->name[i]); j++) {
            bitlib_value_to_buffer( outFile, &curBuffer, outputBuffer, bpdb_buffer_length, &offset, bpdb_write_slice->name[i][j], 8 );
        }

        bitlib_value_to_buffer( outFile, &curBuffer, outputBuffer, bpdb_buffer_length, &offset, bpdb_write_slice->overflowed[i], 1 );
    }

    if(scheme->indicator) {
        for(slice = 0; slice<bpdb_slices; slice++) {

            // Check if the slice has a mapping
            if(bpdb_get_slice_slot( slice, 0 ) != undecided) {
                
                // If so, then check to see if skips must be outputted
                if(consecutiveSkips != 0) {
                    // Put skips into output buffer
                    bpdb_generic_write_varnum( outFile, scheme, &curBuffer, outputBuffer, bpdb_buffer_length, &offset, consecutiveSkips);
                    // Reset skip counter
                    consecutiveSkips = 0;
                }
                bitlib_value_to_buffer( outFile, &curBuffer, outputBuffer, bpdb_buffer_length, &offset, 0, 1 );
                for(slot=0; slot < (bpdb_write_slice->slots); slot++) {
                    bitlib_value_to_buffer( outFile, &curBuffer, outputBuffer, bpdb_buffer_length, &offset, bpdb_get_slice_slot(slice, 2*slot), bpdb_write_slice->size[slot] );
                }

            } else {
                consecutiveSkips++;
            }
        }
    } else {
        // Loop through all records
        for(slice=0; slice<bpdb_slices; slice++) {
            //bitlib_value_to_buffer( outFile, outputBuffer, &offset, bpdb_get_slice_full(slice), bpdb_slice->bits );
            for(slot=0; slot < (bpdb_write_slice->slots); slot++) {
                bitlib_value_to_buffer( outFile, &curBuffer, outputBuffer, bpdb_buffer_length, &offset, bpdb_get_slice_slot(slice, 2*slot), bpdb_write_slice->size[slot] );
            }
        }
    }

    if(consecutiveSkips != 0) {
        bpdb_generic_write_varnum( outFile, scheme, &curBuffer, outputBuffer, bpdb_buffer_length, &offset, consecutiveSkips);
        consecutiveSkips = 0;
    }

    if(curBuffer != outputBuffer || offset != 0) {
        bitlib_file_write_byte(outFile, outputBuffer, curBuffer-outputBuffer+1);
    }

    status = bitlib_file_close(outFile);
    if(!GMSUCCESS(status)) {
        BPDB_TRACE("bpdb_generic_save_database()", "call to bitlib to close file failed", status);
        goto _bailout;
    }

_bailout:
    return status;
}



BOOLEAN
bpdb_load_database( )
{
    GMSTATUS status = STATUS_SUCCESS;
    SLIST cur;

    // filename
    char outfilename[256];
  
    dbFILE *inFile = NULL;
    FILE *testOpen = NULL;

    // file information
    UINT8 fileFormat;

    // open file
    sprintf(outfilename, "./data/m%s_%d_bpdb.dat.gz", kDBName, getOption());

    if(NULL != (testOpen = fopen(outfilename, "r"))) {
        fclose(testOpen);

        status = bitlib_file_open(outfilename, "rb", &inFile);
        if(!GMSUCCESS(status)) {
            BPDB_TRACE("bpdb_load_database()", "call to bitlib to open file failed", status);
            goto _bailout;
        }
    } else {
        return FALSE;
    }
    
    // read file header

    // TO DO: TEST IF BOOLEAN IS TRUE
    bitlib_file_read_byte( inFile, &fileFormat, 1 );

    printf("\n\nDatabase Header Information\n");

    // print fileinfo
    printf("Encoding Scheme: %d\n", fileFormat);
    
    cur = bpdb_schemes;

    while(((SCHEME)cur->obj)->id != fileFormat) {
        cur = cur->next;
    }
    
    status = bpdb_generic_load_database( inFile, (SCHEME) cur->obj );
    if(!GMSUCCESS(status)) {
        BPDB_TRACE("bpdb_load_database()", "call to bpdb_generic_load_database to load db with recognized scheme failed", status);
        goto _bailout;
    }

    // close file
    status = bitlib_file_close(inFile);
    if(!GMSUCCESS(status)) {
        BPDB_TRACE("bpdb_load_database()", "call to bitlib to close file failed", status);
        goto _bailout;
    }

    // debug-temp
    //bpdb_print_database();

_bailout:
    if(!GMSUCCESS(status)) {
        return FALSE;
    } else {
        return TRUE;
    }
}

GMSTATUS
bpdb_generic_load_database(
                dbFILE *inFile,
                SCHEME scheme
                )
{
    GMSTATUS status = STATUS_SUCCESS;

    UINT64 currentSlice = 0;
    UINT8 currentSlot = 0;

    UINT64 numOfSlicesHeader = 0;
    UINT8 bitsPerSliceHeader = 0;
    UINT8 numOfSlotsHeader = 0;
    UINT8 offset = 0;
    UINT64 skips = 0;
    UINT64 i, j;
    char tempchar;
    char * tempname;
    UINT8 tempnamesize = 0;
    UINT8 tempsize = 0;
    UINT32 tempslotindex = 0;
    BOOLEAN tempoverflowed;
    //BOOLEAN slotsMade = FALSE;

    BYTE *inputBuffer = NULL;
    BYTE *curBuffer = NULL;
    
    inputBuffer = alloca( bpdb_buffer_length * sizeof(BYTE) );
    memset( inputBuffer, 0, bpdb_buffer_length );
    curBuffer = inputBuffer;

    // TO DO - check return value
    bitlib_file_read_byte( inFile, inputBuffer, bpdb_buffer_length );

    //inputBuffer = bitlib_file_read_byte( inFile );

    // Read Header
    numOfSlicesHeader = bpdb_generic_read_varnum( inFile, bpdb_headerScheme, &curBuffer, inputBuffer, bpdb_buffer_length, &offset, FALSE );
    bitsPerSliceHeader = bpdb_generic_read_varnum( inFile, bpdb_headerScheme, &curBuffer, inputBuffer, bpdb_buffer_length, &offset, FALSE );
    numOfSlotsHeader = bpdb_generic_read_varnum( inFile, bpdb_headerScheme, &curBuffer, inputBuffer, bpdb_buffer_length, &offset, FALSE );

    printf("Slices: %llu\nBits per slice: %d\nSlots per slice: %d\n\n", numOfSlicesHeader, bitsPerSliceHeader, numOfSlotsHeader);

    bpdb_free_slice( bpdb_write_slice );

    bpdb_write_slice = (SLICE) calloc( 1, sizeof(struct sliceformat) );
    if(NULL == bpdb_write_slice) {
        status = STATUS_NOT_ENOUGH_MEMORY;
        BPDB_TRACE("bpdb_init()", "Could not allocate bpdb_write_slice in memory", status);
        goto _bailout;
    }

    //
    // outputs slots in db
    // perhaps think about what happens if a slot is missing
    //

    for(i = 0; i<numOfSlotsHeader; i++) {
        // read size slice in bits
        tempsize = bpdb_generic_read_varnum( inFile, bpdb_headerScheme, &curBuffer, inputBuffer, bpdb_buffer_length, &offset, FALSE );
        tempsize--;
        
        // read size of slot name in bytes
        tempnamesize = bpdb_generic_read_varnum( inFile, bpdb_headerScheme, &curBuffer, inputBuffer, bpdb_buffer_length, &offset, FALSE );

        // allocate room for slot name
        tempname = (char *) malloc((tempnamesize + 1) * sizeof(char));

        // read name
        for(j = 0; j<tempnamesize; j++) {
            tempchar = (char)bitlib_read_from_buffer( inFile, &curBuffer, inputBuffer, bpdb_buffer_length, &offset, 8 );
            *(tempname + j) = tempchar;
        }
        *(tempname + j) = '\0';

        // read overflowed bit
        tempoverflowed = bitlib_read_from_buffer( inFile, &curBuffer, inputBuffer, bpdb_buffer_length, &offset, 1 );

        // output information on each slot to the user
        printf("Slot: %s (%u bytes)\n", tempname, tempnamesize);
        printf("\tBit Width: %u\n", tempsize);

        if(tempoverflowed) {
            printf("\tWarning: Some values have been capped at %llu due to overflow.\n", (UINT64)pow(2, tempsize) - 1);
        }

        //if( !slotsMade ) {
            status = bpdb_add_slot(tempsize, tempname, TRUE, FALSE, &tempslotindex);
            if(!GMSUCCESS(status)) {
                BPDB_TRACE("bpdb_generic_load_database()", "call to bpdb_add_slot to add slot from database file failed", status);
                goto _bailout;
            }
        //}
    }

    status = bpdb_allocate();
    if(!GMSUCCESS(status)) {
        BPDB_TRACE("bpdb_generic_load_database()", "call to bpdb_allocate failed", status);
        goto _bailout;
    }
    if( scheme->indicator ) {
        while(currentSlice < numOfSlicesHeader) {
            if(bitlib_read_from_buffer( inFile, &curBuffer, inputBuffer, bpdb_buffer_length, &offset, 1 ) == 0) {
        
                for(currentSlot = 0; currentSlot < (bpdb_write_slice->slots); currentSlot++) {
                    bpdb_set_slice_slot( currentSlice, 2*currentSlot,
                                bitlib_read_from_buffer( inFile, &curBuffer, inputBuffer, bpdb_buffer_length, &offset, bpdb_write_slice->size[currentSlot]) );
                }

                currentSlice++;
            } else {
                
                skips = bpdb_generic_read_varnum( inFile, scheme, &curBuffer, inputBuffer, bpdb_buffer_length, &offset, TRUE );

                for(i = 0; i < skips; i++, currentSlice++) {
                    bpdb_set_slice_slot( currentSlice, 0, undecided );
                }
            }
        }
    } else {
        for( currentSlice = 0; currentSlice < numOfSlicesHeader; currentSlice++ ) {
            for(currentSlot = 0; currentSlot < (bpdb_write_slice->slots); currentSlot++) {
                bpdb_set_slice_slot( currentSlice, 2*currentSlot,
                            bitlib_read_from_buffer( inFile, &curBuffer, inputBuffer, bpdb_buffer_length, &offset, bpdb_write_slice->size[currentSlot]) );
            }
        }
    }

_bailout:
    return status;
}

UINT8
bpdb_generic_varnum_size_bits( UINT8 leftBits ) {
    return leftBits;
}

UINT64
bpdb_generic_read_varnum(
                dbFILE *inFile,
                SCHEME scheme,
                BYTE **curBuffer,
                BYTE *inputBuffer,
                UINT32 length,
                UINT8 *offset,
                BOOLEAN alreadyReadFirstBit
                )
{
    UINT8 i;
    UINT64 variableNumber = 0;
    UINT8 leftBits = 0;
    UINT8 rightBits = 0;

    if(alreadyReadFirstBit) {
        leftBits = 1;
    }

    while(bitlib_read_from_buffer( inFile, curBuffer, inputBuffer, length, offset, 1 )) {
        leftBits++;
    }

    //leftBits = bpdb_generic_read_varnum_consecutive_ones( inFile, curBuffer, inputBuffer, length, offset, alreadyReadFirstBit );
    rightBits = scheme->varnum_size_bits(leftBits);

    for(i = 0; i < rightBits; i++) {
        variableNumber = variableNumber << 1;
        variableNumber = variableNumber | bitlib_read_from_buffer( inFile, curBuffer, inputBuffer, length, offset, 1 );
    }

    //variableNumber += bpdb_generic_varnum_implicit_amt( leftBits );
    variableNumber += scheme->varnum_implicit_amt( leftBits );

    return variableNumber;
}
/*
UINT8
bpdb_generic_read_varnum_consecutive_ones(
                dbFILE *inFile,
                BYTE **curBuffer,
                BYTE *inputBuffer,
                UINT32 length,
                UINT8 *offset,
                BOOLEAN alreadyReadFirstBit
                )
{
    UINT8 consecutiveOnes;

    if(alreadyReadFirstBit) {
        consecutiveOnes = 1;
    } else {
        consecutiveOnes = 0;
    }

    while(bitlib_read_from_buffer( inFile, curBuffer, inputBuffer, length, offset, 1 )) {
        consecutiveOnes++;
    }

    return consecutiveOnes;
}
*/
