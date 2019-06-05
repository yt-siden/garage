// MKL ScaLAPACK is assumed
#include <vector>
#include <iostream>
#include <sstream>
#include <cstdint>
#include <cassert>

#include <mkl_scalapack.h>

#ifdef MKL_ILP64
using IndexType = std::int64_t;
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

// mpirun -np nprow*npcol ./a.out n nprow npcol mb nb
// 1*1 n-by-n matrix is distributed into nprow*npcol (mb,nb) 2D-blcok-cyclic submatrices.
int main(int argc, char* argv[])
{
    // constants
    const IndexType one=1, zero=0, negone=-1;

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
    IndexType myrow, mycol;
    blacs_gridinit_(&context, &nprow, &npcol, &myrow, &mycol);

    // matrices
    std::vector<double> mat_A(1);
    if (myrow == 0 && mycol == 0)
    {
        mat_A.resize(static_cast<std::uint64_t>(n)*n);
        std::iota(mat_A.begin(), mat_A.end(), 0);
        for (auto&& v : mat_A)
            v %= 100;
    }

    blacs_barrier_(&context, "A");

    IndexType myROCr = numroc_(&n, &mb, &myrow, &zero, &nprow);
    IndexType myROCc = numroc_(&n, &nb, &mycol, &zero, &npcol);
    std::vector<double> mat_B(static_cast<std::uint64_t>(myROCr)*myROCc);

    // descriptors
    IndexType desc_A[9]= {
        1, // DTYPE_ (block cyclic)
        context, // CTXT_
        n, // M_
        n, // N_
        n, // MB_
        n, // NB_
        0, // RSRC_
        0, // CSRC_
        (myrow == 0 ? n : 1) // LLD_
    };
    IndexType desc_B[9]= {
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

    // distribute
    pdgemr2d(&n, &n,
            &mat_A[0], &one, &one, desc_A,
            &mat_B[0], &one, &one, desc_B,
            &context);

    // release context
    blacs_gridexit_(&context);
}
