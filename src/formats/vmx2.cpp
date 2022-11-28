#include "formats.hpp"
#include <graphics.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <map>
#include <algorithm>

// Ever17 Xbox model format, stored in vpk archives

#define U8(x) (*(u8*) (x))
#define U16LE(x) le16(*(u16*) (x))
#define U32LE(x) le32(*(u32*) (x))
#define U32(x) (*(u32*) (x))

static MeshRef load_mesh(MemDataStream* ds, u32 mesh_idx, u16 motion_count, std::pair<std::string, Material>* materials, u16 material_count, const glm::mat4& transform) {
	//ds->skip(0xE);
	ds->skip(motion_count * 0x44 + 0xE);
	//ds->skip(2);
	u32 mat_id = mesh_idx; ds->read16();
	u32 inverse_bind_count = ds->read32();
	//char bone_guids[0x10][inverse_bind_count];
	//glm::mat4 bone_transforms[inverse_bind_count];
	for(int i = 0; i < inverse_bind_count; i++) {
		//ds->skip(4);
		//ds->skip(0x10); //ds->read(bone_guids[i], 0x10);
		ds->skip(0x14);
		glm::mat4 bone_transform = glm::transpose(glm::mat4 {
			{ds->read_float(), ds->read_float(), ds->read_float(), ds->read_float()},
			{ds->read_float(), ds->read_float(), ds->read_float(), ds->read_float()},
			{ds->read_float(), ds->read_float(), ds->read_float(), ds->read_float()},
			{ds->read_float(), ds->read_float(), ds->read_float(), ds->read_float()}
		});
	}
	u32 vert_count = ds->read32();
	MeshLoadData mesh;
	if(mat_id >= material_count) {
		log_err("too few materials in vmx2");
		platform_throw();
	}
	mesh.material = materials[mat_id].second;
	mesh.verts.reserve(vert_count);
	for(int i = 0; i < vert_count; i++) {
		glm::vec3 pos = {ds->read_float(), ds->read_float(), ds->read_float()};
		glm::vec4 weights = {ds->read_float(), ds->read_float(), ds->read_float(), ds->read_float()};
		glm::u8vec4 bone_ids = {ds->read8(), ds->read8(), ds->read8(), ds->read8()};
		//weight_data_list.append([i, weights, bone_ids])
		glm::vec3 normal = {ds->read_float(), ds->read_float(), ds->read_float()};
		glm::vec2 uv = {ds->read_float(), ds->read_float()};
		ds->skip(8);
		mesh.verts.push_back(Vertex3D {
			.weights = weights,
			.bone_ids = bone_ids,
			.uv = uv,
			.normal = normal,
			.pos = pos
		});
	}
	u32 index_count = ds->read32();
	mesh.indices.reserve(index_count);
	for(int i = 0; i < index_count; i++) {
		mesh.indices.push_back(ds->read16());
	}
	mesh.local_transform = transform;
	return g_upload_mesh(mesh);
}

SceneRef load_vmx2(DataStream* fds) {
	fds->seek(0);
	Mem mem(fds->remaining());
	fds->read(mem.data_ptr(), fds->remaining());
	MemDataStream* ds = new MemDataStream(mem);
	ds->seek(6);
	u16 material_count = ds->read16();
	ds->seek(0xE);
	u16 node_count = ds->read16();
	ds->skip(2);
	u16 motion_count = ds->read16();
	u16 mesh_counter = 0;
	ds->skip(0x50 * motion_count + 0x24);
	std::map<std::string, TextureRef> texs;
	std::pair<std::string, Material> materials[material_count];
	std::string tex_root = fds->filename();
	auto slash_it = tex_root.find_last_of('/');
	if(slash_it != std::string::npos) {
		tex_root = tex_root.substr(0, slash_it + 1);
	} else {
		slash_it = tex_root.find_last_of(':');
		if(slash_it != std::string::npos) {
			tex_root = tex_root.substr(0, slash_it + 1);
		} else {
			tex_root = "";
		}
	}
	for(int i = 0; i < material_count; i++) {
		ds->skip(0x42);
		char material_name[0x20];
		ds->read(material_name, 0x20);
		materials[i].first = material_name;
		auto tex_it = texs.find(material_name);
		if(tex_it != texs.end()) {
			materials[i].second.tex = tex_it->second;
		} else {
			std::string tex_path = tex_root + (const char*) material_name;
			//if(material_name[0] && material_name[0] != '/' && FileDataStream::exists(filename.c_str(), false)) {
			//log_info("%s %s", filename.c_str(), tex_path.c_str(), material_name);
			DataStream* file = fs_file(tex_path.c_str());
			TextureRef tex = g_load_texture(*file);
			delete file;
			materials[i].second.tex = tex;
			texs[material_name] = materials[i].second.tex;
		}
		if(i != material_count - 1) {
			ds->skip(0x82);
		} else {
			ds->skip(0xE0);
		}
	}
	std::string filename = fds->filename();
	if(filename.find("TU") != filename.npos || filename.find("KO") != filename.npos) {
		std::sort(materials, materials + material_count, [](auto a, auto b) {
			return a.first.compare(b.first) < 0;
		});
	}
	SceneRef scene = SceneRef(new SceneData);
	scene->transform = glm::identity<glm::mat4>();
	int mesh_idx = 0;
	for(int i = 0; i < node_count; i++) {
		ds->skip(4);
		char node_name[0x20];
		ds->read(node_name, 0x20);
		char guid[0x10];
		ds->read(guid, 0x10);
		char parent_guid[0x10];
		ds->read(parent_guid, 0x10);
		ds->skip(4);
		glm::mat4 transform = glm::transpose(glm::mat4 {
			{ds->read_float(), ds->read_float(), ds->read_float(), ds->read_float()},
			{ds->read_float(), ds->read_float(), ds->read_float(), ds->read_float()},
			{ds->read_float(), ds->read_float(), ds->read_float(), ds->read_float()},
			{ds->read_float(), ds->read_float(), ds->read_float(), ds->read_float()}
		});
		u16 node_motion_count = ds->read16();
		ds->skip(2);
		switch(node_name[0]) {
			case 'M': {
				MeshRef mesh = load_mesh(ds, mesh_idx, node_motion_count, materials, material_count, transform);
				scene->meshes.push_back(mesh);
				mesh_idx++;
				break;
			}
			case 'B':
				return scene;
				break;
			case 'N':
				ds->skip((node_motion_count * 4) + 4);
				break;
			default:
				log_err("unknown node type in vmx2");
				platform_throw();
				return scene;
		}
	}
	delete ds;
	return scene;
}
