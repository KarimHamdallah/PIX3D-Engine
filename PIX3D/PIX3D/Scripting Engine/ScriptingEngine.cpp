#include "ScriptingEngine.h"
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/tabledefs.h>
#include <fstream>

namespace PIX3D
{
    namespace Utils
    {
        static MonoAssembly* LoadAssemblyFromFile(const std::filesystem::path& assemblyPath)
        {
            if (!std::filesystem::exists(assemblyPath))
            {
                PIX_DEBUG_ERROR_FORMAT("Failed to find assembly at path: {0}", assemblyPath.string());
                return nullptr;
            }

            // Read the file
            std::ifstream stream(assemblyPath, std::ios::binary | std::ios::ate);
            if (!stream)
            {
                PIX_DEBUG_ERROR_FORMAT("Failed to open assembly file: {0}", assemblyPath.string());
                return nullptr;
            }

            std::streampos end = stream.tellg();
            stream.seekg(0, std::ios::beg);
            uint32_t size = static_cast<uint32_t>(end - stream.tellg());

            std::vector<char> fileData(size);
            stream.read(fileData.data(), size);

            // Load the assembly
            MonoImageOpenStatus status;
            MonoImage* image = mono_image_open_from_data_full(
                fileData.data(), size, true, &status, false);

            if (status != MONO_IMAGE_OK)
            {
                //PIX_DEBUG_ERROR_FORMAT("Failed to load assembly image: {0}", mono_image_strerror(status));
                PIX_ASSERT(false);
                return nullptr;
            }

            MonoAssembly* assembly = mono_assembly_load_from_full(
                image, assemblyPath.string().c_str(), &status, false);

            mono_image_close(image);

            return assembly;
        }
    }

    bool ScriptEngine::Init(const std::filesystem::path& corePath)
    {
        PIX_DEBUG_INFO("Initializing Script Engine...");

        // Initialize Mono
        mono_set_assemblies_path("mono/lib");

        s_RootDomain = mono_jit_init("PIX3DRuntime");
        if (!s_RootDomain)
        {
            PIX_DEBUG_ERROR("Failed to initialize Mono runtime!");
            return false;
        }

        s_AppDomain = mono_domain_create_appdomain((char*)"PIX3DRuntime", nullptr);
        if (!s_AppDomain || !mono_domain_set(s_AppDomain, true))
        {
            PIX_DEBUG_ERROR("Failed to create app domain!");
            return false;
        }

        // Load core assembly
        if (!LoadCoreAssembly(corePath))
            return false;

        PIX_DEBUG_SUCCESS("Script Engine initialized successfully!");
        return true;
    }

    bool ScriptEngine::LoadCoreAssembly(const std::filesystem::path& corePath)
    {
        PIX_DEBUG_INFO_FORMAT("Loading core assembly: {0}", corePath.string());

        s_CoreAssemblyPath = corePath;
        s_CoreAssembly = Utils::LoadAssemblyFromFile(corePath);

        if (!s_CoreAssembly)
        {
            PIX_DEBUG_ERROR("Failed to load core assembly!");
            return false;
        }

        // Cache the Entity class
        s_EntityClass = mono_class_from_name(
            mono_assembly_get_image(s_CoreAssembly),
            "PIX3D",
            "PIXEntity"
        );

        if (!s_EntityClass)
        {
            PIX_DEBUG_ERROR("Failed to find PIXEntity class in core assembly!");
            return false;
        }

        PIX_DEBUG_SUCCESS("Core assembly loaded successfully!");
        return true;
    }

    bool ScriptEngine::LoadAppAssembly(const std::filesystem::path& appPath)
    {
        PIX_DEBUG_INFO_FORMAT("Loading app assembly: {0}", appPath.string());

        s_AppAssemblyPath = appPath;
        s_AppAssembly = Utils::LoadAssemblyFromFile(appPath);

        if (!s_AppAssembly)
        {
            PIX_DEBUG_ERROR("Failed to load app assembly!");
            return false;
        }

        // Load script classes
        if (!LoadScriptClasses())
            return false;

        PIX_DEBUG_SUCCESS("App assembly loaded successfully!");
        return true;
    }

    bool ScriptEngine::LoadScriptClasses()
    {
        if (!s_AppAssembly || !s_EntityClass)
        {
            PIX_DEBUG_ERROR("Cannot load script classes - missing required assemblies!");
            return false;
        }

        s_ScriptClasses.clear();

        MonoImage* image = mono_assembly_get_image(s_AppAssembly);
        const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
        int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);

        uint32_t rowCount = 0;

