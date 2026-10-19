#include "camera.h"
#include "hittable.h"
#include "vec3.h"

#include "scene_dispersion.h"
#include "scene_cornell_box.h"
#include "scene_bounding_ball.h"
#include "scene_refraction_rich.h"

int main() {
	// render_dispersion();
	// render_bounding_ball();
	render_refraction_rich();
}