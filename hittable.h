#pragma once

#include <cmath>

#include "ray.h"
#include "vec3.h"
#include "material.h"
#include "mat4.h"




struct aabb {
    Point min;
    Point max;

    aabb() {
        float inf = 1e30f;
        min = Point(inf, inf, inf);
        max = Point(-inf, -inf, -inf);
    }
    aabb(const Point& mn, const Point& mx) : min(mn), max(mx) {}

    bool hit(const ray& r, float t_min, float t_max) const {
        for (int a = 0; a < 3; ++a) {
            float invD = 1.0f / r.direction[a];
            float t0 = (min[a] - r.origin[a]) * invD;
            float t1 = (max[a] - r.origin[a]) * invD;

            if (invD < 0.0f) std::swap(t0, t1);

            t_min = t0 > t_min ? t0 : t_min;
            t_max = t1 < t_max ? t1 : t_max;

            if (t_max <= t_min) return false;
        }
        return true;
    }

    void expand(const aabb& other) {
        min = Point(std::fmin(min.x(), other.min.x()), std::fmin(min.y(), other.min.y()), std::fmin(min.z(), other.min.z()));
        max = Point(std::fmax(max.x(), other.max.x()), std::fmax(max.y(), other.max.y()), std::fmax(max.z(), other.max.z()));
    }
    void expand(const vec3& p) {
        min = Point(std::fmin(min.x(), p.x()), std::fmin(min.y(), p.y()), std::fmin(min.z(), p.z()));
        max = Point(std::fmax(max.x(), p.x()), std::fmax(max.y(), p.y()), std::fmax(max.z(), p.z()));
    }

    void pad(float epsilon = 1e-4f) {
        for (int i = 0; i < 3; ++i) {
            float diff = max[i] - min[i];
            if (diff < epsilon) {
                min[i] -= epsilon;
                max[i] += epsilon;
            }
        }
    }

    vec3 center() const {
        return Point((min.x() + max.x()) * 0.5f,
            (min.y() + max.y()) * 0.5f,
            (min.z() + max.z()) * 0.5f);
    }
};



struct sphere {
    Point center;
    float radius;
    float inv_radius;   // Ô¤¼ÆËã 1/radius
    material mat;

    sphere() = default;

    sphere(const Point& c, float r, const material& m)
        : center(c), radius(r), mat(m) {
        finalize();
    }

    void finalize() { inv_radius = 1.0f / radius; }

    aabb get_aabb() const {
        return aabb(center - vec3(radius, radius, radius), center + vec3(radius, radius, radius));
    }

    inline bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const {
        vec3 oc = r.origin - center;
        float a = r.direction.dot(r.direction);
        float half_b = oc.dot(r.direction);
        float c = oc.dot(oc) - radius * radius;
        float disc = half_b * half_b - a * c;
        if (disc < 0.0f) return false;

        float sq = std::sqrt(disc);

        float t = (-half_b - sq) / a;

        if (t <= t_min || t >= t_max) {
            t = (-half_b + sq) / a;
            if (t <= t_min || t >= t_max) {
                return false;
            }
        }

        rec.p = r.origin + t * r.direction;

        vec3 outward_normal = (rec.p - center) * inv_radius;
        rec.set_face_normal(r, outward_normal);

        rec.t = t;
        rec.mat = &mat;
        return true;
    }
};

struct quad {

    Point p;
    vec3 u, v;

    vec3 normal;
    float u_len_sq;
    float v_len_sq;
    float d;

	material mat;

    quad() = default;

    quad(const Point& p_, const vec3& u_, const vec3& v_, const material& m)
        : p(p_), u(u_), v(v_), mat(m) {
        finalize();
    }

    void finalize() {
        normal = u.cross(v).normalize();
        u_len_sq = u.dot(u);
        v_len_sq = v.dot(v);
        d = normal.dot(p);
    }

    aabb get_aabb() const {
        Point p1 = p + u;
        Point p2 = p + v;
        Point p3 = p + u + v;
        aabb box;
        box.expand(p);
        box.expand(p1);
        box.expand(p2);
        box.expand(p3);

        box.pad(1e-4f);

        return box;
    }

    inline bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const {
        float denom = normal.dot(r.direction);
        if (std::abs(denom) < 1e-6f) return false;

        float t = (d - normal.dot(r.origin)) / denom;
        if (t <= t_min || t >= t_max) return false;

        Point hp = r.origin + t * r.direction;
        vec3 w = hp - p;

        float up = w.dot(u);
        float vp = w.dot(v);
        if (up < 0.0f || up > u_len_sq) return false;
        if (vp < 0.0f || vp > v_len_sq) return false;

        rec.p = hp;
        rec.set_face_normal(r, normal);
        rec.t = t;
        rec.mat = &mat;
        return true;
    }
};


struct triangle {
    Point a, b, c;
    vec3 normal;
    material mat;

    triangle() = default;
    triangle(const Point& a_, const Point& b_, const Point& c_, const material& m)
        : a(a_), b(b_), c(c_), mat(m) {
        normal = (b - a).cross(c - a).normalize();
    }

    aabb get_aabb() const {
        aabb box;
        box.expand(a);
        box.expand(b);
        box.expand(c);

        box.pad(1e-4f);

        return box;
    }

    inline bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const {
        vec3 ab = b - a;
        vec3 ac = c - a;
        vec3 pvec = r.direction.cross(ac);
        float det = ab.dot(pvec);
        if (std::abs(det) < 1e-8f) return false;
        float inv_det = 1.0f / det;
        vec3 tvec = r.origin - a;
        float u = tvec.dot(pvec) * inv_det;
        if (u < 0.0f || u > 1.0f) return false;
        vec3 qvec = tvec.cross(ab);
        float v = r.direction.dot(qvec) * inv_det;
        if (v < 0.0f || u + v > 1.0f) return false;
        float t = ac.dot(qvec) * inv_det;
        if (t <= t_min || t >= t_max) return false;

        rec.p = r.origin + t * r.direction;
        rec.set_face_normal(r, normal);
        rec.t = t;
        rec.mat = &mat;
        return true;
    }
};



