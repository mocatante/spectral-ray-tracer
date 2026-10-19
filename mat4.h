#pragma once
#include <cmath>
#include "RayTracing.h"

struct mat4 {
    // 列主序
    float m[16];


	// 单位矩阵
    static mat4 identity() {
        mat4 r;
        for (int i = 0; i < 16; ++i) r.m[i] = 0.0f;
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
        return r;
    }

	// 平移矩阵
    static mat4 translate(float tx, float ty, float tz) {
        mat4 r = identity();
        r.m[12] = tx; r.m[13] = ty; r.m[14] = tz;
        return r;
    }

	// 绕 Y 轴逆时针旋转矩阵
    static mat4 rotate_y(float degrees) {
        mat4 r = identity();
        float rad = degrees * PI_F / 180.0f;
        float c = std::cos(rad), s = std::sin(rad);
        r.m[0] = c; r.m[2] = -s;
        r.m[8] = s; r.m[10] = c;
        return r;
    }

	// 矩阵左乘
    mat4 operator*(const mat4& b) const {
        mat4 res;
        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                // res[col][row] = sum_k A[k][row] * B[col][k]
                res.m[col * 4 + row] =
                    m[0 * 4 + row] * b.m[col * 4 + 0] +
                    m[1 * 4 + row] * b.m[col * 4 + 1] +
                    m[2 * 4 + row] * b.m[col * 4 + 2] +
                    m[3 * 4 + row] * b.m[col * 4 + 3];
            }
        }
        return res;
    }

    // 矩阵转置
    mat4 transpose() const {
        mat4 res;
        res.m[0] = m[0]; res.m[1] = m[4]; res.m[2] = m[8];  res.m[3] = m[12];
        res.m[4] = m[1]; res.m[5] = m[5]; res.m[6] = m[9];  res.m[7] = m[13];
        res.m[8] = m[2]; res.m[9] = m[6]; res.m[10] = m[10]; res.m[11] = m[14];
        res.m[12] = m[3]; res.m[13] = m[7]; res.m[14] = m[11]; res.m[15] = m[15];
        return res;
    }

    // 矩阵求逆
    mat4 inverse() const {
        mat4 dest;
        float inv[16], det;
        inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
        inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
        inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
        inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
        inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
        inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
        inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
        inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
        inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
        inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
        inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
        inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
        inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
        inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
        inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
        inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

        det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
        if (det == 0.0f) return identity();
        det = 1.0f / det;

        for (int i = 0; i < 16; i++) dest.m[i] = inv[i] * det;
        return dest;
    }


	// 对点变换
    vec3 transform_point(const vec3& p) const {
        return vec3(
            m[0] * p.x() + m[4] * p.y() + m[8] * p.z() + m[12],
            m[1] * p.x() + m[5] * p.y() + m[9] * p.z() + m[13],
            m[2] * p.x() + m[6] * p.y() + m[10] * p.z() + m[14]
        );
    }

	// 对向量变换
    vec3 transform_vector(const vec3& v) const {
        return vec3(
            m[0] * v.x() + m[4] * v.y() + m[8] * v.z(),
            m[1] * v.x() + m[5] * v.y() + m[9] * v.z(),
            m[2] * v.x() + m[6] * v.y() + m[10] * v.z()
        );
    }
};

//列0  列1  列2  列3
//m[0] m[4] m[8]  m[12]
//m[1] m[5] m[9]  m[13]
//m[2] m[6] m[10] m[14]
//m[3] m[7] m[11] m[15]