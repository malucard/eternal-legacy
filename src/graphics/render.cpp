#include "graphics.hpp"
#include <vector>
#include <texture_atlas.h>
#include <glm/gtc/matrix_transform.hpp>

bool unlock_fps = true;
int draw_calls = 0, polys = 0;
int physical_width = 0, physical_height = 0;

UpscaleRequest g_check_upscale(const TextureRef& tex, float w, float h, float tw, float th) {
#ifdef PLATFORM_UPSCALE
	if(tex->upscale && upscaling_enabled && (w > tw && h > th)) {
		int dw = tex->w, dh = tex->h;
		if(w * th >= h * tw) {
			dw = w;
			dh = (dw * tex->h + tex->w - 1) / tex->w;
		} else {
			dh = h;
			dw = (dh * tex->w + tex->h - 1) / tex->h;
		}
		if(dw == 0) dw = 1;
		if(dh == 0) dh = 1;
		return UpscaleRequest {.w = (u16) dw, .h = (u16) dh};
	}
#endif
	return UpscaleRequest {.w = 0, .h = 0};
}

DrawBatchRef g_new_batch() {
	return Rc(new DrawBatch());
}

std::vector<DrawBatchRef> draw_batch_stack;
static DrawBatchRef draw_batch;

void g_init_batch() {
	draw_batch = g_new_batch();
}

void g_push_batch(DrawBatchRef batch) {
	draw_batch_stack.push_back(draw_batch);
	draw_batch = batch;
}

void g_pop_batch() {
	if(draw_batch_stack.empty()) {
		log_err("popped too many batches");
		return;
	}
	draw_batch = std::move(*(draw_batch_stack.end() - 1));
	draw_batch_stack.pop_back();
}

#ifdef TWO_VERTEX_QUADS
#define VERTEX_COUNT 2
#else
#define VERTEX_COUNT 4
#endif

void reserve_quads(const TextureRef& tex, int count) {
	if(!draw_batch->tex.data || draw_batch->tex->id != tex->id) {
		if(draw_batch->vertices.size()) {
			draw_batch->draw();
		}
		draw_batch->vertices.clear();
		draw_batch->indices.clear();
		draw_batch->tex = tex;
	}/*
#ifdef NO_INDEXED_RENDER
#ifdef TWO_VERTEX_QUADS
	draw_batch->vert_count += count * 2;
#else
	draw_batch->vert_count += count * 6;
#endif
#else
#ifdef TWO_VERTEX_QUADS
	draw_batch->vert_count += count * 2;
	draw_batch->index_count += count * 2;
#else
	draw_batch->vert_count += count * 4;
	draw_batch->index_count += count * 6;
#endif
#endif*/
}

/// in TL TR BL BR order
void g_push_quad(Vertex* verts, UpscaleRequest do_upscale = UpscaleRequest {.w = 0, .h = 0}) {
	std::vector<Vertex, BatchAllocator<Vertex>>& vertices = draw_batch->vertices;
	//vertices.reserve(to_pot(draw_batch->vert_count));
#ifdef NO_INDEXED_RENDER
#ifdef TWO_VERTEX_QUADS
	vertices.push_back(verts[0]);
	vertices.push_back(verts[1]);
#else
	vertices.push_back(verts[0]);
	vertices.push_back(verts[2]);
	vertices.push_back(verts[1]);
	vertices.push_back(verts[1]);
	vertices.push_back(verts[2]);
	vertices.push_back(verts[3]);
#endif
#else
	u16 vert_idx = vertices.size();

	std::vector<u16, BatchAllocator<u16>>& indices = draw_batch->indices;
	//indices.reserve(to_pot(draw_batch->index_count));
	indices.push_back(vert_idx);
#ifdef TWO_VERTEX_QUADS
	indices.push_back(vert_idx + 1);
#else
	indices.push_back(vert_idx + 2);
	indices.push_back(vert_idx + 1);
	indices.push_back(vert_idx + 1);
	indices.push_back(vert_idx + 2);
	indices.push_back(vert_idx + 3);
#endif
	vertices.push_back(verts[0]);
	vertices.push_back(verts[1]);
#ifndef TWO_VERTEX_QUADS
	vertices.push_back(verts[2]);
	vertices.push_back(verts[3]);
#endif
#endif
	if(do_upscale.w > draw_batch->upscale_w) {
		draw_batch->upscale_w = do_upscale.w;
		draw_batch->upscale_h = do_upscale.h;
	}
}

void g_flush(DrawBatchRef batch) {
	if(batch->vertices.size()) {
		batch->draw();
		batch->vertices.clear();
		batch->vert_count = 0;
		batch->indices.clear();
		batch->index_count = 0;
	}
	batch->tex.destroy();
}

void g_flush() {
	//for(int i = 0; i < draw_batch_stack.size(); i++) {
	//	g_flush(draw_batch_stack[i]);
	//}
	g_flush(draw_batch);
}

