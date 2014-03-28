
/*
 * Mergesort algorithm.
 *
 * The code presented in this file has been tested with care but is not
 * guaranteed for any purpose. The writer does not offer any warranties
 * nor does he accept any liabilities with respect to the code.
 *
 * Mario Roy, 03/15/2014
 *
 * usage: mr-merge [-r] file [-o sorted]
 */

#include "main.h"

namespace mr_merge {

size_t depth = 0;

static inline void
inssort(char **str, size_t n, size_t d)
{
    char **pj, *s, *t;

    for (char **pi = str + 1; --n > 0; pi++) {
        char *tmp = *pi;

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

static void
xmerge(char **a, char **b, size_t L, size_t N, size_t M)
{
   size_t i, j, k, r; char *s, *t;

   i = L;  j = i + N;  r = j + M;
   M += L + N;  N += L;

   for (k = L; k <= r; k++) {
      if (i == N) { a[k] = b[j++]; continue; }
      if (j == M) { a[k] = b[i++]; continue; }

      s = b[i] + depth;  t = b[j] + depth;
      for (; *s == *t && *s != 0; s++, t++) ;

      a[k] = (*s < *t) ? b[i++] : b[j++];
   }
}

static void
xmergesort(char **a, char **b, size_t L, size_t R)
{
   size_t m;

   if (R - L <= 64) {
      inssort(a + L, R - L + 1, depth);
      return;
   }

   m = (R + L) / 2;

   xmergesort(b, a, L, m);
   xmergesort(b, a, m + 1, R);

   xmerge(a, b, L, m - L + 1, R - m);
}

static void
mergesort(char **a, size_t n)
{
   char **aux;

   if ((aux = (char **)malloc(sizeof(size_t) * (n + 1))) == NULL) {
      fprintf(stderr, "Could not allocate aux ptr array\n");
      exit(1);
   }

   memcpy(aux, a, sizeof(size_t) * n);
   xmergesort(a, aux, 0, n - 1);

   free((void*) aux);
}

} // namespace mr_merge

void sort_main(char **a, size_t n, int mce_flag)
{
   if (mce_flag) mr_merge::depth = 1;
   mr_merge::mergesort(a, n);
}

