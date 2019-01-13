#include "camera.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include "geometry.h"

static const float NEAR = 0.1f;
static const float FAR = 1000;
static const float FOVY = TO_RADIANS(45);
static const vec3_t WORLD_UP = {0, 1, 0};

struct camera {
    vec3_t position;
    vec3_t target;
    float aspect;
};

/* camera creating/releasing/updating */

camera_t *camera_create(vec3_t position, vec3_t target, float aspect) {
    camera_t *camera;

    assert(vec3_length(vec3_sub(position, target)) > EPSILON && aspect > 0);

    camera = (camera_t*)malloc(sizeof(camera_t));
    camera->position = position;
    camera->target   = target;
    camera->aspect   = aspect;

    return camera;
}

void camera_release(camera_t *camera) {
    free(camera);
}

static vec3_t calculate_pan(vec3_t from_camera, motion_t motion) {
    vec3_t forward = vec3_normalize(from_camera);
    vec3_t left = vec3_cross(WORLD_UP, forward);
    vec3_t up = vec3_cross(forward, left);

    float distance = vec3_length(from_camera);
    float factor = distance * (float)tan(FOVY / 2) * 2;
    vec3_t delta_x = vec3_mul(left, motion.pan.x * factor);
    vec3_t delta_y = vec3_mul(up, motion.pan.y * factor);
    return vec3_add(delta_x, delta_y);
}

static double clamp_double(double value, double min, double max) {
    assert(min <= max);
    return (value < min) ? min : ((value > max) ? max : value);
}

static vec3_t calculate_offset(vec3_t from_target, motion_t motion) {
    double radius = vec3_length(from_target);
    double theta = atan2(from_target.x, from_target.z);  /* azimuth angle */
    double phi = acos(from_target.y / radius);           /* polar angle */
    double factor = PI * 2;
    vec3_t offset;

    radius *= pow(0.95, motion.dolly);
    theta -= motion.orbit.x * factor;
    phi -= motion.orbit.y * factor;
    phi = clamp_double(phi, EPSILON, PI - EPSILON);

    offset = vec3_new(
        (float)(radius * sin(phi) * sin(theta)),
        (float)(radius * cos(phi)),
        (float)(radius * sin(phi) * cos(theta))
    );
    return offset;
}

void camera_orbit_update(camera_t *camera, motion_t motion) {
    vec3_t from_target = vec3_sub(camera->position, camera->target);
    vec3_t from_camera = vec3_sub(camera->target, camera->position);
    vec3_t pan = calculate_pan(from_camera, motion);
    vec3_t offset = calculate_offset(from_target, motion);
    camera->target = vec3_add(camera->target, pan);
    camera->position = vec3_add(camera->target, offset);
}

/* propety retrieving */

vec3_t camera_get_position(camera_t *camera) {
    return camera->position;
}

vec3_t camera_get_forward(camera_t *camera) {
    return vec3_normalize(vec3_sub(camera->target, camera->position));
}

mat4_t camera_get_view_matrix(camera_t *camera) {
    return mat4_lookat(camera->position, camera->target, WORLD_UP);
}

mat4_t camera_get_proj_matrix(camera_t *camera) {
    return mat4_perspective(FOVY, camera->aspect, NEAR, FAR);
}
