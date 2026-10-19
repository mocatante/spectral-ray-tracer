#pragma once
#include "camera.h"
#include "scene.h"
#include "material.h"
#include "RayTracing.h"

inline void render_dispersion() {
    scene world;

    material floor_mat = material::Diffuse(Color(0.65f, 0.65f, 0.65f));
    world.add(quad(
        Point(-20.0f, -1.2f, -20.0f),
        vec3(40.0f, 0.0f, 0.0f),
        vec3(0.0f, 0.0f, 40.0f),
        floor_mat
    ));

    material back_wall_mat = material::Diffuse(Color(0.5f, 0.5f, 0.55f));
    world.add(quad(
        Point(-10.0f, -1.2f, -8.0f),
        vec3(20.0f, 0.0f, 0.0f),
        vec3(0.0f, 10.0f, 0.0f),
        back_wall_mat
    ));

    std::vector<float> top_light_spec = make_blackbody_spectrum(6500.0f);
    for (auto& v : top_light_spec) v *= 2.0f;
    material top_light_mat = material::EmissiveSpectrum(top_light_spec);
    world.add(quad(
        Point(-4.0f, 5.0f, -6.0f),
        vec3(8.0f, 0.0f, 0.0f),
        vec3(0.0f, 0.0f, 8.0f),
        top_light_mat
    ));

    std::vector<float> main_light_spec = make_blackbody_spectrum(4500.0f);
    for (auto& v : main_light_spec) v *= 5.0f;
    material main_light_mat = material::EmissiveSpectrum(main_light_spec);
    world.add(sphere(Point(-3.0f, 3.5f, -2.0f), 0.4f, main_light_mat));

    std::vector<float> raster_spec(SpectralColor::kChannelCount, 2.0f);
    material raster_mat = material::EmissiveSpectrum(raster_spec);

    float strip_w = 0.04f;
    float strip_h = 5.0f;
    float center_y = 0.0f;
    float raster_z = -5.0f;

    for (int i = -2; i <= 2; ++i) {
        float x = i * 0.6f;
        world.add(quad(
            Point(x - strip_w * 0.5f, center_y - strip_h * 0.5f, raster_z),
            vec3(strip_w, 0.0f, 0.0f),
            vec3(0.0f, strip_h, 0.0f),
            raster_mat
        ));
    }

    float horizontal_w = 8.0f;
    float horizontal_h = 0.04f;
    world.add(quad(
        Point(-horizontal_w * 0.5f, center_y - horizontal_h * 0.5f, raster_z + 0.1f),
        vec3(horizontal_w, 0.0f, 0.0f),
        vec3(0.0f, horizontal_h, 0.0f),
        raster_mat
    ));

    material glass = material::Glass(1.5046f, 0.0042f);
    world.add(sphere(Point(0.0f, -0.2f, -3.0f), 1.0f, glass));

    material mat_left_back = material::Diffuse(Color(0.2f, 0.6f, 0.3f));
    world.add(sphere(Point(-3.5f, -0.2f, -6.5f), 1.0f, mat_left_back));

    material mat_right_back = material::Diffuse(Color(0.9f, 0.4f, 0.1f));
    world.add(sphere(Point(3.0f, -0.5f, -5.5f), 0.7f, mat_right_back));

    material mat_right_front = material::Diffuse(Color(0.1f, 0.3f, 0.8f));
    world.add(sphere(Point(3.0f, -0.85f, -2.5f), 0.35f, mat_right_front));

    material mat_left_front = material::Diffuse(Color(0.7f, 0.2f, 0.5f));
    world.add(sphere(Point(-2.0f, 0.5f, -1.5f), 0.5f, mat_left_front));

    world.build_BVH();

    camera cam(
        Point(0.0f, 0.3f, 2.0f),
        Point(0.0f, 0.0f, -3.0f),
        vec3(0.0f, 1.0f, 0.0f),
        1.0f,
        60.0f,
        16.0f / 9.0f,
        1260,
        1024,
        12
    );

    cam.render(&world);
}