#pragma once
#include "camera.h"
#include "scene.h"
#include "material.h"
#include "RayTracing.h"
#include <vector>

inline void render_nested_glass() {
    scene world;

    const float R_HALF = 2.5f;
    const float R_H = 5.0f;

    material floor_mat = material::Diffuse(Color(0.8f, 0.8f, 0.8f));
    material wall_mat = material::Diffuse(Color(0.45f, 0.45f, 0.45f));
    material ceil_mat = material::Diffuse(Color(0.6f, 0.6f, 0.6f));

    world.add(quad(Point(-R_HALF, 0, -R_HALF), vec3(2 * R_HALF, 0, 0), vec3(0, 0, 2 * R_HALF), floor_mat));
    world.add(quad(Point(-R_HALF, R_H, -R_HALF), vec3(2 * R_HALF, 0, 0), vec3(0, 0, 2 * R_HALF), ceil_mat));
    world.add(quad(Point(-R_HALF, 0, -R_HALF), vec3(0, R_H, 0), vec3(0, 0, 2 * R_HALF), wall_mat));
    world.add(quad(Point(R_HALF, 0, -R_HALF), vec3(0, 0, 2 * R_HALF), vec3(0, R_H, 0), wall_mat));
    world.add(quad(Point(-R_HALF, 0, -R_HALF), vec3(2 * R_HALF, 0, 0), vec3(0, R_H, 0), wall_mat));

    Point center(0.6f, 0.4f, -0.6f);

    std::vector<float> sun_spec = make_blackbody_spectrum(5778.0f);
    for (auto& v : sun_spec) v *= 10.0f;
    material light_mat = material::EmissiveSpectrum(sun_spec);

    for (int i = 0; i < 25; ++i) {
        float r = random_float(0.015f, 0.03f);
        vec3 dir = random_unit_vector();
        float dist = random_float(0.1f + r + 0.02f, 0.3f - r - 0.02f);

        Point p = center + dir * dist;
        const material& m = light_mat;
        world.add(sphere(p, r, m));
    }

    material glass_mat = material::Glass(1.9f, 0.045f);
    world.add(sphere(center, 0.3f, glass_mat), IS_DIRECT_SAMPLE, 2.0f);
    world.add(sphere(center, 0.4f, glass_mat), IS_DIRECT_SAMPLE, 1.0f);

    // ------------------------------------------------------------------
    // 额外随机小球：半径取发光小球的 0.9 倍，随机漫反射 / 金属 / 玻璃
    // 放在地面上，避开外层玻璃球与已放置的小球
    // ------------------------------------------------------------------
    const int kExtraBallCount = 10;
    std::vector<Point> placed_centers;
    std::vector<float> placed_radii;
    placed_centers.reserve(kExtraBallCount);
    placed_radii.reserve(kExtraBallCount);

    for (int n = 0; n < kExtraBallCount; ++n) {
        const float radius = 0.36;

        Point p(0, 0, 0);
        bool placed = false;

        for (int attempt = 0; attempt < 64 && !placed; ++attempt) {
            const float x = random_float(-R_HALF + radius + 0.05f, R_HALF - radius - 0.05f);
            const float z = random_float(-R_HALF + radius + 0.05f, R_HALF - radius - 0.05f);
            p = Point(x, radius + 1e-3f, z);   // 贴地

            // 避开外层玻璃球
            if ((p - center).length() < 0.4f + radius + 0.05f) continue;

            // 避免与已放置的小球重叠
            bool ok = true;
            for (size_t k = 0; k < placed_centers.size(); ++k) {
                if ((p - placed_centers[k]).length() < radius + placed_radii[k] + 0.02f) {
                    ok = false;
                    break;
                }
            }
            if (ok) placed = true;
        }
        if (!placed) continue;

        placed_centers.push_back(p);
        placed_radii.push_back(radius);

        // 随机挑选材质
        const float t = random_float();
        material m;
        if (t < 0.40f) {
            // 随机颜色漫反射
            Color c(random_float(0.15f, 0.95f),
                random_float(0.15f, 0.95f),
                random_float(0.15f, 0.95f));
            m = material::Diffuse(c);
        }
        else if (t < 0.70f) {
            // 随机颜色金属
            Color c(random_float(0.30f, 0.95f),
                random_float(0.30f, 0.95f),
                random_float(0.30f, 0.95f));
            m = material::Metal(c, random_float(0.0f, 0.30f));
        }
        else {
            // 玻璃
            m = material::Glass(random_float(1.4f, 2.0f), random_float(0.04f, 0.06f));
        }

        world.add(sphere(p, radius, m));
    }
    // ------------------------------------------------------------------

    world.build_BVH();

    Point cam_pos(0.0f, 2.5f, 5.0f);
    Point cam_tgt(0.0f, 1.5f, 0.0f);

    camera cam(
        cam_pos,
        cam_tgt,
        vec3(0, 1, 0),
        (cam_pos - cam_tgt).length(),
        55.0f,
        1.0f,
        1024,   // width
        10000,
        50
    );
    cam.render(&world);
}