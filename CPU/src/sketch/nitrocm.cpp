#include "sketch.h"
#include "defs.h"
#include "util.h"

NitroCM::NitroCM(int _TOTAL_MEM, double _SAMPLE_RATE, int _NSTAGE, int _SLOT_SZ) : 
    TOTAL_MEM(_TOTAL_MEM), NSTAGE(_NSTAGE), LEN(_TOTAL_MEM/(NSTAGE * _SLOT_SZ)), SAMPLE_RATE(_SAMPLE_RATE)
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

NitroCM::~NitroCM()
{
    delete[] seed;
    for (int i=0;i<NSTAGE;i++)
    {
        delete[] nt[i];
    }
    delete[] nt;
}

void NitroCM::insert(data_t item, count_t freq)
{
    for (int i=0;i<NSTAGE;i++)
    {
        if (RandP() < SAMPLE_RATE)
        {
            int pos = HASH::hash(item, seed[i]) % LEN;
            nt[i][pos] += freq;
        }
    }
}

count_t NitroCM::query(data_t item)
{
    int rst[NSTAGE];
    for (int i=0;i<NSTAGE;i++)
    {
        int pos = HASH::hash(item, seed[i]) % LEN;
        rst[i] = nt[i][pos];
    }
    sort(rst, rst+NSTAGE);
    if (NSTAGE % 2)
        return rst[NSTAGE / 2] / SAMPLE_RATE;
    else
        return (rst[NSTAGE/2 -1] + rst[NSTAGE/2]) / (SAMPLE_RATE*2);
}

void NitroCM::test(int K, Dataset& stream)
{
    auto ans = stream.GetTopK();
    K = min(K, int(ans.size()));
    LOG_INFO("Test NitroCM on top-%d items:", K);
    
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

void NitroCM::test(int K, Dataset& stream, TopKFramework& topk)
{
    auto ans = stream.GetTopK();
    K = min(K, int(ans.size()));
    LOG_INFO("Test %s+NitroCM on top-%d items:", topk.GetName(), K);
    
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
