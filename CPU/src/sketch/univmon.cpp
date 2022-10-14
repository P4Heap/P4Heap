#include "sketch.h"
#include "util.h"
#include "logger.h"
#include <cstring>
#include <map>
#include <set>
#include <algorithm>

Univmon::Univmon(int _TOTAL_MEM, int _SLOT_SZ)
{
    NSKETCH = 6;
    sketches = new CountHeap*[NSKETCH];

    for (int i=0;i<NSKETCH;i++)
    {
        sketches[i] = new CountHeap(_TOTAL_MEM>>(i+1), 3, _SLOT_SZ);
    }
    seed = clock();
}

Univmon::~Univmon()
{
    for (int i=0;i<NSKETCH;i++)
    {
        delete sketches[i];
    }
    delete[] sketches;
}

void Univmon::insert(data_t item, count_t freq)
{
    sketches[0]->insert(item, freq);

    uint64_t hashv = HASH::hash(item, seed);
    for (int i=1;i<NSKETCH;i++)
    {
        if ((hashv & 1) == 0)
            break;

        hashv = hashv >> 1;
        sketches[i]->insert(item, freq);
    }
}

count_t Univmon::query(data_t item)
{
    uint64_t pos = HASH::hash(item, seed);
    int level;

    for(level = 1; level < NSKETCH; level++){
        if(pos & 1)
            pos >>= 1;
        else
            break;
    }

    count_t ret = sketches[level - 1]->query(item);
    for(int32_t i = level - 2;i >= 0;i--)
    {
        ret = 2 * ret - sketches[i]->query(item);
    }

    return ret;
}

std::deque<record_t> Univmon::GetTopK()
{
    std::map<data_t, count_t> cntr;
    for (int i=NSKETCH-1;i>=0;i--)
    {
        auto cur = sketches[i]->GetTopK();
        for (auto t : cur)
        {
            if (cntr.find(t.item) == cntr.end())
                cntr.insert(std::make_pair(t.item, t.cnt));
        }
    }

    std::deque<record_t> ans;
    for (auto t : cntr)
    {
        ans.push_back(record_t{t.first, t.second});
    }
    std::sort(ans.begin(), ans.end());
    return ans;
}

void Univmon::test(int K, Dataset& stream)
{
    auto ans = stream.GetTopK();
    K = min(K, int(ans.size()));
    LOG_INFO("Test Univmon on top-%d items:", K);

    // AAE, ARE
    double aae=0, are=0;
    for (int i=0;i<K;i++)
    {
        count_t cur = query(ans[i].item);
        aae += abs(cur-ans[i].cnt);
        are += double(abs(cur-ans[i].cnt)) / ans[i].cnt;
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

void Univmon::test(int K, Dataset& stream, TopKFramework& topk)
{
    auto ans = stream.GetTopK();
    K = min(K, int(ans.size()));
    LOG_INFO("Test %s+Univmon on top-%d items:", topk.GetName(), K);

    // AAE, ARE
    double aae=0, are=0;
    for (int i=0;i<K;i++)
    {
        count_t cur = topk.query(ans[i].item)+query(ans[i].item);
        aae += abs(cur-ans[i].cnt);
        are += double(abs(cur-ans[i].cnt)) / ans[i].cnt;
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
