#include "sketch.h"
#include "util.h"
#include "logger.h"
#include <cstring>
#include <map>
#include <set>
#include <algorithm>

Elastic::Elastic(int _TOTAL_MEM, int _SLOT_SZ)
{
    TOTAL_MEM = _TOTAL_MEM;
    LEN = TOTAL_MEM / (NSTAGE*3*_SLOT_SZ);
    seed = new seed_t[NSTAGE];
    nt = new elastic_slot_t*[NSTAGE];
    for (int i=0;i<NSTAGE;i++)
    {
        nt[i] = new elastic_slot_t[LEN];
        memset(nt[i], 0, sizeof(elastic_slot_t) * LEN);
        seed[i] = clock();
        sleep(1);
    }
}

Elastic::~Elastic()
{
    delete[] seed;
    for (int i=0;i<NSTAGE;i++)
    {
        delete[] nt[i];
    }
    delete[] nt;
}

void Elastic::insert(data_t item, count_t freq)
{
    slot_t cur;
    cur.item=item; cur.cnt=freq;

    for (int i=0;i<NSTAGE;i++)
    {
        int pos = HASH::hash(cur.item, seed[i]) % LEN;

        if (nt[i][pos].vote_all == 0)
        {
            nt[i][pos] = elastic_slot_t{cur.item, cur.cnt, cur.cnt};
            return;
        }
        else if (nt[i][pos].item == cur.item)
        {
            nt[i][pos].vote_all += cur.cnt;
            nt[i][pos].vote_p += cur.cnt;
            return;
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

    return;
}

count_t Elastic::query(data_t item)
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

std::deque<record_t> Elastic::GetTopK()
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

    std::deque<record_t> rst;
    for (auto& it : tpcnt)
    {
        rst.push_back(record_t(it.first, it.second));
    }
    std::sort(rst.begin(), rst.end());
    return rst;
}

void Elastic::test(int K, Dataset& stream)
{
    auto ans = stream.GetTopK();
    K = min(K, int(ans.size()));
    LOG_INFO("Test Elastic on top-%d items:", K);
    
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

void Elastic::test(int K, Dataset& stream, TopKFramework& topk)
{
    auto ans = stream.GetTopK();
    K = min(K, int(ans.size()));
    LOG_INFO("Test %s+Elastic on top-%d items:", topk.GetName(), K);
    
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
