#define M_PI 3.1415926535897932384626433832795028841971693993751


float evaluateGaussian(in float x, in float sigma, in float mu)
{
	return exp(-((x - mu)*(x - mu)) / (2.0*sigma*sigma));
}


float computePolarAngle(in vec2 p)
{
	float x = p.x;
	float z = p.y;

	// push it from (-180,180) to (0,360)
	float phi = 180.0+degrees(atan(z, x)); 

	return phi;
}


// v is assumed to be normalized
vec2 getSphericalCoords(in vec3 v)
{
	float theta = atan(v.z, v.x);
	float phi = asin(v.y); // zero at equator, elevation 0
	return vec2(theta, phi);
}


// X: world point, P: Projection matrix, dim: image dimensions (pixels)
vec2 computeTextureCoord(in vec4 X, in mat4 P, in vec2 dim)
{
	// Convert from world coordinates (OpenGL) to camera coordinates (COLMAP).
	vec4 cameraCoord = P * X;

	// Project from camera to image coordinates.
	vec2 imageCoord = cameraCoord.xy / cameraCoord.z;

	// Normalise image coordinates to texture coordinates.
	vec2 texCoords = imageCoord / dim;

	// Flip y-coordinate (COLMAP assumes y-down, OpenGL textures are y-up.
	texCoords.y = 1. - texCoords.y;

	return texCoords;
}


// Computes the texture coordinates (in [0,1]^2) for a point X projected in the equirectangular camera P.
vec2 computeEquirectTextureCoord(in vec4 X, in mat4 P)
{
	// Direction to the point X, on the unit sphere.
	vec4 dir = normalize(P * X);
	vec2 sph_dir = getSphericalCoords(dir.xyz);

	// Azimuth angle theta [-pi, pi] => u texture coordinates [0, 1]
	float thetaTex = 0.75 - sph_dir.x / (2 * M_PI);
	thetaTex = mod(thetaTex + 1, 1); // wrap to 0..1

	// Elevation angle phi [-pi/2, pi/2] => v texture coordinates [0, 1]
	float phiTex = 0.5 - sph_dir.y / M_PI;

	return vec2(thetaTex, phiTex);
}


vec3 colourFetchArray(in sampler2DArray A, in vec2 tex, in int layer)
{
	// Our images assume the origin is at the top-left, but OpenGL assumes bottom-left.
	// So invert the y-coordinate before the texture lookup.
	if ((tex.x >= 0 && tex.x <= 1) && (tex.y >= 0 && tex.y <= 1))
		return texture(A, vec3(tex.x, 1. - tex.y, layer)).xyz;

	return vec3(0);
}


// Fetch a flow vector from a texture2DArray.
vec2 fetchFlow(in sampler2DArray flows, in vec2 tex, in int layer)
{
	// Our flow fields assume the origin is at the top-left, but OpenGL assumes bottom-left.
	// So invert the y-coordinate before lookup, and invert the y-component of the flow.
	return texture(flows, vec3(tex.x, 1. - tex.y, layer)).xy * vec2(1., -1.);

	// If flow fields are flipped during loading, can use this:
	// return texture(flows, vec3(tex.x, tex.y, layer)).xy;
}


mat4 matrixFetch(in sampler2DArray projectionMatrices, in int index)
{
	mat4 ret;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			// texture coordinate in [0,1] -> / 3.0 !
			ret[i][j] = float(texture(projectionMatrices, vec3(float(i) / 3.0, float(j) / 3.0, index)).x);
		}
	}
	return ret;
}


vec3 textureFetch(in sampler2D I, in vec2 tex)
{
	return texture(I, tex).xyz;
}


float intersectCircle(in float r, in vec3 pos, in vec3 dir)
{
	vec2 inter = vec2(-1.0);
	float a, b, c;
	a = dir.x * dir.x + dir.z * dir.z;
	b = pos.x*dir.x + pos.z*dir.z;
	c = pos.x*pos.x + pos.z*pos.z - r*r;
	float lambda0, lambda1;
	lambda0 = -(b / a) + sqrt((b*b) / (a*a) - (c / a));
	lambda1 = -(b / a) - sqrt((b*b) / (a*a) - (c / a));
	float lambda = max(lambda0, lambda1);
	if (lambda > 0.0)
	{
		return lambda;
	}
	return 0.0;
}


