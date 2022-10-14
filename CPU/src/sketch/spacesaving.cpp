#include "spacesaving.h"
#include "util.h"
#include "logger.h"
#include <cstring>
#include <map>
#include <set>
#include <algorithm>

SpaceSaving::SpaceSaving(int MEM_SZ) : TOTAL_MEM(MEM_SZ), MAX_BUCKET(MEM_SZ / sizeof(SS::SS_bucket_t))
{
    return;
}

slot_t SpaceSaving::insert(data_t item)
{
    auto it = counter.find(item);
    if (it != counter.end())
    {
        heap.erase(heap.find(it->second));
        it->second.cnt++;
        heap.insert(it->second);
        return slot_t{0, 0};
    }
    else if (heap.size() < MAX_BUCKET)
    {
        counter.insert(std::make_pair(item, SS::SS_bucket_t{item, 1}));
        heap.insert(SS::SS_bucket_t{item, 1});
        return slot_t{0, 0};
    }
    else
    {
        SS::SS_bucket_t victim = *heap.begin();
        heap.erase(heap.begin());
        counter.erase(counter.find(victim.item));
        counter.insert(std::make_pair(item, SS::SS_bucket_t{item, victim.cnt+1}));
        heap.insert(SS::SS_bucket_t{item, victim.cnt+1});
        return slot_t{victim.item, victim.cnt};
    }
}

count_t SpaceSaving::query(data_t item)
{
    auto it = counter.find(item);
    if (it == counter.end())
        return 0;
    else
        return it->second.cnt;
}

count_t SpaceSaving::query(partial_t item)
{
    count_t rst = 0;
    for (auto t : heap)
    {
        if (GetPartialKey(t.item) == item)
            rst += t.cnt;
    }
    return rst;
}

void SpaceSaving::aggregate()
{
    if (!aggrst.empty())
        return;
    
    for (auto t : heap)
    {
        partial_t cur = GetPartialKey(t.item);
        auto it = aggrst.find(cur);
        if (it == aggrst.end())
        {
            aggrst.insert(std::make_pair(cur, t.cnt));
        }
        else
        {
            it->second += t.cnt;
        }
    }
}

std::vector<record_t> SpaceSaving::GetTopK()
{
    std::vector<record_t> rst;
    for (auto t : heap)
    {
        rst.push_back(record_t{t.item, t.cnt});
    }
    std::sort(rst.begin(), rst.end());
    return rst;
}

std::vector<partial_record_t> SpaceSaving::GetPartialTopK()
{
    aggregate();

    std::vector<partial_record_t> rst;
    for (auto t : aggrst)
    {
        rst.push_back(partial_record_t{t.first, t.second});
    }
    std::sort(rst.begin(), rst.end());
    return rst;
}

void SpaceSaving::TestTopK(std::vector<record_t>& ans, int K)
{
    K = std::min(K, int(ans.size()));
    LOG_INFO("Test SpaceSaving on top-%d items:", K);

    // Test AAE, ARE
    double aae=0, are=0;
    for (int i=0;i<K;i++)
    {
        auto it=counter.find(ans[i].item);
        count_t cur=0;
        if (it != counter.end())
            cur=it->second.cnt;
        aae += abs(cur - ans[i].cnt);
        are += double(abs(cur - ans[i].cnt)) / ans[i].cnt;
    }
    aae /= K; are /= K;
    LOG_RESULT("AAE = %lf, ARE = %lf", aae, are);

    std::vector<record_t> rst;
    for (auto& it : counter)
    {
        rst.push_back(record_t(it.first, it.second.cnt));
    }
    std::sort(rst.begin(), rst.end());

    // Test RR
    std::set<data_t> rstset;
    for (int i=0; i<rst.size(); i++)
        rstset.insert(rst[i].item);

    double rr=0;
    for (int i=0;i<K;i++)
    {
        auto it=rstset.find(ans[i].item);
        if (it != rstset.end())
            rr++;
    }
    rr /= K;
    LOG_RESULT("Recall Rate (RR): %lf", rr);

    // Test PR
    std::set<data_t> anset;
    for (int i=0;i<K;i++)
        anset.insert(ans[i].item);

    double pr=0;
    for (int i=0; i<K && i<rst.size(); i++)
    {
        if (anset.find(rst[i].item) != anset.end())
            pr++;
    }
    pr /= K;
    LOG_RESULT("Precision Rate (PR): %lf", pr);
}
