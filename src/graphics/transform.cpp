#include <graphics.hpp>
#include <glm/gtc/matrix_transform.hpp>

glm::mat4 g_cur_matrix __attribute__((aligned(16))) = glm::identity<glm::mat4>();
glm::mat4 g_proj_matrix = glm::identity<glm::mat4>();
glm::mat4 g_default_proj = glm::identity<glm::mat4>();
std::vector<glm::mat4> matrix_stack;
glm::vec4 matrix_origin;
glm::vec4 matrix_pixel_step_x;
glm::vec4 matrix_pixel_step_y;
glm::mat4 tmp_mat;

Vertex::vec4_t transform_vertex(glm::vec4 vert) {
	return tmp_mat * vert;
}

void update_matrix() {
#ifdef LOW_BANDWIDTH
	tmp_mat = glm::scale(glm::identity<glm::mat4>(), glm::vec3(4681.f / g_width(), 4681.f / g_height(), 1.f)) * g_cur_matrix;
#else
	tmp_mat = glm::scale(glm::identity<glm::mat4>(), glm::vec3(1.f / g_width(), 1.f / g_height(), 1.f)) * g_cur_matrix;
#endif
	//matrix_origin = tmp_mat * glm::vec4(0, 0, 0, 1);
	//matrix_pixel_step_x = tmp_mat * glm::vec4(1, 0, 0, 0);
	//matrix_pixel_step_y = tmp_mat * glm::vec4(0, 1, 0, 0);
}

void g_identity() {
	g_cur_matrix = glm::identity<glm::mat4>();
	update_matrix();
}

void g_proj_reset() {
	g_proj_matrix = g_default_proj;
}

void g_proj_ortho(float l, float r, float b, float t, float near, float far) {
	g_proj_matrix = glm::ortho(l, r, b, t, near, far);
}

void g_proj_perspective(float fov, float aspect, float near, float far) {
	g_proj_matrix = glm::perspective(fov, aspect, near, far);
}

void g_translate(float x, float y, float z) {
	g_cur_matrix = glm::translate(g_cur_matrix, {x, y, z});
	update_matrix();
}

void g_scale(float x, float y, float z) {
	g_cur_matrix = glm::scale(g_cur_matrix, {x, y, z});
	update_matrix();
}

void g_rotate(float z, glm::vec3 axis) {
	g_cur_matrix = glm::rotate(g_cur_matrix, z, axis);
	update_matrix();
}

void g_push() {
	matrix_stack.push_back(g_cur_matrix);
}

void g_pop() {
	if(matrix_stack.empty()) {
		log_err("popped too many matrices");
		return;
	}
	g_cur_matrix = matrix_stack[matrix_stack.size() - 1];
	matrix_stack.pop_back();
	update_matrix();
}

RectInt g_real_scissor;
RectFloat g_virtual_scissor;

void update_scissor() {
	g_virtual_scissor = g_real_scissor;
	g_real_to_virtual(&g_virtual_scissor.x, &g_virtual_scissor.y);
	g_real_to_virtual(&g_virtual_scissor.w, &g_virtual_scissor.h);
}

static std::vector<RectFloat> scissor_stack;

void g_push_scissor(RectFloat scissor) {
	g_flush();
	scissor_stack.push_back(g_real_scissor);
	glm::vec4 origin = g_cur_matrix * glm::vec4(scissor.x, scissor.y, 0, 1);
	scissor.x = origin.x;
	scissor.y = origin.y;
	g_virtual_to_real(&scissor.x, &scissor.y);
	g_virtual_to_real(&scissor.w, &scissor.h);
	// limit the scissor within the previous scissor and within the screen
	g_real_scissor = RectInt(scissor).intersection(g_real_scissor).intersection(RectInt(0, 0, g_real_width(), g_real_height()));
	update_scissor();
}

void g_pop_scissor() {
	g_flush();
	if(scissor_stack.empty()) {
		log_err("popped too many scissors");
		return;
	}
	g_real_scissor = scissor_stack[scissor_stack.size() - 1];
	scissor_stack.pop_back();
	update_scissor();
}

glm::vec4 g_cur_color __attribute((aligned(16))) = {1, 1, 1, 1};
glm::vec4 g_cur_color_2 __attribute((aligned(16))) = {1, 1, 1, 1};
int g_cur_color_2_blend = G_BLEND_SRC;
int g_color_stack_blend = G_BLEND_MUL;
glm::vec4 g_cur_color_off __attribute((aligned(16))) = {0, 0, 0, 0};
static std::vector<glm::vec4> color_stack;
static std::vector<glm::vec4> color_off_stack;

void g_push_color(float r, float g, float b, float a) {
	color_stack.push_back(g_cur_color);
	g_cur_color = g_blend(g_cur_color, glm::vec4(r, g, b, a), g_color_stack_blend);
}

