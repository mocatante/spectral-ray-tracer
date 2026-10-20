#pragma once
#include <iostream>
#include <vector>
#include <cmath>
#include <string>

#include "camera.h"
#include "scene.h"
#include "material.h"
#include "obj_loader.h"
#include "RayTracing.h"

inline void render_obj() {
    scene world;

    std::string obj_path = "D:/object.obj";

    material obj_mat = material::Glass(1.5f, 0.05f);
    mesh obj = load_obj(obj_path, obj_mat);

    obj.build_BVH();
    aabb bbox = obj.get_aabb();
    if (bbox.min.x() > bbox.max.x()) {
        std::cerr << "FATAL: Failed to load OBJ: " << obj_path << "\n";
        return;
    }

    Point bbox_center = bbox.center();
    vec3 bbox_size = bbox.max - bbox.min;
    float max_extent = std::max({ bbox_size.x(), bbox_size.y(), bbox_size.z() });

    std::clog << "OBJ Loaded! Max Extent: " << max_extent << "\n";


    world.add(obj, mat4::rotate_y(0.0f));

    float ground_y = bbox.min.y() - max_extent * 0.02f;
    float scene_size = max_extent * 20.0f;

    material floor_mat = material::Diffuse(Color(0.15f, 0.15f, 0.18f));
    world.add(quad(
        Point(bbox_center.x() - scene_size * 0.5f, ground_y, bbox_center.z() - scene_size * 0.5f),
        vec3(scene_size, 0, 0),
        vec3(0, 0, scene_size),
        floor_mat
    ));

    float wall_z = bbox.min.z() - max_extent * 2.0f;
    material back_wall_mat = material::Diffuse(Color(0.05f, 0.05f, 0.08f));
    world.add(quad(
        Point(bbox_center.x() - scene_size * 0.5f, ground_y, wall_z),
        vec3(scene_size, 0, 0),
        vec3(0, scene_size, 0),
        back_wall_mat
    ));



    std::vector<float> top_light_spec = make_blackbody_spectrum(4500.0f);
    for (auto& v : top_light_spec) v *= 3.0f;
    world.add(quad(
        Point(bbox_center.x() - max_extent * 2.0f, bbox_center.y() + max_extent * 3.0f, bbox_center.z() - max_extent * 1.0f),
        vec3(max_extent * 4.0f, 0.0f, 0.0f),
        vec3(0.0f, 0.0f, max_extent * 4.0f),
        material::EmissiveSpectrum(top_light_spec)
    ), IS_DIRECT_SAMPLE);

    std::vector<float> strip_spec = make_blackbody_spectrum(6500.0f);
    for (auto& v : strip_spec) v *= 70.0f;
    material strip_mat = material::EmissiveSpectrum(strip_spec);

    float strip_w = max_extent * 0.02f;
    float strip_h = max_extent * 6.0f;

    for (int i = -5; i <= 4; ++i) {
        float x = bbox_center.x() + i * max_extent * 0.4f;
        world.add(quad(
            Point(x - strip_w * 0.5f, ground_y, wall_z + max_extent * 0.1f),
            vec3(strip_w, 0.0f, 0.0f),
            vec3(0.0f, strip_h, 0.0f),
            strip_mat
        ), IS_DIRECT_SAMPLE);
    }

    std::vector<float> glow_spec = make_blackbody_spectrum(3000.0f);
    for (auto& v : glow_spec) v *= 12.0f;
    material glow_mat = material::EmissiveSpectrum(glow_spec);

    for (int i = 0; i < 20; ++i) {
        float x = bbox_center.x() + (random_float() - 0.5f) * max_extent * 4.0f;
        float y = ground_y + max_extent * 0.5f + random_float() * max_extent * 2.5f;
        float r = max_extent * (0.02f + random_float() * 0.06f);
        world.add(sphere(Point(x, y, wall_z + max_extent * 0.1f), r, glow_mat),  IS_DIRECT_SAMPLE);
    }


    material float_glass_mat = material::Glass(1.5f, 0.05f);

    world.add(sphere(bbox_center + vec3(-max_extent * 0.9f, max_extent * 0.5f, -max_extent * 0.2f), max_extent * 0.15f, float_glass_mat));
    world.add(sphere(bbox_center + vec3(max_extent * 0.8f, max_extent * 0.1f, max_extent * 0.4f), max_extent * 0.18f, float_glass_mat));
    world.add(sphere(bbox_center + vec3(-max_extent * 0.7f, -max_extent * 0.4f, max_extent * 0.6f), max_extent * 0.12f, float_glass_mat));
    world.add(sphere(bbox_center + vec3(max_extent * 0.6f, max_extent * 0.8f, -max_extent * 0.5f), max_extent * 0.2f, float_glass_mat));

    world.build_BVH();

    Point cam_lookat = bbox_center;
    Point cam_lookfrom = bbox_center + vec3(0, max_extent * 0.2f, max_extent * 2.6f);

    camera cam(
        cam_lookfrom,
        cam_lookat,
        vec3(0, 1, 0),
        (cam_lookfrom - cam_lookat).length(),
        45.0f,
        16.0f / 9.0f,
        1260,
        128,
        50
    );

    cam.render(&world);
}