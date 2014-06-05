# Sorting "random words" with Many-core Engine for Perl

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
and parallelize using the MCE Perl module. I began with bs-mkqs which is
using 7-bit versus 8-bit for the array type. So, I normalized on using
7-bit for all the examples. The number of keys in the radix implementations
were changed from 256 to 128 as well.

## Directory content

```
  .Inline/   The Perl Inline module creates this directory. Any changes made
             to the embedded C code will cause a small delay when running the
             first time. This is due to Inline compiling C and caching to
             this directory.

  bin/       bs-mkqs, mr-merge, ng-cradix, tb-radix, tr-radix

             mce-sort1 :  95 buckets
             mce-sort2 : 189 buckets (double)

             Each bucket requires a file handle during the pre-sorting stage.

  lib/       Perl modules MCE, Inline, Parse::RecDescent (needed by Inline), 
             and CpuAffinity.

  src/       bs-mkqs.cc, mr-merge.cc, ng-cradix.cc, tb-radix.cc, tr-radix.cc,
             main.h, and the Makefile.
```
      
## Usage

Required Perl modules under CentOS/RedHat.

```
  yum install perl-ExtUtils-MakeMaker
  yum install perl-Time-HiRes  (not included with Perl 5.8.x)
```

Inline and Parse::RecDescent modules are included under the lib dir and
not necessary to install.

Compile the sources.

```
  cd src; make clean; make; cd ../bin
```

Running.

```
  $ ./bs-mkqs [-r] FILE > sorted               ## Compute using 1 core

  $ ./mce-sort1 -e bs-mkqs [-r] FILE > sorted  ## Compute using many cores

  $ ./mce-sort1

  NAME
     mce-sort1 -- wrapper script for parallelizing sort binaries

  SYNOPSIS
     mce-sort1 [options] -e SORTEXE [-r] FILE

  DESCRIPTION
     The mce-sort1 script utilizes MCE to sort FILE in parallel.
     The partition logic is suited for string sorting as of this time.

     The following options are available:

     --maxworkers=<val> Specify the number of workers for MCE (default auto)
     --parallelio       Enable parallel IO for stage A partitioning of data

     --bm               Display benchmark info
     --check            Check array after sorted
     --no-output        Omit sorted output

     -e SORTEXE         Specify the sort command
     -r                 Reverse output order

  EXAMPLES
     mce-sort1 --maxworkers=8 --bm --check --no-output -e tr-radix ascii.4gb
     mce-sort1 --maxworkers=4 -e tr-radix ascii.4gb > sorted.4gb
```

## Description of sequential algorithms

```
  mr-merge.cc    Merge sort implementation (my challenge assignment)

  bs-mkqs.cc     Multikey quicksort by Bentley and Sedgewick [1]
  ng-cradix.cc   Cache efficient radix sort by Waihong Ng    [2]
  tb-radix.cc    Radix sort implementation by Timo Bingmann  [3]
  tr-radix.cc    8-bit in-place radix sort by Tommi Rantala  [4]
```

## References

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

## Testing

Obtain the 1 GB Random test file from below URL and extract to /dev/shm/.
http://panthema.net/2013/parallel-string-sorting/

Create a 32gb file. Change 32 to a smaller number. Ensure at least 60%
available space before running. The following will create a 32gb file
containing over 3.27 billion strings.

```
  cd /dev/shm

  for s in `seq 1 32`; do
     cat random.1073741824 >> random.ascii.32gb
  done
```

Are you running on a system with many CPU sockets? Use numactl to have
memory allocated evenly across all CPU nodes. Check with numactl -H.

```
  RedHat/CentOS:  sudo yum install numactl
  Ubuntu:         sudo apt-get install numactl

  cd /dev/shm

  for s in `seq 1 32`; do
     numactl -i all cat random.1073741824 >> random.ascii.32gb
  done

  numactl -H
```

## Sorting

The sorting process is done in 3 stages. Inline C is used to handle
pre-sorting. CPU affinity is applied under the Linux environment.

```
  A. Partition (fast 1 character pre-sorting into individual buckets)
  B. Sequential sorting (choose an algorithm of your liking)
  C. Serialize output (runs alongside Stage B)
```

The pre-sorting logic is suited for string sorting only. No attempt was made
to sort a file having the same first character for every line. So, not useful
in that regard. The challenge was sorting a file having many random words.

