//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

#include "common/ConstantBuffers.hlsl"
#include "common/macroGLSL.hlsl"

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float2 uv : UV;
};


/////////////////////////////////////////////////
//			INSERT
/////////////////////////////////////////////////


// Tiny Planet: Vulcan
// by Morgan McGuire @CasualEffects
//
// Texturing inspired by Paul Malin https://www.shadertoy.com/view/ldBfDR
//
// I intentionally left the aliasing on the bright locations in...
// it looks like heat distortion.

static const bool autoRotate = true;

/////////////////////////////////////////////////////////////////////////
// Morgan's standard Shadertoy helpers
#define Vector2      vec2
#define Point2       vec2
#define Point3       vec3
#define Vector3      vec3
#define Color3       vec3
#define Radiance3    vec3
#define Irradiance3  vec3
#define Power3       vec3
#define Biradiance3  vec3

static const float pi          = 3.1415926535;
static const float degrees     = pi / 180.0;
static const float inf         = 1.0 / 1e-10;

float square(float x) { return x * x; }
float pow3(float x) { return x * square(x); }
vec3 square(vec3 x) { return x * x; }
vec3 pow3(vec3 x) { return x * square(x); }
float pow4(float x) { return square(square(x)); }
float pow5(float x) { return x * square(square(x)); }
float pow8(float x) { return square(pow4(x)); }
float infIfNegative(float x) { return (x >= 0.0) ? x : inf; }

struct Ray { Point3 origin; Vector3 direction; };	
struct Material { Color3 color; float metal; float smoothness; };
struct Surfel { Point3 position; Vector3 normal; Material material; };
struct Sphere { Point3 center; float radius; Material material; };
   
/** Analytic ray-sphere intersection. */
bool intersectSphere(Point3 C, float r, Ray R, inout float nearDistance, inout float farDistance) { Point3 P = R.origin; Vector3 w = R.direction; Vector3 v = P - C; float b = 2.0 * dot(w, v); float c = dot(v, v) - square(r); float d = square(b) - 4.0 * c; if (d < 0.0) { return false; } float dsqrt = sqrt(d); float t0 = infIfNegative((-b - dsqrt) * 0.5); float t1 = infIfNegative((-b + dsqrt) * 0.5); nearDistance = min(t0, t1); farDistance  = max(t0, t1); return (nearDistance < inf); }

///////////////////////////////////////////////////////////////////////////////////
// The following are from https://www.shadertoy.com/view/4dS3Wd
float hash(float n) { return fract(sin(n) * 1e4); }
float hash(vec2 p) { return fract(1e4 * sin(17.0 * p.x + p.y * 0.1) * (0.1 + abs(sin(p.y * 13.0 + p.x)))); }
float noise(float x) { float i = floor(x); float f = fract(x); float u = f * f * (3.0 - 2.0 * f); return mix(hash(i), hash(i + 1.0), u); }
float noise(vec2 x) { vec2 i = floor(x); vec2 f = fract(x); float a = hash(i); float b = hash(i + vec2(1.0, 0.0)); float c = hash(i + vec2(0.0, 1.0)); float d = hash(i + vec2(1.0, 1.0)); vec2 u = f * f * (3.0 - 2.0 * f); return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y; }
float noise(vec3 x) { const vec3 step = vec3(110, 241, 171); vec3 i = floor(x); vec3 f = fract(x); float n = dot(i, step); vec3 u = f * f * (3.0 - 2.0 * f); return mix(mix(mix( hash(n + dot(step, vec3(0, 0, 0))), hash(n + dot(step, vec3(1, 0, 0))), u.x), mix( hash(n + dot(step, vec3(0, 1, 0))), hash(n + dot(step, vec3(1, 1, 0))), u.x), u.y), mix(mix( hash(n + dot(step, vec3(0, 0, 1))), hash(n + dot(step, vec3(1, 0, 1))), u.x), mix( hash(n + dot(step, vec3(0, 1, 1))), hash(n + dot(step, vec3(1, 1, 1))), u.x), u.y), u.z); }

#define DEFINE_FBM(name, OCTAVES) float name(vec3 x) { float v = 0.0; float a = 0.5; vec3 shift = vec3(100, 100, 100); for (int i = 0; i < OCTAVES; ++i) { v += a * noise(x); x = x * 2.0 + shift; a *= 0.5; } return v; }
DEFINE_FBM(fbm3, 3)
DEFINE_FBM(fbm5, 5)
DEFINE_FBM(fbm6, 6)

