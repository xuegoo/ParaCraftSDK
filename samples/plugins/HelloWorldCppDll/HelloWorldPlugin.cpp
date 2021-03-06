#include "stdafx.h"
#include "HelloWorld.h"

/**
* Optional NPL includes, just in case you want to use some core functions see GetCoreInterface()
*/
#include "INPLRuntime.h"
#include "INPLRuntimeState.h"
#include "IParaEngineCore.h"
#include "IParaEngineApp.h"
#include "IAttributeFields.h"

using namespace ParaEngine;

#ifdef WIN32
#define CORE_EXPORT_DECL    __declspec(dllexport)
#else
#define CORE_EXPORT_DECL
#endif

// forward declare of exported functions. 
extern "C" {
	CORE_EXPORT_DECL const char* LibDescription();
	CORE_EXPORT_DECL int LibNumberClasses();
	CORE_EXPORT_DECL unsigned long LibVersion();
	CORE_EXPORT_DECL ParaEngine::ClassDescriptor* LibClassDesc(int i);
	CORE_EXPORT_DECL void LibInit();
	CORE_EXPORT_DECL void LibActivate(int nType, void* pVoid);
	CORE_EXPORT_DECL void LibInitParaEngine(ParaEngine::IParaEngineCore* pCoreInterface);
}
 
HINSTANCE Instance = NULL;

ClassDescriptor* HelloWorldPlugin_GetClassDesc();
typedef ClassDescriptor* (*GetClassDescMethod)();

GetClassDescMethod Plugins[] = 
{
	HelloWorldPlugin_GetClassDesc,
};

/** This has to be unique, change this id for each new plugin.
*/
#define HelloWorld_CLASS_ID Class_ID(0x2b905a29, 0x47b409af)

class HelloWorldPluginDesc:public ClassDescriptor
{
public:
	void* Create(bool loading = FALSE)
	{
		return new CHelloWorld();
	}

	const char* ClassName()
	{
		return "IHelloWorld";
	}

	SClass_ID SuperClassID()
	{
		return OBJECT_MODIFIER_CLASS_ID;
	}

	Class_ID ClassID()
	{
		return HelloWorld_CLASS_ID;
	}

	const char* Category() 
	{ 
		return "HelloWorld"; 
	}

	const char* InternalName() 
	{ 
		return "HelloWorld"; 
	}

	HINSTANCE HInstance() 
	{ 
		extern HINSTANCE Instance;
		return Instance; 
	}
};

ClassDescriptor* HelloWorldPlugin_GetClassDesc()
{
	static HelloWorldPluginDesc s_desc;
	return &s_desc;
}

CORE_EXPORT_DECL const char* LibDescription()
{
	return "ParaEngine HelloWorld Ver 1.0.0";
}

CORE_EXPORT_DECL unsigned long LibVersion()
{
	return 1;
}

CORE_EXPORT_DECL int LibNumberClasses()
{
	return sizeof(Plugins)/sizeof(Plugins[0]);
}

CORE_EXPORT_DECL ClassDescriptor* LibClassDesc(int i)
{
	if (i < LibNumberClasses() && Plugins[i])
	{
		return Plugins[i]();
	}
	else
	{
		return NULL;
	}
}

ParaEngine::IParaEngineCore* g_pCoreInterface = NULL;
ParaEngine::IParaEngineCore* GetCoreInterface()
{
	return g_pCoreInterface;
}

CORE_EXPORT_DECL void LibInitParaEngine(IParaEngineCore* pCoreInterface)
{
	g_pCoreInterface = pCoreInterface;
}

CORE_EXPORT_DECL void LibInit()
{
}

#ifdef WIN32
BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved)
#else
void __attribute__ ((constructor)) DllMain()
#endif
{
	// TODO: dll start up code here
#ifdef WIN32
	Instance = hinstDLL;				// Hang on to this DLL's instance handle.
	return (TRUE);
#endif
}

extern "C" {
	/** this is an example of c function calling NPL core interface */
	void WriteLog(const char* str) {
		if(GetCoreInterface())
			GetCoreInterface()->GetAppInterface()->WriteToLog(str);
	}
}

/** this is the main activate function to be called. Test with 
```	
	NPL.activate("this_file.dll", msg); 
```
or with synchronous invocation, use
```
	NPL.call("temp/HelloWorldPlugin.dll", {cmd=abc}); 
	echo(msg);
```
*/
CORE_EXPORT_DECL void LibActivate(int nType, void* pVoid)
{
	using namespace ParaEngine;

	if(nType == ParaEngine::PluginActType_STATE)
	{
		NPL::INPLRuntimeState* pState = (NPL::INPLRuntimeState*)pVoid;
		const char* sMsg = pState->GetCurrentMsg();
		int nMsgLength = pState->GetCurrentMsgLength();

		NPLInterface::NPLObjectProxy input_msg = NPLInterface::NPLHelper::MsgStringToNPLTable(sMsg);
		const std::string& sCmd = input_msg["cmd"];
		if(sCmd == "hello" || true)
		{
			NPLInterface::NPLObjectProxy output_msg;
			output_msg["succeed"] = "true";
			output_msg["sample_number_output"] = (double)(1234567);
			output_msg["result"] = "hello world!";

			std::string output;
			NPLInterface::NPLHelper::NPLTableToString("msg", output_msg, output);
			// example output 1: return result using async callback to any thread to remote address
			pState->activate("script/test/echo.lua", output.c_str(), output.size());
			// example output 2: we can also write the result synchronously into a global msg variable.
			pState->call("", output.c_str(), output.size());
		}

		WriteLog("\n---------------------\nthis is called from c++ plugin\n");

		//////////////////////////////////////////////////
		// 
		// example 2: calling C++ attribute object, no header files are required, type safe
		// use NPL code wiki's object browser to find the child name or index
		// 
		//////////////////////////////////////////////////

		auto pParaEngine = GetCoreInterface()->GetAppInterface()->GetAttributeObject();
		// set "WindowText" to "HelloWorldPlugin"
		pParaEngine->GetAttributeClass()->GetField("WindowText")->Set(pParaEngine, "HelloWorldPlugin");
		auto pAsyncLoader = pParaEngine->GetChildAttributeObject("AsyncLoader");
		// get "ItemsLeft" to nItemsLeft
		int nItemsLeft = -1;
		pAsyncLoader->GetAttributeClass()->GetField("ItemsLeft")->Get(pAsyncLoader, &nItemsLeft);

		auto pScene = pParaEngine->GetChildAttributeObject("Scene");
		auto pBlockEngine = pScene->GetChildAttributeObject(1, 1); // block engine is at row:col(1:1)
		int nDirtyBlockCount = -1;
		pBlockEngine->GetAttributeClass()->GetField("DirtyBlockCount")->Get(pBlockEngine, &nDirtyBlockCount);
		
		GetCoreInterface()->GetAppInterface()->WriteToLog("items left: %d, nDirtyBlockCount: %d", nItemsLeft, nDirtyBlockCount);
	}
}
