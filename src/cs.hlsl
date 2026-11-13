static const float PI = 3.14159265358979323846;
static const float hue = -4.0;
static const float chroma_saturation_correction = 2.4;

RWTexture2D<float4> samples : register(u0, space1);

[numthreads(64, 16, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
    uint2 id = threadID.xy;
    
    int i = id.x;
    int center = i + 8;
    int begin = center - 6;
    int end = center + 6;
    
    float Y = 0;
    float U = 0;
    float V = 0;
    
    for (int p = begin; p < end; p++)
    {
        if (p < 0 || p >= 256 * 8)
            continue;
        
        float4 _sample = samples[uint2(p, id.y)];
        int phase = (int)(_sample.w / 100) - 1;
        
        float sample = _sample.w - 150 - (phase * 100);
        
        Y += sample;
        int rotation = (phase + p) % 12;
        U += (sample * sin(PI * (rotation + 3 - 0.5 + hue) / 6) * chroma_saturation_correction);
        V += (sample * cos(PI * (rotation + 3 - 0.5 + hue) / 6) * chroma_saturation_correction);
    }
    
    U = U * 0.5f + 0.5f;
    V = V * 0.5f + 0.5f;
    
    
    float R = 1.164 * (Y - 16 / 256.0) + 1.596 * (V - 128 / 256.0);
    float G = 1.164 * (Y - 16 / 256.0) - 0.392 * (U - 128 / 256.0) - 0.813 * (V - 128 / 256.0);
    float B = 1.164 * (Y - 16 / 256.0) + 2.017 * (U - 128 / 256.0);
    
    if (R < 0)
    {
        R = 0;
    }
    if (R > 1)
    {
        R = 1;
    }
    if (G < 0)
    {
        G = 0;
    }
    if (G > 1)
    {
        G = 1;
    }
    if (B < 0)
    {
        B = 0;
    }
    if (B > 1)
    {
        B = 1;
    }
    
    samples[id].x = R;
    samples[id].y = G;
    samples[id].z = B;
}
