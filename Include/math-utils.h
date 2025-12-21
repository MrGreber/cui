#pragma once
#include <stdio.h>

#ifndef MATH_UTILS_H
#define MATH_UTILS_H

typedef union vec2 {
    struct {
        f32 x, y;
    };
    f32 e[2];
} vec2;

typedef union vec3 {
    struct {
        f32 x, y, z;
    };
    f32 e[3];
} vec3;



typedef union vec4 {
#ifdef SIMD
    __declspec(align(16)) __m128 e;
#else
    f32 e[4];
#endif
    struct {
        f32 x, y, z, w;
    };
} vec4;

typedef union mat4 {
#ifdef SIMD
    __declspec(align(16)) vec4 v[4];
#else
    vec4 v[4];
#endif
    f32 e[16];
} mat4;


__forceinline mat4 m4_transl(const f32 x, const f32 y, const f32 z) {
    mat4 out = { 0 };

#ifndef SIMD
    out.v[0].x = 1.0f;
    out.v[1].y = 1.0f;
    out.v[2].z = 1.0f;
    out.v[3].x = x;
    out.v[3].y = y;
    out.v[3].z = z;
    out.v[3].w = 1.0f;
#endif

    return out;
}

__forceinline mat4 m4_ortho(const f32 l, const f32 r, const f32 b, const f32 t, const f32 n, const f32 f) {
    mat4 out = { 0 };

    const f32 _rl = 1.0f / (l - r);
    const f32 _tb = 1.0f / (b - t);
    const f32 _fn = 1.0f / (n - f);
    const f32 rl = l + r;
    const f32 tb = b + t;
    const f32 fn = n + f;

#ifndef SIMD
    out.e[0] = -2.0f * _rl;
    out.e[3] = rl * _rl;

    out.e[5] = -2.0f * _tb;
    out.e[7] = tb * _tb;

    out.e[10] = 2.0f * _fn;
    out.e[11] = fn * _fn;

    out.e[15] = 1.0f;
#endif

    return out;
}

__forceinline mat4 m4_rotateZ(const f32 x) {
    const f32 sX = sinf(x);
    const f32 cX = cosf(x);

    mat4 out = { 0 };
#ifndef SIMD
    out.e[0] = cX;
    out.e[1] = -sX;
    out.e[4] = sX;
    out.e[5] = cX;
    out.e[10] = 1.0f;
    out.e[15] = 1.0f;
#endif

    return out;
}

__forceinline mat4 m4_scale(const f32 x, const f32 y, const f32 z) {
    mat4 out = { 0 };

#ifndef SIMD
    out.v[0].x = x;
    out.v[1].y = y;
    out.v[2].z = z;
    out.v[3].w = 1.0f;
#endif

    return out;
}

__forceinline mat4 m4_transp(const mat4* in) {
    const f32* I = in->e;

    mat4 out = { 0 };
    f32* O = out.e;

    for (u8 i = 0; i < 4; i++) {
        for (u8 j = 0; j < 4; j++) {
            O[j + 4 * i] = I[i + 4 * j];
        }
    }

    return out;
}

