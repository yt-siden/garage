# scalapack_distribution_lp64

あるプロセスにおいて32bit integerで扱える要素数を超える配列を行列として確保した場合に，
ScaLAPACKの `p*gemr2d` を用いてその行列を分散させると関数内部で確保するメモリ量の計算でオーバーフローしているようである．

この問題がMKL ScaLAPACK LP64インターフェースに起因しているものであるかを調べる．

## 調査(LP64)

```
% mpiicpc -std=c++11 -g -pedantic -Wall -Wextra -o distribution_test_lp64 distribution_test.cpp -I${MKLROOT}/include  -L${MKLROOT}/lib/intel64 -lmkl_scalapack
_lp64 -lmkl_intel_lp64 -lmkl_intel_thread -lmkl_core -lmkl_blacs_intelmpi_lp64 -liomp5 -lpthread -lm -ldl

% mpirun -np 4 ./distribution_test_lp64 40000 2 2 32 32
Generating matrix A...
distribute

===================================================================================
=   BAD TERMINATION OF ONE OF YOUR APPLICATION PROCESSES
=   PID 31907 RUNNING AT **************************
=   EXIT CODE: 139
=   CLEANING UP REMAINING PROCESSES
=   YOU CAN IGNORE THE BELOW CLEANUP MESSAGES
===================================================================================

===================================================================================
=   BAD TERMINATION OF ONE OF YOUR APPLICATION PROCESSES
=   PID 31907 RUNNING AT **************************
=   EXIT CODE: 11
=   CLEANING UP REMAINING PROCESSES
=   YOU CAN IGNORE THE BELOW CLEANUP MESSAGES
===================================================================================
   Intel(R) MPI Library troubleshooting guide:
      https://software.intel.com/node/561764
===================================================================================

% gdb ./distribution_test_lp64 core.31907
(snip)
(gdb) bt
#0  0x00007f85757ea62f in PMPI_Pack (inbuf=0x22b0000, incount=-1109196120, datatype=0, outbuf=0xbeba5dd8, outsize=-1109311360, position=0x0,
    comm=-1006632949) at ../../src/I_MPI/include/i_memcpy_common.h:76
#1  0x00007f8576a3564a in MKLMPI_Pack () from /opt/intel/compilers_and_libraries_2017.4.196/linux/mkl/lib/intel64_lin/libmkl_blacs_intelmpi_lp64.so
#2  0x00007f8576a3a5e7 in BI_Pack () from /opt/intel/compilers_and_libraries_2017.4.196/linux/mkl/lib/intel64_lin/libmkl_blacs_intelmpi_lp64.so
#3  0x00007f8576a24162 in Cdgesd2d () from /opt/intel/compilers_and_libraries_2017.4.196/linux/mkl/lib/intel64_lin/libmkl_blacs_intelmpi_lp64.so
#4  0x00007f857b20ad94 in Cpdgemr2d () from /opt/intel/compilers_and_libraries_2017.4.196/linux/mkl/lib/intel64_lin/libmkl_scalapack_lp64.so
#5  0x00000000004034ee in main () at distribution_test.cpp:113
```

## 調査(ILP64)

```
% mpiicpc -std=c++11 -g -pedantic -Wall -Wextra -o distribution_test_ilp64 distribution_test.cpp -m64 -DMKL_ILP64 -I${MKLROOT}/include  -L${MKLROOT}/lib/intel
64 -lmkl_scalapack_ilp64 -lmkl_intel_ilp64 -lmkl_intel_thread -lmkl_core -lmkl_blacs_intelmpi_ilp64 -liomp5 -lpthread -lm -ldl

% mpirun -np 4 ./distribution_test_ilp64 40000 2 2 32 32
Generating matrix A...
distribute

===================================================================================
=   BAD TERMINATION OF ONE OF YOUR APPLICATION PROCESSES
=   PID 32393 RUNNING AT **************************
=   EXIT CODE: 139
=   CLEANING UP REMAINING PROCESSES
=   YOU CAN IGNORE THE BELOW CLEANUP MESSAGES
===================================================================================

===================================================================================
=   BAD TERMINATION OF ONE OF YOUR APPLICATION PROCESSES
=   PID 32393 RUNNING AT **************************
=   EXIT CODE: 11
=   CLEANING UP REMAINING PROCESSES
=   YOU CAN IGNORE THE BELOW CLEANUP MESSAGES
===================================================================================
   Intel(R) MPI Library troubleshooting guide:
      https://software.intel.com/node/561764
===================================================================================

% gdb ./distribution_test_lp64 core.31907
(snip)
(gdb) bt
#0  0x00007fb69fe8962f in PMPI_Pack (inbuf=0x1999000, incount=-397611480, datatype=0, outbuf=0xbeba6258, outsize=-397725568, position=0x0, comm=-1006632949)
    at ../../src/I_MPI/include/i_memcpy_common.h:76
#1  0x00007fb6a10d7afa in MKLMPI_Pack () from /opt/intel/compilers_and_libraries_2017.4.196/linux/mkl/lib/intel64_lin/libmkl_blacs_intelmpi_ilp64.so
#2  0x00007fb6a10dca97 in BI_Pack () from /opt/intel/compilers_and_libraries_2017.4.196/linux/mkl/lib/intel64_lin/libmkl_blacs_intelmpi_ilp64.so
#3  0x00007fb6a10c6612 in ilp64_Cdgesd2d () from /opt/intel/compilers_and_libraries_2017.4.196/linux/mkl/lib/intel64_lin/libmkl_blacs_intelmpi_ilp64.so
#4  0x00007fb6a10f91c0 in Cdgesd2d () from /opt/intel/compilers_and_libraries_2017.4.196/linux/mkl/lib/intel64_lin/libmkl_blacs_intelmpi_ilp64.so
#5  0x00007fb6a583a185 in Cpdgemr2d () from /opt/intel/compilers_and_libraries_2017.4.196/linux/mkl/lib/intel64_lin/libmkl_scalapack_ilp64.so
#6  0x0000000000403599 in main () at distribution_test.cpp:113
```

