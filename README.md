
<> Sorting "random words" with MCE

   It is March of 2014. A group of folks at work have decided to work on
   a sorting challenge just for fun. I am the author of a Perl module named
   Many-core Engine (MCE) and wanted to see if MCE can benefit sorting.

      http://code.google.com/p/many-core-engine-perl/

   My homework was mr-merge.cc for our team challenge. Although faster than
   the unix sort binary, I wanted faster and searched the web.

   One of the links pointed me to the multikey quicksort implementation by
   Bentley and Sedgewick. [1] That performs quite well and is faster than
   mr-merge. However, I wondered if sorting could go even faster and
   searched again. My search ended at this amazing site by Timo Bingmann.

      http://panthema.net/2013/parallel-string-sorting/

   My goal was nothing more than to take several fast sequential algorithms
   and parallelize using the MCE Perl module. I began with bs-mkqs which
   is using (char *) for the type versus (unsigned char *). So, I normalized
   on using (char *) for all the examples. The number of keys in the radix
   implementations were changed from 256 to 128 as well.

   The mr-merge example was taken from an old awk script I wrote years ago.

<> Sorting via 3 stages

   A. Partition (fast 1 character pre-sorting into individual buckets)
   B. Sequential sorting (choose an algorithm of your liking)
   C. Serialize output (runs alongside Stage B)

   Both A and B run with many cores. MCE is currently only available for Perl.
   Inline C was used to handle pre-sorting.

   CPU affinity is applied under the Linux environment.

<> Directory content

   bin/       mce-sort, bs-mkqs, mr-merge, ng-cradix, tb-radix, tr-radix

   _Inline/   The Perl Inline module creates this directory. Any changes made
              to the embedded C code will cause a small delay when running the
              first time. This is due to Inline compiling C and caching to
              this directory.

   lib/       Perl modules: MCE, Inline, Parse::RecDescent (needed by Inline),
              CpuAffinity

   src/       bs-mkqs.cc, mr-merge.cc, ng-cradix.cc, tb-radix.cc, tr-radix.cc,
              main.h, Makefile
      
<> Usage (the -r option is for reversing the result)

   Perl modules needed under CentOS/RedHat.

      yum install perl-ExtUtils-MakeMaker perl-Test-Warn perl-Test-Simple
      yum install perl-Time-HiRes

      Inline and Parse::RecDescent modules are included under the lib dir
      and not necessary to install.

   Compile the sources.

      cd src; make; cd ../bin

   Running.

   ./bs-mkqs [-r] FILE > sorted                 ## Compute using 1 core
   ./mce-sort -e bs-mkqs [-r] FILE > sorted     ## Compute using many cores

   To specify the number of workers, pass $N after file:

   perl mce-sort -e bs-mkqs FILE $N > sorted
      ./mce-sort -e bs-mkqs FILE $N > sorted

   Where $N is the number of processes. The default is 'auto/2' when
   the box has greater than 15 logical cores, otherwise 'auto'.

   ./mce-sort -e bs-mkqs FILE auto/2 > sorted   ## Total logical_cores / 2
   ./mce-sort -e bs-mkqs FILE auto-1 > sorted   ## Total logical_cores - 1
   ./mce-sort -e bs-mkqs FILE auto   > sorted   ## Total logical_cores
   ./mce-sort -e bs-mkqs FILE 16     > sorted   ## 16 workers

###############################################################################

<> Description of selected sequential algorithms

   mr-merge.cc    Merge sort implementation (my challenge assignment)

   bs-mkqs.cc     Multikey quicksort by Bentley and Sedgewick [1]
   ng-cradix.cc   Cache efficient radix sort by Waihong Ng    [2]
   tb-radix.cc    Radix sort implementation by Timo Bingmann  [3]
   tr-radix.cc    8-bit in-place radix sort by Tommi Rantala  [4]

<> References

1. ** J. Bentley and R. Sedgewick.
   Fast algorithms for sorting and searching strings. In Proceedings
   of 8th Annual ACM-SIAM Symposium on Discrete Algorithms, 1997.
   http://www.cs.princeton.edu/~rs/strings/index.html

