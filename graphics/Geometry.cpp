#include "utils/Assert.h"
#include "Geometry.h"

BLAS::BLAS(std::shared_ptr<GPUAllocator> allocator, std::shared_ptr<CommandPool> command_pool, std::shared_ptr<RingBuffer> ring_buffer, std::vector<VkAabbPositionsKHR> chunks) {
    geometry_buffer_ = std::make_shared<GPUBuffer>(allocator,
					       chunks.size() * sizeof(VkAabbPositionsKHR),
					       sizeof(VkAabbPositionsKHR),
					       VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					       0);
    timeline_ = std::make_shared<Semaphore>(allocator->get_device(), true);
    timeline_->set_signal_value(GEOMETRY_BUFFER_TIMELINE);
    ring_buffer->copy_to_device(geometry_buffer_,
				0,
				std::as_bytes(std::span<VkAabbPositionsKHR>(chunks.data(), chunks.size())),
				{},
				{timeline_});

    const VkDeviceSize alignment = allocator->get_device()->get_acceleration_structure_properties().minAccelerationStructureScratchOffsetAlignment;

    VkAccelerationStructureGeometryAabbsDataKHR geometry_aabbs_data {};
    geometry_aabbs_data.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
    geometry_aabbs_data.data.deviceAddress = geometry_buffer_->get_device_address();
    geometry_aabbs_data.stride = sizeof(VkAabbPositionsKHR);

    VkAccelerationStructureGeometryKHR blas_geometry {};
    blas_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    blas_geometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
    blas_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    blas_geometry.geometry.aabbs = geometry_aabbs_data;

    VkAccelerationStructureBuildRangeInfoKHR blas_build_range_info {};
    blas_build_range_info.firstVertex = 0;
    blas_build_range_info.primitiveCount = static_cast<uint32_t>(chunks.size());
    blas_build_range_info.primitiveOffset = 0;
    blas_build_range_info.transformOffset = 0;
    const VkAccelerationStructureBuildRangeInfoKHR blas_build_range_infos[1][1] = {{blas_build_range_info}};

    VkAccelerationStructureBuildGeometryInfoKHR blas_build_geometry_info {};
    blas_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    blas_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    blas_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    blas_build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    blas_build_geometry_info.geometryCount = 1;
    blas_build_geometry_info.pGeometries = &blas_geometry;

    const uint32_t max_primitive_counts[] = {static_cast<uint32_t>(chunks.size())};

    VkAccelerationStructureBuildSizesInfoKHR blas_build_sizes_info {};
    blas_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizes(allocator->get_device()->get_device(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &blas_build_geometry_info, max_primitive_counts, &blas_build_sizes_info);

    scratch_buffer_ = std::make_shared<GPUBuffer>(allocator,
						  blas_build_sizes_info.buildScratchSize,
						  alignment,
						  VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
						  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
						  0);
    
    storage_buffer_ = std::make_shared<GPUBuffer>(allocator,
						  blas_build_sizes_info.accelerationStructureSize,
						  alignment,
						  VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
						  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
						  0);
    
    VkAccelerationStructureCreateInfoKHR bottom_level_create_info {};
    bottom_level_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    bottom_level_create_info.buffer = storage_buffer_->get_buffer();
    bottom_level_create_info.offset = 0;
    bottom_level_create_info.size = blas_build_sizes_info.accelerationStructureSize;
    bottom_level_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

    VkAccelerationStructureKHR bottom_level_acceleration_structure;
    ASSERT(vkCreateAccelerationStructure(allocator->get_device()->get_device(), &bottom_level_create_info, NULL, &bottom_level_acceleration_structure), "Unable to create bottom level acceleration structure.");

    blas_build_geometry_info.dstAccelerationStructure = bottom_level_acceleration_structure;
    blas_build_geometry_info.scratchData.deviceAddress = scratch_buffer_->get_device_address();

    std::shared_ptr<Command> build_command = std::make_shared<Command>(command_pool);
    build_command->record([&](VkCommandBuffer command) {
	vkCmdBuildAccelerationStructures(command, 1, &blas_build_geometry_info, reinterpret_cast<const VkAccelerationStructureBuildRangeInfoKHR* const*>(blas_build_range_infos));
    });
    timeline_->set_wait_value(GEOMETRY_BUFFER_TIMELINE);
    timeline_->set_signal_value(BLAS_BUILD_TIMELINE);
    allocator->get_device()->submit_command(build_command, {timeline_}, {timeline_});
}

BLAS::~BLAS() {
    vkDestroyAccelerationStructure(geometry_buffer_->get_allocator()->get_device()->get_device(), blas_, nullptr);
}

VkAccelerationStructureKHR BLAS::get_blas() {
    return blas_;
}

std::shared_ptr<Semaphore> BLAS::get_timeline() {
    return timeline_;
}

TLAS::TLAS(std::shared_ptr<GPUAllocator> allocator, std::shared_ptr<CommandPool> command_pool, std::shared_ptr<RingBuffer> ring_buffer, std::vector<std::shared_ptr<BLAS>> bottom_structures, std::vector<VkAccelerationStructureInstanceKHR> instances) {
    instances_buffer_ = std::make_shared<GPUBuffer>(allocator,
					       instances.size() * sizeof(VkAccelerationStructureInstanceKHR),
					       sizeof(VkAccelerationStructureInstanceKHR),
					       VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					       0);
    timeline_ = std::make_shared<Semaphore>(allocator->get_device(), true);
    timeline_->set_signal_value(INSTANCES_BUFFER_TIMELINE);
    ring_buffer->copy_to_device(instances_buffer_,
				0,
				std::as_bytes(std::span<VkAccelerationStructureInstanceKHR>(instances.data(), instances.size())),
				{},
				{timeline_});

    const VkDeviceSize alignment = allocator->get_device()->get_acceleration_structure_properties().minAccelerationStructureScratchOffsetAlignment;
    contained_structures_ = bottom_structures;
}

TLAS::~TLAS() {
    vkDestroyAccelerationStructure(scratch_buffer_->get_allocator()->get_device()->get_device(), tlas_, nullptr);
}
