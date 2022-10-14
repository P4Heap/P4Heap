#include "sketch.h"
#include "defs.h"
#include "util.h"

Count::Count(int _TOTAL_MEM, int _NSTAGE, int _SLOT_SZ) : 
    TOTAL_MEM(_TOTAL_MEM), NSTAGE(_NSTAGE), LEN(_TOTAL_MEM/(NSTAGE * _SLOT_SZ))
{
    nt = new count_t*[NSTAGE];
    seed = new seed_t[NSTAGE];
    sseed = new seed_t[NSTAGE];
    for (int i=0;i<NSTAGE;i++)
    {
        seed[i] = clock();
        sleep(1);
        sseed[i] = clock();
        sleep(1);
        nt[i] = new count_t[LEN];
        memset(nt[i], 0, sizeof(count_t)*LEN);
    }
}

Count::~Count()
{
    delete[] seed;
    delete[] sseed;
    for (int i=0;i<NSTAGE;i++)
    {
        delete[] nt[i];
    }
    delete[] nt;
}

inline int Count::get_sign(data_t item, int stage)
{
    if (HASH::hash(item, sseed[stage]) % 2)
        return 1;
    else
        return -1;
}

void Count::insert(data_t item, count_t freq)
{
    for (int i=0;i<NSTAGE;i++)
    {
        int pos = HASH::hash(item, seed[i]) % LEN;
        nt[i][pos] += freq*get_sign(item, i);
    }
}

count_t Count::query(data_t item)
{
    count_t rst = INT32_MAX;
    for (int i=0;i<NSTAGE;i++)
    {
        int pos = HASH::hash(item, seed[i]) % LEN;
        rst = std::min(rst, nt[i][pos]*get_sign(item, i));
    }
    return rst;
}

void Count::test(int K, Dataset& stream)
{
    auto ans = stream.GetTopK();
    K = min(K, int(ans.size()));
    LOG_INFO("Test Count on top-%d items:", K);
    
    double aae = 0, are = 0;
    for (int i=0;i<K;i++)
    {
        count_t rst = query(ans[i].item);
        aae += abs(rst - ans[i].cnt);
        are += double(abs(rst - ans[i].cnt)) / ans[i].cnt;
    }
    aae /= K;
    are /= K;
    LOG_RESULT("AAE = %lf, ARE = %lf", aae, are);
}

void Count::test(int K, Dataset& stream, TopKFramework& topk)
{
    auto ans = stream.GetTopK();
    K = min(K, int(ans.size()));
    LOG_INFO("Test %s+Count on top-%d items:", topk.GetName(), K);
    
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