void g_push_color(glm::vec4 c) {
	color_stack.push_back(g_cur_color);
	g_cur_color = g_blend(g_cur_color, c, g_color_stack_blend);
}

void g_pop_color() {
	if(color_stack.empty()) {
		log_err("popped too many colors");
		return;
	}
	g_cur_color = color_stack[color_stack.size() - 1];
	color_stack.pop_back();
}

void g_set_color_2(float r, float g, float b, float a, int blend) {
	g_cur_color_2 = glm::vec4(r, g, b, a);
	g_cur_color_2_blend = blend;
}

void g_set_color_2(glm::vec4 c, int blend) {
	g_cur_color_2 = c;
	g_cur_color_2_blend = blend;
}

void g_push_color_off(float r, float g, float b, float a) {
#ifdef COLOR_OFF_FLUSH
	g_flush();
#endif
	color_off_stack.push_back(g_cur_color_off);
	g_cur_color_off += glm::vec4(r, g, b, a);
}

void g_pop_color_off() {
#ifdef COLOR_OFF_FLUSH
	g_flush();
#endif
	if(color_off_stack.empty()) {
		log_err("popped too many color offsets");
		return;
	}
	g_cur_color_off = color_off_stack[color_off_stack.size() - 1];
	color_off_stack.pop_back();
}

glm::vec4 g_blend(glm::vec4 a, glm::vec4 b, int blend) {
	switch(blend) {
		case G_BLEND_SRC:
			return a;
		case G_BLEND_DST:
			return b;
		case G_BLEND_ADD:
			return a + b;
		case G_BLEND_MUL:
			return a * b;
	}
	return b;
}

extern std::vector<DrawBatchRef> draw_batch_stack;
void g_reset_stacks() {
	g_flush();
	while(draw_batch_stack.size()) {
		g_pop_batch();
		g_flush();
	}
	g_set_min_aspect(0, 0);
	g_set_max_aspect(0, 0);
	g_set_virtual_size(-1, -1);
	matrix_stack.clear();
	g_cur_matrix = glm::identity<glm::mat4>();
	//if(color_stack.size() && debug_warns) log_warn("colors not popped correctly");
	color_stack.clear();
	g_cur_color = glm::vec4(1, 1, 1, 1);
	g_cur_color_2 = glm::vec4(1, 1, 1, 1);
	g_cur_color_2_blend = G_BLEND_SRC;
	//if(color_off_stack.size() && debug_warns) log_warn("color offs not popped correctly");
	color_off_stack.clear();
	g_cur_color_off = glm::vec4(0, 0, 0, 0);
	//if(scissor_stack.size() && debug_warns) log_warn("scissors not popped correctly");
	scissor_stack.clear();
	g_real_scissor = RectInt(0, 0, g_real_width(), g_real_height());
	g_virtual_scissor = RectInt(0, 0, g_width(), g_height());
	g_proj_reset();
	g_erase_lights();
	update_screen_size();
	g_set_min_aspect(4, 3);
	g_set_max_aspect(2, 1);
}

void g_calc(float* x, float* y, float* w, float* h, float tw, float th, int corner, int origin) {
	int sw = g_width(), sh = g_height();
	if(*w == -2) {
		*w = tw;
	}
	if(*h == -2) {
		*h = th;
	}
	if(*w == -1) {
		*w = th == 0? 0: *h * tw / th;
	}
	if(*h == -1) {
		*h = tw == 0? 0: *w * th / tw;
	}
	switch(corner) {
		case G_TOP_RIGHT:
			*x -= *w;
			break;
		case G_BOTTOM_LEFT:
			*y -= *h;
			break;
		case G_BOTTOM_RIGHT:
			*x -= *w;
			*y -= *h;
			break;
		case G_CENTER_TOP:
			*x -= *w / 2;
			break;
		case G_CENTER_BOTTOM:
			*x -= *w / 2;
			*y -= *h;
			break;
		case G_CENTER_LEFT:
			*y -= *h / 2;
			break;
		case G_CENTER_RIGHT:
			*x -= *w;
			*y -= *h / 2;
			break;
		case G_CENTER:
			*x -= *w / 2;
			*y -= *h / 2;
			break;
	}
	switch(origin) {
		case G_TOP_RIGHT:
			*x += sw;
			break;
		case G_BOTTOM_LEFT:
			*y += sh;
			break;
		case G_BOTTOM_RIGHT:
			*x += sw;
			*y += sh;
			break;
		case G_CENTER_TOP:
			*x += sw / 2;
			break;
		case G_CENTER_BOTTOM:
			*x += sw / 2;
			*y += sh;
			break;
		case G_CENTER_LEFT:
			*y += sh / 2;
			break;
		case G_CENTER_RIGHT:
			*x += sw;
			*y += sh / 2;
			break;
		case G_CENTER:
			*x += sw / 2;
			*y += sh / 2;
			break;
	}
}

