#pragma once

struct vec3 {
    float v[3];

    vec3() : v{ 0,0,0 } {}
    vec3(float x, float y, float z) : v{ x,y,z } {}

	float x() const { return v[0]; }
	float y() const { return v[1]; }
	float z() const { return v[2]; }

    // 下标索引
    float& operator[](int i) { return v[i]; }
    const float& operator[](int i) const { return v[i]; }

	// 向量运算
    vec3 operator+(const vec3& r) const { return { v[0] + r.v[0], v[1] + r.v[1], v[2] + r.v[2] }; }
    vec3 operator-(const vec3& r) const { return { v[0] - r.v[0], v[1] - r.v[1], v[2] - r.v[2] }; }

    vec3 operator*(float s) const { return { v[0] * s, v[1] * s, v[2] * s }; }
    vec3 operator/(float s) const { return { v[0] / s, v[1] / s, v[2] / s }; }
    
    // 逐分量
    vec3 operator*(const vec3& r) const {
        return { v[0] * r.v[0], v[1] * r.v[1], v[2] * r.v[2] };
    }
    // 点积
    float dot(const vec3& r) const {
        return v[0] * r.v[0] + v[1] * r.v[1] + v[2] * r.v[2];
    }
	// 叉积
    vec3 cross(const vec3& o) const {
        return { v[1] * o.v[2] - v[2] * o.v[1],
                v[2] * o.v[0] - v[0] * o.v[2],
                v[0] * o.v[1] - v[1] * o.v[0] };
    }

    vec3 operator-() const { return { -v[0], -v[1], -v[2] }; }
    vec3& operator+=(const vec3& r) { v[0] += r.v[0]; v[1] += r.v[1]; v[2] += r.v[2]; return *this; }
    vec3& operator-=(const vec3& r) { v[0] -= r.v[0]; v[1] -= r.v[1]; v[2] -= r.v[2]; return *this; }

    float length() const {
        return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	}

    vec3 normalize() const {
        float len_sq = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
        if (len_sq < 1e-20f) return vec3(0.0f, 0.0f, 0.0f);
        float inv_len = 1.0f / std::sqrt(len_sq);
        return vec3(v[0], v[1], v[2])* inv_len;
	}
};

inline vec3 operator*(float s, const vec3& v) { return v * s; }


// 类型别名
using Direction = vec3;
using Color = vec3;
using Point = vec3;