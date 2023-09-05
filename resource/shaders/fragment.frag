#version 330 core

out vec4 FragColor;

in VS_OUT 
{
	vec3 FragmentPosWorld;
	vec3 Normal;
	vec2 TexCoord;
} fs_in;

uniform vec3 CamPosWorld;
uniform vec3 LightPosWorld;

uniform sampler2D DiffuseTexture;
uniform samplerCube ShadowMap;

uniform float FarPlane;

#define SAMPLE_COUNT 20
vec3 SampleOffsetDirections[SAMPLE_COUNT] = vec3[]
(
    vec3( 1, 1, 1), vec3( 1, -1, 1), vec3(-1, -1, 1), vec3(-1, 1, 1),
    vec3( 1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
    vec3( 1, 1, 0), vec3( 1, -1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
    vec3( 1, 0, 1), vec3(-1, 0, 1), vec3( 1, 0, -1), vec3(-1, 0, -1),
    vec3( 0, 1, 1), vec3( 0, -1, 1), vec3( 0, -1, -1), vec3( 0, 1, -1)  
);

float CalculateShadow(vec3 FragmentPosWorld, vec3 LightDir, vec3 Normal)
{
    vec3 LightToFrag = FragmentPosWorld - LightPosWorld;

    float CurrentDepth = length(LightToFrag);

    float Bias = 0.15;
    float Shadow = 0.0;
    int Samples = SAMPLE_COUNT;
    float Radius = 0.04;
    for(int I = 0;  I < Samples;  I++) {
        float ShadowMapDepth = texture(ShadowMap, LightToFrag + SampleOffsetDirections[I] * Radius).r;
        ShadowMapDepth *= FarPlane;
        Shadow += CurrentDepth - Bias > ShadowMapDepth ? 1.0 : 0.0;
    }

    Shadow /= float(Samples);

    return (Shadow);
}

void main()
{	
    vec3 Color = texture(DiffuseTexture, fs_in.TexCoord).rgb;
    vec3 LightColor = vec3(1.0);
    vec3 Normal = normalize(fs_in.Normal);

    vec3 Ambient = 0.3 * Color;

    vec3 LightDir = normalize(LightPosWorld - fs_in.FragmentPosWorld);
    float Diff = max(dot(LightDir, Normal), 0.0);
    vec3 Diffuse = Diff * LightColor;

    vec3 ViewDir = normalize(CamPosWorld - fs_in.FragmentPosWorld);
    vec3 Reflection = reflect(-LightDir, Normal);
    float Spec = pow(max(dot(Reflection, ViewDir), 0.0), 32.0);
    vec3 Specular = Spec * LightColor;

    float Shadow = CalculateShadow(fs_in.FragmentPosWorld, LightDir, Normal);
    vec3 Lighting = (Ambient + (1.0 - Shadow)*(Diffuse + Specular)) * Color;

	FragColor = vec4(Lighting, 1.0);
}