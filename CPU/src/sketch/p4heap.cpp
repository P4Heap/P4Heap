#include "p4heap.h"
#include "util.h"
#include "logger.h"
#include <cstring>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <string>
#include <algorithm>
#include <queue>

P4Heap::P4Heap(int MEM_SZ)
{
    TOTAL_MEM = MEM_SZ;
    double l = 1.5*(1+pow(ratio, 1)+pow(ratio, 2)+pow(ratio, 3))+pow(ratio, 4)+pow(ratio, 5);
    len[0] = TOTAL_MEM/(sizeof(slot_t)*l);
    for (int i=1;i<NSTAGE;i++)
        len[i] = ratio*len[i-1];
    LOG_DEBUG("{%d, %d, %d, %d, %d, %d}", len[0], len[1], len[2], len[3], len[4], len[5]);
    seed_t curseed = clock();
    for (int i=0;i<NSTAGE;i++)
    {
        stages[i]->init(len[i], curseed);
    }
}

P4Heap::~P4Heap()
{
    for (int i=0;i<NSTAGE;i++)
    {
        delete stages[i];
    }
}

slot_t P4Heap::insert(data_t item)
{
    slot_t cur{item, 1};
    for (int i=0;i<NSTAGE;i++)
    {
        if (cur.cnt == 0)
            return slot_t{0, 0};
        cur = stages[i]->insert(cur);
    }
    return cur;
}

count_t P4Heap::query(data_t item)
{
    int rst = 0;
    for (int i=0;i<NSTAGE;i++)
        rst += stages[i]->query(item);
    return rst;
}

count_t P4Heap::query(partial_t item)
{
    aggregate();

    auto it = aggrst.find(item);
    if (it == aggrst.end())
        return 0;
    else
        return it->second;
}

void P4Heap::aggregate()
{
    if (!aggrst.empty())
        return;

    for (int i=0;i<NSTAGE;i++)
    {
        auto cur = stages[i]->GetRecord();
        for (auto& it : cur)
        {
            partial_t curip = GetPartialKey(it.first);
            auto curp = aggrst.find(curip);
            if (curp == aggrst.end())
            {
                aggrst.insert(std::make_pair(curip, it.second));
            }
            else
            {
                curp->second += it.second;
            }
        }
    }
}

std::vector<record_t> P4Heap::GetTopK()
{
    std::map<data_t, count_t> tpcnt;
    for (int i=0;i<NSTAGE;i++)
    {
        auto cur = stages[i]->GetRecord();
        for (auto& it : cur)
        {
            auto curp = tpcnt.find(it.first);
            if (curp == tpcnt.end())
            {
                tpcnt.insert(it);
            }
            else
            {
                curp->second += it.second;
            }
        }
    }

    std::vector<record_t> rst;
    for (auto& it : tpcnt)
    {
        rst.push_back(record_t(it.first, it.second));
    }
    std::sort(rst.begin(), rst.end());
    return rst;
}

std::vector<partial_record_t> P4Heap::GetPartialTopK()
{
    aggregate();

    std::vector<partial_record_t> rst;
    for (auto t : aggrst)
        rst.push_back(partial_record_t{t.first, t.second});
    
    std::sort(rst.begin(), rst.end());
    return rst;
}

void P4Heap::TestTopK(std::vector<record_t>& ans, int K)
{
    K = std::min(K, int(ans.size()));
    LOG_INFO("Test P4Heap Sketch on top-%d items:", K);

    std::map<data_t, count_t> tpcnt;
    for (int i=0;i<NSTAGE;i++)
    {
        auto cur = stages[i]->GetRecord();
        for (auto& it : cur)
        {
            auto curp = tpcnt.find(it.first);
            if (curp == tpcnt.end())
            {
                tpcnt.insert(it);
            }
            else
            {
                curp->second += it.second;
            }
        }
    }

    // Test AAE, ARE
    double aae=0, are=0;
    for (int i=0;i<K;i++)
    {
        auto it=tpcnt.find(ans[i].item);
        count_t cur=0;
        if (it != tpcnt.end())
            cur=it->second;
        assert(ans[i].cnt >= cur);
        aae += (ans[i].cnt - cur);
        are += double(ans[i].cnt - cur) / ans[i].cnt;
    }
    aae /= K; are /= K;
    LOG_RESULT("AAE = %lf, ARE = %lf", aae, are);

    std::vector<record_t> rst;
    for (auto& it : tpcnt)
    {
        rst.push_back(record_t(it.first, it.second));
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

    // Underestimate
    std::map<data_t, count_t> ansmap;
    for (auto& it : ans)
    {
        assert(ansmap.insert(std::make_pair(it.item, it.cnt)).second);
    }
    int ue = 0;
    double sgt = 0;
    for (auto& it : tpcnt)
    {
        int gt = ansmap.find(it.first)->second;
        assert(it.second <= gt);
        sgt += gt;
        ue += (gt - it.second);
    }
    LOG_RESULT("Underestimate %d packets of the total %lf packets", ue, sgt);
}
