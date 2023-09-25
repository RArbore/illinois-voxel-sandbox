#include <algorithm>

#include "Descriptor.h"
#include "utils/Assert.h"

static const uint32_t MAX_MODELS = 256;
static const uint32_t SETS_PER_POOL = 1000;
static const std::vector<VkDescriptorPoolSize> POOL_SIZES = {
    {VK_DESCRIPTOR_TYPE_SAMPLER, SETS_PER_POOL},
    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, SETS_PER_POOL},
    {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, SETS_PER_POOL},
    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, SETS_PER_POOL},
    {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, SETS_PER_POOL},
    {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, SETS_PER_POOL},
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SETS_PER_POOL},
    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, SETS_PER_POOL},
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, SETS_PER_POOL},
    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, SETS_PER_POOL},
    {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, SETS_PER_POOL},
    {VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK, SETS_PER_POOL},
    {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, SETS_PER_POOL}};

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
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        pool_info.maxSets = SETS_PER_POOL;
        pool_info.poolSizeCount = static_cast<uint32_t>(POOL_SIZES.size());
        pool_info.pPoolSizes = POOL_SIZES.data();

        VkDescriptorPool descriptor_pool;
        vkCreateDescriptorPool(device_->get_device(), &pool_info, nullptr,
                               &descriptor_pool);
        used_pools_.push_back(descriptor_pool);
        return descriptor_pool;
    }
}

static bool
operator==(const std::pair<std::vector<VkDescriptorSetLayoutBinding>,
                           VkDescriptorSetLayoutCreateFlags> &a,
           const std::pair<std::vector<VkDescriptorSetLayoutBinding>,
                           VkDescriptorSetLayoutCreateFlags> &b) {
    if (a.first.size() != b.first.size() || a.second != b.second) {
        return false;
    }
    for (int i = 0; i < a.first.size(); i++) {
        if (a.first.at(i).binding != b.first.at(i).binding) {
            return false;
        }
        if (a.first.at(i).descriptorType != b.first.at(i).descriptorType) {
            return false;
        }
        if (a.first.at(i).descriptorCount != b.first.at(i).descriptorCount) {
            return false;
        }
        if (a.first.at(i).stageFlags != b.first.at(i).stageFlags) {
            return false;
        }
    }
    return true;
}

VkDescriptorSetLayout DescriptorAllocator::grab_layout(
    const std::vector<VkDescriptorSetLayoutBinding> &bindings,
    VkDescriptorSetLayoutCreateFlags flags,
    std::unordered_set<uint32_t> bindless_indices) {
    auto it = layout_cache_.find({bindings, flags});
    if (it != layout_cache_.end()) {
        return it->second;
    }

    VkDescriptorBindingFlags bindless_flags =
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
    std::vector<VkDescriptorBindingFlags> tiled_bindless_flags(bindings.size(),
                                                               0);
    for (auto idx : bindless_indices) {
        tiled_bindless_flags.at(idx) = bindless_flags;
    }

    VkDescriptorSetLayoutBindingFlagsCreateInfo
        layout_binding_flags_create_info{};
    layout_binding_flags_create_info.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    layout_binding_flags_create_info.bindingCount =
        static_cast<uint32_t>(tiled_bindless_flags.size());
    layout_binding_flags_create_info.pBindingFlags =
        tiled_bindless_flags.data();

    VkDescriptorSetLayoutCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.pNext = &layout_binding_flags_create_info;
    create_info.flags = flags;
    create_info.bindingCount = static_cast<uint32_t>(bindings.size());
    create_info.pBindings = bindings.data();

    VkDescriptorSetLayout layout;
    vkCreateDescriptorSetLayout(device_->get_device(), &create_info, nullptr,
                                &layout);
    layout_cache_.insert({{bindings, flags}, layout});
    return layout;
}

void DescriptorAllocator::reset_pools() {
    for (auto pool : used_pools_) {
        vkResetDescriptorPool(device_->get_device(), pool, 0);
        free_pools_.push_back(pool);
    }

    used_pools_.clear();
}

