#include "sketch.h"
#include "defs.h"
#include "util.h"

CM::CM(int _TOTAL_MEM, int _NSTAGE, int _SLOT_SZ) : 
    TOTAL_MEM(_TOTAL_MEM), NSTAGE(_NSTAGE), LEN(_TOTAL_MEM/(NSTAGE * _SLOT_SZ))
{
    nt = new count_t*[NSTAGE];
    seed = new seed_t[NSTAGE];

    for (int i=0;i<NSTAGE;i++)
    {
        seed[i] = clock();
        sleep(1);
        nt[i] = new count_t[LEN];
        memset(nt[i], 0, sizeof(count_t)*LEN);
    }
}

CM::~CM()
{
    delete[] seed;
    for (int i=0;i<NSTAGE;i++)
    {
        delete[] nt[i];
    }
    delete[] nt;
}

void CM::insert(data_t item, count_t freq)
{
    for (int i=0;i<NSTAGE;i++)
    {
        int pos = HASH::hash(item, seed[i]) % LEN;
        nt[i][pos] += freq;
    }
}

count_t CM::query(data_t item)
{
    count_t rst = INT32_MAX;
    for (int i=0;i<NSTAGE;i++)
    {
        int pos = HASH::hash(item, seed[i]) % LEN;
        rst = std::min(rst, nt[i][pos]);
    }
    return rst;
}

void CM::test(int K, Dataset& stream)
{
    auto ans = stream.GetTopK();
    K = min(K, int(ans.size()));
    LOG_INFO("Test CM on top-%d items:", K);
    
    double aae = 0, are = 0;
    for (int i=0;i<K;i++)
    {
        count_t rst = query(ans[i].item);
        assert(rst >= ans[i].cnt);
        aae += (rst - ans[i].cnt);
        are += double(rst - ans[i].cnt) / ans[i].cnt;
    }
    aae /= K;
    are /= K;
    LOG_RESULT("AAE = %lf, ARE = %lf", aae, are);
}

void CM::test(int K, Dataset& stream, TopKFramework& topk)
{
    auto ans = stream.GetTopK();
    K = min(K, int(ans.size()));
    LOG_INFO("Test %s+CM on top-%d items:", topk.GetName(), K);
    
    double aae = 0, are = 0;
    for (int i=0;i<K;i++)
    {
        count_t rst = query(ans[i].item) + topk.query(ans[i].item);
        aae += abs(rst - ans[i].cnt);
        are += double(abs(rst - ans[i].cnt)) / ans[i].cnt;
    }
    aae /= K;
    are /= K;
    LOG_RESULT("AAE = %lf, ARE = %lf", aae, are);
}
