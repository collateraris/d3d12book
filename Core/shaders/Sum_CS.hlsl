#include "CommonInclude.hlsl"


RWStructuredBuffer<int> inputArr : register(u0);
RWStructuredBuffer<int> outputPar : register(u1);

//groupshared int counterPar = 0;

groupshared int counterArr[1024];

[numthreads(1024, 1, 1)]
void main(ComputeShaderInput IN)
{
    //ex 1 28\32 occupancy 2.05us 
    //InterlockedAdd(outputPar[0], inputArr[IN.dispatchThreadID.x]);

    //ex 2 29\32 occupancy 2.05us 
    //InterlockedAdd(counterPar, inputArr[IN.dispatchThreadID.x]);
    
    //GroupMemoryBarrierWithGroupSync();

    //if (IN.dispatchThreadID.x == 0)
    //    outputPar[0] = counterPar;

    //ex 3 16\32 occupancy 3.07 us
    /*counterArr[IN.dispatchThreadID.x] = inputArr[IN.dispatchThreadID.x];
    GroupMemoryBarrierWithGroupSync();
    if (IN.dispatchThreadID.x == 0)
    {
        int sum = 0;
        for (int i = 0; i < 1024; i++)
        {
           sum += counterArr[i];
        }
        outputPar[0] = sum;
    }*/
    
    //ex 4 20\32 occupancy 4.10 us
    counterArr[IN.dispatchThreadID.x] = inputArr[IN.dispatchThreadID.x];
    GroupMemoryBarrierWithGroupSync();

    for (int s = 512; s > 0; s >>= 1)
    {
        if (IN.dispatchThreadID.x < s)
            counterArr[IN.dispatchThreadID.x] += counterArr[IN.dispatchThreadID.x + s];
        GroupMemoryBarrierWithGroupSync();
    }

    if (IN.dispatchThreadID.x == 0)
        InterlockedAdd(outputPar[0],counterArr[0]);
}