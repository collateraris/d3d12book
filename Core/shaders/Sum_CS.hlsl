#include "CommonInclude.hlsl"


RWStructuredBuffer<int> inputArr : register(u0);
RWStructuredBuffer<int> outputPar : register(u1);

//groupshared int counterPar = 0;

groupshared int counterArr[1024];

[numthreads(1024, 1, 1)]
//[numthreads(256, 1, 1)]
void main(ComputeShaderInput IN)
{
    //ex 0a  28\32 occupancy 2.05us  [numthreads(256, 1, 1)] rtx4090
    //int var_sum = inputArr[IN.dispatchThreadID.x];
    //InterlockedAdd(outputPar[0], var_sum);

    //ex 0b 29\32   occupancy 2.05us [numthreads(256, 1, 1)] rtx4090
    /*int var_sum = inputArr[IN.dispatchThreadID.x];
    float cos_sum = sin(10);
    var_sum += 100 * cos_sum;
    int x = 100;
    var_sum += x;
    InterlockedAdd(outputPar[0], var_sum);
    */

    //ex 0c 29\32 occupancy 3.07us [numthreads(256, 1, 1)] rtx4090
    //int x[1024];
    //x[IN.dispatchThreadID.x] = inputArr[IN.dispatchThreadID.x];
    //InterlockedAdd(outputPar[0], x[IN.dispatchThreadID.x]);

    //ex 0d 21\32 occupancy 3.07us [numthreads(256, 1, 1)] rtx4090
    /*int x[1024];
    if (IN.dispatchThreadID.x % 32 == 0)
    {
        x[IN.dispatchThreadID.x % 32] = inputArr[IN.dispatchThreadID.x];
    }
    else if (IN.dispatchThreadID.x % 3 == 0)
    {
        x[IN.dispatchThreadID.x % 3] = inputArr[IN.dispatchThreadID.x];
    }
    else 
    {
        x[IN.dispatchThreadID.x] = inputArr[IN.dispatchThreadID.x];
    }
    InterlockedAdd(outputPar[0], x[IN.dispatchThreadID.x]);
    */

    //ex 1 28\32 occupancy 2.05us  [numthreads(1024, 1, 1)] rtx4090
    //ex 1 28\32 occupancy 2.05us  [numthreads(256, 1, 1)] rtx4090
    //ex 1a 28\32 occupancy 6.14us [numthreads(1024, 1, 1)] 262144 elements rtx4090
    //ex 1a 28\32 occupancy 10.24us [numthreads(1024, 1, 1)] 262144 elements rtx3070
    //InterlockedAdd(outputPar[0], inputArr[IN.dispatchThreadID.x]);

    //ex 2 29\32 occupancy 2.05us [numthreads(1024, 1, 1)] rtx4090
    //InterlockedAdd(counterPar, inputArr[IN.dispatchThreadID.x]);
    
    //GroupMemoryBarrierWithGroupSync();

    //if (IN.dispatchThreadID.x == 0)
     //   InterlockedAdd(outputPar[0],counterPar);

    //ex 2a  28\32 occupancy 3.07us [numthreads(1024, 1, 1)] 262144 elements rtx4090
    //ex 2a  28\32 occupancy 8.19 us [numthreads(1024, 1, 1)] 262144 elements rtx3070
    //InterlockedAdd(counterPar, inputArr[IN.dispatchThreadID.x]);
    
    //GroupMemoryBarrierWithGroupSync();

    //if (IN.groupThreadID.x == 0)
    //   InterlockedAdd(outputPar[0],counterPar);

    //ex 3 16\32 occupancy 3.07 [numthreads(1024, 1, 1)] rtx4090
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
    }
    */
    
    //ex 4 20\32 occupancy 4.10 us without [loop] [numthreads(1024, 1, 1)] rtx4090
    /*counterArr[IN.dispatchThreadID.x] = inputArr[IN.dispatchThreadID.x];
    GroupMemoryBarrierWithGroupSync();
    
    [loop] // change to 3.07 us  rtx4090
    for (int s = 512; s > 0; s >>= 1)
    {
        if (IN.dispatchThreadID.x < s)
            counterArr[IN.dispatchThreadID.x] += counterArr[IN.dispatchThreadID.x + s];
        GroupMemoryBarrierWithGroupSync();
    }

    if (IN.dispatchThreadID.x == 0)
        InterlockedAdd(outputPar[0],counterArr[0]);*/
    
    //ex 4a 19\32 occupancy 5.12 us [numthreads(1024, 1, 1)] 262144 elements rtx4090
    //ex 4a 19\32 occupancy 19.46 us [numthreads(1024, 1, 1)] 262144 elements rtx3070
    counterArr[IN.groupThreadID.x] = inputArr[IN.dispatchThreadID.x];
    GroupMemoryBarrierWithGroupSync();
    
    [loop]
    for (int s = 512; s > 0; s >>= 1)
    {
        if (IN.groupThreadID.x < s)
            counterArr[IN.groupThreadID.x] += counterArr[IN.groupThreadID.x + s];
        GroupMemoryBarrierWithGroupSync();
    }

    if (IN.groupThreadID.x == 0)
        InterlockedAdd(outputPar[0],counterArr[0]);

}