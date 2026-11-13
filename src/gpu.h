#pragma once

void createDataTransferBuffers(SDL_GPUDevice* device, SDL_GPUBufferUsageFlags usage, void* data, size_t dataSize, SDL_GPUBuffer** destDataBuffer, SDL_GPUTransferBuffer** destTransferBuffer)
{
    SDL_GPUBufferCreateInfo bufferInfo{};
    bufferInfo.size = (Uint32)dataSize;
    bufferInfo.usage = usage;
    SDL_GPUBuffer* dataBuffer = SDL_CreateGPUBuffer(device, &bufferInfo);

    SDL_GPUTransferBufferCreateInfo transferInfo{};
    transferInfo.size = (Uint32)dataSize;
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);

    void* transferData = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
    SDL_memcpy(transferData, data, dataSize);
    SDL_UnmapGPUTransferBuffer(device, transferBuffer);

    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);

    SDL_GPUTransferBufferLocation location{};
    location.transfer_buffer = transferBuffer;
    location.offset = 0;

    SDL_GPUBufferRegion region{};
    region.buffer = dataBuffer;
    region.size = (Uint32)dataSize;
    region.offset = 0;

    SDL_UploadToGPUBuffer(copyPass, &location, &region, true);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);

    *destDataBuffer = dataBuffer;
    *destTransferBuffer = transferBuffer;
}

SDL_GPUTexture* createTexture(SDL_GPUDevice* device, int w, int h, SDL_GPUTextureFormat format, SDL_GPUTextureUsageFlags usage)
{
    SDL_GPUTextureCreateInfo info = {};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = format;
    info.usage = usage;
    info.width = w;
    info.height = h;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    info.sample_count = SDL_GPU_SAMPLECOUNT_1;

    return SDL_CreateGPUTexture(device, &info);
};

SDL_GPUComputePipeline* createComputePipeline(SDL_GPUDevice* device, const char* file, int uBuffers, int sBuffers, int rwtBuffers, int threadsX, int threadsY, int threadsZ)
{
    size_t codeSize;
    void* code = SDL_LoadFile(file, &codeSize);

    SDL_GPUComputePipelineCreateInfo  info{};
    info.code_size = codeSize;
    info.code = (Uint8*)code;
    info.entrypoint = "main";
    info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    info.num_samplers = 0;
    info.num_readonly_storage_textures = 0;
    info.num_readonly_storage_buffers = sBuffers;
    info.num_readwrite_storage_textures = rwtBuffers;
    info.num_readwrite_storage_buffers = 0;
    info.num_uniform_buffers = uBuffers;

    info.threadcount_x = (Uint32)threadsX;
    info.threadcount_y = (Uint32)threadsY;
    info.threadcount_z = (Uint32)threadsZ;

    SDL_GPUComputePipeline* pipeline = SDL_CreateGPUComputePipeline(device, &info);

    SDL_free(code);

    return pipeline;
}

void submit(SDL_GPUDevice* device, SDL_GPUCommandBuffer* commandBuffer)
{
    SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commandBuffer);

    if (fence)
    {
        SDL_WaitForGPUFences(device, true, &fence, 1);
        SDL_ReleaseGPUFence(device, fence);
    }
    else
    {
        printf(SDL_GetError());
    }
}