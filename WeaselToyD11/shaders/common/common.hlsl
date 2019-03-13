#define NUMBER_OF_CHANNELS 4

#if WT_D3D11

Texture2D tx0 : register(t0);
Texture2D tx1 : register(t1);
Texture2D tx2 : register(t2);
Texture2D tx3 : register(t3);

SamplerState iChannel0 : register(s0);
SamplerState iChannel1 : register(s1);
SamplerState iChannel2 : register(s2);
SamplerState iChannel3 : register(s3);

#elif WT_D3D9

Texture2D tx0;
Texture2D tx1;
Texture2D tx2;
Texture2D tx3;

sampler iChannel0 : register(s0);
sampler iChannel1 : register(s1);
sampler iChannel2 : register(s2);
sampler iChannel3 : register(s3);

#endif