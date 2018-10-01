#include <mkl_blacs.h>
#include <unistd.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <tuple>

template <typename T>
T conv(const char* str)
{
    std::stringstream ss(str);
    T ret;
    ss >> ret;
    return ret;
}

// nproc, nblock, myproc -> start_idx, bsize
std::tuple<int, int> get_range(const int nproc, const int nblock, const int myproc)
{
    int mod = nproc % nblock;
    int bsize = nproc / nblock;
    int start_idx = myproc / bsize * bsize;

    if (mod)
    {
        ++bsize;
        int tmp = myproc / bsize;
        if (tmp < mod)
        {
            start_idx = myproc / bsize * bsize;
        }
        else
        {
            int tmp3 = myproc - mod*bsize;
            --bsize;
            start_idx = tmp3 / bsize * bsize;
            start_idx += mod*(bsize+1);
        }
    }

    return std::make_tuple(start_idx, bsize);
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
    int lrspos, lcspos, lrsize, lcsize;
    std::tie(lrspos, lrsize) = get_range(nprow, npbrow, mygrow);
    std::tie(lcspos, lcsize) = get_range(npcol, npbcol, mygcol);

    // define grid map
    int usermap[lrsize*lcsize];
    for (int i=0; i<lcsize; ++i)
    for (int j=0; j<lrsize; ++j)
        usermap[i*lrsize+j] = (lrspos+j)*npcol + lcspos + i;

    // output 1
    for (int i=0; i<pnum; ++i)
    {
        if (mypnum == i)
        {
            std::cout << "[" << std::setw(3) << mypnum << " (" << std::setw(3) << mygrow << "," << std::setw(3) << mygcol << ")]: ";
            for (int i=0; i<lrsize*lcsize; ++i)
                std::cout << std::setw(3) << usermap[i];
            std::cout << std::endl;
            usleep(10000);
        }
        blacs_barrier_(&global_context, "A");
    }

    // create new sub-grid
    int local_context;
    blacs_get_(&one, &zero, &local_context);
    blacs_gridmap_(&local_context, usermap, &lrsize, &lrsize, &lcsize);

    int mylrow, mylcol, nplrow, nplcol;
    blacs_gridinfo_(&local_context, &nplrow, &nplcol, &mylrow, &mylcol);

    // output 2
    for (int i=0; i<pnum; ++i)
    {
        if (mypnum == i)
        {
            std::cout << "[" << std::setw(3) << mypnum << " (" << std::setw(3) << mygrow << "," << std::setw(3) << mygcol << ")]: "
                << "(" << std::setw(3) << mylrow << "," << std::setw(3) << mylcol << ") / ("
                << std::setw(3) << nplrow << "," << std::setw(3) << nplcol << ")" << std::endl;
            usleep(10000);
        }
        blacs_barrier_(&global_context, "A");
    }

    // release contexts
    blacs_gridexit_(&local_context);
    blacs_gridexit_(&global_context);
}
