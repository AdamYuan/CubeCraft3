#ifndef CUBECRAFT3_RESOURCE_BLOCK_MESH_HPP
#define CUBECRAFT3_RESOURCE_BLOCK_MESH_HPP

#include <common/AABB.hpp>
#include <common/BlockFace.hpp>

#include <resource/texture/BlockTexture.hpp>

#include <cinttypes>
#include <limits>
#include <type_traits>

struct BlockMeshVertex {
	union {
		struct {
			uint8_t x{}, y{}, z{};
		};
		uint8_t pos[3];
	};
	uint8_t ao{4}; // 4 means auto AO
	constexpr BlockMeshVertex() {}
	template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
	constexpr BlockMeshVertex(T x, T y, T z, T ao = 4) : x(x), y(y), z(z), ao(ao) {}
};
struct BlockMeshFace {
	uint8_t axis;
	BlockFace light_face{}, render_face{};
	BlockTexture texture{};
	BlockMeshVertex vertices[4]{};
};
#define BLOCK_MESH_MAX_FACE_COUNT 32
constexpr uint32_t kBlockMeshMaxHitboxCount = 4;
struct BlockMesh {
	// "faces" array's BlockFace property should be sorted for better performance
	BlockMeshFace faces[BLOCK_MESH_MAX_FACE_COUNT];
	uint32_t face_count{};
	u8AABB hitboxes[kBlockMeshMaxHitboxCount];
	uint32_t hitbox_count{};
};
#undef BLOCK_MESH_MAX_FACE_COUNT

// predefined block meshes
struct BlockMeshes {
	inline static constexpr BlockMesh CactusSides() {
		return {{
		            {0,
		             BlockFaces::kRight,
		             BlockFaces::kRight,
		             {BlockTextures::kCactusSide},
		             {
		                 {15, 0, 16},
		                 {15, 0, 0},
		                 {15, 16, 0},
		                 {15, 16, 16},
		             }},
		            {0,
		             BlockFaces::kLeft,
		             BlockFaces::kLeft,
		             {BlockTextures::kCactusSide},
		             {
		                 {1, 0, 0},
		                 {1, 0, 16},
		                 {1, 16, 16},
		                 {1, 16, 0},
		             }},
		            {2,
		             BlockFaces::kFront,
		             BlockFaces::kFront,
		             {BlockTextures::kCactusSide},
		             {
		                 {0, 0, 15},
		                 {16, 0, 15},
		                 {16, 16, 15},
		                 {0, 16, 15},
		             }},
		            {2,
		             BlockFaces::kBack,
		             BlockFaces::kBack,
		             {BlockTextures::kCactusSide},
		             {
		                 {16, 0, 1},
		                 {0, 0, 1},
		                 {0, 16, 1},
		                 {16, 16, 1},
		             }},
		        },
		        4,
		        {{{1, 0, 1}, {15, 16, 15}}},
		        1};
	}
	inline static constexpr BlockMesh Cross(BlockTexID tex_id, int radius, int low, int high, bool double_side = false,
	                                        BlockFace light_face = BlockFaces::kTop) {
		return {{
		            {0,
		             light_face,
		             BlockFaces::kLeft,
		             {tex_id},
		             {
		                 {8 - radius, low, 8 - radius},
		                 {8 + radius, low, 8 + radius},
		                 {8 + radius, high, 8 + radius},
		                 {8 - radius, high, 8 - radius},
		             }},
		            {2,
		             light_face,
		             BlockFaces::kBack,
		             {tex_id},
		             {
		                 {8 - radius, low, 8 + radius},
		                 {8 + radius, low, 8 - radius},
		                 {8 + radius, high, 8 - radius},
		                 {8 - radius, high, 8 + radius},
		             }},
		            {0,
		             light_face,
		             BlockFaces::kRight,
		             {tex_id},
		             {
		                 {8 - radius, high, 8 - radius},
		                 {8 + radius, high, 8 + radius},
		                 {8 + radius, low, 8 + radius},
		                 {8 - radius, low, 8 - radius},
		             }},
		            {2,
		             light_face,
		             BlockFaces::kFront,
		             {tex_id},
		             {
		                 {8 - radius, high, 8 + radius},
		                 {8 + radius, high, 8 - radius},
		                 {8 + radius, low, 8 - radius},
		                 {8 - radius, low, 8 + radius},
		             }},
		        },
		        double_side ? 4u : 2u,
		        {{{8 - radius, low, 8 - radius}, {8 + radius, high, 8 + radius}}},
		        1};
	}
};

#endif