## Absolute run time in seconds

The system is a dual Intel Xeon E5-2660 (v1), 1600 MHz 128GB, running
Fedora 20 x86_64. The box has 16 real cores (32 logical PEs).

Running with 1 core takes 405.848 seconds to sort the pointer array.
Interestingly enough is that it takes 357.870 seconds with mce-sort1.
The pre-sorting stage creates up to 95 partitions which are sorted
individually afterwards.

```
  $ ./tr-radix --bm --check --no-output /dev/shm/random.ascii.32gb

  LOAD:   13.137478      load into memory
  PTRA:   56.706502      fill pointer array
  SORT:  405.848093      sort pointer array
  CHKA:  168.436225      check sorted
  SAVE:    0.000000      0 due to passing --no-output
  FREE:    1.167123      free memory
  PASS:    1             check succeeded, used by MCE

  $ ./mce-sort1 --maxworkers=1 --bm --check --no-output \
       -e tr-radix /dev/shm/random.ascii.32gb 

  pre-sort         :  131.124s
  load partitions  :   13.876s
  fill ptr arrays  :   54.972s
  sort ptr arrays  :  226.746s + 131.124s (pre-sort) = 357.870s
  check sorted     :  144.040s
  free memory      :    1.376s
```

Compare 405.848 seconds (1 core) with 27.462 seconds (16 cores) for
mce-sort1 -e tr-radix. That is quite good considering the later also
includes the pre-sorting time.

```
   PEs                             2        4        8       16       32
  =======================================================================
   mce-sort1 -e bs-mkqs   |  399.223  180.555   99.660   59.073   52.556
   mce-sort1 -e mr-merge  |  659.007  311.366  175.254  106.111   90.435
   mce-sort1 -e ng-cradix |  331.811  150.988   80.833   44.574   34.342
   mce-sort1 -e tb-radix  |  199.510   98.444   53.737   31.283   25.582
   mce-sort1 -e tr-radix  |  172.677   85.836   47.256   27.462   22.286
  =======================================================================
  Random, n = 3.27 billion strings, N = 32 Gi File
```

## Output from -e tr-radix, 32 logical PEs

```
  $ ./mce-sort1 --maxworkers=32 --bm --check --no-output \
       --parallelio -e tr-radix /dev/shm/random.ascii.32gb 

  Stage A   started         :  1398207596.304
  Stage A   finished (part) :  1398207603.347       7.044 seconds

  Stage B/C started         :  1398207603.348
  Stage B   finished (sort) :  1398207633.857      30.509 seconds

            load partitions :        1.486731
            fill ptr arrays :        2.898013
            sort ptr arrays :       15.241647
            check sorted    :       10.768132 (OK)
            free memory     :        0.114815

  Stage C   finished (outp) :  1398207633.991      30.643 seconds

                 total time :  1398207633.991      37.687 seconds

  Absolute run time (partition and sort ptr arrays)

  mce-sort1 -e bs-mkqs      :  7.159 + 45.397   =  52.556s
  mce-sort1 -e mr-merge     :  7.148 + 83.287   =  90.435s
  mce-sort1 -e ng-cradix    :  7.139 + 27.203   =  34.342s
  mce-sort1 -e tb-radix     :  7.084 + 18.498   =  25.582s
  mce-sort1 -e tr-radix     :  7.044 + 15.242   =  22.286s
```

## Output from -e tr-radix, 16 cores

```
  $ ./mce-sort1 --maxworkers=16 --bm --check --no-output \
       --parallelio -e tr-radix /dev/shm/random.ascii.32gb 

  Stage A   started         :  1398210591.483
  Stage A   finished (part) :  1398210600.789       9.306 seconds

  Stage B/C started         :  1398210600.790
  Stage B   finished (sort) :  1398210636.227      35.437 seconds

            load partitions :        1.221781
            fill ptr arrays :        3.966185
            sort ptr arrays :       18.156220
            check sorted    :       11.988952 (OK)
            free memory     :        0.103446

  Stage C   finished (outp) :  1398210636.250      35.460 seconds

                 total time :  1398210636.251      44.767 seconds

  Absolute run time (partition and sort ptr arrays)

  mce-sort1 -e bs-mkqs      :  9.275 + 49.798   =  59.073s
  mce-sort1 -e mr-merge     :  9.235 + 96.876   = 106.111s
  mce-sort1 -e ng-cradix    :  9.367 + 35.207   =  44.574s
  mce-sort1 -e tb-radix     :  9.289 + 21.994   =  31.283s
  mce-sort1 -e tr-radix     :  9.306 + 18.156   =  27.462s
```