HaloSettings HaloSettings::with_shadow() {
	return {.shadow_distance = 2, .shadow_detail = 1, .halo_detail = 0};
}

int HaloSettings::get_shadow_detail() {
	if(shadow_alpha == 0.f || shadow_distance == 0) return 0;
	if(shadow_detail >= 0) return shadow_detail;
	int sh = g_real_height();
	if(sh < 360) {
		return 3;
	}
	int factor = shadow_distance * sh / g_height();
	if(sh < 480) {
		return 6;
	} else if(sh < 720) {
		return 5 * factor / 2;
	}
	// scale indefinitely on each multiple of 720
	return 5 * factor * (sh / 720) / 2;
}

int HaloSettings::get_halo_detail() {
	if(halo_alpha == 0.f || halo_distance == 0) return 0;
	if(halo_detail >= 0) return halo_detail;
	int sh = g_real_height();
	if(sh < 360) {
		return 3;
	}
	int factor = halo_distance * sh / g_height();
	if(sh < 480) {
		return 6;
	} else if(sh < 720) {
		return 8 * factor / 2;
	}
	// scale indefinitely on each multiple of 720
	return 8 * factor * (sh / 720) / 2;
}

void HaloSettings::resolve() {
	if(halo_alpha > 1.f) halo_alpha = 1.f;
	else if(halo_alpha < 0.f) halo_alpha = 0.f;
	if(shadow_alpha > 1.f) shadow_alpha = 1.f;
	else if(shadow_alpha < 0.f) shadow_alpha = 0.f;
	halo_detail = get_halo_detail();
	shadow_detail = get_shadow_detail();
}

glm::mat4 prepare_matrix_for_render(float x, float y, float w, float h);

// from glm::radians implementation
constexpr float full_turn = 360.f * 0.01745329251994329576923690768489f;

void Vertex::set(const Vertex::vec3_t& pos, const glm::vec2& uv, const glm::u8vec4& color) {
	this->uv = uv
#ifdef LOW_BANDWIDTH
		* glm::vec2(32767, 32767)
#endif
	;
#ifndef NO_COLOR_OFF_ATTRIB
	this->color_off = g_cur_color_off;
#endif
	this->pos = pos;
	this->color = color;
}

Vertex::vec4_t transform_vertex(glm::vec4 vert);

