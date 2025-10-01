#version 450

/**
* Creative Commons CC0 1.0 Universal (CC-0)
* A simple christmas tree made from 2D points.
* Refactored for Vulkan - hardcoded version
*/

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform PushConstants {
    float time;
} pc;

#define PI 3.1415926535
#define TAU 6.2831853071
#define ROTATE(v, x) mat2(cos(x), sin(x), -sin(x), cos(x)) * v
#define REMAP_HALF_NDC(x, c, d) (((x + 0.5) * (d - c)) + c)

#define N 1024.0
#define N_ONE_QUARTER N * 0.25
#define N_OFFSET 1.0
#define STAR_N 7.0
#define STAR_BEAM_THICKNESS 3.0

// Hardcoded values
#define RESOLUTION vec2(800.0, 600.0)  // Change to your window size
#define TIME_SCALE 0.5                  // Animation speed

const vec3 LIGHT_COLORS[3] = vec3[3](
    vec3(1.0,  0.05,  0.05), 
    vec3(0.05, 1.0,   0.05), 
    vec3(1.0,  0.25,  0.05)
);

float Hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec2 Hash21(float p)
{
    vec3 p3 = fract(vec3(p) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx + p3.yz) * p3.zy);
}

float SignedDistanceNStar2D(in vec2 p, in float r, in float an, in float bn, in vec2 acs, in float m)
{
    float en = PI / m;
    vec2 ecs = vec2(cos(en), sin(en));
    p = length(p) * vec2(cos(bn), abs(sin(bn)));

    p -= r * acs;
    p += ecs * clamp(-dot(p, ecs), 0.0, r * acs.y / ecs.y);
    return length(p) * sign(p.x);
}

void DrawStar(in vec2 uv, in float t, inout vec3 outColor)
{
    uv -= vec2(0.001, 0.225);
    uv = ROTATE(uv, t * 0.75);
    
    float an = PI / STAR_N;
    float bn = mod(atan(uv.x, uv.y), 2.0 * an) - an;
    vec2 acs = vec2(cos(an), sin(an));
    
    outColor += 5e-4 / pow(abs(SignedDistanceNStar2D(uv, 0.01, an, bn, acs, STAR_N * 0.5)), 1.23) * LIGHT_COLORS[2];
    outColor += smoothstep(STAR_BEAM_THICKNESS / max(RESOLUTION.x, RESOLUTION.y), 0.0, SignedDistanceNStar2D(uv, 1.5, an, bn, acs, STAR_N)) * 
        LIGHT_COLORS[2] * smoothstep(0.75, -5.0, length(uv));
}

void DrawTree(in vec2 uv, in float t, inout vec3 outColor)
{
    float u, theta, pointHeight, invN = 1.0 / N;
    vec2 st, hash, layer;
    vec3 pointOnCone, pointColor = vec3(1.0);
    const vec2 radius = vec2(1.5, 3.2);
    uvec3 colorThreshold;
    
    for (float i = N_OFFSET; i < N; ++i)
    {
        hash = Hash21(2.0 * TAU * i);
        
        colorThreshold.x = uint(hash.x < 0.45);
        colorThreshold.y = 1u - colorThreshold.x;
        colorThreshold.z = uint(hash.x > 0.9);
        pointColor = vec3(colorThreshold | colorThreshold.z);
        
        u = i * invN;
        theta = 1609.0 * hash.x + t * 0.5;
        pointHeight = 1.0 - u;
        
        layer = vec2(3.2 * mod(i, N_ONE_QUARTER) * invN, 0.0);
        pointOnCone = 0.5 * (radius.xyx - layer.xyx) * vec3(pointHeight * cos(theta), u - 0.5, pointHeight * sin(theta));
        
        st = uv * (REMAP_HALF_NDC(pointOnCone.z, 0.5, 1.0) + hash.y) * 4.5;
        
        outColor += REMAP_HALF_NDC(pointOnCone.z, 3.0, 0.6) * 
            2e-5 / pow(length(st - pointOnCone.xy), 1.7) * pointColor;
    }
}

void main()
{
    // Use gl_FragCoord which is built-in
    vec2 fragCoord = gl_FragCoord.xy;
    // Flip Y coordinate to fix upside-down rendering
    fragCoord.y = RESOLUTION.y - fragCoord.y;
    vec2 uv = (fragCoord - RESOLUTION.xy * 0.5) / RESOLUTION.y;
    vec3 outColor = vec3(0.005, 0.01, 0.03);
    
    // Fake time using fragment position (creates static pattern)
    // Or use a constant for a frozen frame
    float time = pc.time;
    // fragCoord.x * 0.001 + fragCoord.y * 0.001; // Creates varying pattern
    // float time = 0.0; // Uncomment for static image
    
    DrawTree(uv, time * TIME_SCALE, outColor);
    DrawStar(uv, time * TIME_SCALE, outColor);
    
    float vignette = dot(uv, uv);
    vignette *= vignette;
    vignette = 1.0 / (vignette * vignette + 1.0);

    fragColor = vec4(pow(outColor * vignette, vec3(0.4545)), 1.0) - Hash12(fragCoord.xy * time + RESOLUTION.yy) * 0.04;

    float r = fragColor.r;
    float b = fragColor.b;

    fragColor.r = b;
    fragColor.b = r;
}
