#pragma once
#include "camera.h"
#include "scene.h"
#include "material.h"
#include "RayTracing.h"

inline void render_refraction_rich() {
    scene world;

    material back_wall_mat = material::Diffuse(Color(0.4f, 0.4f, 0.45f));
    world.add(quad(
        Point(-15.0f, -2.0f, -12.0f),
        vec3(30.0f, 0.0f, 0.0f),
        vec3(0.0f, 15.0f, 0.0f),
        back_wall_mat
    ));

    material floor_mat = material::Diffuse(Color(0.6f, 0.6f, 0.6f));
    world.add(quad(
        Point(-15.0f, -1.2f, -15.0f),
        vec3(30.0f, 0.0f, 0.0f),
        vec3(0.0f, 0.0f, 30.0f),
        floor_mat
    ));

    std::vector<float> top_light_spec(SpectralColor::kChannelCount, 3.0f);
    material top_light_mat = material::EmissiveSpectrum(top_light_spec);
    world.add(quad(
        Point(-5.0f, 6.0f, -8.0f),
        vec3(10.0f, 0.0f, 0.0f),
        vec3(0.0f, 0.0f, 10.0f),
        top_light_mat
    ));

    std::vector<float> glow_spec(SpectralColor::kChannelCount, 8.0f);
    material glow_mat = material::EmissiveSpectrum(glow_spec);
    for (int i = 0; i < 6; ++i) {
        float x = random_float(-6.0f, 6.0f);
        float y = random_float(-1.0f, 4.0f);
        world.add(sphere(Point(x, y, -10.0f), random_float(0.1f, 0.25f), glow_mat));
    }

    float strip_w = 0.08f;
    float strip_h = 7.0f;
    for (int i = -2; i <= 2; ++i) {
        float x = i * 1.2f;
        world.add(quad(
            Point(x - strip_w * 0.5f, -1.2f, -9.0f),
            vec3(strip_w, 0.0f, 0.0f),
            vec3(0.0f, strip_h, 0.0f),
            glow_mat
        ));
    }

    material glass_mat = material::Glass(1.5f, 0.04f);

    world.add(sphere(Point(-2.5f, 0.2f, -3.0f), 1.0f, glass_mat));
    world.add(sphere(Point(0.0f, 0.8f, -5.0f), 0.8f, glass_mat));
    world.add(sphere(Point(2.0f, -0.2f, -2.5f), 0.6f, glass_mat));
    world.add(sphere(Point(-0.8f, -0.5f, -2.0f), 0.5f, glass_mat));
    world.add(sphere(Point(1.2f, 1.2f, -4.0f), 0.4f, glass_mat));
    world.add(sphere(Point(-1.8f, 1.0f, -6.0f), 0.7f, glass_mat));

    material mat_metal_1 = material::MetalSpectrum(make_gaussian_spectrum(600.0f, 50.0f), 0.05f);
    world.add(sphere(Point(-4.0f, -0.7f, -4.0f), 0.5f, mat_metal_1));

    material mat_metal_2 = material::Metal(Color(0.9f, 0.9f, 0.9f), 0.2f);
    world.add(sphere(Point(4.0f, -0.7f, -3.0f), 0.5f, mat_metal_2));

    material mat_diff_1 = material::Diffuse(Color(0.8f, 0.1f, 0.1f));
    world.add(sphere(Point(-3.0f, -0.9f, -1.0f), 0.3f, mat_diff_1));

    material mat_diff_2 = material::Diffuse(Color(0.1f, 0.3f, 0.9f)); 
    world.add(sphere(Point(3.5f, -0.9f, -1.5f), 0.3f, mat_diff_2));

    world.build_BVH();


    camera cam(
        Point(0.0f, 0.5f, 2.5f),
        Point(0.0f, 0.2f, -4.0f),
        vec3(0.0f, 1.0f, 0.0f),
        1.0f,
        50.0f,
        16.0f / 9.0f,
        1260,
        1024,                           // ²ÉÑùÊý
        20
    );

    cam.render(&world);
}