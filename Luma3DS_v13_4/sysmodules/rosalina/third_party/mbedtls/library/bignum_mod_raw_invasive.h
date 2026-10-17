   
                                  
  
                                                                   
                         
   
/**
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef MBEDTLS_BIGNUM_MOD_RAW_INVASIVE_H
#define MBEDTLS_BIGNUM_MOD_RAW_INVASIVE_H

#include "common.h"
#include "mbedtls/bignum.h"
#include "bignum_mod.h"

#if defined(MBEDTLS_TEST_HOOKS)

                                                                            
  
                                                                             
                                                                          
                                                
                                                   
   
MBEDTLS_STATIC_TESTABLE
void mbedtls_mpi_mod_raw_fix_quasi_reduction(mbedtls_mpi_uint *X,
                                             const mbedtls_mpi_mod_modulus *N);

#endif                         

#endif                                        
