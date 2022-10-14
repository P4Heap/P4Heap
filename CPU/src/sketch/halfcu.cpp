#include "sketch.h"
#include "defs.h"
#include "util.h"

HalfCU::HalfCU(int _TOTAL_MEM, int _NSTAGE, int _SLOT_SZ) : 
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

HalfCU::~HalfCU()
{
    delete[] seed;
    for (int i=0;i<NSTAGE;i++)
    {
        delete[] nt[i];
    }
    delete[] nt;
}

void HalfCU::insert(data_t item, count_t freq)
{
	count_t limit = INT32_MAX;
	int change_stage = 0;
	int change_pos = 0;
    for (int i = 0;i < NSTAGE;i++)
    {
	    int pos = HASH::hash(item, seed[i]) % LEN;
		if((nt[i][pos] + freq) < limit)
		{
			limit = nt[i][pos] + freq;
			nt[i][pos] += freq; 
		}
		else if(nt[i][pos] < limit)
		{
			nt[i][pos] = limit;
		}
		else continue;
    }
}

count_t HalfCU::query(data_t item)
{
	count_t ans = INT32_MAX;
    for (int i = 0;i < NSTAGE;i++)
    {
	    int pos = HASH::hash(item, seed[i]) % LEN;
		ans = std::min(ans,nt[i][pos]);
    }
    return ans;
}

void HalfCU::test(int K, Dataset& stream)
{
    auto ans = stream.GetTopK();
    K = min(K, int(ans.size()));
    LOG_INFO("Test HalfCU on top-%d items:", K);
    
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

void HalfCU::test(int K, Dataset& stream, TopKFramework& topk)
{
    auto ans = stream.GetTopK();
    K = min(K, int(ans.size()));
    LOG_INFO("Test %s+HalfCU on top-%d items:", topk.GetName(), K);
    
    double aae = 0, are = 0;
    for (int i=0;i<K;i++)
    {
        count_t rst = query(ans[i].item) + topk.query(ans[i].item);
        aae += (rst - ans[i].cnt);
        are += double(rst - ans[i].cnt) / ans[i].cnt;
    }
    aae /= K;
    are /= K;
    LOG_RESULT("AAE = %lf, ARE = %lf", aae, are);
}
