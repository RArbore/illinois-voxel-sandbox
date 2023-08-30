#include "utils/Assert.h"
#include "Geometry.h"

AccelerationStructureBuilder::AccelerationStructureBuilder(std::shared_ptr<GPUAllocator> allocator, std::shared_ptr<RingBuffer> ring_buffer) {
    cube_buffer_timeline_ = std::make_shared<Semaphore>(allocator->get_device(), true);
    cube_buffer_timeline_->set_wait_value(1);
    cube_buffer_timeline_->set_signal_value(1);
    
    VkAabbPositionsKHR cube {};
    cube.minX = 0.0f;
    cube.minY = 0.0f;
    cube.minZ = 0.0f;
    cube.maxX = 1.0f;
    cube.maxY = 1.0f;
    cube.maxZ = 1.0f;
    
    cube_buffer_ = std::make_shared<GPUBuffer>(allocator,
					       sizeof(VkAabbPositionsKHR),
					       1,
					       VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					       0);

    ring_buffer->copy_to_device(cube_buffer_,
				0,
				std::as_bytes(std::span<VkAabbPositionsKHR>(&cube, 1)),
				{},
				{cube_buffer_timeline_});

    device_ = allocator->get_device();
}
