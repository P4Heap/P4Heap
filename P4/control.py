import json
import sys
import binascii
import sys
# f = open('/root/top++/final/result.txt','w')
# saveStd = sys.stdout
# sys.stdout = f
TopK = {}

sID='bfrt.top_check.pipe.Ingress.elastic_stage'

for stage in range(1,4):
    ID_table = eval(sID+str(stage)+'_1')
    ID_text = ID_table.dump(json=True, from_hw=True)
    IDs = json.loads(ID_text)

    counter_table = eval(sID+str(stage)+'_2')
    counter_text = counter_table.dump(json=True, from_hw=True)
    counters = json.loads(counter_text)

    for i in range(131072>>stage):
        if IDs[i]['data']['Ingress.elastic_stage'+str(stage)+'_1.key'][0]!=0:
            key = IDs[i]['data']['Ingress.elastic_stage'+str(stage)+'_1.key'][0]
            if key in TopK.keys():
                TopK[key]+=counters[i]['data']['Ingress.elastic_stage'+str(stage)+'_2.f1'][0]
            else:
                TopK[key]=counters[i]['data']['Ingress.elastic_stage'+str(stage)+'_2.f1'][0]
        if IDs[i]['data']['Ingress.elastic_stage'+str(stage)+'_1.key'][1]!=0:
            key = IDs[i]['data']['Ingress.elastic_stage'+str(stage)+'_1.key'][1]
            if key in TopK.keys():
                TopK[key]+=counters[i]['data']['Ingress.elastic_stage'+str(stage)+'_2.f1'][1]
            else:
                TopK[key]=counters[i]['data']['Ingress.elastic_stage'+str(stage)+'_2.f1'][1]


sID='bfrt.top_check.pipe.Ingress.lrfu_stage'

for stage in range(1,3):
    ID_table = eval(sID+str(stage)+'_1')
    ID_text = ID_table.dump(json=True, from_hw=True)
    IDs = json.loads(ID_text)

    counter_table = eval(sID+str(stage)+'_2')
    counter_text = counter_table.dump(json=True, from_hw=True)
    counters = json.loads(counter_text)

    for i in range(16384>>stage):
        if IDs[i]['data']['Ingress.lrfu_stage'+str(stage)+'_1.key'][0]!=0:
            key = IDs[i]['data']['Ingress.lrfu_stage'+str(stage)+'_1.key'][0]
            if key in TopK.keys():
                TopK[key]+=counters[i]['data']['Ingress.lrfu_stage'+str(stage)+'_2.f1'][0]
            else:
                TopK[key]=counters[i]['data']['Ingress.lrfu_stage'+str(stage)+'_2.f1'][0]
        if IDs[i]['data']['Ingress.lrfu_stage'+str(stage)+'_1.key'][1]!=0:
            key = IDs[i]['data']['Ingress.lrfu_stage'+str(stage)+'_1.key'][1]
            if key in TopK.keys():
                TopK[key]+=counters[i]['data']['Ingress.lrfu_stage'+str(stage)+'_2.f1'][1]
            else:
                TopK[key]=counters[i]['data']['Ingress.lrfu_stage'+str(stage)+'_2.f1'][1]

Result = sorted(TopK.items(),key=lambda x:x[1])
print(Result)