#ifndef WORLD_HPP
#define WORLD_HPP

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/hash.hpp>
#include <unordered_map>
#include <vector>

#include <concurrentqueue.h>
#include <thread>

#include "Chunk.hpp"
#include "ChunkWorker.hpp"

class World : std::enable_shared_from_this<World> {
public:
	inline static std::shared_ptr<World> Create() { return std::make_shared<World>(); }

private:
	std::unordered_map<glm::i16vec3, std::shared_ptr<Chunk>> m_chunks;

	bool m_chunk_threads_run = true;
	moodycamel::ConcurrentQueue<std::unique_ptr<ChunkWorker>> m_chunk_workers;
	std::vector<std::thread> m_chunk_threads;

	void launch_chunk_threads();
	void chunk_thread_func();

public:
	inline void PushWorker(std::unique_ptr<ChunkWorker> &&worker) { m_chunk_workers.enqueue(std::move(worker)); }
	std::shared_ptr<Chunk> FindChunk(const glm::i16vec3 &position) const {
		auto it = m_chunks.find(position);
		return it == m_chunks.end() ? nullptr : it->second;
	}
	std::shared_ptr<Chunk> PushChunk(const glm::i16vec3 &position) {
		{ // If exists, return the existing one
			const std::shared_ptr<Chunk> &ret = FindChunk(position);
			if (ret)
				return ret;
		}
		// else, create a new chunk
		std::shared_ptr<Chunk> ret = Chunk::Create(weak_from_this(), position);
		m_chunks[position] = ret;

		// assign chunk neighbours
		for (uint32_t i = 0; i < 26; ++i) {
			glm::i16vec3 dp;
			Chunk::NeighbourIndex2CmpXYZ(i, glm::value_ptr(dp));

			const std::shared_ptr<Chunk> &nei = FindChunk(position + dp);
			ret->SetNeighbour(i, nei);
			if (nei)
				nei->SetNeighbour(Chunk::CmpXYZ2NeighbourIndex(-dp.x, -dp.y, -dp.z), ret);
		}
		return ret;
	}
	void EraseChunk(const glm::i16vec3 &position) { m_chunks.erase(position); }

	World() { launch_chunk_threads(); }
	~World();
};

#endif