2. ** Waihong Ng and Katsuhiko Kakehi.
   Cache Efficient Radix Sort for String Sorting
   IEICE Trans. Fundam. Electron. Commun. Comput. Sci.
   Vol. E90-A(2) (2007) 457-466
   http://dx.doi.org/10.1093/ietfec/e90-a.2.457

3. ** Timo Bingmann.
   Experiments with sequential radix sort implementations.
   Based on rantala/msd_c?.h
   http://panthema.net/2013/parallel-string-sorting/

4. ** Tommi Rantala.
   Engineering radix sort for strings. String Processing and Information
   Retrieval. Number 5280 in LNCS. Springer (2009) 3–14
   https://github.com/rantala/string-sorting

###############################################################################

<> Testing

   Obtain the 1 GB Random test file from below URL and extract to /dev/shm/.
   http://panthema.net/2013/parallel-string-sorting/

   Create a 32gb file. Change 32 to a smaller number. Ensure at least 60%
   available space before running.

      cd /dev/shm

      for s in `seq 1 32`; do
         cat random.1073741824 >> random.ascii.32gb
      done

   Are you running on a system with many CPU sockets? Use numactl to have
   memory allocated evenly across all CPU nodes. Check with numactl -H.

      RedHat/CentOS:  sudo yum install numactl
      Ubuntu:         sudo apt-get install numactl

      cd /dev/shm

      for s in `seq 1 32`; do
         numactl -i all cat random.1073741824 >> random.ascii.32gb
      done

<> Dual E5-2660 1600 MHz 128GB, OS: CentOS 6.2 x86_64

   Stage C runs alongside Stage B. The total time is less than 40 seconds
   when sending output to /dev/null and utilizing 32 cores.

   ## Running mce-tr-radix with 16 cores

   ./mce-sort -e tr-radix /dev/shm/random.ascii.32gb 16 >/dev/null

   :: Stage A   started         : 1395506176.416
      Stage A   finished (part) : 1395506185.781       9.365 seconds

   :: Stage B/C started         : 1395506185.783
      Stage B   finished (sort) : 1395506222.619      36.836 seconds
      Stage C   finished (outp) : 1395506222.999      37.216 seconds

                     total time : 1395506222.999      46.583 seconds

   :: Summary Total time

      mce-sort -e bs-mkqs ...   :     87.199s
      mce-sort -e mr-merge ...  :    129.525s
      mce-sort -e ng-cradix ... :     69.120s
      mce-sort -e tb-radix ...  :     48.977s
      mce-sort -e tr-radix ...  :     46.583s

   ## Running mce-tr-radix with 32 cores

   ./mce-sort -e tr-radix /dev/shm/random.ascii.32gb 32 >/dev/null

   :: Stage A   started         : 1395474951.754
      Stage A   finished (part) : 1395474958.455       6.701 seconds

   :: Stage B/C started         : 1395474958.457
      Stage B   finished (sort) : 1395474990.099      31.642 seconds
      Stage C   finished (outp) : 1395474991.450      32.994 seconds

                     total time : 1395474991.450      39.696 seconds

   :: Summary Total time

      mce-sort -e bs-mkqs ...   :     75.004s
      mce-sort -e mr-merge ...  :    107.328s
      mce-sort -e ng-cradix ... :     54.340s
      mce-sort -e tb-radix ...  :     43.547s
      mce-sort -e tr-radix ...  :     39.696s

<> Notes

   The pre-sorting logic in the MCE script is suited for string sorting only.
   No attempt was made to sort a file having the same first character for
   every line. So, not useful in that regard. The challenge was sorting a
   file having many random words.

   With that said, the 3 stage approach seems beneficial. One merely has
   pre-sorting do as little work as necessary. Pre-sorting eradicates the
   need for final merging in the end. The output stage begins once buckets
   have completed. A later bucket completing first will simply remain until
   output has completed for prior buckets.

   Thank you to the folks whom have contributed fast sorting algorithms to
   the world. Sequential sorting executables can be parallelized with
   mce-sort (string sorting currently).

   Kind regards,
   Mario

