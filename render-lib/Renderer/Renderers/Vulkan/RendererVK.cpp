#include "RendererVK.h"
#include "../../../Window/Window.h"
#include <Utils/StringUtils.h>
#include <Utils/DebugHandler.h>
#include "../../../Window/Window.h"

#include "Backend/RenderDeviceVK.h"
#include "Backend/ImageHandlerVK.h"
#include "Backend/TextureHandlerVK.h"
#include "Backend/ModelHandlerVK.h"
#include "Backend/ShaderHandlerVK.h"
#include "Backend/PipelineHandlerVK.h"
#include "Backend/CommandListHandlerVK.h"
#include "Backend/SamplerHandlerVK.h"
#include "Backend/SwapChainVK.h"
#include "Backend/DebugMarkerUtilVK.h"
#include "Backend/BufferBackendVK.h"
#include "Backend/DescriptorSetBackendVK.h"
#include "Backend/DescriptorSetBuilderVK.h"

namespace Renderer
{
    RendererVK::RendererVK(TextureDesc& debugTexture)
        : _device(new Backend::RenderDeviceVK())
    {
        _device->Init();
        _imageHandler = new Backend::ImageHandlerVK();
        _textureHandler = new Backend::TextureHandlerVK();
        _modelHandler = new Backend::ModelHandlerVK();
        _shaderHandler = new Backend::ShaderHandlerVK();
        _pipelineHandler = new Backend::PipelineHandlerVK();
        _commandListHandler = new Backend::CommandListHandlerVK();
        _samplerHandler = new Backend::SamplerHandlerVK();

        _textureHandler->LoadDebugTexture(_device, debugTexture);
    }

    void RendererVK::InitWindow(Window* window)
    {
        _device->InitWindow(_shaderHandler, window);
    }

    void RendererVK::Deinit()
    {
        _device->FlushGPU(); // Make sure it has finished rendering

        delete(_device);
        delete(_imageHandler);
        delete(_textureHandler);
        delete(_modelHandler);
        delete(_shaderHandler);
        delete(_pipelineHandler);
        delete(_commandListHandler);
        delete(_samplerHandler);
    }

    ImageID RendererVK::CreateImage(ImageDesc& desc)
    {
        return _imageHandler->CreateImage(_device, desc);
    }

    DepthImageID RendererVK::CreateDepthImage(DepthImageDesc& desc)
    {
        return _imageHandler->CreateDepthImage(_device, desc);
    }

    SamplerID RendererVK::CreateSampler(SamplerDesc& desc)
    {
        return _samplerHandler->CreateSampler(_device, desc);
    }

    GraphicsPipelineID RendererVK::CreatePipeline(GraphicsPipelineDesc& desc)
    {
        return _pipelineHandler->CreatePipeline(_device, _shaderHandler, _imageHandler, desc);
    }

    ComputePipelineID RendererVK::CreatePipeline(ComputePipelineDesc& /*desc*/)
    {
        NC_LOG_FATAL("Not supported yet");
        return ComputePipelineID::Invalid();
    }

    ModelID RendererVK::CreatePrimitiveModel(PrimitiveModelDesc& desc)
    {
        return _modelHandler->CreatePrimitiveModel(_device, desc);
    }

    void RendererVK::UpdatePrimitiveModel(ModelID model, PrimitiveModelDesc& desc)
    {
        _modelHandler->UpdatePrimitiveModel(_device, model, desc);
    }

    TextureArrayID RendererVK::CreateTextureArray(TextureArrayDesc& desc)
    {
        return _textureHandler->CreateTextureArray(_device, desc);
    }

    TextureID RendererVK::CreateDataTexture(DataTextureDesc& desc)
    {
        return _textureHandler->CreateDataTexture(_device, desc);
    }

    TextureID RendererVK::CreateDataTextureIntoArray(DataTextureDesc& desc, TextureArrayID textureArray, u32& arrayIndex)
    {
        return _textureHandler->CreateDataTextureIntoArray(_device, desc, textureArray, arrayIndex);
    }

    DescriptorSetBackend* RendererVK::CreateDescriptorSetBackend()
    {
        return new Backend::DescriptorSetBackendVK();
    }

    ModelID RendererVK::LoadModel(ModelDesc& desc)
    {
        return _modelHandler->LoadModel(_device, desc);
    }

