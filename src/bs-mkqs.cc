
/*
 * Multikey quicksort algorithm.
 *
 *    ** J. Bentley and R. Sedgewick.
 *    Fast algorithms for sorting and searching strings. In Proceedings
 *    of 8th Annual ACM-SIAM Symposium on Discrete Algorithms, 1997.
 *
 *    http://www.cs.princeton.edu/~rs/strings/index.html
 *
 * The code presented in this file has been tested with care but is not
 * guaranteed for any purpose. The writer does not offer any warranties
 * nor does he accept any liabilities with respect to the code.
 *
 * Stefan Nilsson, 8 jan 1997.
 *
 * Laboratory of Information Processing Science
 * Helsinki University of Technology
 * Stefan.Nilsson@hut.fi
 *
 * Updated with std::swap and std::min by Timo Bingmann in 2012.
 */

/*
 * usage: bs-mkqs [-r] file [-o sorted]
 */

#include "main.h"
#include <string>

namespace bs_mkqs {

// ssort2 -- Faster Version of Multikey Quicksort

static void
vecswap2(char **a, char **b, int n)
{
   while (n-- > 0) {
      char *t = *a;
      *a++ = *b;
      *b++ = t;
   }
}

#define ptr2char(i) (*(*(i) + depth))

static char **
med3func(char **a, char **b, char **c, size_t depth)
{
   int va, vb, vc;

   if ((va = ptr2char(a)) == (vb = ptr2char(b)))
      return a;

   if ((vc = ptr2char(c)) == va || vc == vb)
      return c;

   return va < vb ?
        (vb < vc ? b : (va < vc ? c : a ) )
      : (vb > vc ? b : (va < vc ? a : c ) );
}

static void
inssort(char **a, size_t n, size_t d)
{
   char **pi, **pj, *s, *t;

   for (pi = a + 1; --n > 0; pi++) {
      for (pj = pi; pj > a; pj--) {
         for (s = *(pj - 1) + d, t = *pj + d; *s == *t && *s != 0; s++, t++) ;

         if (*s <= *t) break;

         t = *(pj); *(pj) = *(pj - 1);
         *(pj - 1) = t;
      }
   }
}

static void
ssort2(char **a, size_t n, size_t depth)
{
   int d, r, partval;
   char **pa, **pb, **pc, **pd, **pl, **pm, **pn, *t;

   if (n < 20) {
      inssort(a, n, depth);
      return;
   }

   pl = a;
   pm = a + (n/2);
   pn = a + (n-1);

   if (n > 30) { // On big arrays, pseudomedian of 9
      d = (n/8);
      pl = med3func(pl, pl+d, pl+2*d, depth);
      pm = med3func(pm-d, pm, pm+d, depth);
      pn = med3func(pn-2*d, pn-d, pn, depth);
   }

   pm = med3func(pl, pm, pn, depth);
   std::swap(*a, *pm);
   partval = ptr2char(a);
   pa = pb = a + 1;
   pc = pd = a + n-1;

   for (;;) {
      while (pb <= pc && (r = ptr2char(pb)-partval) <= 0) {
         if (r == 0) std::swap(*pa++, *pb);
         pb++;
      }
      while (pb <= pc && (r = ptr2char(pc)-partval) >= 0) {
         if (r == 0) std::swap(*pc, *pd--);
         pc--;
      }

      if (pb > pc) break;

      std::swap(*pb++, *pc--);
   }

   pn = a + n;
   r = std::min(pa-a, pb-pa);    vecswap2(a,  pb-r, r);
   r = std::min(pd-pc, pn-pd-1); vecswap2(pb, pn-r, r);

   if ((r = pb-pa) > 1)
      ssort2(a, r, depth);

   if (ptr2char(a + r) != 0)
      ssort2(a + r, pa-a + pn-pd-1, depth+1);

   if ((r = pd-pc) > 1)
      ssort2(a + n-r, r, depth);
}

#undef ptr2char

} // namespace bs_mkqs

void sort_main(char **a, size_t n)
{
   bs_mkqs::ssort2(a, n, 0);
}

