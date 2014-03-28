
/******************************************************************************
 * src/tb-radix_sort.h
 *
 * Experiments with sequential radix sort implementations.
 * Based on rantala/msd_c?.h
 *
 ******************************************************************************
 * Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

/*
 * usage: tb-radix [-r] file [-o sorted]
 */

#include "main.h"
#include <string>

namespace tb_radix {

static const size_t g_inssort_threshold = 64;

typedef char* string;

template <typename CharT>
inline CharT get_char(string ptr, size_t depth);

template <>
inline uint16_t get_char<uint16_t>(string str, size_t depth)
{
    uint16_t v = 0;
    if (str[depth] == 0) return v;
    v |= (uint16_t(str[depth]) << 8); ++str;
    v |= (uint16_t(str[depth]) << 0);
    return v;
}

static inline void
inssort(string* str, size_t n, size_t d)
{
    string *pj, s, t;

    for (string* pi = str + 1; --n > 0; pi++) {
        string tmp = *pi;

        for (pj = pi; pj > str; pj--) {
            for (s = *(pj-1)+d, t = tmp+d; *s == *t && *s != 0; ++s, ++t)
                ;
            if (*s <= *t)
                break;
            *pj = *(pj-1);
        }
        *pj = tmp;
    }
}

static inline size_t*
msd_CI5_bktsize(string* strings, size_t n, size_t depth)
{
    // cache characters
    uint8_t* charcache = new uint8_t[n];
    for (size_t i=0; i < n; ++i)
        charcache[i] = strings[i][depth];

    // count character occurances
    size_t* bktsize = new size_t[128];
    memset(bktsize, 0, 128 * sizeof(size_t));
    for (size_t i=0; i < n; ++i)
        ++bktsize[ charcache[i] ];

    // inclusive prefix sum
    size_t bkt[128];
    bkt[0] = bktsize[0];
    size_t last_bkt_size = bktsize[0];
    for (unsigned i=1; i < 128; ++i) {
        bkt[i] = bkt[i-1] + bktsize[i];
        if (bktsize[i]) last_bkt_size = bktsize[i];
    }

    // premute in-place
    for (size_t i=0, j; i < n-last_bkt_size; )
    {
        string perm = strings[i];
        uint8_t permch = charcache[i];
        while ( (j = --bkt[ permch ]) > i )
        {
            std::swap(perm, strings[j]);
            std::swap(permch, charcache[j]);
        }
        strings[i] = perm;
        i += bktsize[ permch ];
    }

    delete [] charcache;

    return bktsize;
}

void
msd_CI5(string* strings, size_t n, size_t depth)
{
    if (n < g_inssort_threshold)
        return inssort(strings, n, depth);

    size_t* bktsize = msd_CI5_bktsize(strings, n, depth);

    // recursion
    size_t bsum = bktsize[0];
    for (size_t i=1; i < 128; ++i) {
        if (bktsize[i] == 0) continue;
        msd_CI5(strings+bsum, bktsize[i], depth+1);
        bsum += bktsize[i];
    }

    delete [] bktsize;
}

static inline size_t*
msd_CI5_16bit_bktsize(string* strings, size_t n, size_t depth)
{
    static const size_t RADIX = 0x10000;

    // cache characters
    uint16_t* charcache = new uint16_t[n];
    for (size_t i=0; i < n; ++i)
        charcache[i] = get_char<uint16_t>(strings[i], depth);

    // count character occurances
    size_t* bktsize = new size_t[RADIX];
    memset(bktsize, 0, RADIX * sizeof(size_t));
    for (size_t i=0; i < n; ++i)
        ++bktsize[ charcache[i] ];

    // inclusive prefix sum
    size_t bkt[RADIX];
    bkt[0] = bktsize[0];
    size_t last_bkt_size = bktsize[0];
    for (unsigned i=1; i < RADIX; ++i) {
        bkt[i] = bkt[i-1] + bktsize[i];
        if (bktsize[i]) last_bkt_size = bktsize[i];
    }

    // premute in-place
    for (size_t i=0, j; i < n-last_bkt_size; )
    {
        string perm = strings[i];
        uint16_t permch = charcache[i];
        while ( (j = --bkt[ permch ]) > i )
        {
            std::swap(perm, strings[j]);
            std::swap(permch, charcache[j]);
        }
        strings[i] = perm;
        i += bktsize[ permch ];
    }

    delete [] charcache;

    return bktsize;
}

static void
msd_CI5_16bit(string* strings, size_t n, size_t depth)
{
    if (n < 0x10000)
        return msd_CI5(strings, n, depth);

    size_t* bktsize = msd_CI5_16bit_bktsize(strings, n, depth);

    // recursion
    size_t bsum = bktsize[0];
    for (size_t i=1; i < 0x10000; ++i) {
        if (bktsize[i] == 0) continue;
        if (i & 0xFF) // not zero-terminated
            msd_CI5_16bit(strings+bsum, bktsize[i], depth+2);
        bsum += bktsize[i];
    }

    delete [] bktsize;
}

} // namespace tb_radix

void sort_main(char **a, size_t n, int mce_flag)
{
   if (mce_flag)
      tb_radix::msd_CI5_16bit(a, n, 1);
   else
      tb_radix::msd_CI5_16bit(a, n, 0);
}

