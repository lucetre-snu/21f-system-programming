//--------------------------------------------------------------------------------------------------
// System Programming                       Memory Lab                                   Fall 2021
//
/// @file
/// @brief null driver (empty malloc interfaces to measure overhead of framework)
/// @author Bernhard Egger <bernhard@csap.snu.ac.kr>
/// @section changelog Change Log
/// 2020/10/04 Bernhard Egger created
///
/// @section license_section License
/// Copyright (c) 2020-2021, Computer Systems and Platforms Laboratory, SNU
/// All rights reserved.
///
/// Redistribution and use in source and binary forms, with or without modification, are permitted
/// provided that the following conditions are met:
///
/// - Redistributions of source code must retain the above copyright notice, this list of condi-
///   tions and the following disclaimer.
/// - Redistributions in binary form must reproduce the above copyright notice, this list of condi-
///   tions and the following disclaimer in the documentation and/or other materials provided with
///   the distribution.
///
/// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
/// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED  TO,  THE IMPLIED WARRANTIES OF MERCHANTABILITY
/// AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
/// CONTRIBUTORS BE LIABLE FOR ANY DIRECT,  INDIRECT, INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR CONSE-
/// QUENTIAL DAMAGES  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
/// LOSS OF USE, DATA,  OR PROFITS; OR BUSINESS INTERRUPTION)  HOWEVER CAUSED AND ON ANY THEORY OF
/// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
/// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
/// DAMAGE.
//--------------------------------------------------------------------------------------------------

#ifndef __NULLDRIVER_H__
#define __NULLDRIVER_H__

#include <stdlib.h>

/// @brief null malloc. Returns (void*)-1
/// @param size size of payload
/// @retval (void*)-1
void* null_malloc(size_t size);


/// @brief null calloc. Returns (void*)-1
/// @param nmemb number of members
/// @param size size of one member
/// @retval (void*)-1
void* null_calloc(size_t nmemb, size_t size);


/// @brief null realloc. Returns (void*)-1
/// @param size size of payload
/// @retval (void*)-1
void* null_realloc(void *ptr, size_t size);


/// @brief null free. Does nothing.
/// @param ptr pointer to free
void null_free(void *ptr);


/// @brief null statistics. Returns 0 for both @a size and @a num_sbrk
/// @param size heap size. Set to 0 if not NULL
/// @param num_sbrk number of sbrk invocations. Set to -1 if not NULL
void null_stat(size_t *size, ssize_t *num_sbrk);

#endif // __NULLDRIVER_H__