    TextureID RendererVK::LoadTexture(TextureDesc& desc)
    {
        return _textureHandler->LoadTexture(_device, desc);
    }

    TextureID RendererVK::LoadTextureIntoArray(TextureDesc& desc, TextureArrayID textureArray, u32& arrayIndex)
    {
        return _textureHandler->LoadTextureIntoArray(_device, desc, textureArray, arrayIndex);
    }

    VertexShaderID RendererVK::LoadShader(VertexShaderDesc& desc)
    {
        return _shaderHandler->LoadShader(_device, desc);
    }

    PixelShaderID RendererVK::LoadShader(PixelShaderDesc& desc)
    {
        return _shaderHandler->LoadShader(_device, desc);
    }

    ComputeShaderID RendererVK::LoadShader(ComputeShaderDesc& desc)
    {
        return _shaderHandler->LoadShader(_device, desc);
    }

    CommandListID RendererVK::BeginCommandList()
    {
        return _commandListHandler->BeginCommandList(_device);
    }

    void RendererVK::EndCommandList(CommandListID commandListID)
    {
        if (_renderPassOpenCount != 0)
        {
            NC_LOG_FATAL("We found unmatched calls to BeginPipeline in your commandlist, for every BeginPipeline you need to also EndPipeline!");
        }

        _commandListHandler->EndCommandList(_device, commandListID);
    }

    void RendererVK::Clear(CommandListID commandListID, ImageID imageID, Color color)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        VkImage image = _imageHandler->GetImage(imageID);

        ImageDesc desc = _imageHandler->GetImageDesc(imageID);
        
        VkClearColorValue clearColorValue = {};
        clearColorValue.float32[0] = color.r;
        clearColorValue.float32[1] = color.g;
        clearColorValue.float32[2] = color.b;
        clearColorValue.float32[3] = color.a;

        VkImageSubresourceRange imageSubresourceRange;
        imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageSubresourceRange.baseMipLevel = 0;
        imageSubresourceRange.levelCount = 1;
        imageSubresourceRange.baseArrayLayer = 0;
        imageSubresourceRange.layerCount = desc.depth;

        // Transition image to TRANSFER_DST_OPTIMAL
        _device->TransitionImageLayout(commandBuffer, image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, desc.depth);

        vkCmdClearColorImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColorValue, 1, &imageSubresourceRange);

        // Transition image back to GENERAL
        _device->TransitionImageLayout(commandBuffer, image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, desc.depth);
    }

    void RendererVK::Clear(CommandListID commandListID, DepthImageID imageID, DepthClearFlags clearFlags, f32 depth, u8 stencil)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        VkImage image = _imageHandler->GetImage(imageID);

        VkClearDepthStencilValue clearDepthValue = {};
        VkImageSubresourceRange range = {};

        if (clearFlags == DepthClearFlags::DEPTH_CLEAR_DEPTH || clearFlags == DepthClearFlags::DEPTH_CLEAR_BOTH)
        {
            clearDepthValue.depth = depth;
            range.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
        }

        if (clearFlags == DepthClearFlags::DEPTH_CLEAR_STENCIL || clearFlags == DepthClearFlags::DEPTH_CLEAR_BOTH)
        {
            clearDepthValue.stencil = stencil;
            range.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        range.layerCount = 1;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.baseMipLevel = 0;

        // Transition image to TRANSFER_DST_OPTIMAL
        _device->TransitionImageLayout(commandBuffer, image, range.aspectMask, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);

        vkCmdClearDepthStencilImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearDepthValue, 1, &range);

        // Transition image back to DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        _device->TransitionImageLayout(commandBuffer, image, range.aspectMask, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
    }

    void RendererVK::Draw(CommandListID commandListID, ModelID modelID)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        // Bind vertex buffer
        VkBuffer vertexBuffer = _modelHandler->GetVertexBuffer(modelID);
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);

        if (_boundModelIndexBuffer != modelID)
        {
            // Bind index buffer
            VkBuffer indexBuffer = _modelHandler->GetIndexBuffer(modelID);
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            _boundModelIndexBuffer = modelID;
        }

        // Draw
        u32 numIndices = _modelHandler->GetNumIndices(modelID);
        vkCmdDrawIndexed(commandBuffer, numIndices, 1, 0, 0, 0);
    }

    void RendererVK::DrawBindless(CommandListID commandListID, u32 numVertices, u32 numInstances)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        // Draw
        vkCmdDraw(commandBuffer, numVertices, numInstances, 0, 0);
    }

    void RendererVK::DrawIndexedBindless(CommandListID commandListID, ModelID modelID, u32 numVertices, u32 numInstances)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        if (_boundModelIndexBuffer != modelID)
        {
            // Bind index buffer
            VkBuffer indexBuffer = _modelHandler->GetIndexBuffer(modelID);
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            _boundModelIndexBuffer = modelID;
        }
        
        // Draw
        vkCmdDrawIndexed(commandBuffer, numVertices, numInstances, 0, 0, 0);
    }

    void RendererVK::PopMarker(CommandListID commandListID)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        Backend::DebugMarkerUtilVK::PopMarker(commandBuffer);
    }

    void RendererVK::PushMarker(CommandListID commandListID, Color color, std::string name)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        Backend::DebugMarkerUtilVK::PushMarker(commandBuffer, color, name);
    }

    void RendererVK::SetConstantBuffer(CommandListID commandListID, u32 slot, void* descriptor, size_t frameIndex)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        GraphicsPipelineID graphicsPipelineID = _commandListHandler->GetBoundGraphicsPipeline(commandListID);

        Backend::DescriptorSetLayoutData& descriptorSetLayoutData = _pipelineHandler->GetDescriptorSetLayoutData(graphicsPipelineID, slot);
        VkDescriptorSetLayout& descriptorSetLayout = _pipelineHandler->GetDescriptorSetLayout(graphicsPipelineID, slot);
        VkPipelineLayout pipelineLayout = _pipelineHandler->GetPipelineLayout(graphicsPipelineID);

        // TODO: This is ugly, we really don't want to do this here, but without reflecting the descriptorSetLayout we need the user to provide it, how can we fix this?
        Backend::BufferBackendVK* buffer = static_cast<Backend::BufferBackendVK*>(descriptor);
        if (buffer->descriptorPool == NULL)
        {
            VkDescriptorPoolSize poolSize = {};
            poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            poolSize.descriptorCount = buffer->descriptorSet.Num+1;

            VkDescriptorPoolCreateInfo poolInfo = {};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.poolSizeCount = 1;
            poolInfo.pPoolSizes = &poolSize;
            poolInfo.maxSets = buffer->descriptorSet.Num;

            if (vkCreateDescriptorPool(_device->_device, &poolInfo, nullptr, &buffer->descriptorPool) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create descriptor pool!");
            }

            VkDescriptorSetAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = buffer->descriptorPool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &descriptorSetLayout;

            for (int i = 0; i < buffer->descriptorSet.Num; i++)
            {
                if (vkAllocateDescriptorSets(_device->_device, &allocInfo, &buffer->descriptorSet.Get(i)) != VK_SUCCESS)
                {
                    NC_LOG_FATAL("Failed to allocate descriptor sets!");
                }

                VkDescriptorBufferInfo descriptorBufferInfo = {};
                descriptorBufferInfo.buffer = buffer->buffers.Get(i);
                descriptorBufferInfo.offset = 0;
                descriptorBufferInfo.range = buffer->bufferSize;

                VkWriteDescriptorSet descriptorWrite = {};
                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = buffer->descriptorSet.Get(i);
                descriptorWrite.dstBinding = 0;
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorWrite.descriptorCount = 1;
                descriptorWrite.pBufferInfo = &descriptorBufferInfo;
                descriptorWrite.pImageInfo = nullptr;
                descriptorWrite.pTexelBufferView = nullptr;

                vkUpdateDescriptorSets(_device->_device, 1, &descriptorWrite, 0, nullptr);
            }
        }

        // Bind descriptor set
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, slot, 1, &buffer->descriptorSet.Get(frameIndex), 0, nullptr);
    }

    void RendererVK::SetStorageBuffer(CommandListID commandListID, u32 slot, void* descriptor, size_t frameIndex)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        GraphicsPipelineID graphicsPipelineID = _commandListHandler->GetBoundGraphicsPipeline(commandListID);

        Backend::DescriptorSetLayoutData& descriptorSetLayoutData = _pipelineHandler->GetDescriptorSetLayoutData(graphicsPipelineID, slot);
        VkDescriptorSetLayout& descriptorSetLayout = _pipelineHandler->GetDescriptorSetLayout(graphicsPipelineID, slot);
        VkPipelineLayout pipelineLayout = _pipelineHandler->GetPipelineLayout(graphicsPipelineID);

        // TODO: This is ugly, we really don't want to do this here, but without reflecting the descriptorSetLayout we need the user to provide it, how can we fix this?
        Backend::BufferBackendVK* buffer = static_cast<Backend::BufferBackendVK*>(descriptor);
        if (buffer->descriptorPool == NULL)
        {
            VkDescriptorPoolSize poolSize = {};
            poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            poolSize.descriptorCount = buffer->descriptorSet.Num+1;

            VkDescriptorPoolCreateInfo poolInfo = {};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.poolSizeCount = 1;
            poolInfo.pPoolSizes = &poolSize;
            poolInfo.maxSets = buffer->descriptorSet.Num;

            if (vkCreateDescriptorPool(_device->_device, &poolInfo, nullptr, &buffer->descriptorPool) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create descriptor pool!");
            }

            VkDescriptorSetAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = buffer->descriptorPool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &descriptorSetLayout;

            for (int i = 0; i < buffer->descriptorSet.Num; i++)
            {
                if (vkAllocateDescriptorSets(_device->_device, &allocInfo, &buffer->descriptorSet.Get(i)) != VK_SUCCESS)
                {
                    NC_LOG_FATAL("Failed to allocate descriptor sets!");
                }

                VkDescriptorBufferInfo descriptorBufferInfo = {};
                descriptorBufferInfo.buffer = buffer->buffers.Get(i);
                descriptorBufferInfo.offset = 0;
                descriptorBufferInfo.range = buffer->bufferSize;

                VkWriteDescriptorSet descriptorWrite = {};
                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = buffer->descriptorSet.Get(i);
                descriptorWrite.dstBinding = 0;
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                descriptorWrite.descriptorCount = 1;
                descriptorWrite.pBufferInfo = &descriptorBufferInfo;
                descriptorWrite.pImageInfo = nullptr;
                descriptorWrite.pTexelBufferView = nullptr;

                vkUpdateDescriptorSets(_device->_device, 1, &descriptorWrite, 0, nullptr);
            }
        }

        // Bind descriptor set
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, slot, 1, &buffer->descriptorSet.Get(frameIndex), 0, nullptr);
    }

    void RendererVK::BeginPipeline(CommandListID commandListID, GraphicsPipelineID pipelineID)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        GraphicsPipelineDesc pipelineDesc = _pipelineHandler->GetDescriptor(pipelineID);
        VkPipeline pipeline = _pipelineHandler->GetPipeline(pipelineID);
        VkRenderPass renderPass = _pipelineHandler->GetRenderPass(pipelineID);
        VkFramebuffer frameBuffer = _pipelineHandler->GetFramebuffer(pipelineID);

        if (_renderPassOpenCount != 0)
        {
            NC_LOG_FATAL("You need to match your BeginPipeline calls with a EndPipeline call before beginning another pipeline!");
        }
        _renderPassOpenCount++;

        // Set up renderpass
        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = frameBuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = { static_cast<u32>(pipelineDesc.states.viewport.width), static_cast<u32>(pipelineDesc.states.viewport.height) };

        // Start renderpass
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Bind pipeline
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        _commandListHandler->SetBoundGraphicsPipeline(commandListID, pipelineID);

        _boundModelIndexBuffer = ModelID::Invalid();
    }

    void RendererVK::EndPipeline(CommandListID commandListID, GraphicsPipelineID /*pipelineID*/)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        if (_renderPassOpenCount <= 0)
        {
            NC_LOG_FATAL("You tried to call EndPipeline without first calling BeginPipeline!");
        }
        _renderPassOpenCount--;

        vkCmdEndRenderPass(commandBuffer);
    }

    void RendererVK::SetPipeline(CommandListID /*commandListID*/, ComputePipelineID /*pipelineID*/)
    {
        
    }

    void RendererVK::SetScissorRect(CommandListID /*commandListID*/, ScissorRect /*scissorRect*/)
    {
        
    }

    void RendererVK::SetViewport(CommandListID /*commandListID*/, Viewport /*viewport*/)
    {
        
    }

    void RendererVK::SetSampler(CommandListID commandListID, u32 slot, SamplerID samplerID)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        GraphicsPipelineID graphicsPipelineID = _commandListHandler->GetBoundGraphicsPipeline(commandListID);
        VkPipelineLayout pipelineLayout = _pipelineHandler->GetPipelineLayout(graphicsPipelineID);

        VkDescriptorSet samplerDescriptor = _samplerHandler->GetDescriptorSet(samplerID);

        // Bind descriptor set
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, slot, 1, &samplerDescriptor, 0, nullptr);
    }

    void RendererVK::SetTexture(CommandListID commandListID, u32 slot, TextureID textureID)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        GraphicsPipelineID graphicsPipelineID = _commandListHandler->GetBoundGraphicsPipeline(commandListID);
        VkPipelineLayout pipelineLayout = _pipelineHandler->GetPipelineLayout(graphicsPipelineID);

        VkDescriptorSet textureDescriptor = _textureHandler->GetDescriptorSet(textureID);

        // Bind descriptor set
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, slot, 1, &textureDescriptor, 0, nullptr);
    }

    void RendererVK::SetTextureArray(CommandListID commandListID, u32 slot, TextureArrayID textureArrayID)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        GraphicsPipelineID graphicsPipelineID = _commandListHandler->GetBoundGraphicsPipeline(commandListID);
        VkPipelineLayout pipelineLayout = _pipelineHandler->GetPipelineLayout(graphicsPipelineID);

        VkDescriptorSet textureArrayDescriptor = _textureHandler->GetDescriptorSet(textureArrayID);

        // Bind descriptor set
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, slot, 1, &textureArrayDescriptor, 0, nullptr);
    }

    void RendererVK::SetVertexBuffer(CommandListID commandListID, u32 slot, ModelID modelID)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        // Bind vertex buffer
        VkBuffer vertexBuffer = _modelHandler->GetVertexBuffer(modelID);
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, slot, 1, &vertexBuffer, offsets);
    }

    void RendererVK::SetIndexBuffer(CommandListID commandListID, ModelID modelID)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        // Bind index buffer
        VkBuffer indexBuffer = _modelHandler->GetIndexBuffer(modelID);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    }

    void RendererVK::SetBuffer(CommandListID commandListID, u32 slot, void* buffer)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        // Bind buffer
        VkBuffer vkBuffer = *static_cast<VkBuffer*>(buffer);
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, slot, 1, &vkBuffer, offsets);
    }

    bool RendererVK::ReflectDescriptorSet(const std::string& name, u32 nameHash, u32 type, i32& set, const std::vector<Backend::BindInfo>& bindInfos, u32& outBindInfoIndex, VkDescriptorSetLayoutBinding* outDescriptorLayoutBinding)
    {
        // Try to find a BindInfo with a matching name
        for(u32 i = 0; i < bindInfos.size(); i++)
        {
            auto& bindInfo = bindInfos[i];

            // If the name and type matches
            if (nameHash == bindInfo.nameHash && type == bindInfo.descriptorType)
            {
                // If we have a set, make sure it's the correct one
                if (set != -1)
                {
                    if (set != bindInfo.set)
                    {
                        NC_LOG_ERROR("While creating DescriptorSet, we found BindInfo with matching name (%s) and type (%u), but it didn't match the location (%i != %i)", bindInfo.name, bindInfo.descriptorType, bindInfo.set, set);
                    }
                }
                else
                {
                    set = bindInfo.set;
                }

                // Fill out descriptor set layout
                outDescriptorLayoutBinding->binding = bindInfo.binding;
                outDescriptorLayoutBinding->descriptorType = bindInfo.descriptorType;
                outDescriptorLayoutBinding->descriptorCount = bindInfo.count;
                outDescriptorLayoutBinding->stageFlags = bindInfo.stageFlags;
                outDescriptorLayoutBinding->pImmutableSamplers = NULL;

                outBindInfoIndex = i;

                return true;
            }
        }

        NC_LOG_ERROR("While creating DescriptorSet we encountered binding (%s) of type (%u) which did not have a matching BindInfo in the bound shaders", name.c_str(), type);
        return false;
    }

    void RendererVK::BindDescriptor(Backend::DescriptorSetBuilderVK* builder, void* imageInfosArraysVoid, Descriptor& descriptor, u32 frameIndex)
    {
        std::vector<std::vector<VkDescriptorImageInfo>>& imageInfosArrays = *static_cast<std::vector<std::vector<VkDescriptorImageInfo>>*>(imageInfosArraysVoid);

        if (descriptor.descriptorType == DescriptorType::DESCRIPTOR_TYPE_SAMPLER)
        {
            VkDescriptorImageInfo imageInfo = {};
            imageInfo.sampler = _samplerHandler->GetSampler(descriptor.samplerID);

            builder->BindSampler(descriptor.nameHash, imageInfo);
        }
        else if (descriptor.descriptorType == DescriptorType::DESCRIPTOR_TYPE_TEXTURE)
        {
            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = _textureHandler->GetImageView(descriptor.textureID);

            builder->BindImage(descriptor.nameHash, imageInfo);
        }
        else if (descriptor.descriptorType == DescriptorType::DESCRIPTOR_TYPE_TEXTURE_ARRAY)
        {
            const std::vector<TextureID>& textureIDs = _textureHandler->GetTextureIDsInArray(descriptor.textureArrayID);
            std::vector<VkDescriptorImageInfo>& imageInfos = imageInfosArrays.emplace_back();

            u32 textureArraySize = _textureHandler->GetTextureArraySize(descriptor.textureArrayID);
            imageInfos.reserve(textureArraySize);
            
            u32 numTextures = static_cast<u32>(textureIDs.size());

            // From 0 to numTextures, add our actual textures
            for (auto textureID : textureIDs)
            {
                VkDescriptorImageInfo& imageInfo = imageInfos.emplace_back();
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo.imageView = _textureHandler->GetImageView(textureID);
                imageInfo.sampler = VK_NULL_HANDLE;
            }

            // from numTextures to textureArraySize, add debug texture
            VkDescriptorImageInfo imageInfoDebugTexture;
            imageInfoDebugTexture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfoDebugTexture.imageView = _textureHandler->GetDebugTextureImageView();
            imageInfoDebugTexture.sampler = VK_NULL_HANDLE;

            for (u32 i = numTextures; i < textureArraySize; i++)
            {
                imageInfos.push_back(imageInfoDebugTexture);
            }
            
            builder->BindImageArray(descriptor.nameHash, imageInfos.data(), static_cast<i32>(imageInfos.size()));
        }
        else if (descriptor.descriptorType == DescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER)
        {
            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = *static_cast<VkBuffer*>(descriptor.constantBuffer->GetBuffer(frameIndex));
            bufferInfo.range = descriptor.constantBuffer->GetSize();

            builder->BindBuffer(descriptor.nameHash, bufferInfo);
        }
        else if (descriptor.descriptorType == DescriptorType::DESCRIPTOR_TYPE_STORAGE_BUFFER)
        {
            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = *static_cast<VkBuffer*>(descriptor.storageBuffer->GetBuffer(frameIndex));
            bufferInfo.range = descriptor.storageBuffer->GetSize();

            builder->BindBuffer(descriptor.nameHash, bufferInfo);
        }
    }

    void RendererVK::BindDescriptorSet(CommandListID commandListID, DescriptorSetSlot slot,Descriptor* descriptors, u32 numDescriptors, u32 frameIndex)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        GraphicsPipelineID graphicsPipelineID = _commandListHandler->GetBoundGraphicsPipeline(commandListID);

        using type = type_safe::underlying_type<GraphicsPipelineID>;
        
        std::vector<std::vector<VkDescriptorImageInfo>> imageInfosArrays; // These need to live until builder->BuildDescriptor()
        imageInfosArrays.reserve(8);

        Backend::DescriptorSetBuilderVK* builder = _pipelineHandler->GetDescriptorSetBuilder(graphicsPipelineID);
        assert(slot != DescriptorSetSlot::GLOBAL); // TODO: this won't need or have a graphicspipelineID, not sure how to do that yet

        for (u32 i = 0; i < numDescriptors; i++)
        {
            Descriptor& descriptor = descriptors[i];
            BindDescriptor(builder, &imageInfosArrays, descriptor, frameIndex);
        }

        VkDescriptorSet descriptorSet = builder->BuildDescriptor(static_cast<i32>(slot), Backend::DescriptorLifetime::PerFrame);
                
        VkPipelineLayout pipelineLayout = _pipelineHandler->GetPipelineLayout(graphicsPipelineID);

        // Bind descriptor set
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, slot, 1, &descriptorSet, 0, nullptr);
    }

    void RendererVK::MarkFrameStart(CommandListID commandListID, u32 frameIndex)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        Backend::DebugMarkerUtilVK::PushMarker(commandBuffer, Color(1,1,1,1), std::to_string(frameIndex));

        _device->_descriptorMegaPool->SetFrame(frameIndex);
    }

    void RendererVK::Present(Window* window, ImageID imageID)
    {
        CommandListID commandListID = _commandListHandler->BeginCommandList(_device);
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        PushMarker(commandListID, Color::Red, "Present Blitting");

        Backend::SwapChainVK* swapChain = static_cast<Backend::SwapChainVK*>(window->GetSwapChain());

        u32 semaphoreIndex = swapChain->frameIndex;

        // Acquire next swapchain image
        u32 frameIndex;

        vkAcquireNextImageKHR(_device->_device, swapChain->swapChain, UINT64_MAX, swapChain->imageAvailableSemaphores[semaphoreIndex], VK_NULL_HANDLE, &frameIndex);
        _commandListHandler->SetWaitSemaphore(commandListID, swapChain->imageAvailableSemaphores[semaphoreIndex]);

        ImageDesc imageDesc = _imageHandler->GetImageDesc(imageID);
        ImageComponentType componentType = ToImageComponentType(imageDesc.format);

        Backend::BlitPipeline& pipeline = swapChain->blitPipelines[componentType];

        VkImage image = _imageHandler->GetImage(imageID);

        // Update SRV descriptor
        VkDescriptorImageInfo imageInfos[2] = {};
        imageInfos[0].sampler = swapChain->sampler;

        imageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[1].imageView = _imageHandler->GetColorView(imageID);

        VkWriteDescriptorSet descriptorWrites[2] = {};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = pipeline.descriptorSet;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = nullptr;
        descriptorWrites[0].pImageInfo = &imageInfos[0];
        descriptorWrites[0].pTexelBufferView = nullptr;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = pipeline.descriptorSet;
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = nullptr;
        descriptorWrites[1].pImageInfo = &imageInfos[1];
        descriptorWrites[1].pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(_device->_device, 2, descriptorWrites, 0, nullptr);

        // Set up renderpass
        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = swapChain->renderPass;
        renderPassInfo.framebuffer = swapChain->framebuffers[frameIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = swapChain->extent;

        VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        // Transition image from GENERAL to SHADER_READ_ONLY_OPTIMAL
        _device->TransitionImageLayout(commandBuffer, image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, imageDesc.depth);
       
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
       
        // Bind pipeline and descriptors and render
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipelineLayout, 0, 1, &pipeline.descriptorSet, 0, nullptr);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        // Transition image from SHADER_READ_ONLY_OPTIMAL to GENERAL
        _device->TransitionImageLayout(commandBuffer, image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, imageDesc.depth);
        PopMarker(commandListID);

        _commandListHandler->EndCommandList(_device, commandListID);

        // Present
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 0;
        presentInfo.pWaitSemaphores = VK_NULL_HANDLE;

        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapChain->swapChain;
        presentInfo.pImageIndices = &frameIndex;
        presentInfo.pResults = nullptr; // Optional

        vkQueuePresentKHR(_device->_presentQueue, &presentInfo);
        vkQueueWaitIdle(_device->_presentQueue);

        // Flip frameIndex between 0 and 1
        swapChain->frameIndex = !swapChain->frameIndex;
    }

    void RendererVK::Present(Window* /*window*/, DepthImageID /*image*/)
    {
        
    }

    Backend::BufferBackend* RendererVK::CreateBufferBackend(size_t size, Backend::BufferBackend::Type type)
    {
        return _device->CreateBufferBackend(size, type);
    }
}