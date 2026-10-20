#pragma once
#include <fstream>
#include <cmath>
#include <algorithm>
#include <iostream>


#include "RayTracing.h"
#include "spectral.h"
#include "vec3.h"
#include "ray.h"
#include "scene.h"
#include "material.h"

struct camera {
    int samples_per_pixel;

    Point position;
    Point target;

    float focus_distance;
    float fov_x_deg;

    // 宽高比 = width / height
    float ratio;

    vec3 up;
    vec3 right;
    vec3 lookAt;

    // 每个像素在屏幕空间的两个方向上的世界坐标步长
    vec3  u_step;        // 沿 right 方向（水平）
    vec3  v_step;        // 沿 up    方向（竖直，向下为负）

    int width;
    int height;

    Point pixel00_center;
	int maxDepth;

    camera(const Point& position,
        const Point& target,
        vec3 up,
        float focus_distance,
        float fov_x_deg,
        float ratio,
        int width,
        int samples_per_pixel,
        int maxDepth)
        : position(position),
        target(target),
        up(up),
        focus_distance(focus_distance),
        fov_x_deg(fov_x_deg),
        ratio(ratio),
        width(width),
        samples_per_pixel(samples_per_pixel),
		maxDepth(maxDepth){
        lookAt = (target - position).normalize();

        if (std::abs(lookAt.dot(up)) > 0.999f) {
            vec3 fallback = (std::abs(lookAt.x()) < 0.9f)
                ? vec3(1, 0, 0)
                : vec3(0, 1, 0);
            up = fallback;
        }

        right = lookAt.cross(up).normalize();
        up = right.cross(lookAt).normalize();

        float fov_x_rad = fov_x_deg * PI_F / 180.0f;
        float viewport_width = 2.0f * std::tan(fov_x_rad * 0.5f) * focus_distance;

        float pixel_size = viewport_width / static_cast<float>(width);
        height = std::max(1, static_cast<int>(std::lround((viewport_width / ratio) / pixel_size)));
        float viewport_height = pixel_size * static_cast<float>(height);

        u_step = right * pixel_size;
        v_step = -1.0 * up * pixel_size;

        Point view_center = position + lookAt * focus_distance;

        Point viewport_top_left = 
            view_center - right * (viewport_width * 0.5f)
            + up * (viewport_height * 0.5f);

        pixel00_center = viewport_top_left
            + right * (pixel_size * 0.5f)
            - up * (pixel_size * 0.5f);
    }

    // 第 (i, j) 个像素的中心点
    Point pixel_center(int i, int j) const {
        return pixel00_center
            + u_step * static_cast<float>(i)
            + v_step * static_cast<float>(j);
    }

    void render(scene* world) {
		
        std::vector<Color> framebuffer(static_cast<size_t>(width) * height);

        #pragma omp parallel for schedule(dynamic, 1)
        for(int j = 0; j < height; ++j) {
            if (j % 10 == 0) {
            #pragma omp critical
                std::cout << "\rRendering row " << j + 1 << " / " << height << std::flush;
            }


            for(int i = 0; i < width; ++i) {
                Point pixel_center_point = pixel_center(i, j);
                vec3 accumulatedXYZ = vec3(0, 0, 0);
                for (int s = 0; s < samples_per_pixel; ++s) {
                    // 在像素内随机采样
                    float u_offset = random_float() - 0.5f;
                    float v_offset = random_float() - 0.5f;
                    Point sample_point = pixel_center_point
                        + u_step * u_offset
                        + v_step * v_offset;
                    Direction ray_dir = (sample_point - position).normalize();

					// 当前采样波长通道
					int channelIndex = random_int(0, SpectralColor::kChannelCount - 1);
                    float lambda = SpectralColor::kWavelengthMinNm + channelIndex * SpectralColor::kWavelengthStepNm;
					
                    // 追踪光线
                    ray r(position, ray_dir);
                    float radiance = trace(world, r, lambda, 1.0f, 0, maxDepth);

                    accumulatedXYZ = accumulatedXYZ
                        + SpectralColor::kXYZWeights5nm[channelIndex]
                        * (radiance * static_cast<float>(SpectralColor::kChannelCount));
                }
                accumulatedXYZ = accumulatedXYZ
                    * (1.0f / static_cast<float>(samples_per_pixel))
                    / SpectralColor::kXYZIntegralNormalization;

                Color linearRGB = SpectralColor::xyzToLinearSRGB(accumulatedXYZ);
                Color srgb = SpectralColor::linearToSRGB(linearRGB);

                framebuffer[static_cast<size_t>(j) * width + i] = srgb;
            }
		}
        // 输出BMP
        toBMP(framebuffer, width, height, "output.bmp");
    }


