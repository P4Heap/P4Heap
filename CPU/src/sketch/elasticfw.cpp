#include "elasticfw.h"
#include "util.h"
#include "logger.h"
#include <cstring>
#include <map>
#include <set>
#include <algorithm>

ElasticFW::ElasticFW(int MEM_SZ)
{
    TOTAL_MEM = MEM_SZ;
    LEN = TOTAL_MEM / (NSTAGE*sizeof(ELASTIC::elastic_slot_t));
    seed = new seed_t[NSTAGE];
    nt = new ELASTIC::elastic_slot_t*[NSTAGE];
    for (int i=0;i<NSTAGE;i++)
    {
        nt[i] = new ELASTIC::elastic_slot_t[LEN];
        memset(nt[i], 0, sizeof(ELASTIC::elastic_slot_t) * LEN);
        seed[i] = clock();
        sleep(1);
    }
}

ElasticFW::~ElasticFW()
{
    delete[] seed;
    for (int i=0;i<NSTAGE;i++)
    {
        delete[] nt[i];
    }
    delete[] nt;
}

slot_t ElasticFW::insert(data_t item)
{
    slot_t cur;
    cur.item=item; cur.cnt=1;

    for (int i=0;i<NSTAGE;i++)
    {
        int pos = HASH::hash(cur.item, seed[i]) % LEN;

        if (nt[i][pos].vote_all == 0)
        {
            nt[i][pos] = ELASTIC::elastic_slot_t{cur.item, cur.cnt, cur.cnt};
            return slot_t{0, 0};
        }
        else if (nt[i][pos].item == cur.item)
        {
            nt[i][pos].vote_all += cur.cnt;
            nt[i][pos].vote_p += cur.cnt;
            return slot_t{0, 0};
        }

        nt[i][pos].vote_all += cur.cnt;
        if (nt[i][pos].vote_all >= nt[i][pos].vote_p*lambda)
        {
            std::swap(nt[i][pos].item, cur.item);
            nt[i][pos].vote_p += cur.cnt;
            nt[i][pos].vote_all += cur.cnt;
            cur.cnt = 1;
        }
    }

    return cur;
}

count_t ElasticFW::query(data_t item)
{
    count_t cnt = 0;
    for (int u=0; u<NSTAGE; u++)
    {
        int pos=HASH::hash(item, seed[u]) % LEN;
        if (nt[u][pos].item == item)
            cnt += nt[u][pos].vote_p;
    }
    return cnt;
}

count_t ElasticFW::query(partial_t item)
{
    aggregate();

    auto it = aggrst.find(item);
    if (it == aggrst.end())
        return 0;
    else
        return it->second;
}

void ElasticFW::aggregate()
{
    if (!aggrst.empty())
        return;

    for (int i=0;i<NSTAGE;i++)
    {
        for (int j=0;j<LEN;j++)
        {
            if (nt[i][j].vote_p > 0)
            {
                partial_t curip = GetPartialKey(nt[i][j].item);
                auto it = aggrst.find(curip);
                if (it == aggrst.end())
                {
                    aggrst.insert(std::make_pair(nt[i][j].item, nt[i][j].vote_p));
                }
                else
                {
                    it->second += nt[i][j].vote_p;
                }
            }
        }
    }
}

std::vector<record_t> ElasticFW::GetTopK()
{
    std::map<data_t, count_t> tpcnt;
    for (int i=0;i<NSTAGE;i++)
    {
        for (int j=0;j<LEN;j++)
        {
            auto it=tpcnt.find(nt[i][j].item);
            if (it==tpcnt.end())
            {
                tpcnt.insert(std::make_pair(nt[i][j].item, nt[i][j].vote_p));
            }
            else
            {
                it->second += nt[i][j].vote_p;
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

std::vector<partial_record_t> ElasticFW::GetPartialTopK()
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

void ElasticFW::TestTopK(std::vector<record_t>& ans, int K)
{
    K = std::min(K, int(ans.size()));
    LOG_INFO("Test Elastic on top-%d items:", K);

    std::map<data_t, count_t> tpcnt;
    for (int i=0;i<NSTAGE;i++)
    {
        for (int j=0;j<LEN;j++)
        {
            if (nt[i][j].vote_p > 0)
            {
                auto it = tpcnt.find(nt[i][j].item);
                if (it == tpcnt.end())
                {
                    tpcnt.insert(std::make_pair(nt[i][j].item, nt[i][j].vote_p));
                }
                else
                {
                    it->second += nt[i][j].vote_p;
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
        aae += abs(ans[i].cnt - cur);
        are += double(abs(ans[i].cnt - cur)) / ans[i].cnt;
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
}
