/*
 *  Hamlib Interface - toolbox header
 *  Copyright (c) 2000-2004 by Stephane Fillod
 *
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
/* SPDX-License-Identifier: LGPL-2.1-or-later */

#ifndef _MISC_H
#define _MISC_H 1

#include "hamlib/rig.h"
#include "hamlib/config.h"


/*
 */
#ifdef HAVE_PTHREAD
#include <pthread.h>
#define set_transaction_active(rig) {pthread_mutex_lock(&STATE(rig)->mutex_set_transaction);STATE(rig)->transaction_active = 1;}
#define set_transaction_inactive(rig) {STATE(rig)->transaction_active = 0;pthread_mutex_unlock(&STATE(rig)->mutex_set_transaction);}
#else
#define set_transaction_active(rig) {STATE(rig)->transaction_active = 1;}
#define set_transaction_inactive(rig) {STATE(rig)->transaction_active = 0;}
#endif

__BEGIN_DECLS

// a function to return just a string of spaces for indenting rig debug lines
HAMLIB_EXPORT (const char *) spaces(int len);
/*
 * Do a hex dump of the unsigned char array.
 */

void dump_hex(const unsigned char ptr[], size_t size);

/*
 * BCD conversion routines.
 *
 * to_bcd() converts a long long int to a little endian BCD array,
 *  and return a pointer to this array.
 *
 * from_bcd() converts a little endian BCD array to long long int
 *  representation, and return it.
 *
 * bcd_len is the number of digits in the BCD array.
 */
extern HAMLIB_EXPORT(unsigned char *) to_bcd(unsigned char bcd_data[],
                                             unsigned long long freq,
                                             unsigned bcd_len);

extern HAMLIB_EXPORT(unsigned long long) from_bcd(const unsigned char
                                                  bcd_data[],
                                                  unsigned bcd_len);

/*
 * same as to_bcd() and from_bcd(), but in Big Endian mode
 */
extern HAMLIB_EXPORT(unsigned char *) to_bcd_be(unsigned char bcd_data[],
                                                unsigned long long freq,
                                                unsigned bcd_len);

extern HAMLIB_EXPORT(unsigned long long) from_bcd_be(const unsigned char
                                                     bcd_data[],
                                                     unsigned bcd_len);

extern HAMLIB_EXPORT(size_t) to_hex(size_t source_length,
                                    const unsigned char *source_data,
                                    size_t dest_length,
                                    char *dest_data);

extern HAMLIB_EXPORT(double) morse_code_dot_to_millis(int wpm);
extern HAMLIB_EXPORT(int) dot10ths_to_millis(int dot10ths, int wpm);
extern HAMLIB_EXPORT(int) millis_to_dot10ths(int millis, int wpm);

extern HAMLIB_EXPORT(int) sprintf_freq(char *str, int str_len, freq_t);

/* flag that determines if AI mode should be restored on exit on
   applicable rigs - See rig_no_restore_ai() */
extern int no_restore_ai;

/* check if it's any of CR or LF */
#define isreturn(c) ((c) == 10 || (c) == 13)


/* needs config.h included beforehand in .c file */
#ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
#endif

#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif

extern HAMLIB_EXPORT(int) rig_check_cache_timeout(const struct timeval *tv,
                                                  int timeout);

extern HAMLIB_EXPORT(void) rig_force_cache_timeout(struct timeval *tv);

extern HAMLIB_EXPORT(setting_t) rig_idx2setting(int i);

extern HAMLIB_EXPORT(int) hl_usleep(rig_useconds_t usec);

extern HAMLIB_EXPORT(double) elapsed_ms(struct timespec *start, int start_flag);

extern HAMLIB_EXPORT(vfo_t) vfo_fixup(RIG *rig, vfo_t vfo, split_t split);
extern HAMLIB_EXPORT(vfo_t) vfo_fixup2a(RIG *rig, vfo_t vfo, split_t split, const char *func, const int line);
#define vfo_fixup(r,v,s) vfo_fixup2a(r,v,s,__func__,__LINE__)

extern HAMLIB_EXPORT(int) parse_hoststr(char *hoststr, int hoststr_len, char host[256], char port[6]);

extern HAMLIB_EXPORT(uint32_t) CRC32_function(const uint8_t *buf, uint32_t len);

extern HAMLIB_EXPORT(char *)date_strget(char *buf, int buflen, int localtime);

#ifdef PRId64
/** \brief printf(3) format to be used for long long (64bits) type */
#  define PRIll PRId64
#  define PRXll PRIx64
#else
#  ifdef FBSD4
#    define PRIll "qd"
#    define PRXll "qx"
#  else
#    define PRIll "lld"
#    define PRXll "lld"
#  endif
#endif

#ifdef SCNd64
/** \brief scanf(3) format to be used for long long (64bits) type */
#  define SCNll SCNd64
#  define SCNXll SCNx64
#else
#  ifdef FBSD4
#    define SCNll "qd"
#    define SCNXll "qx"
#  else
#    define SCNll "lld"
#    define SCNXll "llx"
#  endif
#endif

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
void errmsg(int err, char *s, const char *func, const char *file, int line);
#define ERRMSG(err, s) errmsg(err,  s, __func__, __FILENAME__, __LINE__)
#define ENTERFUNC {     ++STATE(rig)->depth;				\
    rig_debug(RIG_DEBUG_VERBOSE, "%s%d:%s(%d):%s entered\n", spaces(STATE(rig)->depth), STATE(rig)->depth, __FILENAME__, __LINE__, __func__); \
                  }