std::shared_ptr<Device> DescriptorAllocator::get_device() { return device_; }

DescriptorSetLayout::DescriptorSetLayout(
    std::shared_ptr<DescriptorAllocator> allocator,
    VkDescriptorSetLayoutCreateFlags flags) {
    allocator_ = allocator;
    flags_ = flags;
}

void DescriptorSetLayout::add_binding(VkDescriptorSetLayoutBinding binding,
                                      bool bindless) {
    bindings_.push_back(binding);
    if (bindless) {
        bindless_indices_.insert(binding.binding);
    }
}

VkDescriptorSetLayout DescriptorSetLayout::get_layout() {
    std::sort(
        bindings_.begin(), bindings_.end(),
        [](VkDescriptorSetLayoutBinding &a, VkDescriptorSetLayoutBinding &b) {
            return a.binding < b.binding;
        });
    return allocator_->grab_layout(bindings_, flags_, bindless_indices_);
}

DescriptorSet::DescriptorSet(std::shared_ptr<DescriptorAllocator> allocator,
                             std::shared_ptr<DescriptorSetLayout> layout) {
    VkDescriptorPool pool = allocator->grab_pool();
    layout_ = layout->get_layout();

    VkDescriptorSetAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.pNext = nullptr;
    allocate_info.pSetLayouts = &layout_;
    allocate_info.descriptorPool = pool;
    allocate_info.descriptorSetCount = 1;

    VkResult could_allocate = vkAllocateDescriptorSets(
        allocator->get_device()->get_device(), &allocate_info, &set_);

    if (could_allocate == VK_ERROR_FRAGMENTED_POOL ||
        could_allocate == VK_ERROR_OUT_OF_POOL_MEMORY) {
        allocate_info.descriptorPool = allocator->grab_new_pool();
        ASSERT(vkAllocateDescriptorSets(allocator->get_device()->get_device(),
                                        &allocate_info, &set_),
               "Couldn't allocate descriptor set out of new pool.");
    } else {
        ASSERT(could_allocate, "Couldn't allocate descriptor set out of pool.");
    }

    allocator_ = allocator;
}

VkDescriptorSet DescriptorSet::get_set() { return set_; }

VkDescriptorSetLayout DescriptorSet::get_layout() { return layout_; }

DescriptorSetBuilder::DescriptorSetBuilder(
    std::shared_ptr<DescriptorAllocator> allocator,
    VkDescriptorSetLayoutCreateFlags layout_flags) {
    allocator_ = allocator;
    layout_ = std::make_shared<DescriptorSetLayout>(allocator_, layout_flags);
    buffer_infos_.reserve(128);
    image_infos_.reserve(128);
    acceleration_structure_infos_.reserve(128);
}

DescriptorSetBuilder &DescriptorSetBuilder::bind_buffer(
    uint32_t binding, VkDescriptorBufferInfo buffer_info, VkDescriptorType type,
    VkShaderStageFlags stages) {
    VkDescriptorSetLayoutBinding layout_binding{};
    layout_binding.descriptorCount = 1;
    layout_binding.descriptorType = type;
    layout_binding.pImmutableSamplers = nullptr;
    layout_binding.stageFlags = stages;
    layout_binding.binding = binding;
    layout_->add_binding(layout_binding);

    buffer_infos_.push_back(buffer_info);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = &buffer_infos_.back();
    write.dstBinding = binding;
    writes_.push_back(write);

    return *this;
}

DescriptorSetBuilder &DescriptorSetBuilder::bind_buffers(
    uint32_t binding,
    std::vector<std::pair<VkDescriptorBufferInfo, uint32_t>> buffer_infos,
    VkDescriptorType type, VkShaderStageFlags stages) {
    VkDescriptorSetLayoutBinding layout_binding{};
    layout_binding.descriptorCount = MAX_MODELS;
    layout_binding.descriptorType = type;
    layout_binding.pImmutableSamplers = nullptr;
    layout_binding.stageFlags = stages;
    layout_binding.binding = binding;
    layout_->add_binding(layout_binding, true);

    for (auto [buffer_info, _] : buffer_infos) {
        buffer_infos_.push_back(buffer_info);
    }

    for (size_t i = 0; i < buffer_infos.size(); ++i) {
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.descriptorCount = 1;
        write.descriptorType = type;
        write.pBufferInfo = buffer_infos.empty() ? nullptr
                                               : &buffer_infos_.back() -
                                                     buffer_infos.size() + i + 1;
        write.dstBinding = binding;
        write.dstArrayElement = buffer_infos.at(i).second;
        writes_.push_back(write);
    }

    return *this;
}