__forceinline mat4 m4_inverse(const mat4* in) {
    const f32* I = in->e;

    const f32 t1 = I[10]*I[15];
    const f32 t2 = I[11]*I[14];
    const f32 t3 = I[9]*I[15];
    const f32 t4 = I[11]*I[13];
    const f32 t5 = I[9]*I[14];
    const f32 t6 = I[10]*I[13];
    const f32 t7 = I[8]*I[15];
    const f32 t8 = I[11]*I[12];
    const f32 t9 = I[8]*I[14];
    const f32 t10 = I[10]*I[12];
    const f32 t11 = I[8]*I[13];
    const f32 t12 = I[9]*I[12];
    const f32 t13 = I[6]*I[15];
    const f32 t14 = I[7]*I[14];
    const f32 t15 = I[5]*I[15];
    const f32 t16 = I[7]*I[13];
    const f32 t17 = I[5]*I[14];
    const f32 t18 = I[6]*I[13];
    const f32 t19 = I[4]*I[15];
    const f32 t20 = I[7]*I[12];
    const f32 t21 = I[4]*I[14];
    const f32 t22 = I[6]*I[12];
    const f32 t23 = I[4]*I[13];
    const f32 t24 = I[5]*I[12];
    const f32 t25 = I[6]*I[11];
    const f32 t26 = I[7]*I[10];
    const f32 t27 = I[4]*I[11];
    const f32 t28 = I[7]*I[8];
    const f32 t29 = I[4]*I[10];
    const f32 t30 = I[6]*I[8];
    const f32 t31 = I[4]*I[9];
    const f32 t32 = I[5]*I[8];
    const f32 t33 = I[5]*I[11];
    const f32 t34 = I[7]*I[9];
    const f32 t35 = I[5]*I[10];
    const f32 t36 = I[6]*I[9];

    const f32 s1 = t1 - t2;
    const f32 s2 = t3 - t4;
    const f32 s3 = t5 - t6;
    const f32 s4 = t7 - t8;
    const f32 s5 = t9 - t10;
    const f32 s6 = t11 - t12;
    const f32 s7 = t13 - t14;
    const f32 s8 = t15 - t16;
    const f32 s9 = t17 - t18;
    const f32 s10 = t19 - t20;
    const f32 s11 = t21 - t22;
    const f32 s12 = t23 - t24;
    const f32 s13 = t25 - t26;
    const f32 s14 = t33 - t34;
    const f32 s15 = t35 - t36;
    const f32 s16 = t27 - t28;
    const f32 s17 = t29 - t30;
    const f32 s18 = t31 - t32;

    const f32 m00 = +(I[5]*s1 - I[6]*s2 + I[7]*s3);
    const f32 m01 = -(I[4]*s1 - I[6]*s4 + I[7]*s5);
    const f32 m02 = +(I[4]*s2 - I[5]*s4 + I[7]*s6);
    const f32 m03 = -(I[4]*s3 - I[5]*s5 + I[6]*s6);

    const f32 m10 = -(I[1]*s1 - I[2]*s2 + I[3]*s3);
    const f32 m11 = +(I[0]*s1 - I[2]*s4 + I[3]*s5);
    const f32 m12 = -(I[0]*s2 - I[1]*s4 + I[3]*s6);
    const f32 m13 = +(I[0]*s3 - I[1]*s5 + I[2]*s6);

    const f32 m20 = +(I[1]*s7 - I[2]*s8 + I[3]*s9);
    const f32 m21 = -(I[0]*s7 - I[2]*s10 + I[3]*s11);
    const f32 m22 = +(I[0]*s8 - I[1]*s10 + I[3]*s12);
    const f32 m23 = -(I[0]*s9 - I[1]*s11 + I[2]*s12);

    const f32 m30 = -(I[1]*s13 - I[2]*s14 + I[3]*s15);
    const f32 m31 = +(I[0]*s13 - I[2]*s16 + I[3]*s17);
    const f32 m32 = -(I[0]*s14 - I[1]*s16 + I[3]*s18);
    const f32 m33 = +(I[0]*s15 - I[1]*s17 + I[2]*s18);

    const f32 inv_d = 1.0f / (I[0] * m00 + I[1] * m01 + I[2] * m02 + I[3] * m03);

    mat4 out = { 0 };
    f32* O = out.e;
    O[0] = inv_d * m00;
    O[1] = inv_d * m10;
    O[2] = inv_d * m20;
    O[3] = inv_d * m30;

    O[4] = inv_d * m01;
    O[5] = inv_d * m11;
    O[6] = inv_d * m21;
    O[7] = inv_d * m31;

    O[8] = inv_d * m02;
    O[9] = inv_d * m12;
    O[10] = inv_d * m22;
    O[11] = inv_d * m32;

    O[12] = inv_d * m03;
    O[13] = inv_d * m13;
    O[14] = inv_d * m23;
    O[15] = inv_d * m33;

    return out;
}

__forceinline mat4 m4_mul(const mat4* A, const mat4* B) {
    mat4 out = { 0 };

    for (u8 i = 0; i < 4; i++) {
            const f32 a0 = A->v[i].x;
            const f32 a1 = A->v[i].y;
            const f32 a2 = A->v[i].z;
            const f32 a3 = A->v[i].w;

            out.v[i].x = a0 * B->v[0].x + a1 * B->v[1].x + a2 * B->v[2].x + a3 * B->v[3].x;
            out.v[i].y = a0 * B->v[0].y + a1 * B->v[1].y + a2 * B->v[2].y + a3 * B->v[3].y;
            out.v[i].z = a0 * B->v[0].z + a1 * B->v[1].z + a2 * B->v[2].z + a3 * B->v[3].z;
            out.v[i].w = a0 * B->v[0].w + a1 * B->v[1].w + a2 * B->v[2].w + a3 * B->v[3].w;
    }

    return out;
}

