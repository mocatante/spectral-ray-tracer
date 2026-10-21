// scene_lamp_box.h
#pragma once
#include "camera.h"
#include "scene.h"
#include "material.h"
#include "RayTracing.h"

inline void render_lamp_box() {
    scene world;

    // ============================================================
    // 房间：3×3×2.5，天花板 3×3 网格，开口在 (-x,-z) 角落
    // 康奈尔视角：缺前墙 z=Z3
    // ============================================================
    const float X0 = -1.5f, X1 = -0.5f, X2 = 0.5f, X3 = 1.5f;
    const float Z0 = -1.5f, Z1 = -0.5f, Z2 = 0.5f, Z3 = 1.5f;
    const float R_H = 2.5f;

    material floor_mat = material::Diffuse(Color(0.88f, 0.88f, 0.88f));
    material wall_mat = material::Diffuse(Color(0.42f, 0.42f, 0.45f));

    // 地板
    world.add(quad(Point(X0, 0, Z0), vec3(X3 - X0, 0, 0), vec3(0, 0, Z3 - Z0), floor_mat));
    // 左墙 x=X0
    world.add(quad(Point(X0, 0, Z0), vec3(0, R_H, 0), vec3(0, 0, Z3 - Z0), wall_mat));
    // 右墙 x=X3
    world.add(quad(Point(X3, 0, Z0), vec3(0, 0, Z3 - Z0), vec3(0, R_H, 0), wall_mat));
    // 后墙 z=Z0
    world.add(quad(Point(X0, 0, Z0), vec3(X3 - X0, 0, 0), vec3(0, R_H, 0), wall_mat));
    // 前墙 z=Z3：缺失（康奈尔视角需要）

    // ============================================================
    // 天花板：3 块 L 形（开口在左上角）
    //   [开口] [A A]
    //   [B   ] [C C]
    //   [B   ] [C C]
    // C 与开口对角，发光亮度 1
    // ============================================================
    material ceil_dark_mat = material::Diffuse(Color(0.32f, 0.32f, 0.35f));

    // A：上排右侧
    world.add(quad(
        Point(X1, R_H, Z0),
        vec3(X3 - X1, 0, 0),
        vec3(0, 0, Z1 - Z0),
        ceil_dark_mat
    ));
    // B：左列下半
    world.add(quad(
        Point(X0, R_H, Z1),
        vec3(X1 - X0, 0, 0),
        vec3(0, 0, Z3 - Z1),
        ceil_dark_mat
    ));
    // C：右下 2×2，与开口对角，发光亮度 1
    {
        std::vector<float> ceil_light_spec(SpectralColor::kChannelCount, 1.0f);
        material ceil_light_mat = material::EmissiveSpectrum(ceil_light_spec);
        world.add(quad(
            Point(X1, R_H, Z1),
            vec3(X3 - X1, 0, 0),
            vec3(0, 0, Z3 - Z1),
            //ceil_light_mat
            ceil_dark_mat
        ), IS_DIRECT_SAMPLE, 1.0f);
    }

    // ============================================================
    // 盒子：立方 1×1×1，位于开口上方，底 = 天花板平面
    // ============================================================
    const float BX0 = X0, BX1 = X1;
    const float BZ0 = Z0, BZ1 = Z1;
    const float BY0 = R_H;          // 2.5，底（开口所在平面）
    const float BY1 = R_H + 1.0f;   // 3.5，顶

    material box_mat = material::Metal(Color(0.92f, 0.92f, 0.94f), 0.02f);

    // 顶面
    world.add(quad(
        Point(BX0, BY1, BZ0),
        vec3(BX1 - BX0, 0, 0),
        vec3(0, 0, BZ1 - BZ0),
        box_mat
    ));
    // 前壁 z=BZ1
    world.add(quad(
        Point(BX0, BY0, BZ1),
        vec3(BX1 - BX0, 0, 0),
        vec3(0, BY1 - BY0, 0),
        box_mat
    ));
    // 后壁 z=BZ0
    world.add(quad(
        Point(BX1, BY0, BZ0),
        vec3(-(BX1 - BX0), 0, 0),
        vec3(0, BY1 - BY0, 0),
        box_mat
    ));
    // 左壁 x=BX0
    world.add(quad(
        Point(BX0, BY0, BZ0),
        vec3(0, 0, BZ1 - BZ0),
        vec3(0, BY1 - BY0, 0),
        box_mat
    ));
    // 右壁 x=BX1
    world.add(quad(
        Point(BX1, BY0, BZ0),
        vec3(0, BY1 - BY0, 0),
        vec3(0, 0, BZ1 - BZ0),
        box_mat
    ));
    // 底面空缺 = 开口

    // ============================================================
    // 5 条平行光栅（盒内顶，太阳 5778K）
    // ============================================================
    std::vector<float> sun_spec = make_blackbody_spectrum(5778.0f);
    for (auto& v : sun_spec) v *= 25.0f;
    material sun_mat = material::EmissiveSpectrum(sun_spec);

    const float BCX = (BX0 + BX1) * 0.5f;   // -1.0
    const float BCZ = (BZ0 + BZ1) * 0.5f;   // -1.0
    const float SUN_Y = BY1 - 0.005f;  // 3.495
    const float STRIP_LEN = 0.8f;          // x 方向
    const float STRIP_W = 0.06f;         // z 方向
    const int   STRIP_N = 5;
    const float STRIP_SPAN = 0.8f;          // z 总跨度

    for (int i = 0; i < STRIP_N; ++i) {
        float t = (float(i) / float(STRIP_N - 1) - 0.5f);  // -0.5..0.5
        float z = BCZ + t * STRIP_SPAN;
        world.add(quad(
            Point(BCX - STRIP_LEN * 0.5f, SUN_Y, z - STRIP_W * 0.5f),
            vec3(STRIP_LEN, 0, 0),
            vec3(0, 0, STRIP_W),
            sun_mat
        ), IS_DIRECT_SAMPLE, 1.0f);
    }

    // ============================================================
    // 玻璃球（盒内内接，r=0.49，留 5mm 避 z-fighting）
    // ============================================================
    material glass_mat = material::Glass(1.52f, 0.05f);
    const float GLASS_R = 0.49f;
    const float GLASS_CY = 3.0f;
    world.add(sphere(Point(BCX, GLASS_CY, BCZ), GLASS_R, glass_mat));

    // ============================================================
    // 采样代理 quad（开口正下方 1cm，不渲染）
    // ============================================================
    {
        AssetRef proxy = world.add_asset(quad(
            Point(BX0, BY0 - 0.01f, BZ0),
            vec3(BX1 - BX0, 0, 0),
            vec3(0, 0, BZ1 - BZ0),
            material::Diffuse(Color(0, 0, 0))   // 永远不会被 hit
        ));
        world.add_sampling_only(proxy, 6.0f);
    }

    world.build_BVH();

    Point cam_pos(0.0f, 1.25f, 4.5f); 
    Point lookat(0.0f, 1.25f, -1.5f);

    camera cam(
        cam_pos,
        lookat,
        vec3(0, 1, 0),
        (cam_pos - lookat).length(),
        55.0f,
        1.0f,
        800,
        1024,
        20
    );

    cam.render(&world);
}