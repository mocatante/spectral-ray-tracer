#pragma once
#include "camera.h"
#include "scene.h"
#include "material.h"
#include "RayTracing.h"

inline void render_bounding_ball() {
    scene world;

    material ground_mat = material::Diffuse(Color(0.5f, 0.5f, 0.5f));
    world.add(quad(
        Point(-1000, 0, -1000),
        vec3(2000, 0, 0),
        vec3(0, 0, 2000),
        ground_mat
    ));

    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            float choose_mat = random_float();
            Point center(a + 0.9f * random_float(), 0.2f, b + 0.9f * random_float());

            if ((center - Point(4, 0.2f, 0)).length() > 0.9f) {
                if (choose_mat < 0.8f) {
                    Color albedo(
                        random_float() * random_float(),
                        random_float() * random_float(),
                        random_float() * random_float()
                    );
                    world.add(sphere(center, 0.2f, material::Diffuse(albedo)));
                }
                else if (choose_mat < 0.95f) {
                    Color albedo(
                        0.5f * (1.0f + random_float()),
                        0.5f * (1.0f + random_float()),
                        0.5f * (1.0f + random_float())
                    );
                    world.add(sphere(center, 0.2f, material::Metal(albedo, 0.3f * random_float())));
                }
                else {
                    world.add(sphere(center, 0.2f, material::Glass(1.5f)));
                }
            }
        }
    }

    world.add(sphere(Point(0, 1, 0), 1.0f, material::Glass(1.5f)));
    world.add(sphere(Point(-4, 1, 0), 1.0f, material::Diffuse(Color(0.4f, 0.2f, 0.1f))));

    std::vector<float> gold_spec = make_gaussian_spectrum(580.0f, 50.0f);
    world.add(sphere(Point(4, 1, 0), 1.0f, material::MetalSpectrum(gold_spec, 0.01f)));

    std::vector<float> light_spec = make_blackbody_spectrum(4500.0f);
    for (auto& v : light_spec) v *= 4.0f;
    material light_mat = material::EmissiveSpectrum(light_spec);

    world.add(quad(
        Point(-50, 50, -50),
        vec3(100, 0, 0),
        vec3(0, 0, 100),
        light_mat
    ));

    world.build_BVH();

    camera cam(
        Point(13, 2, 3),
        Point(0, 0, 0),
        vec3(0, 1, 0),
        10.0f,
        20.0f,
        1.5f,
        1260,
        1024,
        10
    );

    cam.render(&world);
}