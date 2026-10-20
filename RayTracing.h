#pragma once
// 此文件定义了光线追踪的辅助数据, 函数

#include <random>

#include "vec3.h"
#include "spectral.h"

constexpr float  PI_F = 3.14159265358979323846f;

inline float random_float() {
    static thread_local std::mt19937 rng(std::random_device{}());
    // 取高 24 位得到 [0, 1)
    return static_cast<float>(rng() >> 8) * (1.0f / 16777216.0f);
}
inline float random_float(float min, float max) {
    return min + (max - min) * random_float();
}
inline int random_int(int min, int max) {
	return min + static_cast<int>(random_float() * (max - min + 1));
}

// 镜面反射
inline vec3 reflect(const vec3& v, const vec3& n) {
    return v - n * 2.0f * v.dot(n);
}

// 斯涅尔定律 
// uv: 入射光线方向
// n: 是法线
// etai_over_etat: 是折射率比
inline vec3 refract(const vec3& uv, const vec3& n, float etai_over_etat) {
    float cos_theta = std::fmin((-1.0f*uv).dot(n), 1.0f);
    vec3 r_out_perp = (uv + n * cos_theta) * etai_over_etat;
    vec3 r_out_parallel = n * -std::sqrt(std::fabs(1.0f - r_out_perp.dot(r_out_perp)));
    return r_out_perp + r_out_parallel;
}

// 菲涅尔反射近似
// cosine: 是入射角的余弦
// ior: 是相对折射率
inline float fresnel_schlick(float cosine, float n1, float n2) {
    float r0 = (n1 - n2) / (n1 + n2);
    r0 = r0 * r0;
    return r0 + (1.0f - r0) * std::pow(1.0f - cosine, 5.0f);
}

inline void build_onb(const vec3& n, vec3& t, vec3& b) {
    float sign = (n.z() >= 0.0f) ? 1.0f : -1.0f;
    float a = -1.0f / (sign + n.z());
    float c = n.x() * n.y() * a;

    t = vec3(1.0f + sign * n.x() * n.x() * a,
        sign * c,
        -sign * n.x());
    b = vec3(c,
        sign + n.y() * n.y() * a,
        -n.y());
}

// 单位球内随机向量, 用于模糊镜面
inline vec3 random_in_unit_sphere() {
    while (true) {
        vec3 p(random_float() * 2.0f - 1.0f,
            random_float() * 2.0f - 1.0f,
            random_float() * 2.0f - 1.0f);
        if (p.dot(p) <= 1.0f) return p;
    }
}

// 均匀半球采样，PDF = 1 / (2π)
inline Direction uniform_hemisphere(const vec3& n) {
    float u1 = random_float();
    float u2 = random_float();

    float z = u1;                                 // 在 [0,1] 均匀
    float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
    float phi = 2.0f * PI_F * u2;

    float x = r * std::cos(phi);
    float y = r * std::sin(phi);

    vec3 nn = n.normalize();
    vec3 t, b;
    build_onb(nn, t, b);
    return (t * x + b * y + nn * z).normalize();
}

// 余弦加权半球采样，PDF = cos(theta) / PI
inline Direction cosine_sample_hemisphere(const vec3 & n) {
    float u1 = random_float();
    float u2 = random_float();

    float r = std::sqrt(u1);
    float phi = 2.0f * PI_F * u2;

    float x = r * std::cos(phi);
    float y = r * std::sin(phi);
    float z = std::sqrt(std::max(0.0f, 1.0f - u1));

    vec3 nn = n.normalize();
    vec3 t, b;
    build_onb(nn, t, b);
    return (t * x + b * y + nn * z).normalize();
}

// 根据温度生成黑体辐射光谱
inline std::vector<float> make_blackbody_spectrum(float temp_K) {
    std::vector<float> spec(SpectralColor::kChannelCount);
    const float h = 6.62607e-34f;   // 普朗克常数
    const float c = 2.99792e8f;     // 光速
    const float k = 1.38065e-23f;   // 玻尔兹曼常数
    float max_val = 0.0f;

    for (int i = 0; i < SpectralColor::kChannelCount; ++i) {
        float lambda_nm = SpectralColor::kWavelengthMinNm + i * SpectralColor::kWavelengthStepNm;
        float lambda_m = lambda_nm * 1e-9f;
        // 普朗克公式
        float num = 2.0f * h * c * c / std::pow(lambda_m, 5.0f);
        float den = std::exp((h * c) / (lambda_m * k * temp_K)) - 1.0f;
        spec[i] = num / den;
        if (spec[i] > max_val) max_val = spec[i];
    }

    // 将峰值归一化到 1.0，方便后续乘上倍数
    for (int i = 0; i < SpectralColor::kChannelCount; ++i) spec[i] /= max_val;
    return spec;
}

// 高斯光谱
// center_nm: 峰值波长
// width_nm: 峰值宽度
inline std::vector<float> make_gaussian_spectrum(float center_nm, float width_nm) {
    std::vector<float> spec(SpectralColor::kChannelCount);
    for (int i = 0; i < SpectralColor::kChannelCount; ++i) {
        float lambda_nm = SpectralColor::kWavelengthMinNm + i * SpectralColor::kWavelengthStepNm;
        float d = (lambda_nm - center_nm) / width_nm;
        spec[i] = std::exp(-0.5f * d * d);
    }
    return spec;
}


// 带通滤波光谱
inline std::vector<float> make_bandpass_spectrum(float min_nm, float max_nm) {
    std::vector<float> spec(SpectralColor::kChannelCount, 0.0f);
    for (int i = 0; i < SpectralColor::kChannelCount; ++i) {
        float lambda_nm = SpectralColor::kWavelengthMinNm + i * SpectralColor::kWavelengthStepNm;
        if (lambda_nm >= min_nm && lambda_nm <= max_nm) {
            spec[i] = 1.0f;
        }
    }
    return spec;
}


// 模拟真实的黄金/红铜颜色
inline std::vector<float> make_metal_gold_spectrum() {
    std::vector<float> spec(SpectralColor::kChannelCount);
    for (int i = 0; i < SpectralColor::kChannelCount; ++i) {
        float lambda_nm = SpectralColor::kWavelengthMinNm + i * SpectralColor::kWavelengthStepNm;

        // 金属基础反射率通常在 0.9 左右
        float base = 0.9f;

        // 黄金在 450nm 左右吸收蓝光，导致反射率凹陷
        float d = (lambda_nm - 450.0f) / 40.0f;
        spec[i] = base - 0.6f * std::exp(-0.5f * d * d);
    }
    return spec;
}