    float trace(scene* world,
        ray           r,
        float         lambda,
        float         throughput,
        int           depth,
        int           max_depth) const
    {
        if (depth >= max_depth) return 0.0f;

        hit_record rec;

        // 未命中
        if (!world->hit(r, 1e-4f, 1e30f, rec)) {
            return 0.0f;
        }

        const material& mat = *rec.mat;

        // 发光材质
        if (mat.type == MAT_EMISSIVE) {
            return throughput * mat.emissionAt(lambda);
        }

        // 材质使用pdf采样
        if (mat.use_pdf) {
            // 抽取方向
            Direction new_dir = mat.sample_scatter(rec.normal);

            // 该采样方向的概率密度
            float pdf = mat.pdf_scatter(rec.normal, new_dir);
            if (pdf <= 1e-8f) return 0.0f;

            // 该方向贡献权重
            float geo_term = mat.eval_geometry_term(rec.normal, new_dir);
            // 材质吸收率
            float albedo = mat.albedoAt(lambda);

            // 蒙特卡洛估计
            float newThroughput = throughput * albedo * (geo_term / pdf);
            if (newThroughput <= 0.0f) return 0.0f;

            // 递归追踪
            Point newOrigin = rec.p + new_dir * 1e-3f;
            return trace(world, ray(newOrigin, new_dir), lambda, newThroughput, depth + 1, max_depth);
        }

        // 不适用pdf采样的 Delta材质
        scatter_result sr = mat.scatter(r.direction, rec.p, rec.normal, rec.front_face, lambda);
        if (!sr.scattered) return 0.0f;
        float newThroughput = throughput * sr.weight;
        if (newThroughput <= 0.0f) return 0.0f;
        Point newOrigin = rec.p + sr.dir * 1e-3f;
        return trace(world, ray(newOrigin, sr.dir), lambda, newThroughput, depth + 1, max_depth);
    }


    void toBMP(const std::vector<Color>& framebuffer,
        int w, int h,
        const char* filename = "output.bmp") const
    {
        // 每行字节数按 4 字节对齐
        const int rowBytes = w * 3;
        const int padding = (4 - (rowBytes % 4)) % 4;
        const int stride = rowBytes + padding;
        const uint32_t imgSize = static_cast<uint32_t>(stride) * static_cast<uint32_t>(h);
        const uint32_t fileSize = 14 + 40 + imgSize;

        std::ofstream out(filename, std::ios::binary);
        if (!out) return;

        auto writeU16 = [&](uint16_t v) {
            out.put(static_cast<char>(v & 0xFF));
            out.put(static_cast<char>((v >> 8) & 0xFF));
            };
        auto writeU32 = [&](uint32_t v) {
            out.put(static_cast<char>(v & 0xFF));
            out.put(static_cast<char>((v >> 8) & 0xFF));
            out.put(static_cast<char>((v >> 16) & 0xFF));
            out.put(static_cast<char>((v >> 24) & 0xFF));
            };

        // -------- BITMAPFILEHEADER (14 bytes) --------
        out.put('B'); out.put('M');
        writeU32(fileSize);
        writeU16(0);            // reserved1
        writeU16(0);            // reserved2
        writeU32(14 + 40);      // pixel data offset

        // -------- BITMAPINFOHEADER (40 bytes) --------
        writeU32(40);           // header size
        writeU32(static_cast<uint32_t>(w));
        writeU32(static_cast<uint32_t>(h));   // 正数 = 自下而上
        writeU16(1);            // planes
        writeU16(24);           // bpp
        writeU32(0);            // compression = BI_RGB
        writeU32(imgSize);
        writeU32(2835);         // x ppm ≈ 72 DPI
        writeU32(2835);         // y ppm
        writeU32(0);            // colors in palette
        writeU32(0);            // important colors

        // -------- 像素数据（自下而上，BGR） --------
        const uint8_t pad[3] = { 0, 0, 0 };
        for (int j = h - 1; j >= 0; --j) {
            for (int i = 0; i < w; ++i) {
                const Color& c = framebuffer[static_cast<size_t>(j) * w + i];
                uint8_t r = static_cast<uint8_t>(255.0f * std::clamp(c.x(), 0.0f, 1.0f) + 0.5f);
                uint8_t g = static_cast<uint8_t>(255.0f * std::clamp(c.y(), 0.0f, 1.0f) + 0.5f);
                uint8_t b = static_cast<uint8_t>(255.0f * std::clamp(c.z(), 0.0f, 1.0f) + 0.5f);
                out.put(static_cast<char>(b));
                out.put(static_cast<char>(g));
                out.put(static_cast<char>(r));
            }
            out.write(reinterpret_cast<const char*>(pad), padding);
        }
    }
};