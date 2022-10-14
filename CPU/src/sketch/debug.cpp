#include "debug.h"
#include "util.h"
#include "logger.h"
#include <cstring>
#include <map>
#include <set>
#include <algorithm>

namespace HASHPIPE
{
    const int NSTAGE = 10;
    const int len = 1000;
    seed_t seed[NSTAGE] = {};
    
    slot_t nt[NSTAGE][len] = {};
    
    void init()
    {
        for (int i=0;i<NSTAGE;i++)
        {
            seed[i] = clock();
            sleep(1);
        }
    }

    void insert(data_t item)
    {
        slot_t cur;
        cur.item=item; cur.cnt=1;

        // First stage: LRU
        {
            int pos = HASH::hash(cur.item, seed[0]) % len;
            if (nt[0][pos].item == cur.item)
            {
                nt[0][pos].cnt++;
                return;
            }
            else
                std::swap(cur, nt[0][pos]);
        }

        for (int u=1;u<NSTAGE;u++)
        {
            if (cur.cnt == 0)
                return;
            
            int pos = HASH::hash(cur.item, seed[u]) % len;
            if (nt[u][pos].item == cur.item)
            {
                nt[u][pos].cnt += cur.cnt;
                return;
            }
            else if (cur.cnt > nt[u][pos].cnt)
            {
                std::swap(cur, nt[u][pos]);
            }
        }

        return;
    }

    count_t query(data_t item)
    {
        count_t cnt = 0;
        for (int u=0; u<NSTAGE; u++)
        {
            int pos=HASH::hash(item, seed[u]);
            if (nt[u][pos].item == item)
                cnt += nt[u][pos].cnt;
        }
        return cnt;
    }

    std::vector<record_t> GetTopK()
    {
        std::map<data_t, count_t> tpcnt;
        for (int i=0;i<NSTAGE;i++)
        {
            for (int j=0;j<len;j++)
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

    void TestTopK(std::vector<record_t>& ans, int K)
    {
        LOG_INFO("HASHPIPE::Test TopK (K = %d)", K);

        LOG_INFO("Stat info of each stage:");
        std::string s;
        for (int i=1;i<NSTAGE;i++)
        {
            int mx = 0, mn = INT32_MAX, s=0;
            for (int j=0;j<len;j++)
            {
                mx = std::max(mx, nt[i][j].cnt);
                mn = std::min(mn, nt[i][j].cnt);
                s += nt[i][j].cnt;
            }
            LOG_INFO("\tStage #%d: AVG = %d, MAX = %d, MIN = %d", i, s/len, mx, mn);
        }

        std::map<data_t, count_t> tpcnt;
        for (int i=0;i<NSTAGE;i++)
        {
            for (int j=0;j<len;j++)
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
        for (int i=0; i<K && i<rst.size(); i++)
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
}