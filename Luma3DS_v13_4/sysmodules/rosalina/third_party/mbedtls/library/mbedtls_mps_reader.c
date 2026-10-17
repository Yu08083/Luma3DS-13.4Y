/*
 *  Message Processing Stack, Reader implementation
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#include "common.h"

#if defined(MBEDTLS_SSL_PROTO_TLS1_3)

#include "mps_reader.h"
#include "mps_common.h"
#include "mps_trace.h"

#include <string.h>

#if defined(MBEDTLS_MPS_ENABLE_TRACE)
static int mbedtls_mps_trace_id = MBEDTLS_MPS_TRACE_BIT_READER;
#endif                               

  
                               
  
                                                          
                                                             
                                                         
                                                         
                                                      
  
                                  
                                                      
                                    
                                                 
                                                       
                                                    
                      
                                                           
                                                  
                                                   
                                                  
                                                           
                                                               
                                                         
                                   
                                                       
                                                      
                                       
                                                       
                                                         
                                    
  
   

static inline int mps_reader_is_accumulating(
    mbedtls_mps_reader const *rd)
{
    mbedtls_mps_size_t acc_remaining;
    if (rd->acc == NULL) {
        return 0;
    }

    acc_remaining = rd->acc_share.acc_remaining;
    return acc_remaining > 0;
}

static inline int mps_reader_is_producing(
    mbedtls_mps_reader const *rd)
{
    unsigned char *frag = rd->frag;
    return frag == NULL;
}

static inline int mps_reader_is_consuming(
    mbedtls_mps_reader const *rd)
{
    return !mps_reader_is_producing(rd);
}

static inline mbedtls_mps_size_t mps_reader_get_fragment_offset(
    mbedtls_mps_reader const *rd)
{
    unsigned char *acc = rd->acc;
    mbedtls_mps_size_t frag_offset;

    if (acc == NULL) {
        return 0;
    }

    frag_offset = rd->acc_share.frag_offset;
    return frag_offset;
}

static inline mbedtls_mps_size_t mps_reader_serving_from_accumulator(
    mbedtls_mps_reader const *rd)
{
    mbedtls_mps_size_t frag_offset, end;

    frag_offset = mps_reader_get_fragment_offset(rd);
    end = rd->end;

    return end < frag_offset;
}

static inline void mps_reader_zero(mbedtls_mps_reader *rd)
{
                                                       
                                                     
                                                               
                                                      
                                                    
                                                   
    static mbedtls_mps_reader const zero =
    { .frag          = NULL,
      .frag_len      = 0,
      .commit        = 0,
      .end           = 0,
      .pending       = 0,
      .acc           = NULL,
      .acc_len       = 0,
      .acc_available = 0,
      .acc_share     = { .acc_remaining = 0 } };
    *rd = zero;
}

int mbedtls_mps_reader_init(mbedtls_mps_reader *rd,
                            unsigned char *acc,
                            mbedtls_mps_size_t acc_len)
{
    MBEDTLS_MPS_TRACE_INIT("mbedtls_mps_reader_init");
    MBEDTLS_MPS_TRACE(MBEDTLS_MPS_TRACE_TYPE_COMMENT,
                      "* Accumulator size: %u bytes", (unsigned) acc_len);
    mps_reader_zero(rd);
    rd->acc = acc;
    rd->acc_len = acc_len;
    MBEDTLS_MPS_TRACE_RETURN(0);
}

int mbedtls_mps_reader_free(mbedtls_mps_reader *rd)
{
    MBEDTLS_MPS_TRACE_INIT("mbedtls_mps_reader_free");
    mps_reader_zero(rd);
    MBEDTLS_MPS_TRACE_RETURN(0);
}

int mbedtls_mps_reader_feed(mbedtls_mps_reader *rd,
                            unsigned char *new_frag,
                            mbedtls_mps_size_t new_frag_len)
{
    mbedtls_mps_size_t copy_to_acc;
    MBEDTLS_MPS_TRACE_INIT("mbedtls_mps_reader_feed");
    MBEDTLS_MPS_TRACE(MBEDTLS_MPS_TRACE_TYPE_COMMENT,
                      "* Fragment length: %u bytes", (unsigned) new_frag_len);

    if (new_frag == NULL) {
        MBEDTLS_MPS_TRACE_RETURN(MBEDTLS_ERR_MPS_READER_INVALID_ARG);
    }

    MBEDTLS_MPS_STATE_VALIDATE_RAW(mps_reader_is_producing(
                                       rd),
                                   "mbedtls_mps_reader_feed() requires reader to be in producing mode");

    if (mps_reader_is_accumulating(rd)) {
        unsigned char *acc    = rd->acc;
        mbedtls_mps_size_t acc_remaining = rd->acc_share.acc_remaining;
        mbedtls_mps_size_t acc_available = rd->acc_available;

                                                                               
        acc += acc_available;

        copy_to_acc = acc_remaining;
        if (copy_to_acc > new_frag_len) {
            copy_to_acc = new_frag_len;
        }

                                               
        memcpy(acc, new_frag, copy_to_acc);

        MBEDTLS_MPS_TRACE(MBEDTLS_MPS_TRACE_TYPE_COMMENT,
                          "Copy new data of size %u of %u into accumulator at offset %u",
                          (unsigned) copy_to_acc, (unsigned) new_frag_len,
                          (unsigned) acc_available);

                                                                   
        acc_remaining -= copy_to_acc;
        if (acc_remaining > 0) {
                                                                          
            acc_available += copy_to_acc;
            rd->acc_share.acc_remaining = acc_remaining;
            rd->acc_available = acc_available;
            MBEDTLS_MPS_TRACE_RETURN(MBEDTLS_ERR_MPS_READER_NEED_MORE);
        }

                                                                     

        MBEDTLS_MPS_TRACE(MBEDTLS_MPS_TRACE_TYPE_COMMENT,
                          "Enough data available to serve user request");

                                                           
        rd->acc_share.frag_offset = acc_available;
        acc_available += copy_to_acc;
        rd->acc_available = acc_available;
    } else {                       
        rd->acc_share.frag_offset = 0;
    }

    rd->frag = new_frag;
    rd->frag_len = new_frag_len;
    rd->commit = 0;
    rd->end = 0;
    MBEDTLS_MPS_TRACE_RETURN(0);
}


int mbedtls_mps_reader_get(mbedtls_mps_reader *rd,
                           mbedtls_mps_size_t desired,
                           unsigned char **buffer,
                           mbedtls_mps_size_t *buflen)
{
    unsigned char *frag;
    mbedtls_mps_size_t frag_len, frag_offset, end, frag_fetched, frag_remaining;
    MBEDTLS_MPS_TRACE_INIT("mbedtls_mps_reader_get");
    MBEDTLS_MPS_TRACE(MBEDTLS_MPS_TRACE_TYPE_COMMENT,
                      "* Bytes requested: %u", (unsigned) desired);

    MBEDTLS_MPS_STATE_VALIDATE_RAW(mps_reader_is_consuming(
                                       rd),
                                   "mbedtls_mps_reader_get() requires reader to be in consuming mode");

    end = rd->end;
    frag_offset = mps_reader_get_fragment_offset(rd);

                                                            
    if (mps_reader_serving_from_accumulator(rd)) {
                                                           
          
                       
          
                                                                         
                                                                         
                                                                         
          
                                      
                                 
                                               
                                               
                                               
                                               
                                                         
          
                       
          
                                                                         
                                                                         
                                                                         
          
                                                    
                                               
                                               
                                               
                                               
                                               
                                                      
          
                                                                            
          
                                                                     
                                                                     
                                                                     
          
                                              
                                        
                                               
                                               
                                               
                                               
                                                     
          
          
                                                                       
          
                                                                     
                                                                     
                                                                     
          
                                                         
                                                   
                                                   
                                               
                                               
                                               
                                                     
          
                                                              
                                                                          
           

        unsigned char *acc;

        MBEDTLS_MPS_TRACE(MBEDTLS_MPS_TRACE_TYPE_COMMENT,
                          "Serve the request from the accumulator");
        if (frag_offset - end < desired) {
            mbedtls_mps_size_t acc_available;
            acc_available = rd->acc_available;
            if (acc_available - end != desired) {
                                                                            
                                                                             
                                                          
                                                                           
                                                                               
                                                                                
                                                                             
                                                                              
                                                                   
                                    
                MBEDTLS_MPS_TRACE_RETURN(
                    MBEDTLS_ERR_MPS_READER_INCONSISTENT_REQUESTS);
            }
        }

        acc = rd->acc;
        acc += end;

        *buffer = acc;
        if (buflen != NULL) {
            *buflen = desired;
        }

        end += desired;
        rd->end = end;
        rd->pending = 0;

        MBEDTLS_MPS_TRACE_RETURN(0);
    }

                                                                
    MBEDTLS_MPS_TRACE(MBEDTLS_MPS_TRACE_TYPE_COMMENT,
                      "Serve the request from the current fragment.");

    frag_len = rd->frag_len;
    frag_fetched = end - frag_offset;                                       
                                                                              
                                                        
    frag_remaining = frag_len - frag_fetched;                                 

                                                                   
    if (frag_remaining < desired) {
        MBEDTLS_MPS_TRACE(MBEDTLS_MPS_TRACE_TYPE_COMMENT,
                          "There's not enough data in the current fragment "
                          "to serve the request.");
                                                           
                                                         
        if (buflen == NULL) {
            if (frag_remaining > 0) {
                rd->pending = desired - frag_remaining;
                MBEDTLS_MPS_TRACE(MBEDTLS_MPS_TRACE_TYPE_COMMENT,
                                  "Remember to collect %u bytes before re-opening",
                                  (unsigned) rd->pending);
            }
            MBEDTLS_MPS_TRACE_RETURN(MBEDTLS_ERR_MPS_READER_OUT_OF_DATA);
        }

        desired = frag_remaining;
    }

                                                               
                                              

    frag = rd->frag;
    frag += frag_fetched;

    *buffer = frag;
    if (buflen != NULL) {
        *buflen = desired;
    }

    end += desired;
    rd->end = end;
    rd->pending = 0;
    MBEDTLS_MPS_TRACE_RETURN(0);
}

int mbedtls_mps_reader_commit(mbedtls_mps_reader *rd)
{
    mbedtls_mps_size_t end;
    MBEDTLS_MPS_TRACE_INIT("mbedtls_mps_reader_commit");
    MBEDTLS_MPS_STATE_VALIDATE_RAW(mps_reader_is_consuming(
                                       rd),
                                   "mbedtls_mps_reader_commit() requires reader to be in consuming mode");

    end = rd->end;
    rd->commit = end;

    MBEDTLS_MPS_TRACE_RETURN(0);
}

int mbedtls_mps_reader_reclaim(mbedtls_mps_reader *rd,
                               int *paused)
{
    unsigned char *frag, *acc;
    mbedtls_mps_size_t pending, commit;
    mbedtls_mps_size_t acc_len, frag_offset, frag_len;
    MBEDTLS_MPS_TRACE_INIT("mbedtls_mps_reader_reclaim");

    if (paused != NULL) {
        *paused = 0;
    }

    MBEDTLS_MPS_STATE_VALIDATE_RAW(mps_reader_is_consuming(
                                       rd),
                                   "mbedtls_mps_reader_reclaim() requires reader to be in consuming mode");

    frag     = rd->frag;
    acc      = rd->acc;
    pending  = rd->pending;
    commit   = rd->commit;
    frag_len = rd->frag_len;

    frag_offset = mps_reader_get_fragment_offset(rd);

    if (pending == 0) {
        MBEDTLS_MPS_TRACE(MBEDTLS_MPS_TRACE_TYPE_COMMENT,
                          "No unsatisfied read-request has been logged.");

                                                        
        if (commit < frag_offset || commit - frag_offset < frag_len) {
            MBEDTLS_MPS_TRACE(MBEDTLS_MPS_TRACE_TYPE_COMMENT,
                              "There is data left to be consumed.");
            rd->end = commit;
            MBEDTLS_MPS_TRACE_RETURN(MBEDTLS_ERR_MPS_READER_DATA_LEFT);
        }

        rd->acc_available = 0;
        rd->acc_share.acc_remaining = 0;

        MBEDTLS_MPS_TRACE(MBEDTLS_MPS_TRACE_TYPE_COMMENT,
                          "Fragment has been fully processed and committed.");
    } else {
        int overflow;

        mbedtls_mps_size_t acc_backup_offset;
        mbedtls_mps_size_t acc_backup_len;
        mbedtls_mps_size_t frag_backup_offset;
        mbedtls_mps_size_t frag_backup_len;

        mbedtls_mps_size_t backup_len;
        mbedtls_mps_size_t acc_len_needed;

        MBEDTLS_MPS_TRACE(MBEDTLS_MPS_TRACE_TYPE_COMMENT,
                          "There has been an unsatisfied read with %u bytes overhead.",
                          (unsigned) pending);

        if (acc == NULL) {
            MBEDTLS_MPS_TRACE(MBEDTLS_MPS_TRACE_TYPE_COMMENT,
                              "No accumulator present");
            MBEDTLS_MPS_TRACE_RETURN(
                MBEDTLS_ERR_MPS_READER_NEED_ACCUMULATOR);
        }
        acc_len = rd->acc_len;

                                                       
                                                            
        if (commit < frag_offset) {
                                                           
            frag_backup_offset = 0;
            frag_backup_len = frag_len;
            acc_backup_offset = commit;
            acc_backup_len = frag_offset - commit;
        } else {
                                                            
            frag_backup_offset = commit - frag_offset;
            frag_backup_len = frag_len - frag_backup_offset;
            acc_backup_offset = 0;
            acc_backup_len = 0;
        }

        backup_len = acc_backup_len + frag_backup_len;
        acc_len_needed = backup_len + pending;

        overflow  = 0;
        overflow |= (backup_len     < acc_backup_len);
        overflow |= (acc_len_needed < backup_len);

        if (overflow || acc_len < acc_len_needed) {
                                                                    
                                                                   
                                        
            rd->end = commit;
            rd->pending = 0;
            MBEDTLS_MPS_TRACE(MBEDTLS_MPS_TRACE_TYPE_ERROR,
                              "The accumulator is too small to handle the backup.");
            MBEDTLS_MPS_TRACE(MBEDTLS_MPS_TRACE_TYPE_ERROR,
                              "* Size: %u", (unsigned) acc_len);
            MBEDTLS_MPS_TRACE(MBEDTLS_MPS_TRACE_TYPE_ERROR,
                              "* Needed: %u (%u + %u)",
                              (unsigned) acc_len_needed,
                              (unsigned) backup_len, (unsigned) pending);
            MBEDTLS_MPS_TRACE_RETURN(
                MBEDTLS_ERR_MPS_READER_ACCUMULATOR_TOO_SMALL);
        }

        MBEDTLS_MPS_TRACE(MBEDTLS_MPS_TRACE_TYPE_COMMENT,
                          "Fragment backup: %u", (unsigned) frag_backup_len);
        MBEDTLS_MPS_TRACE(MBEDTLS_MPS_TRACE_TYPE_COMMENT,
                          "Accumulator backup: %u", (unsigned) acc_backup_len);

                                                                   
                                 
        memmove(acc, acc + acc_backup_offset, acc_backup_len);

                                                                
                          
        memcpy(acc + acc_backup_len,
               frag + frag_backup_offset, frag_backup_len);

        rd->acc_available = backup_len;
        rd->acc_share.acc_remaining = pending;

        if (paused != NULL) {
            *paused = 1;
        }
    }

    rd->frag     = NULL;
    rd->frag_len = 0;

    rd->commit  = 0;
    rd->end     = 0;
    rd->pending = 0;

    MBEDTLS_MPS_TRACE(MBEDTLS_MPS_TRACE_TYPE_COMMENT,
                      "Final state: aa %u, al %u, ar %u",
                      (unsigned) rd->acc_available, (unsigned) rd->acc_len,
                      (unsigned) rd->acc_share.acc_remaining);
    MBEDTLS_MPS_TRACE_RETURN(0);
}

#endif                               