## Output from -e tr-radix, 8 cores

```
  $ ./mce-sort1 --maxworkers=8 --bm --check --no-output \
       --parallelio -e tr-radix /dev/shm/random.ascii.32gb 

  Stage A   started         :  1398211434.990
  Stage A   finished (part) :  1398211451.131      16.141 seconds

  Stage B/C started         :  1398211451.132
  Stage B   finished (sort) :  1398211511.304      60.171 seconds

            load partitions :        2.001858
            fill ptr arrays :        7.352689
            sort ptr arrays :       31.115341
            check sorted    :       19.534765 (OK)
            free memory     :        0.166683

  Stage C   finished (outp) :  1398211511.326      60.193 seconds

                 total time :  1398211511.326      76.336 seconds

  Absolute run time (partition and sort ptr arrays)

  mce-sort1 -e bs-mkqs      : 16.152 +  83.508  =  99.660s
  mce-sort1 -e mr-merge     : 16.324 + 158.930  = 175.254s
  mce-sort1 -e ng-cradix    : 16.480 +  64.353  =  80.833s
  mce-sort1 -e tb-radix     : 16.256 +  37.481  =  53.737s
  mce-sort1 -e tr-radix     : 16.141 +  31.115  =  47.256s
```

## Output from -e tr-radix, 4 cores

```
  $ ./mce-sort1 --maxworkers=4 --bm --check --no-output \
       --parallelio -e tr-radix /dev/shm/random.ascii.32gb 

  Stage A   started         :  1398212323.491
  Stage A   finished (part) :  1398212353.307      29.816 seconds

  Stage B/C started         :  1398212353.308
  Stage B   finished (sort) :  1398212460.899     107.592 seconds

            load partitions :        3.667963
            fill ptr arrays :       13.764580
            sort ptr arrays :       56.020127
            check sorted    :       33.846885 (OK)
            free memory     :        0.291969

  Stage C   finished (outp) :  1398212460.923     107.616 seconds

                 total time :  1398212460.924     137.433 seconds

  Absolute run time (partition and sort ptr arrays)

  mce-sort1 -e bs-mkqs      : 29.797 + 150.758  = 180.555s
  mce-sort1 -e mr-merge     : 29.599 + 281.767  = 311.366s
  mce-sort1 -e ng-cradix    : 30.003 + 120.985  = 150.988s
  mce-sort1 -e tb-radix     : 29.989 +  68.455  =  98.444s
  mce-sort1 -e tr-radix     : 29.816 +  56.020  =  85.836s
```

## Output from -e tr-radix, 2 cores

```
  $ ./mce-sort1 --maxworkers=2 --bm --check --no-output \
       --parallelio -e tr-radix /dev/shm/random.ascii.32gb 

  Stage A   started         :  1398260292.321
  Stage A   finished (part) :  1398260355.075      62.753 seconds

  Stage B/C started         :  1398260355.075
  Stage B   finished (sort) :  1398260566.739     211.664 seconds

            load partitions :        6.808906
            fill ptr arrays :       27.260062
            sort ptr arrays :      109.924428
            check sorted    :       66.974057 (OK)
            free memory     :        0.696213

  Stage C   finished (outp) :  1398260566.780     211.704 seconds

                 total time :  1398260566.780     274.458 seconds

  Absolute run time (partition and sort ptr arrays)

  mce-sort1 -e bs-mkqs      : 63.247 + 335.976  = 399.223s
  mce-sort1 -e mr-merge     : 63.181 + 595.826  = 659.007s
  mce-sort1 -e ng-cradix    : 63.172 + 268.639  = 331.811s
  mce-sort1 -e tb-radix     : 62.960 + 136.550  = 199.510s
  mce-sort1 -e tr-radix     : 62.753 + 109.924  = 172.677s
```

Thank you to the folks whom have contributed fast sorting algorithms to
the world. Fast sequential algorithms can be parallelized with mce-sort1
(string sorting currently).