## 追調査

どのあたりから失敗するか調査

```
% mpirun -np 4 ./distribution_test_ilp64 30000 2 2 32 32
Generating matrix A...
distribute
release context
mpirun -np 4 ./distribution_test_ilp64 30000 2 2 32 32  146.09s user 11.48s system 399% cpu 39.434 total

# n=30000 -> ok

% mpirun -np 4 ./distribution_test_ilp64 32768 2 2 32 32
Generating matrix A...
distribute

===================================================================================
=   BAD TERMINATION OF ONE OF YOUR APPLICATION PROCESSES
=   PID 33761 RUNNING AT **************************
=   EXIT CODE: 139
=   CLEANING UP REMAINING PROCESSES
=   YOU CAN IGNORE THE BELOW CLEANUP MESSAGES
===================================================================================

===================================================================================
=   BAD TERMINATION OF ONE OF YOUR APPLICATION PROCESSES
=   PID 33761 RUNNING AT **************************
=   EXIT CODE: 11
=   CLEANING UP REMAINING PROCESSES
=   YOU CAN IGNORE THE BELOW CLEANUP MESSAGES
===================================================================================
   Intel(R) MPI Library troubleshooting guide:
      https://software.intel.com/node/561764
===================================================================================
mpirun -np 4 ./distribution_test_ilp64 32768 2 2 32 32  388.92s user 50.01s system 351% cpu 2:05.02 total

# n=32768 (=2^15) -> ng

% mpirun -np 4 ./distribution_test_ilp64 32767 2 2 32 32
Generating matrix A...
distribute
release context
mpirun -np 4 ./distribution_test_ilp64 32767 2 2 32 32  171.97s user 13.30s system 399% cpu 46.368 total

# n=32767 -> ok
```

どうやら他のプロセスに 8 x 16384^2 = 2^3 x (2^14)^2 = **2^31** byte以上送ろうとすると落ちるようだ．
これはMPI(の実装?)の問題だと思われる．

Intel MPIでは現状**FortranのみILP64に対応**しているようだ．

C/C++では，ScaLAPACKの2D block cyclicで持つsub-matrixの大きさは，
**(myROCr) x (myROCc) x sizeof(T) < 2^31**
となるようにするべきでしょう．

## 追記 (2019-06-12)

タレコミ元の後輩のコードと同じパラメータで調査

### 調査(LP64, n=49302, 2x2)

```
% mpirun -np 4 ./distribution_test_lp64 49302 2 2 32 32
Generating matrix A...
distribute
MKL_SCALAPACK_ALLOCATE in mr2d_malloc.c is unsucceseful, size = 18446744058795310880
mpirun -np 4 ./distribution_test_lp64 49302 2 2 32 32  346.81s user 18.74s system 399% cpu 1:31.41 total
```

### 調査(LP64, n=49302, 4x4)

```
% mpirun -np 16 ./distribution_test_lp64 49302 4 4 32 32
Generating matrix A...
distribute
MKL_SCALAPACK_ALLOCATE in mr2d_malloc.c is unsucceseful, size = 18446744058795310880
mpirun -np 16 ./distribution_test_lp64 49302 4 4 32 32  1536.50s user 80.02s system 1599% cpu 1:41.06 total
```

### 調査(ILP64, n=49302, 2x2)

```
% mpirun -np 4 ./distribution_test_ilp64 49302 2 2 32 32
Generating matrix A...
distribute

===================================================================================
=   BAD TERMINATION OF ONE OF YOUR APPLICATION PROCESSES
=   PID 15913 RUNNING AT **************************
=   EXIT CODE: 139
=   CLEANING UP REMAINING PROCESSES
=   YOU CAN IGNORE THE BELOW CLEANUP MESSAGES
===================================================================================

===================================================================================
=   BAD TERMINATION OF ONE OF YOUR APPLICATION PROCESSES
=   PID 15913 RUNNING AT **************************
=   EXIT CODE: 11
=   CLEANING UP REMAINING PROCESSES
=   YOU CAN IGNORE THE BELOW CLEANUP MESSAGES
===================================================================================
   Intel(R) MPI Library troubleshooting guide:
     https://software.intel.com/node/561764
===================================================================================
mpirun -np 4 ./distribution_test_ilp64 49302 2 2 32 32  825.28s user 110.82s system 361% cpu 4:18.67 total
```

### 調査(ILP64, n=49302, 4x4)

```
% mpirun -np 16 ./distribution_test_ilp64 49302 4 4 32 32
Generating matrix A...
distribute
release context
mpirun -np 16 ./distribution_test_ilp64 49302 4 4 32 32  1641.60s user 98.03s system 1597% cpu 1:48.93 total
```

## 結論

~~MKL ScaLAPACK LP64インターフェースの問題ではなく，MPI側の問題．~~

- デカい行列を扱うときはILP64インターフェースを用いるようにしよう
- 1プロセスあたりの配列の~~要素数~~バイト数がintをオーバーフローしないようにしよう

## 参考文献

- [ILP64 Support | Intel® MPI Library for Linux*](https://software.intel.com/en-us/mpi-developer-guide-linux-ilp64-support)
