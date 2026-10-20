#pragma once
#include "camera.h"
#include "scene.h"
#include "material.h"

inline mesh make_box_mesh(const Point& a, const Point& b, const material& mat) {
    mesh m;
    Point min(std::fmin(a.x(), b.x()), std::fmin(a.y(), b.y()), std::fmin(a.z(), b.z()));
    Point max(std::fmax(a.x(), b.x()), std::fmax(a.y(), b.y()), std::fmax(a.z(), b.z()));

    Point p0(min.x(), min.y(), min.z()), p1(max.x(), min.y(), min.z());
    Point p2(max.x(), max.y(), min.z()), p3(min.x(), max.y(), min.z());
    Point p4(min.x(), min.y(), max.z()), p5(max.x(), min.y(), max.z());
    Point p6(max.x(), max.y(), max.z()), p7(min.x(), max.y(), max.z());

    m.add(triangle(p0, p2, p1, mat)); m.add(triangle(p0, p3, p2, mat));
    m.add(triangle(p4, p5, p6, mat)); m.add(triangle(p4, p6, p7, mat));
    m.add(triangle(p0, p4, p7, mat)); m.add(triangle(p0, p7, p3, mat));
    m.add(triangle(p1, p2, p6, mat)); m.add(triangle(p1, p6, p5, mat));
    m.add(triangle(p3, p7, p6, mat)); m.add(triangle(p3, p6, p2, mat));
    m.add(triangle(p0, p1, p5, mat)); m.add(triangle(p0, p5, p4, mat));

    return m;
}

inline void render_cornell_box() {
    scene world;

    material red_mat = material::Diffuse(Color(0.65f, 0.05f, 0.05f));
    material white_mat = material::Diffuse(Color(0.73f, 0.73f, 0.73f));
    material green_mat = material::Diffuse(Color(0.12f, 0.45f, 0.15f));

    std::vector<float> light_spec(SpectralColor::kChannelCount, 15.0f);
    material light_mat = material::EmissiveSpectrum(light_spec);

    world.add(quad(Point(555, 0, 0), vec3(0, 0, 555), vec3(0, 555, 0), green_mat));
    world.add(quad(Point(0, 0, 555), vec3(0, 0, -555), vec3(0, 555, 0), red_mat));
    world.add(quad(Point(0, 555, 0), vec3(555, 0, 0), vec3(0, 0, 555), white_mat));
    world.add(quad(Point(0, 0, 555), vec3(555, 0, 0), vec3(0, 0, -555), white_mat));
    world.add(quad(Point(555, 0, 555), vec3(-555, 0, 0), vec3(0, 555, 0), white_mat));
    world.add(quad(Point(213, 554, 227), vec3(130, 0, 0), vec3(0, 0, 105), light_mat));

    mesh box1 = make_box_mesh(Point(0, 0, 0), Point(165, 330, 165), white_mat);
    mesh box2 = make_box_mesh(Point(0, 0, 0), Point(165, 165, 165), white_mat);

    box1.build_BVH();
    box2.build_BVH();


    world.add(box1, mat4::translate(265, 0, 295) * mat4::rotate_y(15));
    world.add(box2, mat4::translate(130, 0, 65) * mat4::rotate_y(-18));

    world.build_BVH();

    camera cam(
        Point(278, 278, -800),
        Point(278, 278, 0),
        vec3(0, 1, 0),
        1.0f,
        40.0f,
        1.0f,
        600,
        512,
        10
    );

    cam.render(&world);
}