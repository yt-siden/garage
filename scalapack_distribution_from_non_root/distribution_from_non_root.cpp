// MKL ScaLAPACK is assumed
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <numeric>
#include <cstdint>
#include <cassert>

#include <mkl_scalapack.h>
#include <mkl_blacs.h>

#ifdef MKL_ILP64
using IndexType = long long;
#else
using IndexType = std::int32_t;
#endif

extern "C" {
    IndexType numroc_(IndexType*, IndexType*, IndexType*, IndexType*, IndexType*);
}


template <typename T>
T convert(const char* str)
{
    std::stringstream ss(str);
    T ret;
    ss >> ret;
    return ret;
}

// mpirun -np nprow*npcol ./a.out n nprow npcol mb nb srcrow srccol
// 1*1 n-by-n matrix is distributed into nprow*npcol (mb,nb) 2D-blcok-cyclic submatrices
// from (srcrow,srccol) process.
int main(int argc, char* argv[])
{
    if (argc != 8)
    {
        std::cerr << "The number of input parameters differs expected number" << std::endl;
        return EXIT_FAILURE;
    }

    // constants
    IndexType one=1, zero=0;

    // process numbers
    IndexType myrank, pnum;
    blacs_pinfo_(&myrank, &pnum);

    // context
    IndexType context;
    blacs_get_(&zero, &zero, &context);

    // nprow and npcol
    // nprow*npcol == pnum is assumed.
    IndexType nprow = convert<IndexType>(argv[2]);
    IndexType npcol = convert<IndexType>(argv[3]);
    assert(nprow*npcol == pnum);

    // n, mb, nb
    // n >= mb && n >= nb is assumed.
    IndexType n  = convert<IndexType>(argv[1]);
    IndexType mb = convert<IndexType>(argv[4]);
    IndexType nb = convert<IndexType>(argv[5]);
    assert(n >= mb);
    assert(n >= nb);

    // set nprow*npcol grid
    IndexType myrow, mycol, new_nprow, new_npcol;
    blacs_gridinit_(&context, "R", &nprow, &npcol);
    blacs_gridinfo_(&context, &new_nprow, &new_npcol, &myrow, &mycol);
    assert(new_nprow == nprow);
    assert(new_npcol == npcol);

    // srcrow, srccol
    IndexType srcrow = convert<IndexType>(argv[6]);
    IndexType srccol = convert<IndexType>(argv[7]);
    assert(0 <= srcrow && srcrow < nprow);
    assert(0 <= srccol && srccol < npcol);

    // matrices
    std::vector<double> mat_A(1);
    if (myrow == srcrow && mycol == srccol)
    {
        std::cout << "(" << myrow << "," << mycol << "): Generating matrix A..." << std::endl;
        mat_A.resize(static_cast<std::uint64_t>(n)*n);
        std::iota(mat_A.begin(), mat_A.end(), 0);

        for (IndexType row = 0; row < n; ++row)
        {
            for (IndexType col = 0; col < n; ++col)
            {
                std::cout << std::setw(8) << mat_A[col*n + row];
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    blacs_barrier_(&context, "A");

    IndexType myROCr = numroc_(&n, &mb, &myrow, &zero, &nprow);
    IndexType myROCc = numroc_(&n, &nb, &mycol, &zero, &npcol);
    std::vector<double> mat_B(static_cast<std::uint64_t>(myROCr)*myROCc);

    // descriptors
    IndexType desc_A[9] = {
        1, // DTYPE_ (block cyclic)
        context, // CTXT_
        n, // M_
        n, // N_
        n, // MB_
        n, // NB_
        srcrow, // RSRC_
        srccol, // CSRC_
        (myrow == srcrow ? n : 1) // LLD_
    };
    IndexType desc_B[9] = {
        1, // DTYPE_ (block cyclic)
        context, // CTXT_
        n, // M_
        n, // N_
        mb, // MB_
        nb, // NB_
        0, // RSRC_
        0, // CSRC_
        myROCr // LLD_
    };

    if (myrow == 0 && mycol == 0)
        std::cout << "distribute" << std::endl << std::endl;

    // distribute
    pdgemr2d(&n, &n,
            mat_A.data(), &one, &one, desc_A,
            mat_B.data(), &one, &one, desc_B,
            &context);

    // show result
    for (int i=0; i<pnum; ++i)
    {
        if (i == myrank)
        {
            std::cout << "(" << myrow << "," << mycol << ")" << std::endl;
            for (IndexType row = 0; row < myROCr; ++row)
            {
                for (IndexType col = 0; col < myROCc; ++col)
                {
                    std::cout << std::setw(8) << mat_B[col*myROCr + row];
                }
                std::cout << std::endl;
            }
            std::cout << std::endl;
        }
        blacs_barrier_(&context, "A");
    }

    if (myrow == 0 && mycol == 0)
        std::cout << "release context" << std::endl;
    blacs_barrier_(&context, "A");

    // release context
    blacs_gridexit_(&context);
}
