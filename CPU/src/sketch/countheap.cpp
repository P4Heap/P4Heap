#include "sketch.h"
#include "defs.h"
#include "util.h"

CountHeap::CountHeap(int _TOTAL_MEM, int _NSTAGE, int _SLOT_SZ) : 
    TOTAL_MEM(_TOTAL_MEM), NSTAGE(_NSTAGE), LEN(_TOTAL_MEM/(2 * NSTAGE * _SLOT_SZ)), HEAPSIZE(_TOTAL_MEM/(2*_SLOT_SZ))
{
    nt = new count_t*[NSTAGE];
    seed = new seed_t[NSTAGE];
    sseed = new seed_t[NSTAGE];
    heap = new Heap(_TOTAL_MEM/2);
    for (int i=0;i<NSTAGE;i++)
    {
        seed[i] = HASH::hash(clock(), i);
        sseed[i] = seed[i]+101;
        nt[i] = new count_t[LEN];
        memset(nt[i], 0, sizeof(count_t)*LEN);
    }
}

CountHeap::~CountHeap()
{
    delete[] seed;
    delete[] sseed;
    for (int i=0;i<NSTAGE;i++)
    {
        delete[] nt[i];
    }
    delete[] nt;
}

inline int CountHeap::get_sign(data_t item, int stage)
{
    if (HASH::hash(item, sseed[stage]) % 2)
        return 1;
    else
        return -1;
}

void CountHeap::insert(data_t item, count_t freq)
{
    vector<count_t> tprst;
    for (int i=0;i<NSTAGE;i++)
    {
        int pos = HASH::hash(item, seed[i]) % LEN;
        int cursign = get_sign(item, i);
        nt[i][pos] += freq*cursign;
        tprst.push_back(nt[i][pos]*cursign);
    }
    std::sort(tprst.begin(), tprst.end());
    count_t curfreq;
    if (NSTAGE % 2)
        curfreq = tprst[NSTAGE/2];
    else
        curfreq = (tprst[NSTAGE/2 - 1]+tprst[NSTAGE/2])/2;
    
    heap->Insert(item, curfreq);
}

count_t CountHeap::query(data_t item)
{
    // count_t rst = INT32_MAX;
    // for (int i=0;i<NSTAGE;i++)
    // {
    //     int pos = HASH::hash(item, seed[i]) % LEN;
    //     rst = std::min(rst, nt[i][pos]*get_sign(item, i));
    // }
    // return rst;
    return heap->Query(item);
}

std::deque<record_t> CountHeap::GetTopK()
{
    std::deque<record_t> rst;
    for (auto t : heap->mp)
    {
        rst.push_back(record_t{t.first, heap->heaps[t.second].counter});
    }
    sort(rst.begin(), rst.end());
    return rst;
}

void CountHeap::test(int K, Dataset& stream)
{
    auto ans = stream.GetTopK();
    K = min(K, int(ans.size()));
    LOG_INFO("Test CountHeap on top-%d items:", K);
    
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

    auto rst = GetTopK();
    // PR
    std::map<data_t, count_t> anscnt;
    for (int i=0;i<K;i++)
    {
        assert(anscnt.insert(std::make_pair(ans[i].item, ans[i].cnt)).second);
    }

    double pr = 0;
    for (int i=0;i<K && i<rst.size();i++)
    {
        if (anscnt.find(rst[i].item)!=anscnt.end())
            pr++;
    }
    pr /= K;
    LOG_RESULT("Precision Rate (PR): %lf", pr);

    // RR
    std::map<data_t, count_t> cntr;
    for (auto t : rst)
    {
        assert(cntr.insert(std::make_pair(t.item, t.cnt)).second);
    }

    double rr = 0;
    for (int i=0;i<K;i++)
    {
        if (cntr.find(ans[i].item)!=cntr.end())
            rr++;
    }
    rr /= K;
    LOG_RESULT("Recall Rate (RR): %lf", rr);
}

void CountHeap::test(int K, Dataset& stream, TopKFramework& topk)
{
    auto ans = stream.GetTopK();
    K = min(K, int(ans.size()));
    LOG_INFO("Test %s+CountHeap on top-%d items:", topk.GetName(), K);
    
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

    auto rst1 = topk.GetTopK();
    auto rst2 = GetTopK();
    std::map<data_t, count_t> cntr;
    for (auto t : rst1)
    {
        assert(cntr.insert(std::make_pair(t.item, t.cnt)).second);
    }
    for (auto t : rst2)
    {
        auto it = cntr.find(t.item);
        if (it != cntr.end())
            it->second += t.cnt;
        else
            cntr.insert(std::make_pair(t.item, t.cnt));
    }
    std::vector<record_t> rst;
    for (auto t : cntr)
    {
        rst.push_back(record_t{t.first, t.second});
    }
    std::sort(rst.begin(), rst.end());

    // PR
    std::map<data_t, count_t> anscnt;
    for (int i=0;i<K;i++)
    {
        assert(anscnt.insert(std::make_pair(ans[i].item, ans[i].cnt)).second);
    }

    double pr = 0;
    for (int i=0;i<K && i<rst.size();i++)
    {
        if (anscnt.find(rst[i].item)!=anscnt.end())
            pr++;
    }
    pr /= K;
    LOG_RESULT("Precision Rate (PR): %lf", pr);

    // RR
    double rr = 0;
    for (int i=0;i<K;i++)
    {
        if (cntr.find(ans[i].item)!=cntr.end())
            rr++;
    }
    rr /= K;
    LOG_RESULT("Recall Rate (RR): %lf", rr);
}
