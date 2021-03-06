#ifndef MYVK_IMGUI_RENDERER_HPP
#define MYVK_IMGUI_RENDERER_HPP

#include "Buffer.hpp"
#include "CommandBuffer.hpp"
#include "CommandPool.hpp"
#include "DescriptorPool.hpp"
#include "DescriptorSet.hpp"
#include "GraphicsPipeline.hpp"
#include "Image.hpp"
#include "ImageView.hpp"
#include "Sampler.hpp"

namespace myvk {
class ImGuiRenderer {
private:
	std::shared_ptr<myvk::Image> m_font_texture;
	std::shared_ptr<myvk::ImageView> m_font_texture_view;
	std::shared_ptr<myvk::Sampler> m_font_texture_sampler;

	std::shared_ptr<myvk::DescriptorSetLayout> m_descriptor_set_layout;
	std::shared_ptr<myvk::DescriptorPool> m_descriptor_pool;
	std::shared_ptr<myvk::DescriptorSet> m_descriptor_set;

	std::shared_ptr<myvk::PipelineLayout> m_pipeline_layout;
	std::shared_ptr<myvk::GraphicsPipeline> m_pipeline;

	mutable std::vector<std::shared_ptr<myvk::Buffer>> m_vertex_buffers, m_index_buffers;

	void create_font_texture(const std::shared_ptr<myvk::CommandPool> &graphics_command_pool);

	void create_descriptor(const std::shared_ptr<myvk::Device> &device);

	void create_pipeline(const std::shared_ptr<myvk::RenderPass> &render_pass, uint32_t subpass);

	void setup_render_state(const std::shared_ptr<myvk::CommandBuffer> &command_buffer, int fb_width, int fb_height,
	                        uint32_t current_frame) const;

public:
	void Initialize(const std::shared_ptr<myvk::CommandPool> &command_pool,
	                const std::shared_ptr<myvk::RenderPass> &render_pass, uint32_t subpass, uint32_t frame_count);

	void CmdDrawPipeline(const std::shared_ptr<myvk::CommandBuffer> &command_buffer, uint32_t current_frame) const;
};
} // namespace myvk

#endif
