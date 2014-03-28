
/*
 * Contains the main function. Also helper functions for fast IO.
 *
 * The code presented in this file has been tested with care but is not
 * guaranteed for any purpose. The writer does not offer any warranties
 * nor does he accept any liabilities with respect to the code.
 *
 * Mario Roy, 03/25/2014
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

extern void sort_main(char **a, size_t n, int mce_flag);

#define ALLOC_SIZE 8388608   // 8192K
char output_buf[196608];     //  192K

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

static void
output_ascending(char *name, int fd, char **a, size_t n)
{
   size_t i, j, size;
   char *s, *t = output_buf;

   for (i = 0, j = 0; i < n; i++) {
      s = a[i];  while (*s) { *t++ = *s++; }  *t++ = '\n';

      if (i % 768 == 0) {
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

      if (i % 768 == 0) {
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

static off_t
get_size(int fd)
{
   struct stat st;

   if (fstat(fd, &st) == 0 && S_ISREG(st.st_mode))
      return st.st_size;

   return -1;
}

char *space, **a; size_t n;

int main(int argc, char *argv[])
{
   size_t size;
   char *fname = NULL, *oname = NULL;
   int fd, opt, reverse_flag = 0, mce_flag = 0;
   FILE *fp, *op;

   // Parse command-line arguments
   while ((opt = getopt(argc, argv, "ro:e:")) != -1) {
      switch (opt) {
         case 'e':
            // For MCE use only, i.e. start at depth 1
            if (strcmp(optarg, "__MCE__") == 0) mce_flag = 1;
            break;
         case 'r':
            reverse_flag = 1;
            break;
         case 'o':
            oname = optarg;
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
   if (oname != NULL) {
      if ((op = fopen(oname, "w")) == NULL) {
         fprintf(stderr, "%s: Could not open %s for writing\n", argv[0], oname);
         free((void*)space); exit(1);
      }
   }

   // Read entire file into memory
   if ((space = (char *)malloc(sizeof(char) * size)) == NULL) {
      fprintf(stderr, "%s: Could not allocate memory for file\n", argv[0]);
      fclose(fp); exit(1);
   }

   fread(space, sizeof(char), size, fp);
   fclose(fp);

   // Sort
   a = create_pointer_array(argv[0], space, size, &n);

   sort_main(a, n, mce_flag);

   // Output
   fd = (oname != NULL) ? fileno(op) : fileno(stdout);

   if (reverse_flag)
      output_descending(argv[0], fd, a, n);
   else
      output_ascending(argv[0], fd, a, n);

   if (oname != NULL) fclose(op);

   // Cleanup
   free((void*)a); free((void*)space);

   return 0;
}

