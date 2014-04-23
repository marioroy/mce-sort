
/*
 * Contains the main function. Also helper functions for fast IO.
 *
 * The code presented in this file has been tested with care but is not
 * guaranteed for any purpose. The writer does not offer any warranties
 * nor does he accept any liabilities with respect to the code.
 *
 * Mario Roy, 04/22/2014
 *
 * usage: binary [-r] file [-o sorted]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>

#include <ctime>

extern void sort_main(char **a, size_t n);

#define STOPWATCH_BEGIN() std::clock()
#define STOPWATCH_END(start) (std::clock() - start) / (double) CLOCKS_PER_SEC;

#define ALLOC_SIZE 8388608   // 8192K
char output_buf[524288];     //  512K

// ############################################################################

static char **
create_pointer_array(char *name, char *space, size_t size, size_t *np)
{
   size_t asize = ALLOC_SIZE, i, j, n = *np;
   char *s, **a;

   if ((a = (char **)malloc(sizeof(char **) * asize)) == NULL) {
      fprintf(stderr, "%s: Could not allocate ptr array\n", name);
      free((void*)space); exit(1);
   }

   s = space; a[0] = s;

   for (i = 0, j = 0; i < size; i++) {
      if (*s == '\n') {
         if (++j == ALLOC_SIZE) {
            asize += ALLOC_SIZE; j = 0;

            if ((a = (char **)realloc(a, sizeof(char **) * asize)) == NULL) {
               fprintf(stderr, "%s: Could not reallocate ptr array\n", name);
               free((void*)space); free((void*)a); exit(1);
            }
         }
         *s++ = '\0'; a[++n] = s;

      } else {
         s++;
      }
   }

   *np = n;
   return a;
}

// ############################################################################

static void
output_ascending(char *name, int fd, char **a, size_t n)
{
   size_t i, j, size;
   char *s, *t = output_buf;

   for (i = 0, j = 0; i < n; i++) {
      s = a[i];  while (*s) { *t++ = *s++; }  *t++ = '\n';

      if (i % 2048 == 0) {
         j = t - output_buf;
         if ((size = write(fd, output_buf, j)) != j) {
            fprintf(stderr, "%s: Could not write to output stream\n", name);
            j = 0; break;
         }
         t = output_buf;
      }
   }

   j = t - output_buf;

   if (j > 0) {
      if ((size = write(fd, output_buf, j)) != j)
         fprintf(stderr, "%s: Could not write to output stream\n", name);
   }
}

static void
output_descending(char *name, int fd, char **a, size_t n)
{
   size_t i, j, size;
   char *s, *t = output_buf;

   for (i = n - 1, j = 0; i >= 1; i--) {  // i is unsigned, a[0] is done below
      s = a[i];  while (*s) { *t++ = *s++; }  *t++ = '\n';

      if (i % 2048 == 0) {
         j = t - output_buf;
         if ((size = write(fd, output_buf, j)) != j) {
            fprintf(stderr, "%s: Could not write to output stream\n", name);
            j = 0; break;
         }
         t = output_buf;
      }
   }

   s = a[0];  while (*s) { *t++ = *s++; }  *t++ = '\n';
   j = t - output_buf;

   if ((size = write(fd, output_buf, j)) != j)
      fprintf(stderr, "%s: Could not write to output stream\n", name);
}

// ############################################################################

static void
output_bm(FILE *fp, double load_t, double ptrary_t, double sort_t, double check_t, double save_t, double free_t, int check_status)
{
   fprintf(fp, \
      "LOAD: %f\nPTRA: %f\nSORT: %f\nCHKA: %f\nSAVE: %f\nFREE: %f\n", \
      load_t, ptrary_t, sort_t, check_t, save_t, free_t);

   if (check_status)
      fprintf(fp, "FAIL: 1\n");
   else
      fprintf(fp, "PASS: 1\n");
}

static int
check_array(char **a, size_t n)
{
   size_t i;
   char *s1, *s2;

   for (i = 1; i < n; i++) {
      for (s1 = a[i - 1], s2 = a[i]; *s1 == *s2 && *s1 != 0; s1++, s2++) ;
      if (*s1 > *s2) return 1;
   }

   return 0;
}

static off_t
get_size(int fd)
{
   struct stat st;

   if (fstat(fd, &st) == 0 && S_ISREG(st.st_mode))
      return st.st_size;

   return -1;
}

// ############################################################################

char *space, **a; size_t n;

int main(int argc, char *argv[])
{
   double load_t, ptrary_t, sort_t, check_t, save_t, free_t;   // duration
   std::clock_t start;

   int fd, opt, reverse_flag = 0, check_status = 0;
   int bm_flag = 0, bm_out_flag = 0, check_flag = 0, no_output_flag = 0;
   char *bname = NULL, *fname = NULL, *oname = NULL;
   FILE *bp, *fp, *op;
   size_t size;

   static struct option longopts[] = {
      { "bm",         no_argument,        &bm_flag,         1 },
      { "bm-out",     required_argument,  &bm_out_flag,     1 },
      { "check",      no_argument,        &check_flag,      1 },
      { "no-output",  no_argument,        &no_output_flag,  1 },
      { NULL,         0,                  NULL,             0 }
   };

   // Parse command-line arguments
   while ((opt = getopt_long(argc, argv, "ro:", longopts, NULL)) != -1) {
      switch (opt) {
         case 'r':
            reverse_flag = 1;
            break;
         case 'o':
            oname = optarg;
            break;
         case 0:
            if (bm_out_flag) {
               bm_out_flag = 0;
               bname = optarg;
            }
            break;
         default:
            fprintf(stderr, "usage: %s [-r] file [-o output]\n", argv[0]);
            exit(1);
      }
   }

   if (optind >= argc) {
      fprintf(stderr, "%s: missing file, ", argv[0]);
      fprintf(stderr, "usage: %s [-r] file [-o sorted]\n", argv[0]);
      exit(1);
   }

   fname = argv[optind];

   // =========================================================================

   // Error checking
   if ((fp = fopen(fname, "r")) == NULL) {
      fprintf(stderr, "%s: Could not open %s for reading\n", argv[0], fname);
      exit(1);
   }
   if ((size = get_size(fileno(fp))) == -1) {
      fprintf(stderr, "%s: %s is not a regular file\n", argv[0], fname);
      fclose(fp); exit(1);
   }
   if (size == 0) {
      fclose(fp); exit(0);
   }

   if (oname != NULL && no_output_flag == 0) {
      if ((op = fopen(oname, "w")) == NULL) {
         fprintf(stderr, "%s: Could not open %s for writing\n", argv[0], oname);
         fclose(fp); exit(1);
      }
   }

   // =========================================================================

   // Load file into memory
   start = STOPWATCH_BEGIN();

   if ((space = (char *)malloc(sizeof(char) * size)) == NULL) {
      fprintf(stderr, "%s: Could not allocate memory for file\n", argv[0]);
      fclose(fp); exit(1);
   }

   fread(space, sizeof(char), size, fp);
   fclose(fp);

   load_t = STOPWATCH_END(start);

   // Create pointer array
   start = STOPWATCH_BEGIN();
   a = create_pointer_array(argv[0], space, size, &n);
   ptrary_t = STOPWATCH_END(start);

   // Sort pointer array
   start = STOPWATCH_BEGIN();
   sort_main(a, n);
   sort_t = STOPWATCH_END(start);

   // Check sorted
   if (check_flag) {
      start = STOPWATCH_BEGIN();
      check_status = check_array(a, n);
      check_t = STOPWATCH_END(start);
   } else {
      check_t = 0.0;
   }

   // Output sorted
   if (no_output_flag == 0) {
      start = STOPWATCH_BEGIN();
      fd = (oname != NULL) ? fileno(op) : fileno(stdout);

      if (reverse_flag)
         output_descending(argv[0], fd, a, n);
      else
         output_ascending(argv[0], fd, a, n);

      if (oname != NULL) fclose(op);
      save_t = STOPWATCH_END(start);

   } else {
      save_t = 0.0;
   }

   // Free memory
   start = STOPWATCH_BEGIN();
   free((void*)a); free((void*)space);
   free_t = STOPWATCH_END(start);

   // =========================================================================

   if (bm_flag) {
      if (bname != NULL) {
         bp = fopen(bname, "w");

         output_bm(bp, \
            load_t, ptrary_t, sort_t, check_t, save_t, free_t, check_status);

         fclose(bp);

      } else {
         output_bm(stderr, \
            load_t, ptrary_t, sort_t, check_t, save_t, free_t, check_status);
      }
   }

   return 0;
}