///////////////////////////////////////////////////////////////////////////////////

// The red channel defines the height, but all channels
// are used for color.
#define elevationMap iChannel0
#define colorMap iChannel1

static const float       verticalFieldOfView = 25.0 * degrees;

// Directional light source
static const Vector3     w_i             = Vector3(1.0, 1.0, -0.8) / 1.6248076;
static const Biradiance3 B_i             = Biradiance3(4, 4, 4);

static const Point3      planetCenter    = Point3(0, 0, 0);

// Including mountains
static const float       planetMaxRadius = 1.0;
static const float       maxMountain = 0.13;


static const float planetMinRadius = planetMaxRadius - maxMountain;

static mat3 planetRotation;

/** Returns color, coverage under world-space point wsPoint.
    e is the relative height of the surface. 
    k is the relative height of the ray
*/
Color3 samplePlanet(Point3 osPosition, out float e, out float k) {
	Point3 s = normalize(osPosition);    
    
    // Cylindrical map coords
    Point2 cylCoord = vec2(atan(s.z, -s.x) / pi, s.y * 0.5 + 0.5);

    // Length of osPosition = elevation
    float sampleElevation  = length(osPosition);//dot(osPosition, s);
    
    // Relative height of the sample point [0, 1]
    k = (sampleElevation - planetMinRadius) * (1.0 / maxMountain);

    // Use explicit MIPs, since derivatives
    // will be random based on the ray marching
    float lod = (iResolution.x > 800.0) ? 1.0 : 2.0;
    
    // Relative height of the surface [0, 1]
    e = mix(tx0.SampleLevel(elevationMap, cylCoord, lod).r, tx0.SampleLevel(elevationMap, s.xz, lod).r, abs(s.y));
    e = square((e - 0.2) / 0.8) * 0.5;
    
    // Soften glow at high elevations, using the mip chain
    // (also blurs 
    lod += k * 6.0;
    
    // Planar map for poles mixed into cylindrical map
    Color3 material = mix(tx1.SampleLevel(colorMap, cylCoord * vec2(2.0, 2.0), lod).rgb,
                          tx1.SampleLevel(colorMap, s.xz, lod).rgb, abs(s.y));

    // Increase contrast
    material = pow3(material);
    
    
    // Object space height of the surface
    float surfaceElevation = mix(planetMinRadius, planetMaxRadius, e);

    return material;
}


/** Relative to mountain range */
float elevation(Point3 osPoint) {
    float e, k;
    samplePlanet(osPoint, e, k);
    return e;
}





