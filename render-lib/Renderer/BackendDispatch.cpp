#include "BackendDispatch.h"
#include "Renderer.h"
#include <tracy/Tracy.hpp>

#include "Commands/Clear.h"
#include "Commands/Draw.h"
#include "Commands/DrawBindless.h"
#include "Commands/DrawIndexedBindless.h"
#include "Commands/PopMarker.h"
#include "Commands/PushMarker.h"
#include "Commands/SetPipeline.h"
#include "Commands/SetScissorRect.h"
#include "Commands/SetViewport.h"
#include "Commands/SetVertexBuffer.h"
#include "Commands/SetIndexBuffer.h"
#include "Commands/SetBuffer.h"
#include "Commands/BindDescriptorSet.h"
#include "Commands/MarkFrameStart.h"

namespace Renderer
{
    void BackendDispatch::ClearImage(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3)
        const Commands::ClearImage* actualData = static_cast<const Commands::ClearImage*>(data);
        renderer->Clear(commandList, actualData->image, actualData->color);
    }
    void BackendDispatch::ClearDepthImage(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3)
        const Commands::ClearDepthImage* actualData = static_cast<const Commands::ClearDepthImage*>(data);
        renderer->Clear(commandList, actualData->image, actualData->flags, actualData->depth, actualData->stencil);
    }

    void BackendDispatch::Draw(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3)
        const Commands::Draw* actualData = static_cast<const Commands::Draw*>(data);
        renderer->Draw(commandList, actualData->model);
    }

    void BackendDispatch::DrawBindless(Renderer * renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3)
        const Commands::DrawBindless* actualData = static_cast<const Commands::DrawBindless*>(data);
        renderer->DrawBindless(commandList, actualData->numVertices, actualData->numInstances);
    }

    void BackendDispatch::DrawIndexedBindless(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3)
        const Commands::DrawIndexedBindless* actualData = static_cast<const Commands::DrawIndexedBindless*>(data);
        renderer->DrawIndexedBindless(commandList, actualData->modelID, actualData->numVertices, actualData->numInstances);
    }

    void BackendDispatch::MarkFrameStart(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3)
        const Commands::MarkFrameStart* actualData = static_cast<const Commands::MarkFrameStart*>(data);
        renderer->MarkFrameStart(commandList, actualData->frameIndex);
    }

    void BackendDispatch::PopMarker(Renderer* renderer, CommandListID commandList, const void* /*data*/)
    {
        ZoneScopedC(tracy::Color::Red3)
        renderer->PopMarker(commandList);
    }

    void BackendDispatch::PushMarker(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3)
        const Commands::PushMarker* actualData = static_cast<const Commands::PushMarker*>(data);
        renderer->PushMarker(commandList, actualData->color, actualData->marker);
    }

    void BackendDispatch::BeginGraphicsPipeline(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3)
        const Commands::BeginGraphicsPipeline* actualData = static_cast<const Commands::BeginGraphicsPipeline*>(data);
        renderer->BeginPipeline(commandList, actualData->pipeline);
    }

    void BackendDispatch::EndGraphicsPipeline(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3)
        const Commands::EndGraphicsPipeline* actualData = static_cast<const Commands::EndGraphicsPipeline*>(data);
        renderer->EndPipeline(commandList, actualData->pipeline);
    }

    void BackendDispatch::SetComputePipeline(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3)
        const Commands::SetComputePipeline* actualData = static_cast<const Commands::SetComputePipeline*>(data);
        renderer->SetPipeline(commandList, actualData->pipeline);
    }

    void BackendDispatch::BindDescriptorSet(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3)
        const Commands::BindDescriptorSet* actualData = static_cast<const Commands::BindDescriptorSet*>(data);
        renderer->BindDescriptorSet(commandList, actualData->slot, actualData->descriptors, actualData->numDescriptors, actualData->frameIndex);
    }

    void BackendDispatch::SetScissorRect(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3)
        const Commands::SetScissorRect* actualData = static_cast<const Commands::SetScissorRect*>(data);
        renderer->SetScissorRect(commandList, actualData->scissorRect);
    }

    void BackendDispatch::SetViewport(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3)
        const Commands::SetViewport* actualData = static_cast<const Commands::SetViewport*>(data);
        renderer->SetViewport(commandList, actualData->viewport);
    }

    void BackendDispatch::SetVertexBuffer(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3)
        const Commands::SetVertexBuffer* actualData = static_cast<const Commands::SetVertexBuffer*>(data);
        renderer->SetVertexBuffer(commandList, actualData->slot, actualData->modelID);
    }

    void BackendDispatch::SetIndexBuffer(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3)
        const Commands::SetIndexBuffer* actualData = static_cast<const Commands::SetIndexBuffer*>(data);
        renderer->SetIndexBuffer(commandList, actualData->modelID);
    }

    void BackendDispatch::SetBuffer(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3)
        const Commands::SetBuffer* actualData = static_cast<const Commands::SetBuffer*>(data);
        renderer->SetBuffer(commandList, actualData->slot, actualData->buffer);
    }
}