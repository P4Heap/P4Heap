#include "precision.h"
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

Precision::Precision(int MEM_SZ) : TOTAL_MEM(MEM_SZ), NSTAGE(6)
{
    LEN = TOTAL_MEM / (sizeof(slot_t)*NSTAGE);
    N_RECYC = 0;
    nt = new slot_t*[NSTAGE];
    seed = new seed_t[NSTAGE];
    for (int i=0;i<NSTAGE;i++)
    {
        seed[i] = clock();
        sleep(1);
        nt[i] = new slot_t[LEN];
        memset(nt[i], 0, sizeof(slot_t)*LEN);
    }
}

Precision::~Precision()
{
    for (int i=0;i<NSTAGE;i++)
        delete[] nt[i];
    delete[] nt;
    delete[] seed;
}

slot_t Precision::insert(data_t item) 
{
    count_t carry_min = INT32_MAX, min_stage = -1;
    for (int i=0;i<NSTAGE;i++)
    {
        int pos = HASH::hash(item, seed[i]) % LEN;
        if (nt[i][pos].cnt == 0)
        {
            nt[i][pos] = slot_t{item, 1};
            return slot_t{0, 0};
        }
        else if (nt[i][pos].item == item)
        {
            nt[i][pos].cnt++;
            return slot_t{0, 0};
        }
        else if (nt[i][pos].cnt < carry_min)
        {
            carry_min = nt[i][pos].cnt;
            min_stage = i;
        }
    }

    slot_t cur = slot_t{item, 1};
    int new_val = pow(2, ceil(log2(carry_min)));
    int R = rand() % new_val;

    if (R == 0) // Recirculated
    {
        N_RECYC++;
        int pos = HASH::hash(item, seed[min_stage]) % LEN;
        std::swap(nt[min_stage][pos], cur);
    }
    return cur;
}

count_t Precision::query(data_t item) 
{
    count_t cnt = 0;
    for (int u=0; u<NSTAGE; u++)
    {
        int pos=HASH::hash(item, seed[u]) % LEN;
        if (nt[u][pos].item == item)
            cnt += nt[u][pos].cnt;
    }
    return cnt;
}

count_t Precision::query(partial_t item) 
{
    aggregate();

    auto it = aggrst.find(item);
    if (it == aggrst.end())
        return 0;
    else
        return it->second;
}

void Precision::aggregate()
{
    if (!aggrst.empty())
        return;

    for (int i=0;i<NSTAGE;i++)
    {
        for (int j=0;j<LEN;j++)
        {
            if (nt[i][j].cnt > 0)
            {
                partial_t curip = GetPartialKey(nt[i][j].item);
                auto it = aggrst.find(curip);
                if (it == aggrst.end())
                {
                    aggrst.insert(std::make_pair(nt[i][j].item, nt[i][j].cnt));
                }
                else
                {
                    it->second += nt[i][j].cnt;
                }
            }
        }
    }
}

std::vector<record_t> Precision::GetTopK() 
{
    std::map<data_t, count_t> tpcnt;
    for (int i=0;i<NSTAGE;i++)
    {
        for (int j=0;j<LEN;j++)
        {
            auto it=tpcnt.find(nt[i][j].item);
            if (it==tpcnt.end())
            {
                tpcnt.insert(std::make_pair(nt[i][j].item, nt[i][j].cnt));
            }
            else
            {
                it->second += nt[i][j].cnt;
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

std::vector<partial_record_t> Precision::GetPartialTopK() 
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

void Precision::TestTopK(std::vector<record_t>& ans, int K) 
{
    K = std::min(K, int(ans.size()));
    LOG_INFO("Test PRECISION on top-%d items:", K);
    LOG_INFO("Recyculate %d packets", N_RECYC);

    std::map<data_t, count_t> tpcnt;
    for (int i=0;i<NSTAGE;i++)
    {
        for (int j=0;j<LEN;j++)
        {
            if (nt[i][j].cnt > 0)
            {
                auto it = tpcnt.find(nt[i][j].item);
                if (it == tpcnt.end())
                {
                    tpcnt.insert(std::make_pair(nt[i][j].item, nt[i][j].cnt));
                }
                else
                {
                    it->second += nt[i][j].cnt;
                }
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