        for (int32_t i = 0; i < numTypes; i++)
        {
            uint32_t cols[MONO_TYPEDEF_SIZE];
            mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

            const char* nameSpace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);
            const char* className = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);

            MonoClass* monoClass = mono_class_from_name(image, nameSpace, className);
            if (monoClass && mono_class_is_subclass_of(monoClass, s_EntityClass, false))
            {
                s_ScriptClasses[className] = true;
                rowCount++;
            }
        }

        PIX_DEBUG_SUCCESS_FORMAT("Loaded {0} script classes", rowCount);
        return true;
    }

    bool ScriptEngine::ReloadAppAssembly()
    {
        if (s_AppAssemblyPath.empty())
        {
            PIX_DEBUG_WARNING("No app assembly to reload!");
            return false;
        }

        PIX_DEBUG_INFO("Reloading app assembly...");

        // Unload current domain
        mono_domain_set(mono_get_root_domain(), false);
        mono_domain_unload(s_AppDomain);

        // Create new domain
        s_AppDomain = mono_domain_create_appdomain((char*)"PIX3DRuntime", nullptr);
        if (!mono_domain_set(s_AppDomain, true))
        {
            PIX_DEBUG_ERROR("Failed to create new app domain for reload!");
            return false;
        }

        // Reload core assembly first
        if (!LoadCoreAssembly(s_CoreAssemblyPath))
            return false;

        // Then reload app assembly
        return LoadAppAssembly(s_AppAssemblyPath);
    }

    void ScriptEngine::Shutdown()
    {
        if (s_AppDomain)
        {
            mono_domain_set(mono_get_root_domain(), false);
            mono_domain_unload(s_AppDomain);
            s_AppDomain = nullptr;
        }

        if (s_RootDomain)
        {
            mono_jit_cleanup(s_RootDomain);
            s_RootDomain = nullptr;
        }

        s_CoreAssembly = nullptr;
        s_AppAssembly = nullptr;
        s_EntityClass = nullptr;
        s_ScriptClasses.clear();
    }

    void ScriptEngine::OnRuntimeStart(Scene* scene)
    {
        PIX_DEBUG_INFO("Starting script runtime...");
        s_Scene = scene;
    }

    void ScriptEngine::OnRuntimeStop()
    {
        PIX_DEBUG_INFO("Stopping script runtime...");
        s_Scene = nullptr;
    }

    bool ScriptEngine::EntityClassExists(const std::string& className)
    {
        return s_ScriptClasses.contains(className);
    }

    ScriptInstance::ScriptInstance(const std::string& NameSpace, const std::string& className)
        : m_ClassName(className), m_NameSpace(NameSpace)
    {   
        ScriptEngine::GetAppImage();

        // Get the script class from the game assembly
        MonoClass* monoClass = mono_class_from_name(
            ScriptEngine::GetAppImage(),
            NameSpace.c_str(),
            className.c_str()
        );


        if (!monoClass)
        {
            PIX_DEBUG_ERROR_FORMAT("Failed to find script class: {0}", className);
            PIX_ASSERT(monoClass);
        }

        // Create instance of the class
        m_Instance = mono_object_new(mono_domain_get(), monoClass);
        if (!m_Instance)
        {
            PIX_DEBUG_ERROR_FORMAT("Failed to create instance of script class: {0}", className);
            PIX_ASSERT(monoClass);
        }

        // Initialize the instance
        mono_runtime_object_init(m_Instance);

        // Create GC handle to prevent the instance from being collected
        m_GCHandle = mono_gchandle_new(m_Instance, false);

        // Cache method pointers
        m_OnCreateMethod = mono_class_get_method_from_name(monoClass, "OnCreate", 0);
        m_OnStartMethod = mono_class_get_method_from_name(monoClass, "OnStart", 0);
        m_OnUpdateMethod = mono_class_get_method_from_name(monoClass, "OnUpdate", 1);
        m_OnDestroyMethod = mono_class_get_method_from_name(monoClass, "OnDestroy", 0);
    }

    ScriptInstance::~ScriptInstance()
    {
        if (m_GCHandle)
        {
            mono_gchandle_free(m_GCHandle);
            m_GCHandle = 0;
        }

        m_Instance = nullptr;
    }

    void ScriptInstance::OnCreate()
    {
        if (!m_Instance)
        {
            PIX_DEBUG_ERROR_FORMAT("Trying to call OnCreate on invalid script instance: {0}", m_ClassName);
            return;
        }

        if (m_OnCreateMethod)
        {
            MonoObject* exception = nullptr;
            mono_runtime_invoke(m_OnCreateMethod, m_Instance, nullptr, &exception);

            if (exception)
            {
                mono_print_unhandled_exception(exception);
                PIX_DEBUG_ERROR_FORMAT("Exception in OnCreate for script: {0}", m_ClassName);
            }
        }
    }

    void ScriptInstance::OnStart()
    {
        if (!m_Instance)
        {
            PIX_DEBUG_ERROR_FORMAT("Trying to call OnStart on invalid script instance: {0}", m_ClassName);
            return;
        }

        if (m_OnStartMethod)
        {
            MonoObject* exception = nullptr;
            mono_runtime_invoke(m_OnStartMethod, m_Instance, nullptr, &exception);

            if (exception)
            {
                mono_print_unhandled_exception(exception);
                PIX_DEBUG_ERROR_FORMAT("Exception in OnStart for script: {0}", m_ClassName);
            }
        }
    }

    void ScriptInstance::OnUpdate(float dt)
    {
        if (!m_Instance)
        {
            PIX_DEBUG_ERROR_FORMAT("Trying to call OnUpdate on invalid script instance: {0}", m_ClassName);
            return;
        }

        if (m_OnUpdateMethod)
        {
            void* params[] = { &dt };
            MonoObject* exception = nullptr;
            mono_runtime_invoke(m_OnUpdateMethod, m_Instance, params, &exception);

            if (exception)
            {
                mono_print_unhandled_exception(exception);
                PIX_DEBUG_ERROR_FORMAT("Exception in OnUpdate for script: {0}", m_ClassName);
            }
        }
    }

    void ScriptInstance::OnDestroy()
    {
        if (!m_Instance)
        {
            PIX_DEBUG_ERROR_FORMAT("Trying to call OnDestroy on invalid script instance: {0}", m_ClassName);
            return;
        }

        if (m_OnDestroyMethod)
        {
            MonoObject* exception = nullptr;
            mono_runtime_invoke(m_OnDestroyMethod, m_Instance, nullptr, &exception);

            if (exception)
            {
                mono_print_unhandled_exception(exception);
                PIX_DEBUG_ERROR_FORMAT("Exception in OnDestroy for script: {0}", m_ClassName);
            }
        }
    }

    void ScriptInstance::SetEntityUUID(PIX3D::UUID uuid)
    {
        if (!m_Instance)
        {
            PIX_DEBUG_ERROR_FORMAT("Trying to set UUID on invalid script instance: {0}", m_ClassName);
            return;
        }

        MonoClass* entityClass = ScriptEngine::GetEntityClass();
        if (!entityClass)
        {
            PIX_DEBUG_ERROR("Entity class not found in core assembly!");
            return;
        }

        MonoClassField* uuidField = mono_class_get_field_from_name(entityClass, "UUID");
        if (!uuidField)
        {
            PIX_DEBUG_ERROR("UUID field not found in Entity class!");
            return;
        }

        uint64_t id = static_cast<uint64_t>(uuid);
        mono_field_set_value(m_Instance, uuidField, &id);
        //PIX_DEBUG_INFO_FORMAT("Set UUID {0} for script: {1}", uuid, m_ClassName);
    }

    void ScriptEngine::OnCreateEntity(ScriptComponentCSharp* scriptComponent, PIX3D::UUID uuid)
    {
        if (!EntityClassExists(scriptComponent->ClassName))
        {
            PIX_DEBUG_ERROR_FORMAT("Script class not found: {0}", scriptComponent->ClassName);
            PIX_ASSERT(false);
        }

        scriptComponent->Script = new ScriptInstance(scriptComponent->NameSpaceName, scriptComponent->ClassName);

        scriptComponent->Script->SetEntityUUID(uuid);
        scriptComponent->Script->OnCreate();
    }

    void ScriptEngine::OnUpdateEntity(ScriptComponentCSharp* scriptComponent, PIX3D::UUID uuid, float dt)
    {
        if (!scriptComponent || !scriptComponent->Script)
            return;

        if (!scriptComponent->OnStartCalled)
        {
            scriptComponent->Script->OnStart();
            scriptComponent->OnStartCalled = true;
        }

        scriptComponent->Script->OnUpdate(dt);
    }

    void ScriptEngine::OnDestroyEntity(ScriptComponentCSharp* scriptComponent, PIX3D::UUID uuid)
    {
        if (!scriptComponent || !scriptComponent->Script)
            return;

        scriptComponent->Script->OnDestroy();
        delete scriptComponent->Script;
        scriptComponent->Script = nullptr;
        scriptComponent->OnStartCalled = false;
    }
}
