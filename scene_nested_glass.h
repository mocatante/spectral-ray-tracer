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
        1024,
        4096,
        50
    );
    cam.render(&world);
}