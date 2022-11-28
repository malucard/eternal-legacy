#include "graphics.hpp"
#include <formats/formats.hpp>
#include <glm/gtc/matrix_transform.hpp>
//#include <pspge.h>
//#include <pspgu.h>

Light lights[MAX_LIGHTS];
bool lights_on = false;

void g_light(int i, int type, glm::vec3 pos, glm::vec3 color) {
	g_flush();
	lights_on = true;
	//sceGuLight(i, GU_POINTLIGHT, GU_DIFFUSE_AND_SPECULAR, (ScePspFVector3*) &pos);
	//sceGuLightColor(i, GU_DIFFUSE, 0);
	lights[i].type = type;
	lights[i].pos = g_cur_matrix * glm::vec4(pos, 1);
	lights[i].color = color;
}

glm::vec3 g_ambient_color;

void g_light_ambient(glm::vec3 ambient) {
	g_flush();
	lights_on = true;
	g_ambient_color = ambient;
}

void g_light_info() {

}

void g_erase_lights() {
	g_flush();
	lights_on = false;
	g_ambient_color = glm::vec3();
	for(int i = 0; i < MAX_LIGHTS; i++) {
		lights[i].type = 0;
	}
}

// mesh loading

SceneRef g_load_scene(DataStream* ds) {
	SceneRef scene = SceneFormat::open(ds);
	if(!scene.data) {
		log_err("unsupported scene format");
		_platform_throw();
	}
	return scene;
}

// mesh rendering

void g_draw_scene(const SceneRef& scene) {
	g_flush();
	g_push();
	lights_on = true;
	glm::mat4 scene_mat = g_cur_matrix;
	for(int i = 0; i < scene->lights.size() && i < MAX_LIGHTS; i++) {
		lights[i] = scene->lights[i];
		lights[i].pos = scene_mat * glm::vec4(lights[i].pos, 1);
	}
	for(int i = scene->lights.size(); i < MAX_LIGHTS; i++) {
		//lights[i].type = 0;
	}
	for(int i = 0; i < scene->meshes.size(); i++) {
		g_cur_matrix = scene_mat * scene->meshes[i]->local_transform;
		//g_cur_matrix = glm::translate(glm::identity<glm::mat4>(), {0, 0, -15});
		g_draw_mesh(scene->meshes[i]);
	}
	g_pop();
	g_erase_lights();
}
