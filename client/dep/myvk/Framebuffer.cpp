#include "Framebuffer.hpp"

namespace myvk {

std::shared_ptr<Framebuffer> Framebuffer::Create(const std::shared_ptr<RenderPass> &render_pass,
                                                 const std::vector<std::shared_ptr<ImageView>> &image_views,
                                                 const VkExtent2D &extent, uint32_t layers) {
	std::shared_ptr<Framebuffer> ret = std::make_shared<Framebuffer>();
	ret->m_render_pass_ptr = render_pass;
	ret->m_image_view_ptrs = image_views;
	ret->m_extent = extent;
	ret->m_layers = layers;

	std::vector<VkImageView> attachments(image_views.size());
	for (uint32_t i = 0; i < image_views.size(); ++i)
		attachments[i] = image_views[i]->GetHandle();

	VkFramebufferCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	create_info.renderPass = render_pass->GetHandle();
	create_info.attachmentCount = attachments.size();
	create_info.pAttachments = attachments.data();
	create_info.width = extent.width;
	create_info.height = extent.height;
	create_info.layers = layers;

	if (vkCreateFramebuffer(render_pass->GetDevicePtr()->GetHandle(), &create_info, nullptr, &ret->m_framebuffer) !=
	    VK_SUCCESS)
		return nullptr;

	return ret;
}

std::shared_ptr<Framebuffer> Framebuffer::Create(const std::shared_ptr<RenderPass> &render_pass,
                                                 const std::shared_ptr<ImageView> &image_view) {
	return Create(render_pass, {image_view}, image_view->GetImagePtr()->GetExtent2D(), 1);
}
} // namespace myvk