vec3 intersectCylinder(in float r, in vec3 pos, in vec3 dir, in vec3 normal)
{
	// project direction into cylinderspace
	float a = dot(dir, dir);
	float b = dot(pos, dir);
	float c = dot(pos, pos) - r * r;
	
	float test = (b * b) / (a * a) - (c / a);
	if(test < 0) // no intersection
	{
		return vec3(0., 0., 0.);
	}

	float t1 = -(b / a) + sqrt((b * b) / (a * a) - (c / a));
	float t2 = -(b / a) - sqrt((b * b) / (a * a) - (c / a)); // unused
	float t = 0;
	if (t1 > 0)
	{
		t = t1; // furthest intersection along direction 'dir'
		//if (t2 > 0) // looking at cylinder from outside
		//{
		//	if (t2 < t1) // always the case
		//	{
		//		t = t2;
		//	}
		//}
	}

	vec3 inter = vec3(-1.0);
	inter.x = pos.x + t * dir.x;
	inter.z = pos.z + t * dir.z;
	return inter;
}


vec3 intersectSphereCentredAtOrigin(in float radius, in vec3 pos, in vec3 dir)
{
	float a = dot(dir, dir);
	float b = 2 * dot(pos, dir);
	float c = dot(pos, pos) - radius * radius;
	
	float t = (b * b) - (4. * a * c);
	if(t < 0.) // no intersections
		return vec3(0.001, 0., 0.); // sentinel value to help debugging
	
	// Return the first intersection if it is along the ray direction.
	float t1 = (-b - sqrt(t)) / (2 * a);
	if(t1 > 0) // < 0 means behind the ray's starting point
		return pos + t1 * dir;

	// Return the second intersection if it is along the ray direction.
	float t2 = (-b + sqrt(t)) / (2 * a);
	if(t2 > 0) // < 0 means behind the ray's starting point
		return pos + t2 * dir;

	// Both intersections are behind the ray's starting point.
	return vec3(0., 0.001, 0.); // sentinel value to help debugging
}


vec3 colourWheel(in int k)
{
	// relative lengths of color transitions:
	// these are chosen based on perceptual similarity
	// (e.g. one can distinguish more shades between red and yellow
	//  than between yellow and green)
	int RY = 15;
	int YG = 6;
	int GC = 4;
	int CB = 11;
	int BM = 13;
	int MR = 6;

	int i = k;
	if (i < RY) return vec3(1.f, float(i) / float(RY), 0.f);       else i -= RY;
	if (i < YG) return vec3(1.f - float(i) / float(YG), 1.f, 0.f); else i -= YG;
	if (i < GC) return vec3(0.f, 1.f, float(i) / float(GC));       else i -= GC;
	if (i < CB) return vec3(0.f, 1.f - float(i) / float(CB), 1.f); else i -= CB;
	if (i < BM) return vec3(float(i) / float(BM), 0.f, 1.f);       else i -= BM;
	if (i < MR) return vec3(1.f, 0.f, 1.f - float(i) / float(MR)); else i -= MR;

	return vec3(0., 0., 0.);
}


vec3 flowColour(in vec2 flow, in float maxRad)
{
	flow /= maxRad;

	float radius = float(length(flow));
	//float angle = computePolarAngle(flow);
	//float angle = (computePolarAngle(flow) / 180.0) * 3.14159265;
	float angle = atan(-flow.y, -flow.x) / 3.14159265;
	//float angle = float((atan(flow[0], flow[1]) / 3.14159265));
	//return vec3(angle);

	// Note: The original Middlebury flow colour code uses (ncols - 1) == 54 in the line below,
	//       but this causes a colour discontinuity at flow (positive, +-epsilon). For flow
	//       (positive, -epsilon), angle is slightly less than 1.0 (but never quite reaches it),
	//       so k0 <= 53 (because int rounds down). With flow (positive, 0), angle = -1 and k0 = 0.
	//       k0 can never be 54, so this colour wheel position is skipped, leading to a discontinuty.
	//       By multiplying by 55, this problem is avoided.
	float fk = (angle + 1.0f) / 2.0f * 55.f;
	int k0 = int(fk) % 55;
	int k1 = (k0 + 1) % 55;
	float f = float((fk - floor(fk))); // fract(fk)
	//float f = fk - k0; // original code (equivalent)

	vec3 col = (1.f - f) * colourWheel(k0) + f * colourWheel(k1);
	if (radius <= 1.) col = vec3(1., 1., 1.) - radius * (vec3(1., 1., 1.) - col); // increase saturation with radius
	else              col *= .75; // out of range

	return col;
	//return vec3(flow, 0.1);
}
