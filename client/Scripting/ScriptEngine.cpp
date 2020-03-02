#include "ScriptEngine.h"
#include <Utils/DebugHandler.h>

// Types to register
#include "Addons/scriptarray/scriptarray.h"
#include "Addons/scriptstdstring/scriptstdstring.h"
#include "Classes/Player.h"
#include "Classes/UI/UIWidget.h"
#include "Classes/UI/UIPanel.h"

thread_local asIScriptEngine* ScriptEngine::_scriptEngine = nullptr;
thread_local asIScriptContext* ScriptEngine::_scriptContext = nullptr;
thread_local std::string ScriptEngine::_scriptCurrentObjectName = "";

void ScriptEngine::Initialize()
{
    if (!_scriptEngine)
    {
        _scriptEngine = asCreateScriptEngine();
        _scriptEngine->SetEngineProperty(asEP_DISALLOW_GLOBAL_VARS, true);
        RegisterFunctions();
    }

    if (!_scriptContext)
    {
        _scriptContext = _scriptEngine->CreateContext();
    }
}
asIScriptEngine* ScriptEngine::GetScriptEngine()
{
    Initialize();
    return _scriptEngine;
}
asIScriptContext* ScriptEngine::GetScriptContext()
{
    Initialize();
    return _scriptContext;
}

i32 ScriptEngine::RegisterScriptClass(std::string name, i32 byteSize, u32 flags)
{
    _scriptCurrentObjectName = name;
    return _scriptEngine->RegisterObjectType(name.c_str(), byteSize, flags);
}
i32 ScriptEngine::RegisterScriptClassFunction(std::string declaration, const asSFuncPtr& functionPointer, bool isStatic, void* auxiliary, i32 compositeOffset, bool isCompositeIndirect)
{
    return _scriptEngine->RegisterObjectMethod(_scriptCurrentObjectName.c_str(), declaration.c_str(), functionPointer, isStatic ? asECallConvTypes::asCALL_THISCALL_ASGLOBAL : asECallConvTypes::asCALL_THISCALL, auxiliary, compositeOffset, isCompositeIndirect);
}
i32 ScriptEngine::RegisterScriptClassProperty(std::string declaration, i32 byteOffset, i32 compositeOffset, bool isCompositeIndirect)
{
    return _scriptEngine->RegisterObjectProperty(_scriptCurrentObjectName.c_str(), declaration.c_str(), byteOffset, compositeOffset, isCompositeIndirect);
}

i32 ScriptEngine::RegisterScriptFunction(std::string declaration, const asSFuncPtr& functionPointer, void* auxiliary)
{
    return _scriptEngine->RegisterGlobalFunction(declaration.c_str(), functionPointer, asCALL_CDECL, auxiliary);
}

void ScriptEngine::RegisterFunctions()
{
    // Defaults
    int result;
    result = _scriptEngine->SetMessageCallback(asFUNCTION(ScriptEngine::MessageCallback), 0, asCALL_CDECL);
    assert(result >= 0);
    RegisterScriptArray(_scriptEngine, true);
    RegisterStdString(_scriptEngine);
    RegisterStdStringUtils(_scriptEngine);

    // NovusCore Types
    Player::RegisterType();
    UIWidget::RegisterType();
    UIPanel::RegisterType();

    ScriptEngine::RegisterScriptFunction("void Print(string msg)", asFUNCTION(ScriptEngine::Print));
}

void ScriptEngine::MessageCallback(const asSMessageInfo* msg, void* param)
{
    if (msg->type == asMSGTYPE_ERROR)
    {
        NC_LOG_ERROR("[Script]: %s (%d, %d) : %s\n", msg->section, msg->row, msg->col, msg->message);
    }
    else if (msg->type == asMSGTYPE_WARNING)
    {
        NC_LOG_WARNING("[Script]: %s (%d, %d) : %s\n", msg->section, msg->row, msg->col, msg->message);
    }
    else if (msg->type == asMSGTYPE_INFORMATION)
    {
        NC_LOG_MESSAGE("[Script]: %s (%d, %d) : %s\n", msg->section, msg->row, msg->col, msg->message);
    }
}
void ScriptEngine::Print(std::string& message)
{
    NC_LOG_MESSAGE("[Script]: %s", message.c_str());
}