#include <client/DepthHierarchy.hpp>

inline uint32_t group_x_8(uint32_t x) { return (x >> 3u) + (((x & 0x7u) > 0u) ? 1u : 0u); }

void DepthHierarchy::create_images() {
	VkExtent2D size = m_frame_manager_ptr->GetExtent();
	uint32_t mip_level = myvk::Image::QueryMipLevel(size);
	const std::shared_ptr<myvk::Device> &device = m_frame_manager_ptr->GetDevicePtr();

	{
		VkSamplerCreateInfo create_info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
		create_info.magFilter = VK_FILTER_LINEAR;
		create_info.minFilter = VK_FILTER_LINEAR;
		create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		create_info.minLod = 0.0f;
		create_info.maxLod = (float)mip_level;
		VkSamplerReductionModeCreateInfo reduction_create_info = {VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO};
		reduction_create_info.reductionMode = VK_SAMPLER_REDUCTION_MODE_MAX;
		create_info.pNext = &reduction_create_info;

		m_sampler = myvk::Sampler::Create(device, create_info);
	}

	// for clearing the images
	std::shared_ptr<myvk::CommandPool> command_pool =
	    myvk::CommandPool::Create(m_frame_manager_ptr->GetSwapchain()->GetGraphicsQueuePtr());
	std::shared_ptr<myvk::CommandBuffer> command_buffer = myvk::CommandBuffer::Create(command_pool);
	command_buffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	for (auto &data : m_frame_data) {
		data.m_image =
		    myvk::Image::CreateTexture2D(device, size, mip_level, VK_FORMAT_R32_SFLOAT,
		                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
		                                     VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
		{
			command_buffer->CmdPipelineBarrier(
			    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, {}, {},
			    {data.m_image->GetMemoryBarrier(VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
			                                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)});
			command_buffer->CmdClearColorImage(data.m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			                                   {{1.0, 1.0, 1.0, 1.0}});
			command_buffer->CmdPipelineBarrier(
			    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, {}, {},
			    {data.m_image->GetMemoryBarrier(VK_IMAGE_ASPECT_COLOR_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
			                                    VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)});
		}
		data.m_image_view = myvk::ImageView::Create(data.m_image, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT);
		data.m_lod_image_views.clear();
		data.m_lod_image_views.resize(mip_level);
		for (uint32_t i = 0; i < mip_level; ++i) {
			data.m_lod_image_views[i] =
			    myvk::ImageView::Create(data.m_image, i, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT);
		}
	}

	command_buffer->End();
	std::shared_ptr<myvk::Fence> fence = myvk::Fence::Create(m_frame_manager_ptr->GetDevicePtr());
	command_buffer->Submit(fence);
	fence->Wait();
}

void DepthHierarchy::create_descriptor_pool() {
	// must be called after create_images
	const std::shared_ptr<myvk::Device> &device = m_frame_manager_ptr->GetDevicePtr();
	uint32_t mip_level = m_frame_data[0].m_image->GetMipLevels();
	m_descriptor_pool =
	    myvk::DescriptorPool::Create(device, mip_level * kFrameCount,
	                                 {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, mip_level * kFrameCount},
	                                  {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, mip_level * kFrameCount}});
}