float4 main(PS_INPUT input) : SV_Target

	{// Rotate over time
		float yaw   = -((iMouse.x / iResolution.x) * 2.5 - 1.25) + (autoRotate ? -iTime * 0.02 : 0.0);
		float pitch = ((iMouse.y > 0.0 ? iMouse.y : iResolution.y * 0.3) / iResolution.y) * 2.5 - 1.25;
	 	planetRotation = 
	    	mul(mat3(cos(yaw), 0, -sin(yaw), 0, 1, 0, sin(yaw), 0, cos(yaw)),
	    	mat3(1, 0, 0, 0, cos(pitch), sin(pitch), 0, -sin(pitch), cos(pitch)));
	
	    
	    Vector2 invResolution = 1.0 / iResolution.xy;
		
		// Outgoing light
		Radiance3 L_o;
		
		Surfel surfel;	
		
		Ray eyeRay;
		eyeRay.origin = Point3(0.0, 0.0, 5.0);
		eyeRay.direction = normalize(Vector3(fragCoord.xy - iResolution.xy / 2.0, iResolution.y / (-2.0 * tan(verticalFieldOfView / 2.0))));
		    
	    Point3 hitPoint;    
	    float minDistanceToPlanet, maxDistanceToPlanet;
	    
	    bool hitBounds = intersectSphere(planetCenter, planetMaxRadius, eyeRay, minDistanceToPlanet, maxDistanceToPlanet);
	
	    // Stars
	    L_o = vec3(1,1,1) * max(0.0, hash(fragCoord * 0.5 + 10.0) - 0.999) / 0.0001 +
	        pow5(fbm3(fragCoord.xyy * -0.007 + iTime * 0.1)) * square(tx1.Sample(iChannel1, fragCoord * invResolution.x).rbr) * max(0.0, hash(fragCoord * 0.1) - 0.99) / 0.0003;
	        
	    // Background wash gradient
	    float gradCoord = (fragCoord.x + fragCoord.y) * (invResolution.x * 0.5);
	    L_o += mix(Color3(0.025, 0, 0.02), Color3(0.11, 0.06, 0.0), gradCoord) * 1.5 *
	         (0.15 + smoothstep(0.0, 1.0, 2.5 * abs(gradCoord - 0.4)));
	    
	    // Background glow ("atmosphere")
	    L_o += Color3(0.6, 0.06, 0.01) * (17.0 * pow8(max(0.0, 1.0 - length((fragCoord - iResolution.xy * 0.5) * invResolution.y))));
	
	    // Sun
	    Point2 sunCoord = vec2(-0.7, -0.5) + (fragCoord - iResolution.xy * 0.5) * invResolution.y;
	    float sunDist = max(0.0,  1.0 - length(sunCoord));
	    L_o += Color3(15, 9, 6) * (pow(sunDist, 12.0) * (1.0 + 0.25 * noise(sin(iTime * 0.1) + iTime * 0.1 + 20.0 * atan(sunCoord.x, -sunCoord.y))));
	    
	    
	    if (hitBounds) 
	    {
	        Color3 glow = Color3(0,0,0);
	        // Planet surface + atmospherics
	        
	        // March to surface
	        const int NUM_STEPS = 250;
	        
	        // Total traversal should be either 25% of the thickness of the planet,
	        // the distance between total, or the max mountain height
	        float dt = (maxDistanceToPlanet - minDistanceToPlanet) / float(NUM_STEPS);
	        Color3 material = Color3(0,0,0);
	        Vector3 wsNormal = Vector3(0,0,0);
	        Color3 p;
	        float coverage = 0.0;
	        float e = 1.0, k = 0.0;
	
	        // Take the ray to the planet's object space
	        eyeRay.origin = mul((eyeRay.origin - planetCenter), planetRotation);
	        eyeRay.direction = mul(eyeRay.direction, planetRotation);
	        
	        Point3 X;
	        for (int i = 0; (i < NUM_STEPS) && (coverage < 1.0); ++i) 
	        {
	            // Point on the ray in object space
	            X = eyeRay.origin + eyeRay.direction * (dt * float(i) + minDistanceToPlanet);
	            
	            // color, coverage
		        p = samplePlanet(X, e, k);
	            if (e > k) 
	            {
	                // Hit the surface
	                material = p;
	                coverage = 1.0;
	                
	                // Surface emission
		            glow += pow(p, p * 2.5 + 7.0) * 3e3;
	            } else 
	            {
	                // Passing through atmosphere above lava; accumulate glow
		            glow += pow(p, p + 7.0) * square(square(1.0 - k)) * 25.0;
	            }
	        }
	
	        // Planetary sphere normal
	        Vector3 sphereNormal = normalize(mul(planetRotation, X));
	            
	        // Surface normal
	        const float eps = 0.01;
	        wsNormal = mul(planetRotation,
	            normalize(Vector3(elevation(X + Vector3(eps, 0, 0)), 
	                              elevation(X + Vector3(0, eps, 0)), 
	                              elevation(X + Vector3(0, 0, eps))) - 
	                              e));
	        
	        
	        wsNormal = normalize(mix(wsNormal, sphereNormal, 0.95));
	                
	        // Lighting and compositing
	        L_o =  mix(L_o, material * 
	                   // Sun
	                   (max(dot(wsNormal, w_i) + 0.1, 0.0) * B_i + 
	                    
	                    
	                    // Rim light
	                    square(max(0.8 - sphereNormal.z, 0.0)) * Color3(2.0, 1.5, 0.5)), coverage);
	        L_o += glow;
	        
	         
	        if (false && coverage > 0.0) 
	        {
	            // Show normals
		      //  L_o = wsNormal * 0.5 + 0.5;
	          //  L_o = max(0.0, dot(wsNormal, w_i)) * vec3(1);
	        }
	    }
	    
		vec3 sq = sqrt(L_o);
	    return vec4(sq, 1.0); //maxDistanceToPlanet
	}