#include <algorithm>

#include "utils/Assert.h"
#include "Descriptor.h"

static const uint32_t SETS_PER_POOL = 1000;
static const std::vector<VkDescriptorPoolSize> POOL_SIZES =
    {
	{ VK_DESCRIPTOR_TYPE_SAMPLER, SETS_PER_POOL },
	{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, SETS_PER_POOL },
	{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, SETS_PER_POOL },
	{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, SETS_PER_POOL },
	{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, SETS_PER_POOL },
	{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, SETS_PER_POOL },
	{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SETS_PER_POOL },
	{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, SETS_PER_POOL },
	{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, SETS_PER_POOL },
	{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, SETS_PER_POOL },
	{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, SETS_PER_POOL },
	{ VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK, SETS_PER_POOL },
	{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, SETS_PER_POOL }
    };

DescriptorAllocator::DescriptorAllocator(std::shared_ptr<Device> device) {
    device_ = device;
}

DescriptorAllocator::~DescriptorAllocator() {
    for (auto pool : used_pools_) {
	vkDestroyDescriptorPool(device_->get_device(), pool, nullptr);
    }
    for (auto pool : free_pools_) {
	vkDestroyDescriptorPool(device_->get_device(), pool, nullptr);
    }
    for (auto [_, layout] : layout_cache_) {
	vkDestroyDescriptorSetLayout(device_->get_device(), layout, nullptr);
    }
}

VkDescriptorPool DescriptorAllocator::grab_pool() {
    if (used_pools_.size() > 0) {
	return used_pools_.back();
    } else {
	return grab_new_pool();
    }
}

VkDescriptorPool DescriptorAllocator::grab_new_pool() {
    if (free_pools_.size() > 0) {
	VkDescriptorPool pool = free_pools_.back();
	free_pools_.pop_back();
	used_pools_.push_back(pool);
	return pool;
    } else {
	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = 0;
	pool_info.maxSets = SETS_PER_POOL;
	pool_info.poolSizeCount = static_cast<uint32_t>(POOL_SIZES.size());
	pool_info.pPoolSizes = POOL_SIZES.data();
	
	VkDescriptorPool descriptor_pool;
	vkCreateDescriptorPool(device_->get_device(), &pool_info, nullptr, &descriptor_pool);
	used_pools_.push_back(descriptor_pool);
	return descriptor_pool;
    }
}

static bool operator==(const std::pair<std::vector<VkDescriptorSetLayoutBinding>, VkDescriptorSetLayoutCreateFlagBits> &a, const std::pair<std::vector<VkDescriptorSetLayoutBinding>, VkDescriptorSetLayoutCreateFlagBits> &b) {
    if (a.first.size() != b.first.size() || a.second != b.second) {
	return false;
    }
    for (int i = 0; i < a.first.size(); i++) {
	if (a.first.at(i).binding != b.first.at(i).binding){
	    return false;
	}
	if (a.first.at(i).descriptorType != b.first.at(i).descriptorType){
	    return false;
	}
	if (a.first.at(i).descriptorCount != b.first.at(i).descriptorCount){
	    return false;
	}
	if (a.first.at(i).stageFlags != b.first.at(i).stageFlags){
	    return false;
	}
    }
    return true;
}

VkDescriptorSetLayout DescriptorAllocator::grab_layout(const std::vector<VkDescriptorSetLayoutBinding> &bindings, VkDescriptorSetLayoutCreateFlagBits flags) {
    auto it = layout_cache_.find({bindings, flags});
    if (it != layout_cache_.end()) {
	return it->second;
    }

    VkDescriptorSetLayoutCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = flags;
    create_info.bindingCount = static_cast<uint32_t>(bindings.size());
    create_info.pBindings = bindings.data();

    VkDescriptorSetLayout layout;
    vkCreateDescriptorSetLayout(device_->get_device(), &create_info, nullptr, &layout);
    layout_cache_.insert({{bindings, flags}, layout});
    return layout;
}

void DescriptorAllocator::reset_pools() {
    for (auto pool : used_pools_){
	vkResetDescriptorPool(device_->get_device(), pool, 0);
	free_pools_.push_back(pool);
    }
    
    used_pools_.clear();
}

std::shared_ptr<Device> DescriptorAllocator::get_device() {
    return device_;
}

DescriptorSetLayout::DescriptorSetLayout(std::shared_ptr<DescriptorAllocator> allocator, VkDescriptorSetLayoutCreateFlagBits flags) {
    allocator_ = allocator;
    flags_ = flags;
}

void DescriptorSetLayout::add_binding(VkDescriptorSetLayoutBinding binding) {
    bindings_.push_back(binding);
}

VkDescriptorSetLayout DescriptorSetLayout::get_layout() {
    std::sort(bindings_.begin(), bindings_.end(), [](VkDescriptorSetLayoutBinding& a, VkDescriptorSetLayoutBinding& b ){
	return a.binding < b.binding;
    });
    return allocator_->grab_layout(bindings_, flags_);
}

DescriptorSet::DescriptorSet(std::shared_ptr<DescriptorAllocator> allocator, std::shared_ptr<DescriptorSetLayout> layout) {
    VkDescriptorPool pool = allocator->grab_pool();
    VkDescriptorSetLayout stack_layout = layout->get_layout();
    
    VkDescriptorSetAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.pNext = nullptr;
    allocate_info.pSetLayouts = &stack_layout;
    allocate_info.descriptorPool = pool;
    allocate_info.descriptorSetCount = 1;

    VkResult could_allocate = vkAllocateDescriptorSets(allocator->get_device()->get_device(), &allocate_info, &set_);

    if (could_allocate == VK_ERROR_FRAGMENTED_POOL || could_allocate == VK_ERROR_OUT_OF_POOL_MEMORY) {
	allocate_info.descriptorPool = allocator->grab_new_pool();
	ASSERT(vkAllocateDescriptorSets(allocator->get_device()->get_device(), &allocate_info, &set_), "Couldn't allocate descriptor set out of new pool.");
    } else {
	ASSERT(could_allocate, "Couldn't allocate descriptor set out of pool.");
    }

    allocator_ = allocator;
}