void DepthHierarchy::create_build_descriptors() {
	const std::shared_ptr<myvk::Device> &device = m_frame_manager_ptr->GetDevicePtr();
	uint32_t mip_level = m_frame_data[0].m_image->GetMipLevels();
	m_build_descriptor_set_layout = myvk::DescriptorSetLayout::Create(
	    device, {{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
	             {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT}});
	if (mip_level == 1)
		return;
	for (auto &data : m_frame_data) {
		data.m_build_descriptor_sets.clear();
		data.m_build_descriptor_sets.resize(mip_level - 1);
		for (uint32_t i = 0; i < mip_level - 1; ++i) {
			data.m_build_descriptor_sets[i] =
			    myvk::DescriptorSet::Create(m_descriptor_pool, m_build_descriptor_set_layout);
			data.m_build_descriptor_sets[i]->UpdateCombinedImageSampler(m_sampler, data.m_lod_image_views[i],
			                                                            0); // upper lod
			data.m_build_descriptor_sets[i]->UpdateStorageImage(data.m_lod_image_views[i + 1], 1);
		}
	}
}

void DepthHierarchy::create_output_descriptors() {
	const std::shared_ptr<myvk::Device> &device = m_frame_manager_ptr->GetDevicePtr();
	m_output_descriptor_set_layout = myvk::DescriptorSetLayout::Create(
	    device, {{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT}});

	for (auto &data : m_frame_data) {
		data.m_output_descriptor_set = myvk::DescriptorSet::Create(m_descriptor_pool, m_output_descriptor_set_layout);
		data.m_output_descriptor_set->UpdateCombinedImageSampler(m_sampler, data.m_image_view, 0);
	}
}

/* void DepthHierarchy::create_build_buffers() {
    const std::shared_ptr<myvk::Device> &device = m_canvas_ptr->GetFrameManagerPtr()->GetDevicePtr();
    VkExtent2D image_size = m_canvas_ptr->GetFrameManagerPtr()->GetExtent();
    VkDeviceSize buffer_size = image_size.width * image_size.height * sizeof(float);

    for (auto &data : m_frame_data)
        data.m_build_buffer = myvk::Buffer::Create(device, buffer_size, VMA_MEMORY_USAGE_GPU_ONLY,
                                                   VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
} */

void DepthHierarchy::create_build_pipeline() {
	const std::shared_ptr<myvk::Device> &device = m_frame_manager_ptr->GetDevicePtr();
	m_build_pipeline_layout = myvk::PipelineLayout::Create(device, {m_build_descriptor_set_layout}, {});

	constexpr const uint32_t kDepthHierarchyCompSpv[] = {
#include <client/shader/depth_hierarchy.comp.u32>
	};
	m_build_pipeline = myvk::ComputePipeline::Create(
	    m_build_pipeline_layout,
	    myvk::ShaderModule::Create(device, kDepthHierarchyCompSpv, sizeof(kDepthHierarchyCompSpv)));
}

void DepthHierarchy::CmdBuild(const std::shared_ptr<myvk::CommandBuffer> &command_buffer) const {
	const FrameData &data = m_frame_data[m_frame_manager_ptr->GetCurrentFrame()];
	uint32_t width = data.m_image->GetExtent().width, height = data.m_image->GetExtent().height,
	         mip_level = data.m_image->GetMipLevels();

	/* command_buffer->CmdPipelineBarrier(
	    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, {}, {},
	    {// set first lod for sampling
	     data.m_image->GetMemoryBarrier({VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	                                    VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)}); */

	command_buffer->CmdPipelineBarrier(
	    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, {}, {},
	    {// set second lod to be generated
	     data.m_image->GetMemoryBarrier({VK_IMAGE_ASPECT_COLOR_BIT, 1, 1, 0, 1}, 0, VK_ACCESS_SHADER_WRITE_BIT,
	                                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL)});

	{
		command_buffer->CmdBindPipeline(m_build_pipeline);
		for (uint32_t i = 1, w = width, h = height; i < mip_level; ++i) {
			w = std::max(w >> 1u, 1u);
			h = std::max(h >> 1u, 1u);
			command_buffer->CmdBindDescriptorSets({data.m_build_descriptor_sets[i - 1]}, m_build_pipeline);
			command_buffer->CmdDispatch(group_x_8(w), group_x_8(h), 1);
			if (i < mip_level - 1) {
				command_buffer->CmdPipelineBarrier(
				    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, {}, {},
				    {// set current lod for sampling
				     data.m_image->GetMemoryBarrier({VK_IMAGE_ASPECT_COLOR_BIT, i, 1, 0, 1}, VK_ACCESS_SHADER_WRITE_BIT,
				                                    VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL,
				                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
				     // set next lod to be generated
				     data.m_image->GetMemoryBarrier({VK_IMAGE_ASPECT_COLOR_BIT, i + 1, 1, 0, 1}, 0,
				                                    VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				                                    VK_IMAGE_LAYOUT_GENERAL)});
			}
		}
	}

	command_buffer->CmdPipelineBarrier(
	    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, {}, {},
	    {// set last lod for sampling
	     data.m_image->GetMemoryBarrier({VK_IMAGE_ASPECT_COLOR_BIT, mip_level - 1, 1, 0, 1}, VK_ACCESS_SHADER_WRITE_BIT,
	                                    VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL,
	                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)});
}