#define ENTERFUNC2 {    rig_debug(RIG_DEBUG_VERBOSE, "%s(%d):%s entered\n", __FILENAME__, __LINE__, __func__); \
                   }
// we need to refer to rc just once as it 
// could be a function call 
#define RETURNFUNC(rc) {do { \
            int rctmp = rc; \
            rig_debug(RIG_DEBUG_VERBOSE, "%s%d:%s(%d):%s returning(%ld) %s\n", spaces(STATE(rig)->depth), STATE(rig)->depth, __FILENAME__, __LINE__, __func__, (long int) (rctmp), rctmp<0?rigerror2(rctmp):""); \
            --STATE(rig)->depth;					\
            return (rctmp); \
            } while(0);}
#define RETURNFUNC2(rc) {do { \
            int rctmp = rc; \
            rig_debug(RIG_DEBUG_VERBOSE, "%s(%d):%s returning2(%ld) %s\n",  __FILENAME__, __LINE__, __func__, (long int) (rctmp), rctmp<0?rigerror2(rctmp):""); \
            return (rctmp); \
            } while(0);}

#define CACHE_RESET {\
    elapsed_ms(&CACHE(rig)->time_freqMainA, HAMLIB_ELAPSED_INVALIDATE);\
    elapsed_ms(&CACHE(rig)->time_freqMainB, HAMLIB_ELAPSED_INVALIDATE);\
    elapsed_ms(&CACHE(rig)->time_freqSubA, HAMLIB_ELAPSED_INVALIDATE);\
    elapsed_ms(&CACHE(rig)->time_freqSubB, HAMLIB_ELAPSED_INVALIDATE);\
    elapsed_ms(&CACHE(rig)->time_vfo, HAMLIB_ELAPSED_INVALIDATE);\
    elapsed_ms(&CACHE(rig)->time_modeMainA, HAMLIB_ELAPSED_INVALIDATE);\
    elapsed_ms(&CACHE(rig)->time_modeMainB, HAMLIB_ELAPSED_INVALIDATE);\
    elapsed_ms(&CACHE(rig)->time_modeMainC, HAMLIB_ELAPSED_INVALIDATE);\
    elapsed_ms(&CACHE(rig)->time_modeSubA, HAMLIB_ELAPSED_INVALIDATE);\
    elapsed_ms(&CACHE(rig)->time_modeSubB, HAMLIB_ELAPSED_INVALIDATE);\
    elapsed_ms(&CACHE(rig)->time_modeSubC, HAMLIB_ELAPSED_INVALIDATE);\
    elapsed_ms(&CACHE(rig)->time_widthMainA, HAMLIB_ELAPSED_INVALIDATE);\
    elapsed_ms(&CACHE(rig)->time_widthMainB, HAMLIB_ELAPSED_INVALIDATE);\
    elapsed_ms(&CACHE(rig)->time_widthMainC, HAMLIB_ELAPSED_INVALIDATE);\
    elapsed_ms(&CACHE(rig)->time_widthSubA, HAMLIB_ELAPSED_INVALIDATE);\
    elapsed_ms(&CACHE(rig)->time_widthSubB, HAMLIB_ELAPSED_INVALIDATE);\
    elapsed_ms(&CACHE(rig)->time_widthSubC, HAMLIB_ELAPSED_INVALIDATE);\
    elapsed_ms(&CACHE(rig)->time_ptt, HAMLIB_ELAPSED_INVALIDATE);\
    elapsed_ms(&CACHE(rig)->time_split, HAMLIB_ELAPSED_INVALIDATE);\
     }


typedef enum settings_value_e
{
    e_CHAR, e_INT, e_LONG, e_FLOAT, e_DOUBLE
} settings_value_t;


extern HAMLIB_EXPORT(int) rig_settings_save(const char *setting, void *value, settings_value_t valuet);
extern HAMLIB_EXPORT(int) rig_settings_load(char *setting, void *value, settings_value_t valuet);
extern HAMLIB_EXPORT(int) rig_settings_load_all(char *settings_file);

extern int check_level_param(RIG *rig, setting_t level, value_t val, gran_t **gran);

extern int queue_deferred_config(deferred_config_header_t *head, hamlib_token_t token, const char *val);

// Takes rig-specific band result and maps it the bandlist int the rig's backend
extern HAMLIB_EXPORT(hamlib_band_t) rig_get_band(RIG *rig, freq_t freq, int band);
extern HAMLIB_EXPORT(const char*) rig_get_band_str(RIG *rig, hamlib_band_t band, int which);
extern HAMLIB_EXPORT(int) rig_get_band_rig(RIG *rig, freq_t freq, const char *band);

extern HAMLIB_EXPORT(int) rig_test_2038(RIG *rig);

__END_DECLS

#endif /* _MISC_H */
