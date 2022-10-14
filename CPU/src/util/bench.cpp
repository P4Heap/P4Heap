#include "bench.h"

void test(Dataset& stream, TopKFramework& framework)
{
    for (int i=0;i<stream.TOTAL_PACKETS;i++)
    {
        framework.insert(stream.raw_data[i]);
    }

    auto ans = stream.GetTopK();
    framework.TestTopK(ans, 3000);
    LOG_SEP();
}

void test(Dataset& stream, BaseSketch& sketch)
{
    for (int i=0;i<stream.TOTAL_PACKETS;i++)
    {
        sketch.insert(stream.raw_data[i]);
    }

    sketch.test(3000, stream);
    LOG_SEP();
}

void test(Dataset& stream, TopKFramework& framework, BaseSketch& sketch)
{
    for (int i=0;i<stream.TOTAL_PACKETS;i++)
    {
        auto out = framework.insert(stream.raw_data[i]);
        if (out.cnt > 0)
            sketch.insert(out.item, out.cnt);
    }

    sketch.test(3000, stream, framework);
    LOG_SEP();
}