void g_draw_quad_crop_halo(const TextureRef& tex, float x, float y, float w, float h, float tx, float ty, float tw, float th, HaloSettings halo, int corner, int origin) {
	if(tw == -1) tw = tex->w;
	if(th == -1) th = tex->h;
	g_calc(&x, &y, &w, &h, tw, th, corner, origin);
	if(w == 0 || h == 0) return;
	float rw = w, rh = h;
	g_virtual_to_real(&rw, &rh);
#ifdef TEXTURE_POWER_OF_TWO
	glm::vec4 uv = glm::vec4(tx, ty, tw, th) / glm::vec4(tex->buf_w, tex->buf_h, tex->buf_w, tex->buf_h);
#else
	glm::vec4 uv = glm::vec4(tx, ty, tw, th) / glm::vec4(tex->w, tex->h, tex->w, tex->h);
#endif
	glm::u8vec4 color1 = glm::clamp(g_cur_color, 0.f, 1.f) * 255.f + .5f;
	glm::u8vec4 color2 = glm::clamp(g_blend(g_cur_color, g_cur_color_2, g_cur_color_2_blend), 0.f, 1.f) * 255.f + .5f;
#ifdef TWO_VERTEX_QUADS
	Vertex verts[2];
	verts[0].set(transform_vertex(glm::vec4(x, y, 0.f, 1.f)), {uv.x, uv.y}, color1);
	verts[1].set(transform_vertex(glm::vec4(x + w, y + h, 0.f, 1.f)), {uv.x + uv.z, uv.y + uv.w}, color2);
	Vertex::vec3_t og_pos[] {verts[0].pos, verts[1].pos};
	Vertex::vec2_t bound_tl(glm::min(og_pos[0].x, og_pos[1].x), glm::min(og_pos[0].y, og_pos[1].y));
	Vertex::vec2_t bound_br(glm::max(og_pos[0].x, og_pos[1].x), glm::max(og_pos[0].y, og_pos[1].y));
#else
	Vertex verts[4];
	verts[0].set(transform_vertex(glm::vec4(x, y, 0.f, 1.f)), {uv.x, uv.y}, color1);
	verts[1].set(transform_vertex(glm::vec4(x + w, y, 0.f, 1.f)), {uv.x + uv.z, uv.y}, color1);
	verts[2].set(transform_vertex(glm::vec4(x, y + h, 0.f, 1.f)), {uv.x, uv.y + uv.w}, color2);
	verts[3].set(transform_vertex(glm::vec4(x + w, y + h, 0.f, 1.f)), {uv.x + uv.z, uv.y + uv.w}, color2);
	Vertex::vec3_t og_pos[] {verts[0].pos, verts[1].pos, verts[2].pos, verts[3].pos};
	Vertex::vec2_t bound_tl(glm::min(glm::min(og_pos[0].x, og_pos[1].x), glm::min(og_pos[2].x, og_pos[3].x)), glm::min(glm::min(og_pos[0].y, og_pos[1].y), glm::min(og_pos[2].y, og_pos[3].y)));
	Vertex::vec2_t bound_br(glm::max(glm::max(og_pos[0].x, og_pos[1].x), glm::max(og_pos[2].x, og_pos[3].x)), glm::max(glm::max(og_pos[0].y, og_pos[1].y), glm::max(og_pos[2].y, og_pos[3].y)));
#endif
	if(bound_tl.x > 1.f || bound_tl.y > 1.f || bound_br.x < 0.f || bound_br.y < 0.f) return;
	reserve_quads(tex, 1);
	if(halo.shadow_detail) {
		glm::u8vec4 shadow_color1(0, 0, 0, halo.shadow_alpha * color1.a + 0.5f);
		glm::u8vec4 shadow_color2(0, 0, 0, halo.shadow_alpha * color2.a + 0.5f);
		for(int i = 0; i < VERTEX_COUNT; i++)
			verts[i].color = i >= VERTEX_COUNT / 2? shadow_color1: shadow_color2;
		float shadow_step = (float) halo.shadow_distance / halo.shadow_detail;
		Vertex::vec3_t step_vec = transform_vertex(glm::vec4(shadow_step, shadow_step, 0, 0));
		reserve_quads(tex, halo.shadow_detail);
		for(int i = 0; i < halo.shadow_detail; i++) {
			for(int i = 0; i < VERTEX_COUNT; i++)
				verts[i].pos += step_vec;
			g_push_quad(verts, g_check_upscale(tex, rw, rh, tw, th));
		}
	}
	if(halo.halo_detail) {
		glm::u8vec4 halo_color1(0, 0, 0, halo.halo_alpha * color1.a + 0.5f);
		glm::u8vec4 halo_color2(0, 0, 0, halo.halo_alpha * color2.a + 0.5f);
		for(int i = 0; i < VERTEX_COUNT; i++)
			verts[i].color = i >= VERTEX_COUNT / 2? halo_color2: halo_color1;
#ifndef NO_COLOR_OFF_ATTRIB
		glm::vec4 halo_color_off = g_cur_color_off + glm::vec4(halo.halo_lightness, halo.halo_lightness, halo.halo_lightness, 0);
		for(int i = 0; i < VERTEX_COUNT; i++)
			verts[i].color_off = halo_color_off;
#endif
		float halo_detail_f = halo.halo_detail;
		float halo_step = full_turn / halo_detail_f;
		reserve_quads(tex, halo.halo_detail);
		float r = 0.f;
		for(int i = 0; i < halo.halo_detail; i++) {
			float off_x = glm::cos(r) * halo.halo_distance;
			float off_y = glm::sin(r) * halo.halo_distance;
			Vertex::vec3_t off_vec = transform_vertex(glm::vec4(off_x, off_y, 0, 0));
			for(int i = 0; i < VERTEX_COUNT; i++)
				verts[i].pos = og_pos[i] + off_vec;
			g_push_quad(verts, g_check_upscale(tex, rw, rh, tw, th));
			r += halo_step;
		}
#ifndef NO_COLOR_OFF_ATTRIB
		for(int i = 0; i < VERTEX_COUNT; i++)
			verts[i].color_off = g_cur_color_off;
#endif
	}
	if(halo.halo_detail || halo.shadow_detail) {
		for(int i = 0; i < VERTEX_COUNT; i++) {
			verts[i].pos = og_pos[i];
			verts[i].color = i >= VERTEX_COUNT / 2? color2: color1;
		}
	}
	g_push_quad(verts, g_check_upscale(tex, rw, rh, tw, th));
}

void g_draw_quad_crop(const TextureRef& tex, float x, float y, float w, float h, float tx, float ty, float tw, float th, int corner, int origin) {
	g_draw_quad_crop_halo(tex, x, y, w, h, tx, ty, tw, th, HaloSettings {.shadow_detail = 0, .halo_detail = 0}, corner, origin);
}

void g_draw_quad(const TextureRef& tex, float x, float y, float w, float h, int corner, int origin) {
	g_draw_quad_crop(tex, x, y, w, h, 0, 0, tex->w, tex->h, corner, origin);
}

void g_draw_crop(const TextureRef& tex, float x, float y, int tx, int ty, int tw, int th, int corner, int origin) {
	g_draw_quad_crop(tex, x, y, tw, th, tx, ty, tw, th, corner, origin);
}

void g_draw(const TextureRef& tex, float x, float y, int corner, int origin) {
	g_draw_quad_crop(tex, x, y, tex->w, tex->h, 0, 0, tex->w, tex->h, corner, origin);
}
