#pragma once

#include "vec3.h"
#include "spectral.h"
#include "RayTracing.h"


enum MaterialType : int {
    MAT_DIFFUSE = 0,
    MAT_GLASS = 1,
    MAT_EMISSIVE = 2,
    MAT_METAL = 3
};

struct scatter_result {
    bool      scattered = false;
    Direction dir;
    float     weight = 0.0f;
    // float pdf = 0.0f;
};

struct material {
    MaterialType type;
	
    // 是否是光谱材质
    bool is_spectral = false;

	// RGB材质属性
    Color        albedo;     // 漫反射反照率
    Color        emission;   // 发光

	// 光谱材质属性
    std::vector<float> spectral_albedo;
    std::vector<float> spectral_emission;

    // 介质属性
    float ior=1.0;
    float dispersion=0.0;

    // 金属粗糙度
    float fuzz = 0.0f;

	// RGB->lambda吸收率, 会clamp到[0,1]
    float albedoAt(float lambda) const {
        if (is_spectral && !spectral_albedo.empty()) {
            int i = wavelengthToChannelIndex(lambda);
            return std::clamp(spectral_albedo[i], 0.0f, 1.0f);
        }
        return rgbToReflectanceAtWavelength(albedo, lambda);
    }
	// RGB->lambda发射率, 允许大于1.0f
    float emissionAt(float lambda) const {
        if (is_spectral && !spectral_emission.empty()) {
            int i = wavelengthToChannelIndex(lambda);
            return spectral_emission[i];
        }
        // 避免kB矩阵造成负光
        return std::max(0.0f, rgbToSpectralAtWavelength(emission, lambda));
    }

	// 介质在指定波长下的折射率(柯西色散方程)
    float iorAt(float lambda) const {
        float l_um = lambda * 0.001f;
        return ior + dispersion / (l_um * l_um);
    }

    scatter_result scatter(const Direction& in_dir,
        const Point& p,
        const vec3& n,
        bool front_face,
        float lambda) const
    {
        scatter_result sr;

        switch (type) {

            case MAT_DIFFUSE: {
                Direction d = uniform_hemisphere(n);
                float cosT = d.dot(n);
                if (cosT <= 0.0f) return sr;

                sr.scattered = true;
                sr.dir = d;
                sr.weight = 2.0f * cosT * albedoAt(lambda);
                return sr;
            }

            case MAT_GLASS: {
                float current_ior = iorAt(lambda);

                float n1 = front_face ? 1.0f : current_ior;
                float n2 = front_face ? current_ior : 1.0f;
                float eta = n1 / n2;

                float cos_theta = std::fmin((-1.0f*in_dir).dot(n), 1.0f);
                float sin_theta = std::sqrt(1.0f - cos_theta * cos_theta);
                
				// 全反射
                bool cannot_refract = eta * sin_theta > 1.0f;
                Direction direction;

                if (cannot_refract || fresnel_schlick(cos_theta, n1, n2) > random_float()) {
                    direction = reflect(in_dir, n);
                }
                else {
                    direction = refract(in_dir, n, eta);
                }

                sr.scattered = true;
                sr.dir = direction;
                sr.weight = 1.0f;
                return sr;
            }
            case MAT_EMISSIVE: {
                return sr;
            }
            case MAT_METAL: {
                vec3 reflected = reflect(in_dir.normalize(), n);
                sr.dir = (reflected + random_in_unit_sphere() * fuzz).normalize();
                sr.scattered = true;
                if (sr.dir.dot(n) <= 0.0f) {
                    sr.scattered = false;
                    return sr;
                }
                sr.weight = albedoAt(lambda);
                return sr;
            }
            default: {
				return sr; // 默认不散射
            }
            return sr;
        }
    }


    static material Diffuse(const Color& c) {
        material m{};m.type = MAT_DIFFUSE;m.albedo = c;m.emission = Color(0, 0, 0);m.ior = 1.0f;return m;
    }
    static material DiffuseSpectrum(const std::vector<float>& spec) {
        material m{}; m.type = MAT_DIFFUSE; m.is_spectral = true; m.spectral_albedo = spec; return m;
    }

    static material Glass(float ior_base, float disp = 0.0f) {
        material m{}; m.type = MAT_GLASS; m.is_spectral = false; m.ior = ior_base; m.dispersion = disp; return m;
    }

    static material Emissive(const Color& rgb) {
        material m{}; m.type = MAT_EMISSIVE; m.is_spectral = false; m.emission = rgb; return m;
    }
    static material EmissiveSpectrum(const std::vector<float>& spec) {
        material m{}; m.type = MAT_EMISSIVE; m.is_spectral = true; m.spectral_emission = spec; return m;
    }

    static material Metal(const Color& c, float fuzz = 0.0f) {
        material m{};m.type = MAT_METAL;m.albedo = c;m.fuzz = fuzz;return m;
    }
    static material MetalSpectrum(const std::vector<float>& spec, float fuzz = 0.0f) {
        material m{};m.type = MAT_METAL;m.is_spectral = true;m.spectral_albedo = spec;m.fuzz = fuzz;return m;
    }
};