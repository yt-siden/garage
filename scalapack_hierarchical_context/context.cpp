#include <mkl_blacs.h>
#include <unistd.h>

#include <iostream>
#include <iomanip>
#include <sstream>

template <typename T>
T conv(const char* str)
{
    std::stringstream ss(str);
    T ret;
    ss >> ret;
    return ret;
}

int main(int argc, char* argv[])
{
    /* Usage
        mpirun -np nprow*npcol ./a.out nprow npcol npbrow npbcol

        ex. 4x4 processes to 2x2 process blocks
         0  1  2  3       0  1 |  2  3
         4  5  6  7       4  5 |  6  7
         8  9 10 11  ->  ------+------
        12 13 14 15       8  9 | 10 11
                         12 13 | 14 15
    */

    // parameters
    // NOTE: Followings should be const, however, they are variable due to technical limitation
    int one = 1, zero = 0, negone = -1;

    // get process information
    int mypnum, pnum;
    blacs_pinfo_(&mypnum, &pnum);

    // global(default) context
    int global_context;
    blacs_get_(&one, &zero, &global_context);

    int nprow = conv<int>(argv[1]);
    int npcol = conv<int>(argv[2]);
    if (pnum < nprow*npcol)
    {
        // abort
        if (pnum == 0)
        {
            std::cerr << "pnum " << pnum << " is less than nprow*npcol " << nprow*npcol << std::endl;
        }
        blacs_barrier_(&global_context, "A");
        blacs_abort_(&global_context, &negone);
    }

    int npbrow = conv<int>(argv[3]);
    int npbcol = conv<int>(argv[4]);
    if (nprow < npbrow || npcol < npbcol)
    {
        // abort
        if (pnum == 0)
        {
            std::cerr << "nprow " << nprow << " is less than npbrow " << npbrow <<
                " or npcol " << npcol << " is less than npbcol " << npbcol << std::endl;
        }
        blacs_barrier_(&global_context, "A");
        blacs_abort_(&global_context, &negone);
    }

    // set nprow*npcol grid
    blacs_gridinit_(&global_context, "R", &nprow, &npcol);

    // get position of each process in the global grid
    int mygrow, mygcol;
    blacs_gridinfo_(&global_context, &nprow, &npcol, &mygrow, &mygcol);

    // calc each process block size
    int nplrow = nprow / npbrow;
    int nplcol = npcol / npbcol;
    int mypbrow = mygrow / nplrow;
    int mypbcol = mygcol / nplcol;
    if (nprow % npbrow)
    {
        int tmp = nplrow + 1;
        nplrow += (mygrow / tmp < nprow % tmp);
    }
    if (npcol % npbcol)
    {
        int tmp = nplcol + 1;
        nplcol += (mygcol / tmp < npcol % tmp);
    }

    // output
    for (int i=0; i<pnum; ++i)
    {
        if (mypnum == i)
        {
            std::cout << "mypnum=" << std::setw(3) << mypnum
                << ", mygrow=" << std::setw(3) << mygrow
                << ", mygcol=" << std::setw(3) << mygcol
                << ", nplrow=" << std::setw(3) << nplrow
                << ", nplcol=" << std::setw(3) << nplcol
                << std::endl;
            usleep(10000);
        }
        blacs_barrier_(&global_context, "A");
    }

    blacs_gridexit_(&global_context);
}
