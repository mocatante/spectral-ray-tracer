#include "camera.h"
#include "hittable.h"
#include "vec3.h"

#include "scene_dispersion.h"
#include "scene_cornell_box.h"
#include "scene_refraction_rich.h"
#include "scene_obj.h"

int main() {
	// render_dispersion();
	// render_bounding_ball();
	// render_refraction_rich();
	render_obj();
}