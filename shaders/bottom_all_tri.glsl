#version 430 compatibility
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_shader_storage_buffer_object: enable

layout(std140, binding=4) buffer cur
{
    vec4 cur_map[ ];
};

layout(std140, binding=5) buffer bottom_light_tri
{
    vec4 bottom_tri[ ];
};

layout(std140, binding=6) buffer bottom_light_pos // floor0, wall1, wall2, wall3, wall4, sphere5
{
    vec4 bottom_pos[ ];
};

layout(local_size_x=128, local_size_y=1, local_size_z=1) in;
uniform uint N_water;
uniform vec3 light;
float PI = 3.141592653589793238462;

float clamp(float x, float min_value, float max_value) {
    return x < min_value ? min_value : (x > max_value ? max_value : x);
}

float length(vec3 v) {
    return sqrt(dot(v, v));
}

vec3 norm(vec3 v) {
    return v / length(v);
}

vec3 refract(vec3 vec_in, vec3 normal, float theta) {
    // normal must be of unit length!

    // Large angle test
    vec3 n_proj = vec3(normal.x, 0, normal.z);
    float slope;
    if (length(n_proj) == 0) {
        slope = 0;
    } else {
        slope = dot(normal, norm(n_proj));
    }

    float theta_sq = theta * theta;

    float dot_v_n = dot(vec_in, normal);
    if (dot_v_n < 0 && slope < 0.2) {
        float norm_h = abs(dot_v_n);

        vec3 u = vec_in + normal * norm_h;

        float cos_a = norm_h / length(vec_in);
        float sin_a_sq = 1 - cos_a * cos_a;

        // Refract through surface
        float sin_a = sqrt(sin_a_sq);
        vec3 refracted = -normal * sqrt(theta_sq - sin_a_sq) +
                u / length(u) * sin_a;

        return refracted;
    } else {
        return vec3(0, 0, 0);
    }
}

mat4 rotationZ( float angle ) {
	return mat4(	cos(angle),		-sin(angle),	0,	0,
			 		sin(angle),		cos(angle),		0,	0,
							0,				0,		1,	0,
							0,				0,		0,	1);
}

vec4 intersect(vec3 cur_point, vec3 refracted) {
    // Returns vec4(x, y, z, int obj_idx)

    int obj_idx = 0;
    float min_t = 1000000;
    const float n = float(N_water - 1) * 0.01;
    if (refracted.y != 0) {
        float t, x, y, z, u, v;
        // FLOOR 0
        // Intersect with bottom, y = -1
        t = (-1 - cur_point.y) / refracted.y;
        if (t > 0 && t < min_t) {
            x = cur_point.x + t * refracted.x;
            y = -1;
            z = cur_point.z + t * refracted.z;
            x = x / n;
            z = z / n;
            min_t = t;
            obj_idx = 0;
        }

        if (refracted.z != 0) {
            // WALL 1
            // Intersect with front, z = 0
            t = (0 - cur_point.z) / refracted.z;
            if (t > 0 && t < min_t) {
                x = cur_point.x + t * refracted.x;
                y = cur_point.y + t * refracted.y;
                z = 0;
                x = x / n;
                y = y + 1;
                min_t = t;
                obj_idx = 1;
            }

            // WALL 3
            // Intersect with back, z = n = (N_water - 1) * 0.01
            t = (n - cur_point.z) / refracted.z;
            if (t > 0 && t < min_t) {
                x = cur_point.x + t * refracted.x;
                y = cur_point.y + t * refracted.y;
                z = n;
                x = x / n;
                y = y + 1;
                min_t = t;
                obj_idx = 3;
            }
        }

        if (refracted.x != 0) {
            // WALL 2
            // Intersect with left, x = n = (N_water - 1) * 0.01
            t = (n - cur_point.x) / refracted.x;
            if (t > 0 && t < min_t) {
                x = n;
                y = cur_point.y + t * refracted.y;
                z = cur_point.z + t * refracted.z;
                z = z / n;
                y = y + 1;
                min_t = t;
                obj_idx = 2;
            }

            // WALL 4
            // Intersect with right, x = 0
            t = (0 - cur_point.x) / refracted.x;
            if (t > 0 && t < min_t) {
                x = 0;
                y = cur_point.y + t * refracted.y;
                z = cur_point.z + t * refracted.z;
                z = z / n;
                y = y + 1;
                min_t = t;
                obj_idx = 4;
            }

            // BALL 5
            // Intersect with C=(1.5, -0.7, 0.5), r=0.3
            // t^2 (B, B) + 2t (B, A-C) + (A-C, A-C) - R^2 >= 0 - discriminant
            vec3 C = vec3(1.5, -0.7, 0.5);
            float r = 0.3;
            float a = dot(refracted, refracted);
    		float b = dot(refracted, cur_point - C); // Ommited *2
    		float c = dot(cur_point - C, cur_point - C) - r * r;
    		float d = b * b - a * c;

    		if (d > 0) {
    			float x1 = (-b - sqrt(d)) / a;
                t = x1;
                if (t > 0 && t < min_t) {
                    vec3 point_on_sphere = cur_point + t * refracted;
                    vec3 l = point_on_sphere - C;
                    vec4 q = vec4(l.xyz, 1.0);
                    q = q * rotationZ(PI / 4);
                    l = q.xyz;

                    float psi = asin(l.y / length(l));
                    float phi = atan(l.z, l.x);
                    if (phi < 0) {
                        phi += 2 * PI;
                    }
                    x = phi / (2 * PI);
                    y = -1;
                    z = psi / PI + 0.5;

                    min_t = t;
                    obj_idx = 5;
                }

    			//float x2 = (-b + sqrt(d)) / a;
    		}
        }

        return vec4(x, y, z, obj_idx);
    } else {
        return vec4(0, 0, 0, 0);
    }
}

