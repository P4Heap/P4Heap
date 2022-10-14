#include "dataset.h"
#include "topkframework.h"
#include "p4heap.h"
#include "hashpipe.h"
#include "precision.h"
#include "spacesaving.h"
#include "elasticfw.h"
#include "sketch.h"
#include "bench.h"
#include "debug.h"
#include <set>

int main()
{
    Dataset stream("../dataset/caida.dat", 21);
    // Dataset stream("../dataset/webpage.dat", 4);

    int mem = 60'000;

    {
        CM cm(mem*2, 4, 4);
        test(stream, cm);

        P4Heap fn(mem);
        CM cm1(mem, 4, 2);
        test(stream, fn, cm1);
    }

    {
        Count cm(mem*2, 4, 4);
        test(stream, cm);

        P4Heap fn(mem);
        Count cm1(mem, 4, 2);
        test(stream, fn, cm1);
    }

    {
        FCM cm(mem*2, 4);
        test(stream, cm);

        P4Heap fn(mem);
        FCM cm1(mem, 2);
        test(stream, fn, cm1);
    }

    {
        Univmon cm(mem*2, 4);
        test(stream, cm);

        P4Heap fn(mem);
        Univmon cm1(mem, 2);
        test(stream, fn, cm1);
    }

    {
        NitroCM cm(mem*2, 0.1, 4, 4);
        test(stream, cm);

        P4Heap fn(mem);
        NitroCM cm1(mem, 0.1, 4, 2);
        test(stream, fn, cm1);
    }

    {
        Elastic cm(mem*2, 4);
        test(stream, cm);

        P4Heap fn(mem);
        Elastic cm1(mem, 2);
        test(stream, fn, cm1);
    }

    // for (int mem = 50'000; mem<=500'000;mem+=50'000)
    // {
    //     LOG_INFO("Memory: %d", mem);
    //     RHHH cm(mem, 0);
    //     test(stream, cm);

    //     RHHH cm1(mem, 1);
    //     test(stream, cm1);
    // }

    // for (int mem=5'000; mem<=50'000; mem+=5'000)
    // {
    //     LOG_INFO("Memory: %d", mem);
    //     Elastic cm(mem*2, 4);
    //     test(stream, cm);

    //     Funnel fn(mem);
    //     Elastic cm1(mem, 2);
    //     test(stream, fn, cm1);
    // }

    // for (int mem=10'000; mem<=100'000; mem+=10'000)
    // {
    //     LOG_INFO("Memory: %d", mem);
    //     Coco cm(mem*2, 2);
    //     test(stream, cm);

    //     Funnel fn(mem);
    //     Coco cm1(mem, 2);
    //     test(stream, fn, cm1);
    // }
}
