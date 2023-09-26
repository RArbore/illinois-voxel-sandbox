#include "Geometry.h"
#include "utils/Assert.h"

static inline uint64_t round_up_pow2(uint64_t x) {
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    x++;
    return x;
}

BLAS::BLAS(std::shared_ptr<GPUAllocator> allocator,
           std::shared_ptr<CommandPool> command_pool,
           std::shared_ptr<RingBuffer> ring_buffer,
           std::vector<VkAabbPositionsKHR> aabbs) {
    geometry_buffer_ = std::make_shared<GPUBuffer>(
        allocator, aabbs.size() * sizeof(VkAabbPositionsKHR),
        round_up_pow2(sizeof(VkAabbPositionsKHR)),
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
    timeline_ = std::make_shared<Semaphore>(allocator->get_device(), true);
    timeline_->set_signal_value(GEOMETRY_BUFFER_TIMELINE);
    ring_buffer->copy_to_device(geometry_buffer_, 0,
                                std::as_bytes(std::span<VkAabbPositionsKHR>(
                                    aabbs.data(), aabbs.size())),
                                {}, {timeline_});

    const VkDeviceSize alignment =
        allocator->get_device()
            ->get_acceleration_structure_properties()
            .minAccelerationStructureScratchOffsetAlignment;

    VkAccelerationStructureGeometryAabbsDataKHR geometry_aabbs_data{};
    geometry_aabbs_data.sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
    geometry_aabbs_data.data.deviceAddress =
        geometry_buffer_->get_device_address();
    geometry_aabbs_data.stride = sizeof(VkAabbPositionsKHR);

    VkAccelerationStructureGeometryKHR blas_geometry{};
    blas_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    blas_geometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
    blas_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    blas_geometry.geometry.aabbs = geometry_aabbs_data;

    VkAccelerationStructureBuildRangeInfoKHR blas_build_range_info{};
    blas_build_range_info.firstVertex = 0;
    blas_build_range_info.primitiveCount = static_cast<uint32_t>(aabbs.size());
    blas_build_range_info.primitiveOffset = 0;
    blas_build_range_info.transformOffset = 0;
    const VkAccelerationStructureBuildRangeInfoKHR
        *const blas_build_range_infos[] = {&blas_build_range_info};

    VkAccelerationStructureBuildGeometryInfoKHR blas_build_geometry_info{};
    blas_build_geometry_info.sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    blas_build_geometry_info.type =
        VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    blas_build_geometry_info.flags =
        VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
        VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    blas_build_geometry_info.mode =
        VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    blas_build_geometry_info.geometryCount = 1;
    blas_build_geometry_info.pGeometries = &blas_geometry;

    const uint32_t max_primitive_counts[] = {
        static_cast<uint32_t>(aabbs.size())};

    VkAccelerationStructureBuildSizesInfoKHR blas_build_sizes_info{};
    blas_build_sizes_info.sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizes(
        allocator->get_device()->get_device(),
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &blas_build_geometry_info, max_primitive_counts,
        &blas_build_sizes_info);

    scratch_buffer_ = std::make_shared<GPUBuffer>(
        allocator, blas_build_sizes_info.buildScratchSize, alignment,
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);

    storage_buffer_ = std::make_shared<GPUBuffer>(
        allocator, blas_build_sizes_info.accelerationStructureSize, alignment,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);

    VkAccelerationStructureCreateInfoKHR bottom_level_create_info{};
    bottom_level_create_info.sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    bottom_level_create_info.buffer = storage_buffer_->get_buffer();
    bottom_level_create_info.offset = 0;
    bottom_level_create_info.size =
        blas_build_sizes_info.accelerationStructureSize;
    bottom_level_create_info.type =
        VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

    ASSERT(vkCreateAccelerationStructure(allocator->get_device()->get_device(),
                                         &bottom_level_create_info, NULL,
                                         &blas_),
           "Unable to create bottom level acceleration structure.");

    blas_build_geometry_info.dstAccelerationStructure = blas_;
    blas_build_geometry_info.scratchData.deviceAddress =
        scratch_buffer_->get_device_address();

    std::shared_ptr<Command> build_command =
        std::make_shared<Command>(command_pool);
    build_command->record([&](VkCommandBuffer command) {
        vkCmdBuildAccelerationStructures(
            command, 1, &blas_build_geometry_info,
            reinterpret_cast<const VkAccelerationStructureBuildRangeInfoKHR
                                 *const *>(blas_build_range_infos));
    });
    timeline_->set_wait_value(GEOMETRY_BUFFER_TIMELINE);
    timeline_->set_signal_value(BLAS_BUILD_TIMELINE);
    allocator->get_device()->submit_command(build_command, {timeline_},
                                            {timeline_});
}

BLAS::~BLAS() {
    vkDestroyAccelerationStructure(
        geometry_buffer_->get_allocator()->get_device()->get_device(), blas_,
        nullptr);
}

VkAccelerationStructureKHR BLAS::get_blas() { return blas_; }

std::shared_ptr<Semaphore> BLAS::get_timeline() { return timeline_; }

VkDeviceAddress BLAS::get_device_address() {
    VkAccelerationStructureDeviceAddressInfoKHR
        acceleration_structure_device_address_info{};
    acceleration_structure_device_address_info.sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    acceleration_structure_device_address_info.accelerationStructure = blas_;
    return vkGetAccelerationStructureDeviceAddress(
        scratch_buffer_->get_allocator()->get_device()->get_device(),
        &acceleration_structure_device_address_info);
}

TLAS::TLAS(std::shared_ptr<GPUAllocator> allocator,
           std::shared_ptr<CommandPool> command_pool,
           std::shared_ptr<RingBuffer> ring_buffer,
           std::vector<std::shared_ptr<BLAS>> bottom_structures,
           std::vector<VkAccelerationStructureInstanceKHR> instances) {
    instances_buffer_ = std::make_shared<GPUBuffer>(
        allocator,
        instances.size() * sizeof(VkAccelerationStructureInstanceKHR),
        round_up_pow2(sizeof(VkAccelerationStructureInstanceKHR)),
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
    timeline_ = std::make_shared<Semaphore>(allocator->get_device(), true);
    timeline_->set_signal_value(INSTANCES_BUFFER_TIMELINE);
    ring_buffer->copy_to_device(
        instances_buffer_, 0,
        std::as_bytes(std::span<VkAccelerationStructureInstanceKHR>(
            instances.data(), instances.size())),
        {}, {timeline_});

    const VkDeviceSize alignment =
        allocator->get_device()
            ->get_acceleration_structure_properties()
            .minAccelerationStructureScratchOffsetAlignment;

    VkAccelerationStructureGeometryInstancesDataKHR geometry_instances_data{};
    geometry_instances_data.sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry_instances_data.data.deviceAddress =
        instances_buffer_->get_device_address();

    VkAccelerationStructureGeometryKHR tlas_geometry{};
    tlas_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    tlas_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    tlas_geometry.geometry.instances = geometry_instances_data;

    VkAccelerationStructureBuildRangeInfoKHR tlas_build_range_info{};
    tlas_build_range_info.firstVertex = 0;
    tlas_build_range_info.primitiveCount =
        static_cast<uint32_t>(instances.size());
    tlas_build_range_info.primitiveOffset = 0;
    tlas_build_range_info.transformOffset = 0;
    const VkAccelerationStructureBuildRangeInfoKHR
        *const tlas_build_range_infos[] = {&tlas_build_range_info};

    VkAccelerationStructureBuildGeometryInfoKHR tlas_build_geometry_info{};
    tlas_build_geometry_info.sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    tlas_build_geometry_info.type =
        VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    tlas_build_geometry_info.flags =
        VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
        VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    tlas_build_geometry_info.mode =
        VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    tlas_build_geometry_info.geometryCount = 1;
    tlas_build_geometry_info.pGeometries = &tlas_geometry;

    const uint32_t max_instances_counts[] = {
        static_cast<uint32_t>(instances.size())};

    VkAccelerationStructureBuildSizesInfoKHR tlas_build_sizes_info{};
    tlas_build_sizes_info.sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizes(
        allocator->get_device()->get_device(),
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &tlas_build_geometry_info, max_instances_counts,
        &tlas_build_sizes_info);

    scratch_buffer_ = std::make_shared<GPUBuffer>(
        allocator, tlas_build_sizes_info.buildScratchSize, alignment,
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);

    storage_buffer_ = std::make_shared<GPUBuffer>(
        allocator, tlas_build_sizes_info.accelerationStructureSize, alignment,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);

    VkAccelerationStructureCreateInfoKHR top_level_create_info{};
    top_level_create_info.sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    top_level_create_info.buffer = storage_buffer_->get_buffer();
    top_level_create_info.offset = 0;
    top_level_create_info.size =
        tlas_build_sizes_info.accelerationStructureSize;
    top_level_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

    ASSERT(vkCreateAccelerationStructure(allocator->get_device()->get_device(),
                                         &top_level_create_info, NULL, &tlas_),
           "Unable to create top level acceleration structure.");

    tlas_build_geometry_info.dstAccelerationStructure = tlas_;
    tlas_build_geometry_info.scratchData.deviceAddress =
        scratch_buffer_->get_device_address();

    std::shared_ptr<Command> build_command =
        std::make_shared<Command>(command_pool);
    build_command->record([&](VkCommandBuffer command) {
        vkCmdBuildAccelerationStructures(
            command, 1, &tlas_build_geometry_info,
            reinterpret_cast<const VkAccelerationStructureBuildRangeInfoKHR
                                 *const *>(tlas_build_range_infos));
    });
    std::vector<std::shared_ptr<Semaphore>> wait_semaphores;
    timeline_->set_wait_value(INSTANCES_BUFFER_TIMELINE);
    timeline_->set_signal_value(TLAS_BUILD_TIMELINE);
    wait_semaphores.push_back(timeline_);
    for (auto blas : bottom_structures) {
        std::shared_ptr<Semaphore> timeline = blas->get_timeline();
        timeline->set_wait_value(BLAS::BLAS_BUILD_TIMELINE);
        wait_semaphores.push_back(timeline);
    }
    allocator->get_device()->submit_command(
        build_command, std::move(wait_semaphores), {timeline_});

    contained_structures_ = bottom_structures;
    instances_ = instances;
    command_pool_ = command_pool;
    ring_buffer_ = ring_buffer;
}

void TLAS::update_model_sbt_offsets(
    std::unordered_map<uint64_t, uint32_t> models) {
    for (auto &instance : instances_) {
        if (instance.instanceShaderBindingTableRecordOffset ==
            UNLOADED_SBT_OFFSET) {
            auto it = models.find(instance.instanceCustomIndex);
            if (it != models.end()) {
                instance.instanceShaderBindingTableRecordOffset = it->second;
            }
        }
    }
    timeline_ = std::make_shared<Semaphore>(command_pool_->get_device(), true);
    timeline_->set_signal_value(INSTANCES_BUFFER_TIMELINE);
    ring_buffer_->copy_to_device(
        instances_buffer_, 0,
        std::as_bytes(std::span<VkAccelerationStructureInstanceKHR>(
            instances_.data(), instances_.size())),
        {}, {timeline_});

    VkAccelerationStructureGeometryInstancesDataKHR geometry_instances_data{};
    geometry_instances_data.sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry_instances_data.data.deviceAddress =
        instances_buffer_->get_device_address();

    VkAccelerationStructureGeometryKHR tlas_geometry{};
    tlas_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    tlas_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    tlas_geometry.geometry.instances = geometry_instances_data;

    VkAccelerationStructureBuildRangeInfoKHR tlas_build_range_info{};
    tlas_build_range_info.firstVertex = 0;
    tlas_build_range_info.primitiveCount =
        static_cast<uint32_t>(instances_.size());
    tlas_build_range_info.primitiveOffset = 0;
    tlas_build_range_info.transformOffset = 0;
    const VkAccelerationStructureBuildRangeInfoKHR
        *const tlas_build_range_infos[] = {&tlas_build_range_info};

    VkAccelerationStructureBuildGeometryInfoKHR tlas_build_geometry_info{};
    tlas_build_geometry_info.sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    tlas_build_geometry_info.type =
        VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    tlas_build_geometry_info.flags =
        VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
        VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    tlas_build_geometry_info.mode =
        VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
    tlas_build_geometry_info.srcAccelerationStructure = tlas_;
    tlas_build_geometry_info.dstAccelerationStructure = tlas_;
    tlas_build_geometry_info.scratchData.deviceAddress =
        scratch_buffer_->get_device_address();
    tlas_build_geometry_info.geometryCount = 1;
    tlas_build_geometry_info.pGeometries = &tlas_geometry;

    std::shared_ptr<Command> update_command =
        std::make_shared<Command>(command_pool_);
    update_command->record([&](VkCommandBuffer command) {
        vkCmdBuildAccelerationStructures(
            command, 1, &tlas_build_geometry_info,
            reinterpret_cast<const VkAccelerationStructureBuildRangeInfoKHR
                                 *const *>(tlas_build_range_infos));
    });
    timeline_->set_wait_value(INSTANCES_BUFFER_TIMELINE);
    timeline_->set_signal_value(TLAS_BUILD_TIMELINE);
    command_pool_->get_device()->submit_command(update_command, {timeline_},
                                                {timeline_});
}

TLAS::~TLAS() {
    vkDestroyAccelerationStructure(
        scratch_buffer_->get_allocator()->get_device()->get_device(), tlas_,
        nullptr);
}

VkAccelerationStructureKHR TLAS::get_tlas() { return tlas_; }

std::shared_ptr<Semaphore> TLAS::get_timeline() { return timeline_; }

VkDeviceAddress TLAS::get_device_address() {
    VkAccelerationStructureDeviceAddressInfoKHR
        acceleration_structure_device_address_info{};
    acceleration_structure_device_address_info.sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    acceleration_structure_device_address_info.accelerationStructure = tlas_;
    return vkGetAccelerationStructureDeviceAddress(
        scratch_buffer_->get_allocator()->get_device()->get_device(),
        &acceleration_structure_device_address_info);
}