__forceinline vec4 mv4_mul(const mat4* M, const vec4* v) {
    vec4 out = { 0 };
    const f32 v0 = v->x;
    const f32 v1 = v->y;
    const f32 v2 = v->z;
    const f32 v3 = v->w;

    out.x = v0 * M->v[0].x + v1 * M->v[1].x + v2 * M->v[2].x + v3 * M->v[3].x;
    out.y = v0 * M->v[0].y + v1 * M->v[1].y + v2 * M->v[2].y + v3 * M->v[3].y;
    out.z = v0 * M->v[0].z + v1 * M->v[1].z + v2 * M->v[2].z + v3 * M->v[3].z;
    out.w = v0 * M->v[0].w + v1 * M->v[1].w + v2 * M->v[2].w + v3 * M->v[3].w;

    return out;
}

__forceinline void print_m4(const mat4* matrix) {
    const f32* m = matrix->e;

    printf("%.2f %.2f %.2f %.2f\n""%.2f %.2f %.2f %.2f\n""%.2f %.2f %.2f %.2f\n""%.2f %.2f %.2f %.2f\n",
        m[0], m[1], m[2], m[3],
        m[4], m[5], m[6], m[7],
        m[8], m[9], m[10], m[11],
        m[12], m[13], m[14], m[15]
    );
}

__forceinline void print_v2(const vec2* vector) {
    const f32* v = vector->e;
    printf("<%.2f, %.2f>\n", v[0], v[1]);
}

__forceinline void print_v4(const vec4* vector) {
    const f32* v = vector->e;
    printf("<%.2f, %.2f, %.2f, %.2f>\n", v[0], v[1], v[2], v[3]);
}

static __forceinline f32 inv_sqrt(const f32 n) {
    i32 i;
    f32 x2, y;
    const f32 threehalfs = 1.5f;

    x2 = n * 0.5f;
    y = n;
    i = *(i32*)&y;
    i = 0x5f3759df - (i >> 1);
    y = *(f32*)&i;
    y = y * (threehalfs - (x2 * y * y));
    y = y * (threehalfs - (x2 * y * y));

    return y;
}

__forceinline void rotZvp(f32* x, f32* y, const f32 a) {
    static f32 prev_angle = 0.0f;
    static f32 c = 1.0f;
    static f32 s = 0.0f;
    if (a != prev_angle) {
        prev_angle = a;
        c = cosf(-a);
        s = sinf(-a);
    }

    const f32 _x = *x * c - *y * s;
    const f32 _y = *x * s + *y * c;
    *x = _x;
    *y = _y;
}

__forceinline vec2 v2_scale(const vec2* v, const f32 s) {
    return (vec2){v->x * s, v->y * s};
}

__forceinline f32 v3_normal(const vec3* v) {
    return v->x * v->x + v->y * v->y + v->z * v->z;
}

__forceinline void v3_normalize(vec3* v) {
    const f32 inv_mag = inv_sqrt(v3_normal(v));
    v->x *= inv_mag;
    v->y *= inv_mag;
    v->z *= inv_mag;
}

__forceinline f32 v3_dot(const vec3* a, const vec3* b) {
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

__forceinline vec3 v3_sub(const vec3* a, const vec3* b) {
    return (vec3){a->x - b->x, a->y - b->y, a->z - b->z};
}

__forceinline vec3 v3_add(const vec3* a, const vec3* b) {
    return (vec3){a->x + b->x, a->y + b->y, a->z + b->z};
}

__forceinline vec3 v3_cross(const vec3* a, const vec3* b) {
    return (vec3){
        a->y * b->z - a->z * b->y,
        a->z * b->x - a->x * b->z,
        a->x * b->y - a->y * b->x
    };
}

__forceinline mat4 lookAt2D() {
    mat4 out = { 0 };

    out.e[0] = 1.0f;
    out.e[5] = 1.0f;
    out.e[10] = 1.0f;
    out.e[14] = -1.0f;
    out.e[15] = 1.0f;

    return out;
}

#define PI 3.1415926535897932384626433832795f
#define PI2 6.283185307179586476925286766559f
#define rad(theta) (0.01745329251994329576923690768489f * (theta))
#define deg(theta) (57.295779513082320876798154814092f * (theta))

#endif //MATH_UTILS_H