DescriptorSetBuilder &DescriptorSetBuilder::bind_image(
    uint32_t binding, VkDescriptorImageInfo image_info, VkDescriptorType type,
    VkShaderStageFlags stages) {
    VkDescriptorSetLayoutBinding layout_binding{};
    layout_binding.descriptorCount = 1;
    layout_binding.descriptorType = type;
    layout_binding.pImmutableSamplers = nullptr;
    layout_binding.stageFlags = stages;
    layout_binding.binding = binding;
    layout_->add_binding(layout_binding);

    image_infos_.push_back(image_info);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &image_infos_.back();
    write.dstBinding = binding;
    writes_.push_back(write);

    return *this;
}

DescriptorSetBuilder &DescriptorSetBuilder::bind_images(
    uint32_t binding,
    std::vector<std::pair<VkDescriptorImageInfo, uint32_t>> image_infos,
    VkDescriptorType type, VkShaderStageFlags stages) {
    VkDescriptorSetLayoutBinding layout_binding{};
    layout_binding.descriptorCount = MAX_MODELS;
    layout_binding.descriptorType = type;
    layout_binding.pImmutableSamplers = nullptr;
    layout_binding.stageFlags = stages;
    layout_binding.binding = binding;
    layout_->add_binding(layout_binding, true);

    for (auto [image_info, _] : image_infos) {
        image_infos_.push_back(image_info);
    }

    for (size_t i = 0; i < image_infos.size(); ++i) {
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.descriptorCount = 1;
        write.descriptorType = type;
        write.pImageInfo = image_infos.empty() ? nullptr
                                               : &image_infos_.back() -
                                                     image_infos.size() + i + 1;
        write.dstBinding = binding;
        write.dstArrayElement = image_infos.at(i).second;
        writes_.push_back(write);
    }

    return *this;
}

DescriptorSetBuilder &DescriptorSetBuilder::bind_acceleration_structure(
    uint32_t binding,
    VkWriteDescriptorSetAccelerationStructureKHR acceleration_structure_info,
    VkShaderStageFlags stages) {
    VkDescriptorSetLayoutBinding layout_binding{};
    layout_binding.descriptorCount = 1;
    layout_binding.descriptorType =
        VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    layout_binding.pImmutableSamplers = nullptr;
    layout_binding.stageFlags = stages;
    layout_binding.binding = binding;
    layout_->add_binding(layout_binding);

    acceleration_structure_info.sType =
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    acceleration_structure_infos_.push_back(acceleration_structure_info);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = &acceleration_structure_infos_.back();
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    write.pImageInfo = NULL;
    write.pBufferInfo = NULL;
    write.pTexelBufferView = NULL;
    write.dstBinding = binding;
    writes_.push_back(write);

    return *this;
}

std::shared_ptr<DescriptorSet> DescriptorSetBuilder::build() {
    auto set = std::make_shared<DescriptorSet>(allocator_, layout_);
    update(set);
    return set;
}

void DescriptorSetBuilder::update(std::shared_ptr<DescriptorSet> set) {
    ASSERT(set->get_layout() == layout_->get_layout(),
           "Can't update descriptor set with different layout.");
    for (auto &write : writes_) {
        write.dstSet = set->get_set();
    }
    vkUpdateDescriptorSets(allocator_->get_device()->get_device(),
                           writes_.size(), writes_.data(), 0, nullptr);
}

std::shared_ptr<DescriptorSetLayout> DescriptorSetBuilder::get_layout() {
    return layout_;
}
