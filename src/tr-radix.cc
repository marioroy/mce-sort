
/*
 * Copyright 2007-2008 by Tommi Rantala <tt.rantala@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/* 
 * msd_ci() is an implementation of the MSD radix sort using
 *  - double sweep counting sort
 *  - O(n) oracle to reduce cache misses and memory stalls
 *  - the in-place distribution method described by McIlroy, Bostic & McIlroy
 */

/*
 * usage: tr-radix [-r] file [-o sorted]
 */

#include "main.h"

namespace rantala {

template <typename CharT>
inline bool is_end(CharT c);

template <> inline bool is_end(char c)
{
    return c==0;
}

static inline void
insertion_sort(char** strings, size_t n, size_t depth)
{
    for (char** i = strings + 1; --n > 0; ++i) {
        char** j = i;
        char* tmp = *i;
        while (j > strings) {
            char* s = *(j-1)+depth;
            char* t = tmp+depth;
            while (*s == *t and not is_end(*s)) {
                ++s;
                ++t;
            }
            if (*s <= *t) break;
            *j = *(j-1);
            --j;
        }
        *j = tmp;
    }
}

template <typename BucketType>
struct distblock {
    char* ptr;
    BucketType bucket;
};

template <typename BucketsizeType>
static void
msd_ci(char** strings, size_t n, size_t depth){
    if (n < 64) {
        insertion_sort(strings, n, depth);
        return;
    }
    BucketsizeType bucketsize[128] = {0};
    char* restrict oracle =
        (char*) malloc(n);
    for (size_t i=0; i < n; ++i)
        oracle[i] = strings[i][depth];
    for (size_t i=0; i < n; ++i)
        ++bucketsize[oracle[i]];
    static size_t bucketindex[128];
    bucketindex[0] = bucketsize[0];
    BucketsizeType last_bucket_size = bucketsize[0];
    for (unsigned i=1; i < 128; ++i) {
        bucketindex[i] = bucketindex[i-1] + bucketsize[i];
        if (bucketsize[i]) last_bucket_size = bucketsize[i];
    }
    for (size_t i=0; i < n-last_bucket_size; ) {
        distblock<int8_t> tmp = { strings[i], oracle[i] };
        while (1) {
            // Continue until the current bucket is completely in
            // place
            if (--bucketindex[tmp.bucket] <= i)
                break;
            // backup all information of the position we are about
            // to overwrite
            size_t backup_idx = bucketindex[tmp.bucket];
            distblock<int8_t> tmp2 = { strings[backup_idx], oracle[backup_idx] };
            // overwrite everything, ie. move the string to correct
            // position
            strings[backup_idx] = tmp.ptr;
            oracle[backup_idx]  = tmp.bucket;
            tmp = tmp2;
        }
        // Commit last pointer to place. We don't need to copy the
        // oracle entry, it's not read after this.
        strings[i] = tmp.ptr;
        i += bucketsize[tmp.bucket];
    }
    free(oracle);
    size_t bsum = bucketsize[0];
    for (size_t i=1; i < 128; ++i) {
        if (bucketsize[i] == 0) continue;
        msd_ci<BucketsizeType>(strings+bsum, bucketsize[i], depth+1);
        bsum += bucketsize[i];
    }
}

void msd_ci(char** strings, size_t n, size_t depth)
{ msd_ci<size_t>(strings, n, depth); }

} // namespace rantala

void sort_main(char **a, size_t n, int mce_flag)
{
   if (mce_flag)
      rantala::msd_ci(a, n, 1);
   else
      rantala::msd_ci(a, n, 0);
}