vec3 water_normal(uint i, uint j) {
    float h = cur_map[i * N_water + j].y;
    vec3 grad_x = vec3(0.1, cur_map[i * N_water + j + 1].y - h, 0.0);
    vec3 grad_z = vec3(0.0, cur_map[(i + 1) * N_water + j].y - h, 0.1);
    vec3 normal = cross(grad_z, grad_x);
    return norm(normal);
}

float area(vec3 p1, vec3 p2, vec3 p3) {
    if (p1.y == 0 || p2.y == 0 || p3.y == 0) {
        return 1000000;
    }
    vec3 a = p1 - p2;
    vec3 b = p3 - p2;
    vec3 n = cross(a, b);
    vec3 p = cross(n, a);
    p = p / length(p);
    return abs(length(a) * dot(b, p)) / 2; // standard formula: a * h
}

void main()
{
    uint gid = gl_GlobalInvocationID.x; // Index of square
    uint i = gid / (N_water - 1);
    uint j = gid - i * (N_water - 1);

    vec3 p1 = vec3( // i, j = 0, 0
        float(j) * 0.01,
        cur_map[i * N_water + j].y,
        float(i) * 0.01
    );
    vec3 p2 = vec3( // i, j = 1, 0
        float(j) * 0.01,
        cur_map[(i + 1) * N_water + j].y,
        float(i + 1) * 0.01
    );
    vec3 p3 = vec3( // i, j = 0, 1
        float(j + 1) * 0.01,
        cur_map[i * N_water + j + 1].y,
        float(i) * 0.01
    );
    vec3 p4 = vec3( // i, j = 1, 1
        float(j + 1) * 0.01,
        cur_map[(i + 1) * N_water + j + 1].y,
        float(i + 1) * 0.01
    );

    // Normal of water, +x +z components
    vec3 n1 = water_normal(i, j);
    vec3 n2 = water_normal(i + 1, j);
    vec3 n3 = water_normal(i, j + 1);
    vec3 n4 = water_normal(i + 1, j + 1);

    // Light source
    float theta = 1.3;

    vec3 r1 = refract(p1 - light, n1, theta);
    vec3 r2 = refract(p2 - light, n2, theta);
    vec3 r3 = refract(p3 - light, n3, theta);
    vec3 r4 = refract(p4 - light, n4, theta);

    vec4 p1_bottom = intersect(p1, r1); // vec4(x, y, z, obj_idx)
    vec4 p2_bottom = intersect(p2, r2);
    vec4 p3_bottom = intersect(p3, r3);
    vec4 p4_bottom = intersect(p4, r4);

    bottom_pos[i * N_water + j] = p1_bottom;
    if (i == N_water - 2) {
        bottom_pos[(i + 1) * N_water + j] = p2_bottom; // i + 1, j
        if (j == N_water - 2) {
            bottom_pos[(i + 1) * N_water + j + 1] = p4_bottom; // i + 1, j + 1
        }
    }
    if (j == N_water - 2) {
        bottom_pos[i * N_water + j + 1] = p3_bottom; // i, j + 1
    }

    bottom_tri[gid * 2 + 0].y = area(p1_bottom.xyz, p2_bottom.xyz, p3_bottom.xyz)*100000;
    bottom_tri[gid * 2 + 1].y = area(p3_bottom.xyz, p2_bottom.xyz, p4_bottom.xyz)*100000;
}
