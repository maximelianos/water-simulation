#version 430 core

//in vec3 vertex_uv;
in vec3 cur_pos;
in vec3 refracted;
in vec3 reflected;
uniform sampler2D myTextureSampler;
uniform sampler2D myTextureSampler1;
uniform sampler2D myTextureSampler2;
uniform sampler2D myTextureSampler3;
uniform sampler2D myTextureSampler4;
uniform sampler2D myTextureSampler5;
uniform sampler2D myTextureSampler6;
uniform uint N_water;

float PI = 3.141592653589793238462;

out vec4 color;

uint float_to_uint(float idx) {
    uint down = uint(idx);
    if (idx < float(down) + 0.5) {
        return down;
    } else {
        return down + 1;
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

vec2 sky_intersect(vec3 cur_point, vec3 d_reflected) {
    float t, x, y, z;
    const float n = float(N_water - 1) * 0.01 * 10.0;

    // Intersect with sky, y = 1
    t = (1 - cur_point.y) / d_reflected.y;
    x = cur_point.x + t * d_reflected.x;
    y = 1;
    z = cur_point.z + t * d_reflected.z;
    x = x / n;
    z = z / n;

    return vec2(x, z);
}

vec3 separate_point(vec3 p, uint obj_idx) {
    float u, v;
    if (obj_idx == 0) {
        u = p.x;
        v = p.z;
    } else if (obj_idx == 1) {
        u = p.x;
        v = p.y;
    } else if (obj_idx == 2) {
        u = p.z;
        v = p.y;
    } else if (obj_idx == 3) {
        u = p.x;
        v = p.y;
    } else if (obj_idx == 4) {
        u = p.z;
        v = p.y;
    } else if (obj_idx == 5) {
        u = p.x;
        v = p.z;
    }
    return vec3(u, v, obj_idx);
}

void main() {
    vec4 all_uv = intersect(cur_pos, refracted); // (x, y, z, obj_idx)
    vec3 uv_obj = separate_point(all_uv.xyz, uint(all_uv.w));

    vec2 coors = uv_obj.xy;
    uint obj_idx = float_to_uint(uv_obj.z);

    if (obj_idx == 0) {
        color = texture(myTextureSampler, coors);
    } else if (obj_idx == 1) {
        color = texture(myTextureSampler1, coors);
    } else if (obj_idx == 2) {
        color = texture(myTextureSampler2, coors);
    } else if (obj_idx == 3) {
        color = texture(myTextureSampler3, coors);
    } else if (obj_idx == 4) {
        color = texture(myTextureSampler4, coors);
    } else if (obj_idx == 5) {
        color = texture(myTextureSampler5, coors);
    }

    vec2 sky_coors = sky_intersect(cur_pos, reflected);
    vec4 sky_color = texture(myTextureSampler6, sky_coors);
    float sky_alpha = 0.2;
    if (reflected.y < 0) {
        sky_alpha = 0.5;
    }

    color = color * (1 - sky_alpha) + sky_color * sky_alpha;
}