void g_calc_point(int* x, int* y, int origin) {
	int sw = g_width(), sh = g_height();
	switch(origin) {
		case G_TOP_RIGHT:
			*x += sw;
			break;
		case G_BOTTOM_LEFT:
			*y += sh;
			break;
		case G_BOTTOM_RIGHT:
			*x += sw;
			*y += sh;
			break;
		case G_CENTER_TOP:
			*x += sw / 2;
			break;
		case G_CENTER_BOTTOM:
			*x += sw / 2;
			*y += sh;
			break;
		case G_CENTER_LEFT:
			*y += sh / 2;
			break;
		case G_CENTER_RIGHT:
			*x += sw;
			*y += sh / 2;
			break;
		case G_CENTER:
			*x += sw / 2;
			*y += sh / 2;
			break;
	}
}

void g_calc_corner(i16& obj_x, i16& obj_y, u16 obj_w, u16 obj_h, u16 bound_w, u16 bound_h, u8 corner) {
	switch(corner) {
		case G_CENTER:
			obj_x += bound_w / 2 - obj_w / 2;
			obj_y += bound_h / 2 - obj_h / 2;
			break;
		case G_TOP_LEFT:
			break;
		case G_TOP_RIGHT:
			obj_x += bound_w - obj_w;
			break;
		case G_BOTTOM_LEFT:
			obj_y += bound_h - obj_h;
			break;
		case G_BOTTOM_RIGHT:
			obj_x += bound_w - obj_w;
			obj_y += bound_h - obj_h;
			break;
		case G_CENTER_TOP:
			obj_x += bound_w / 2 - obj_w / 2;
			break;
		case G_CENTER_BOTTOM:
			obj_x += bound_w / 2 - obj_w / 2;
			obj_y += bound_h - obj_h;
			break;
		case G_CENTER_LEFT:
			obj_y += bound_h / 2 - obj_h / 2;
			break;
		case G_CENTER_RIGHT:
			obj_x += bound_w - obj_w;
			obj_y += bound_h / 2 - obj_h / 2;
			break;
	}
}

int scale(int to_scale, int sh, int ogh) {
	return to_scale * sh / ogh;
}

int _720p(int v) {
	return v * g_height() / 720;
}

int _600p(int v) {
	return v * g_height() / 600;
}

float _600pf(float v) {
	return v * g_height() / 600.f;
}

int _480p(int v) {
	return v * g_height() / 480;
}

static int virtual_width = -1, virtual_height = -1;

void g_set_virtual_size(int w, int h) {
	g_flush();
	if(w == -1 && h != -1) {
		w = h * g_real_width() / g_real_height();
	} else if(w != -1 && h == -1) {
		h = w * g_real_height() / g_real_width();
	}
	virtual_width = w;
	virtual_height = h;
	update_screen_size();
}

void g_virtual_to_real(float* x, float* y) {
	float vw = g_width(), vh = g_height();
	float rw = g_real_width(), rh = g_real_height();
	if(x) *x = *x * rw / vw;
	if(y) *y = *y * rh / vh;
}

void g_real_to_virtual(float* x, float* y) {
	float vw = g_width(), vh = g_height();
	float rw = g_real_width(), rh = g_real_height();
	if(x) *x = *x * vw / rw;
	if(y) *y = *y * vh / rh;
}

int max_aspect_ratio_w = 0, max_aspect_ratio_h = 0; //static int max_aspect_ratio_w = 7, max_aspect_ratio_h = 3;
int min_aspect_ratio_w = 0, min_aspect_ratio_h = 0; //static int min_aspect_ratio_w = 4, min_aspect_ratio_h = 3;
int width, height, real_width, real_height, real_screen_x, real_screen_y;

void api_update_screen_size();
void update_screen_size() {
	g_flush();
	if(physical_width * max_aspect_ratio_h > physical_height * max_aspect_ratio_w) {
		real_width = physical_height * max_aspect_ratio_w / max_aspect_ratio_h;
	} else {
		real_width = physical_width;
	}
	if(physical_width * min_aspect_ratio_h < physical_height * min_aspect_ratio_w) {
		real_height = physical_width * min_aspect_ratio_h / min_aspect_ratio_w;
	} else {
		real_height = physical_height;
	}
	width = virtual_width == -1? real_width: virtual_width;
	height = virtual_height == -1? real_height: virtual_height;
	real_screen_x = (physical_width - real_width) / 2;
	real_screen_y = (physical_height - real_height) / 2;
	update_matrix();
	api_update_screen_size();
}

void g_set_max_aspect(int w, int h) {
	g_flush();
	max_aspect_ratio_w = w;
	max_aspect_ratio_h = h;
	update_screen_size();
}

void g_set_min_aspect(int w, int h) {
	g_flush();
	min_aspect_ratio_w = w;
	min_aspect_ratio_h = h;
	update_screen_size();
}
