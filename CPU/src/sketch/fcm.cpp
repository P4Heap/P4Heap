#include "sketch.h"
#include "defs.h"
#include "util.h"

FCM::FCM(int _TOTAL_MEM, int _SLOT_SZ) : 
    TOTAL_MEM(_TOTAL_MEM), HEIGHT(log2(_SLOT_SZ) + 1), LEN(_TOTAL_MEM/(NTREES * HEIGHT * _SLOT_SZ))
{
    assert(HEIGHT <= 3);
    nt = new uint64_t**[NTREES];
    seed = new seed_t[NTREES];

    for (int i=0;i<NTREES;i++)
    {
        nt[i] = new uint64_t*[HEIGHT];
        seed[i] = clock();
        sleep(1);
        for (int j=0;j<HEIGHT;j++)
        {
            nt[i][j] = new uint64_t[LEN << (2-j)];
            memset(nt[i][j], 0, sizeof(uint64_t)*(LEN<<(2-j)));
        }
    }
}

FCM::~FCM()
{
    delete[] seed;
    for (int i=0;i<NTREES;i++)
    {
        delete[] nt[i];
    }
    delete[] nt;
}

void FCM::insert(data_t item, count_t freq)
{
    for (int i=0;i<NTREES;i++)
    {
        int pos = HASH::hash(item, seed[i]) % (LEN << 2);
        for (int j=0;j<HEIGHT;j++)
        {
            if (nt[i][j][pos] + freq < THRESHOLD[j])
            {
                nt[i][j][pos] += freq;
                break;
            }
            else if (nt[i][j][pos] < THRESHOLD[j])
            {
                freq = (THRESHOLD[j]-1) - nt[i][j][pos];
                nt[i][j][pos] = THRESHOLD[j];
            }
            
            pos /= 2;
        }
    }
}

count_t FCM::query(data_t item)
{
    count_t rst = INT32_MAX;
    for (int i=0;i<NTREES;i++)
    {
        int pos = HASH::hash(item, seed[i]) % (LEN << 2);
        int cur = 0;
        for (int j=0;j<HEIGHT;j++)
        {
            if (nt[i][j][pos] == THRESHOLD[j])
            {
                cur += THRESHOLD[j]-1;
                pos /= 2;
            }
            else
            {
                cur += nt[i][j][pos];
                break;
            }
        }
        rst = std::min(rst, cur);
    }
    return rst;
}

void FCM::test(int K, Dataset& stream)
{
    auto ans = stream.GetTopK();
    K = min(K, int(ans.size()));
    LOG_INFO("Test FCM on top-%d items:", K);
    
    double aae = 0, are = 0;
    for (int i=0;i<K;i++)
    {
        count_t rst = query(ans[i].item);
        // assert(rst >= ans[i].cnt);
        aae += abs(rst - ans[i].cnt);
        are += double(abs(rst - ans[i].cnt)) / ans[i].cnt;
    }
    aae /= K;
    are /= K;
    LOG_RESULT("AAE = %lf, ARE = %lf", aae, are);
}

void FCM::test(int K, Dataset& stream, TopKFramework& topk)
{
    auto ans = stream.GetTopK();
    K = min(K, int(ans.size()));
    LOG_INFO("Test %s+FCM on top-%d items:", topk.GetName(), K);
    
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
