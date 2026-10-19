#pragma once

#include "vec3.h"
#include "material.h"

struct ray {
	Point origin;
	Direction direction;

	ray(Point o, Direction d) : origin(o), direction(d) {}
};

struct hit_record {
	Point p;
	vec3 normal;
	float t;
	const material* mat;
	bool front_face;

	inline void set_face_normal(const ray& r, const vec3& outward_normal) {
		front_face = r.direction.dot(outward_normal) < 0;
		normal = front_face ? outward_normal : outward_normal * -1.0f;
	}
};
