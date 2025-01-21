#pragma once
#include <mono/metadata/object.h>
#include <mono/metadata/class.h>
#include <mono/metadata/assembly.h>
#include <Core/UUID.h>
#include <map>
#include <vector>
#include <string>
#include <Scene/Scene.h>
#include <Core/Core.h>

namespace PIX3D
{
    struct ScriptFieldInstance
    {
        ScriptFieldInstance()
        {
            memset(m_Buffer, 0, sizeof(m_Buffer));
        }

        template<typename T>
        T GetValue()
        {
            static_assert(sizeof(T) <= 16, "Type too large!");
            return *(T*)m_Buffer;
        }

        template<typename T>
        void SetValue(T value)
        {
            static_assert(sizeof(T) <= 16, "Type too large!");
            memcpy(m_Buffer, &value, sizeof(T));
        }
    private:
        uint8_t m_Buffer[16];
    };

    enum class ScriptFieldType
    {
        None = 0,
        Float, Int, UInt, Bool,
        Vec2, Vec3, Vec4,
        UUID,
        String
    };

    struct ScriptField
    {
        std::string Name;
        ScriptFieldType Type;
        MonoClassField* Field = nullptr;
    };

    class ScriptInstance
    {
    public:
        ScriptInstance() = default;
        ScriptInstance(const std::string& NameSpace, const std::string& className);
        ~ScriptInstance();

        void OnCreate();
        void OnStart();
        void OnUpdate(float dt);
        void OnDestroy();

        void SetEntityUUID(PIX3D::UUID uuid);
        MonoObject* GetManagedObject() { return mono_gchandle_get_target(m_GCHandle); }

        std::map<std::string, ScriptFieldInstance>& GetFieldInstances() { return m_FieldInstances; }
        const std::vector<std::string>& GetFieldNames() const { return m_FieldNames; }
        const std::vector<ScriptFieldType>& GetFieldTypes() const { return m_FieldTypes; }

        const std::string& GetClassName() const { return m_ClassName; }
    private:
        std::string m_ClassName;
        std::string m_NameSpace;
        MonoObject* m_Instance = nullptr;
        uint32_t m_GCHandle = 0;

        MonoMethod* m_OnCreateMethod = nullptr;
        MonoMethod* m_OnStartMethod = nullptr;
        MonoMethod* m_OnUpdateMethod = nullptr;
        MonoMethod* m_OnDestroyMethod = nullptr;

        std::map<std::string, ScriptFieldInstance> m_FieldInstances;
        std::vector<std::string> m_FieldNames;
        std::vector<ScriptFieldType> m_FieldTypes;
    };

    class ScriptEngine
    {
    public:
        // Core functions
        static bool Init(const std::filesystem::path& corePath);
        static void Shutdown();

        // Assembly management
        static bool LoadCoreAssembly(const std::filesystem::path& corePath);
        static bool LoadAppAssembly(const std::filesystem::path& appPath);
        static bool ReloadAppAssembly();

        // Script class management
        static bool LoadScriptClasses();
        static bool EntityClassExists(const std::string& className);

        // Runtime management
        static void OnRuntimeStart(Scene* scene);
        static void OnRuntimeStop();

        // Entity lifecycle management
        static void OnCreateEntity(ScriptComponentCSharp* scriptComponent, PIX3D::UUID uuid);
        static void OnUpdateEntity(ScriptComponentCSharp* scriptComponent, PIX3D::UUID uuid, float dt);
        static void OnDestroyEntity(ScriptComponentCSharp* scriptComponent, PIX3D::UUID uuid);

        // Getters
        static inline Scene* GetScene() { return s_Scene; }
        static inline MonoImage* GetCoreImage() { return mono_assembly_get_image(s_CoreAssembly); }
        static inline MonoImage* GetAppImage() { return mono_assembly_get_image(s_AppAssembly); }
        static inline MonoClass* GetEntityClass() { return s_EntityClass; }

    private:
        // Mono runtime
        inline static MonoDomain* s_RootDomain;
        inline static MonoDomain* s_AppDomain;
        inline static MonoAssembly* s_CoreAssembly;
        inline static MonoAssembly* s_AppAssembly;
        inline static MonoClass* s_EntityClass;

        // Scene management
        inline static Scene* s_Scene;

        // Assembly paths
        inline static std::filesystem::path s_CoreAssemblyPath;
        inline static std::filesystem::path s_AppAssemblyPath;

        // Script class registry
        inline static std::unordered_map<std::string, bool> s_ScriptClasses; // className -> isLoaded

        friend class ScriptInstance;
    };
}
