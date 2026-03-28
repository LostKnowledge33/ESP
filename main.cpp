#define NOMINMAX
#include "includes.h"
#include "Rendering.hpp"
#include "Font.h"
#include "sdk.h"
#include "Menu.h"
#include "Functions.h"
#include "kiero/minhook/include/MinHook.h"
#include "il2cpp_resolver.hpp"
#include "Lists.hpp"
#include "Callback.hpp"
#include <intrin.h>
#include <Utils/VFunc.hpp>
#include <iostream>
#include <PaternScan.hpp>
#include <vector>
#include <atomic>
#include <thread>
#include <cstdint>
#include <string>
#include <codecvt>
#include <locale>
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <chrono>
#include <future>

#include <DirectXTex.h>
#pragma comment(lib, "DirectXTex.lib")

#define DEBUG

extern bool LoadTexturesFromArchive(ID3D11Device* device, const std::wstring& archivePath, std::vector<void*>& outSRVs_Char, std::vector<void*>& outSRVs_Mon, std::vector<void*>& outSRVs_Map);
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Cache line alignment
alignas(64) std::atomic<bool> g_SpinLock{false};

Present oPresent;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;

// Cache variables
static thread_local void* tls_ThreadHandle = nullptr;
static ImDrawList* g_BackgroundDrawList = nullptr;
static ImDrawList* g_ForegroundDrawList = nullptr;
static bool init = false;
static Blis_Client_UIMapDefault_o* GameMap = nullptr;

// Structure definitions
struct InputVector3 
{
    float x, y, z;
    InputVector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    InputVector3(Unity::Vector3 input) : x(input.x), y(input.y), z(input.z) {}
};

struct InputVector4 
{
    float uv1, uv2, uv3, uv4;
};

struct OutputVector2 
{
    float x, y;
};

// Type definitions
typedef void (*LocalMonsterCtor)(void*);
typedef void (*LocalSpawnScratchCtor)(void*);
typedef void (*LocalSpawnScratchGroupCtor)(void*);
typedef void (*UIMapDefaultCtor)(void*);
typedef void (*UIMapIconPlayerCtor)(void*);
typedef void (*GameUICtor)(void*);
typedef void (*MinimapUICtor)(void*);
typedef void (*LocalTouringObjectCtor)(void*);
typedef void (*ItemBoxWindowCtor)(void*);
typedef void (*ItemAutoLootingManagerCtor)(void*);
typedef void (*OpenBoxCtor)(void*);
typedef void (*ItemBoxStoreCtor)(void*);
typedef void (*LocalSummonCameraCtor)(void*);
typedef void (*BaseSightAgentCtor)(void*);

typedef void (*OnChangeCurrentArea)(void*, int32_t, int32_t);
typedef void (*ShowUI)(void*, bool);
typedef void (*ExecuteAutoItemLooting)(void*);
typedef void (*OnOpen)(void*);
typedef void (*StartAutoLooting)(void*, bool); 
typedef void (*UpdateBoxUI)(void*);
typedef void (*OnItemBoxStoreUpdate)(void*, Blis_Client_UI_ItemBoxStore_o*);
typedef void (*ActionHandle)(void*, Blis_Client_UI_UIAction_o*, void*);
typedef void (*StartActionCasting)(void*, int32_t, float, float, int32_t, int32_t, int32_t);
typedef bool (*IsViewableCharacter)(void*);
typedef bool (*IsInSight)(void*);

// Function pointer definitions
using FuncGetMemberValue_int_t = int (*)(void*);
using FuncGetMemberValue_bool_t = bool (*)(void*);
using FuncMapIconPlayer_t = UnityEngine_Sprite_o* (__cdecl*)(Blis_Client_LocalPlayerCharacter_o*, void*);
using FuncMapIconLumi_t = UnityEngine_Sprite_o * (__cdecl*)(Blis_Client_LocalTouringObject_o*, void*);
using FuncGetTexture_t = UnityEngine_Texture2D_o* (__cdecl*)(UnityEngine_Sprite_o*);
using FuncGetNativeTexture_t = intptr_t(__cdecl*)(UnityEngine_Texture2D_o*, void*);
//using FuncGetNativeTexturePtr_t = intptr_t(__fastcall*)(UnityEngine::Texture_o* _this, void* methodInfo);
using FuncGetSpriteTextureRect_t = RectF(__cdecl*)(UnityEngine_Sprite_o*);
using FuncGetTextureWidth_t = int(__cdecl*)(UnityEngine_Texture2D_o*);
using FuncGetTextureHeight_t = int(__cdecl*)(UnityEngine_Texture2D_o*);
using FuncGetInnerUVs_Injected_t = void(__cdecl*)(UnityEngine_Sprite_o*, InputVector4*, void*);
using Transform_get_position_t = Unity::Vector3(__cdecl*)(void*);
using FuncGetCharacterSpriteSuffix_t = Il2CppString* (__cdecl*)(void*, int, int);
using FuncGetCharacterSpriteName_t = Il2CppString* (__cdecl*)(void*, Il2CppString*, int32_t, int32_t, int32_t);
using FuncGetCharacterSprite_t = UnityEngine_Sprite_o* (__cdecl*)(int32_t, int32_t);
using FuncGetHp_t = int (*)(void*);
using FuncGetMaxHp_t = int (*)(void*);
using FuncGetAdditionalMaxHp_t = int (*)(void*);
using FuncGetRemainTime_t = float (*)(void*);
using FuncWorldPosToUIPos_t = OutputVector2(__cdecl*)(Blis_Client_UIMapDefault_o*, InputVector3);
using FuncTryGetSpawnCharacter_t = bool(__cdecl*)(Blis_Client_LocalSpawnScratch_o* __this, Blis_Client_LocalCharacter_o** result);
using FuncGetAreaCode_t = int (__cdecl*)(Blis_Client_LocalMonster_o* __this);
using FuncIsInSafeZone_t = bool (__cdecl*)(Blis_Client_ClientService_o* __this, InputVector3, int32_t, void*);
using FuncTryFindPlayerIcon_t = bool (__cdecl*)(Blis_Client_UIMapDefault_o*, int32_t, Blis_Client_UIMapIconPlayer_o**, void*);
using FuncGetRect_t = UnityEngine_Rect_o (__cdecl*)(UnityEngine_Sprite_o*, void*);
using FuncGetGameObject_t = UnityEngine_GameObject_o* (__cdecl*)(UnityEngine_Component_o*, void*);
using FuncGetComponent_t = Il2CppObject* (__cdecl*)(UnityEngine_GameObject_o*, Il2CppObject*, void*);
using FuncGetUIMap_t = Blis_Client_UIMap_o* (__cdecl*)(Blis_Client_MinimapUI_o*, void*);
using FuncCheckDyingCondition_t = bool (__cdecl*)(Blis_Client_LocalPlayerCharacter_o*);
using FuncGetCurrentAreaCode_t = int (__cdecl*)(Blis_Client_LocalObject_o*);
using FuncGetPlayerCharacterName_t = Il2CppString* (__cdecl*)(Blis_Client_ClientService_o*, int32_t);
using FuncFindBoxSlot_t = Blis_Client_BoxItemSlot_o* (__cdecl*)(Blis_Client_ItemBoxWindow_o*, int32_t, void*);
using FuncOnSlotLeftClick_t = void(__cdecl*)(Blis_Client_ItemBoxWindow_o*, Il2CppObject*, void*);
using FuncGetConsumableType_t = int32_t(__cdecl*)(Blis_Common_ItemData_o*);
using FuncGetSightRange_t = float(__cdecl*)(void*);
//using FuncGetName_t = Il2CppString * (__cdecl*)(void*);
using FuncGetName_t = System_String_o * (__cdecl*)(void*);


// Global function pointers
static FuncGetMemberValue_int_t GetCharCodeFunc = nullptr;
static FuncGetMemberValue_bool_t GetAliveFunc = nullptr;
static FuncMapIconPlayer_t GetMapIconPlayerFunc = nullptr;
static FuncMapIconLumi_t GetMapIconLumiFunc = nullptr;
static FuncGetTexture_t GetTextureFunc = nullptr;
static FuncGetNativeTexture_t GetNativeTextureFunc = nullptr;
//static FuncGetNativeTexturePtr_t GetNativeTextureFunc = nullptr;
static FuncGetSpriteTextureRect_t GetSpriteTextureRectFunc = nullptr; // get_textureRect
static FuncGetTextureWidth_t GetTextureWidthFunc = nullptr; // Texture2D.get_width
static FuncGetTextureHeight_t GetTextureHeightFunc = nullptr; // Texture2D.get_height
static FuncGetInnerUVs_Injected_t GetInnerUVs_InjectedFunc = nullptr;
static Transform_get_position_t GetPosition = nullptr;
static FuncGetCharacterSpriteSuffix_t GetCharacterSpriteSuffixFunc = nullptr;
static FuncGetCharacterSpriteName_t GetCharacterSpriteNameFunc = nullptr;
static FuncGetCharacterSprite_t GetCharacterSpriteFunc = nullptr;
static FuncGetHp_t GetHpFunc = nullptr;
static FuncGetMaxHp_t GetMaxHpFunc = nullptr;
static FuncGetAdditionalMaxHp_t GetAdditionalMaxHpFunc = nullptr;
static FuncGetRemainTime_t GetRemainTimeFunc = nullptr;
static FuncWorldPosToUIPos_t GetWorldPosToUIPosFunc = nullptr;
static FuncTryGetSpawnCharacter_t TryGetSpawnCharacterFunc = nullptr;
static FuncGetAreaCode_t GetAreaCodeFunc = nullptr;
static FuncIsInSafeZone_t IsInSafeZoneFunc = nullptr;
static FuncTryFindPlayerIcon_t TryFindPlayerIconFunc = nullptr;
static FuncGetRect_t GetRectFunc = nullptr;
static FuncGetGameObject_t GetGameObjectFunc = nullptr;
static FuncGetComponent_t GetComponentFunc = nullptr;
static FuncGetUIMap_t GetUIMapFunc = nullptr;
static FuncCheckDyingCondition_t CheckDyingConditionFunc = nullptr;
static FuncGetCurrentAreaCode_t GetCurrentAreaCodeFunc = nullptr;
static FuncGetPlayerCharacterName_t GetPlayerCharacterNameFunc = nullptr;
static FuncFindBoxSlot_t GetFuncFindBoxSlotFunc = nullptr;
static FuncOnSlotLeftClick_t OnSlotLeftClickFunc = nullptr;
static FuncGetConsumableType_t GetConsumableTypeFunc = nullptr;
static FuncGetSightRange_t GetSightRangeFunc = nullptr;
static FuncGetName_t GetNameFunc = nullptr;

// Original function pointers
static LocalMonsterCtor orig_LocalMonsterctor = nullptr;
static LocalSpawnScratchCtor orig_LocalSpawnScratchCtor = nullptr;
static LocalSpawnScratchGroupCtor orig_LocalSpawnScratchGroupCtor = nullptr;
static UIMapDefaultCtor orig_UIMapDefaultCtor = nullptr;
static UIMapIconPlayerCtor orig_UIMapIconPlayerCtor = nullptr;
static GameUICtor orig_GameUICtor = nullptr;
static MinimapUICtor orig_MinimapUICtor = nullptr;
static LocalTouringObjectCtor orig_LocalTouringObjectCtor = nullptr;
static ItemBoxWindowCtor orig_ItemBoxWindowCtor = nullptr;
static ItemAutoLootingManagerCtor orig_ItemAutoLootingManagerCtor = nullptr;
static OpenBoxCtor orig_OpenBoxCtor = nullptr;
static ItemBoxStoreCtor orig_ItemBoxStoreCtor = nullptr;
static LocalSummonCameraCtor orig_LocalSummonCameraCtor = nullptr;

static OnChangeCurrentArea orig_OnChangeCurrentArea = nullptr;
static ShowUI orig_ShowUI = nullptr;
static ExecuteAutoItemLooting orig_ExecuteAutoItemLooting = nullptr;
static OnOpen orig_OnOpen = nullptr;
static StartAutoLooting orig_StartAutoLooting = nullptr;
static UpdateBoxUI orig_UpdateBoxUI = nullptr;
static OnItemBoxStoreUpdate orig_OnItemBoxStoreUpdate = nullptr;
static ActionHandle orig_ActionHandle = nullptr;
static StartActionCasting orig_StartActionCasting = nullptr;
static IsViewableCharacter orig_IsViewableCharacter = nullptr;
static IsInSight orig_IsInSight = nullptr;
static BaseSightAgentCtor orig_BaseSightAgentCtor = nullptr;

// UTF8 conversion cache
static std::unordered_map<const char*, std::string> g_UTF8Cache;

// Fast lock implementation
inline void FastLock() 
{
    bool expected = false;
    while (!g_SpinLock.compare_exchange_weak(expected, true, std::memory_order_acquire)) {
        expected = false;
        _mm_pause();
    }
}

inline void FastUnlock() 
{
    g_SpinLock.store(false, std::memory_order_release);
}

// Vector conversion optimization
inline Vector3 ChangeVectorType(const Unity::Vector3& input) {
    return *reinterpret_cast<const Vector3*>(&input);
}

inline Unity::Vector3 ChangeVectorType(const Vector3& input) {
    return *reinterpret_cast<const Unity::Vector3*>(&input);
}

inline Vector2 ChangeVectorType(const Unity::Vector2& input) {
    return *reinterpret_cast<const Vector2*>(&input);
}

inline Unity::Vector2 ChangeVectorType(const Vector2& input) {
    return *reinterpret_cast<const Unity::Vector2*>(&input);
}


// Hook functions
void __fastcall Hooked_LocalMonster_Ctor(void* thisPtr) 
{
    if (orig_LocalMonsterctor) orig_LocalMonsterctor(thisPtr);
    MonsterObjects.emplace_back(thisPtr);
}

void __fastcall Hooked_LocalSpawnScratch_Ctor(void* thisPtr) 
{
    if (orig_LocalSpawnScratchCtor) orig_LocalSpawnScratchCtor(thisPtr);
    SpawnScratchObjects.emplace_back(thisPtr);
}

void __fastcall Hooked_LocalSpawnScratchGroup_Ctor(void* thisPtr) 
{
    if (orig_LocalSpawnScratchGroupCtor) orig_LocalSpawnScratchGroupCtor(thisPtr);
    SpawnScratchGroupObjects.emplace_back(thisPtr);
}

void __fastcall Hooked_UIMapDefault_Ctor(void* thisPtr) 
{
    if (orig_UIMapDefaultCtor) orig_UIMapDefaultCtor(thisPtr);
    UIMap.emplace_back(thisPtr);
}

void __fastcall Hooked_UIMapIconPlayer_Ctor(void* thisPtr) 
{
    if (orig_UIMapIconPlayerCtor) orig_UIMapIconPlayerCtor(thisPtr);
    PlayerIcon.emplace_back(thisPtr);
}

void __fastcall Hooked_GameUI_Ctor(void* thisPtr) 
{
    if (orig_GameUICtor) orig_GameUICtor(thisPtr);
    GameUI.emplace_back(thisPtr);
}

void __fastcall Hooked_MinimapUI_Ctor(void* thisPtr)
{
    if (orig_MinimapUICtor) orig_MinimapUICtor(thisPtr);
    MiniMapUI.emplace_back(thisPtr);
}

void __fastcall Hooked_LocalTouringObject_Ctor(void* thisPtr)
{
    if (orig_LocalTouringObjectCtor) orig_LocalTouringObjectCtor(thisPtr);
    
    TouringObjects.emplace_back(thisPtr);
}

void __fastcall Hooked_ItemBoxStoreCtor(void* thisPtr)
{
    if (orig_ItemBoxStoreCtor) orig_ItemBoxStoreCtor(thisPtr);

    printf("ItemBoxStore Ctor Called \n");
    ItemBoxStores.emplace_back(thisPtr);
}

void __fastcall Hooked_ItemBoxWindowCtor(void* thisPtr)
{
    if (orig_ItemBoxWindowCtor) orig_ItemBoxWindowCtor(thisPtr);

    ItemBoxWindows.emplace_back(thisPtr);
    printf("ItemBoxWindow Ctor Called \n");
}

void __fastcall Hooked_LocalSummonCameraCtor(void* thisPtr)
{
    printf("LocalSummonBase Ctor Called \n");
    if (orig_LocalSummonCameraCtor) orig_LocalSummonCameraCtor(thisPtr);

    Blis_Client_LocalSummonCamera_o* Object = static_cast<Blis_Client_LocalSummonCamera_o*>(thisPtr);
    Blis_Common_SummonData_o* SummonData = Object->fields.summonData;
    
    printf("Summon Object %d \n", SummonData->fields.objectType);


	FastLock();
	SummonCameras.emplace_back(thisPtr);
	FastUnlock();
}

void __fastcall Hooked_StartActionCasting(void* thisPtr, int32_t type, float time1, float time2, int32_t param, int32_t id, int32_t area)
{
    printf("%d, %f, %f, %d, %d, %d \n", type, time1, time2, param, id, area);

    if (orig_StartActionCasting) orig_StartActionCasting(thisPtr, type, time1, time2, param, id, area);
}

void __fastcall Hooked_IsViewableCharacter(void* thisPtr)
{
    printf("IsViewableCharacter Called \n");
    if (orig_IsViewableCharacter) orig_IsViewableCharacter(thisPtr);
}

bool __fastcall Hooked_IsInSight(void* thisPtr)
{
    printf("IsInSight Called \n");

    return true;
    //if (orig_IsInSight) orig_IsInSight(thisPtr);
}

void __fastcall Hooked_BaseSightAgent_Ctor(void* thisPtr)
{
    printf("BaseSightAgent Ctor Called \n");
    if (orig_BaseSightAgentCtor) orig_BaseSightAgentCtor(thisPtr);
    //BaseSightAgents.emplace_back(thisPtr);
}

// Hook installation
inline bool InstallHook(void** target, void* detour, void** original) 
{
    if (!target || !detour) return false;
    
    DWORD oldProtect;
    if (!VirtualProtect(target, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect))
        return false;
    
    if (original) *original = *target;
    *target = detour;
    
    VirtualProtect(target, sizeof(void*), oldProtect, &oldProtect);
    return true;
}

// Console creation
inline void CreateConsole()
{
#ifdef DEBUG
    AllocConsole();
    AttachConsole(GetCurrentProcessId());
    SetConsoleTitle("IL2CPP");
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
#endif
}

// Initialization
inline void initvars() {
    if (!IL2CPP::Initialize(true)) {
        printf("[ DEBUG ] Il2Cpp initialize failed, quitting...");
        Sleep(300);
        exit(0);
    }

    sdk::Base = (uintptr_t)GetModuleHandleA(NULL);
    sdk::GameAssembly = (uintptr_t)GetModuleHandleA("GameAssembly.dll");
    sdk::UnityPlayer = (uintptr_t)GetModuleHandleA("UnityPlayer.dll");

#ifdef DEBUG
    printf("[ DEBUG ] Il2Cpp initialized\n");
    printf("[ DEBUG ] Base: 0x%llX | GameAssembly: 0x%llX | UnityPlayer: 0x%llX\n",
        sdk::Base, sdk::GameAssembly, sdk::UnityPlayer);
#endif
}

// Function signature finding
bool find_sigs() 
{
    printf("===============  Find Method Pointer Start  =============== \n");

    struct MethodSig 
    {
        void** outFunc;
        const char* className;
        const char* methodName;
        int paramCount;
    };

    MethodSig sigs[] = 
    {
        { (void**)&GetAliveFunc, "Blis.Client.LocalCharacter", "get_IsAlive", 0 },
        { (void**)&GetCharCodeFunc, "Blis.Client.LocalPlayerCharacter", "GetCharacterCode", 0 },
        { (void**)&GetPosition, "Blis.Client.LocalObject", "get_position", 0 },
        { (void**)&GetMapIconPlayerFunc, "Blis.Client.LocalPlayerCharacter", "GetMapIcon", 0 },
        { (void**)&GetMapIconLumiFunc, "Blis.Client.LocalTouringObject", "GetMapIcon", 0 },
        { (void**)&GetTextureFunc, "UnityEngine.Sprite", "get_texture", 0 },
        { (void**)&GetNativeTextureFunc, "UnityEngine.Texture", "GetNativeTexturePtr", 0 },
        { (void**)&GetSpriteTextureRectFunc, "UnityEngine.Sprite", "get_textureRect", 0 },
        { (void**)&GetTextureWidthFunc, "UnityEngine.Texture", "get_width", 0 },
        { (void**)&GetTextureHeightFunc, "UnityEngine.Texture", "get_height", 0 },
        { (void**)&GetInnerUVs_InjectedFunc, "UnityEngine.Sprite", "GetInnerUVs_Injected", 1 },
        { (void**)&GetCharacterSpriteSuffixFunc, "Blis.Common.ResourceManager", "GetCharacterSpriteSuffix", 2 },
        { (void**)&GetCharacterSpriteNameFunc, "Blis.Common.ResourceManager", "GetCharacterSpriteName", 4 },
        { (void**)&GetHpFunc, "Blis.Common.CharacterStatus", "get_Hp", 0 },
        { (void**)&GetMaxHpFunc, "Blis.Common.CharacterStatBase", "get_MaxHp", 0 },
        { (void**)&GetAdditionalMaxHpFunc, "Blis.Common.CharacterStatBase", "get_AdditionalMaxHp", 0 },
        { (void**)&GetRemainTimeFunc, "Blis.Client.LocalSpawnScratch", "GetRemainTime", 0 },
        { (void**)&GetWorldPosToUIPosFunc, "Blis.Client.UIMapDefault", "WorldPosition2UiPosition", 1 },
        { (void**)&TryGetSpawnCharacterFunc, "Blis.Client.LocalSpawnScratch", "TryGetSpawnCharacter", 1 },
        { (void**)&GetAreaCodeFunc, "Blis.Client.LocalMonster", "GetAreaCode", 0 },
        { (void**)&IsInSafeZoneFunc, "Blis.Client.ClientService", "IsInSafeZone", 2 },
        { (void**)&TryFindPlayerIconFunc, "Blis.Client.UIMapBase", "TryFindPlayerIcon", 2 },
        { (void**)&GetRectFunc, "UnityEngine.Sprite", "get_rect", 0 },
        { (void**)&GetGameObjectFunc, "UnityEngine.Component", "get_gameObject", 0 },
        { (void**)&GetComponentFunc, "UnityEngine.GameObject", "GetComponent", 1 },
        { (void**)&GetUIMapFunc, "Blis.Client.MinimapUI", "get_UIMap", 0 },
        { (void**)&CheckDyingConditionFunc, "Blis.Client.LocalPlayerCharacter", "CheckDyingCondition", 0 },
        { (void**)&GetCurrentAreaCodeFunc, "Blis.Client.LocalObject", "GetCurrentAreaCode", 0 },
        { (void**)&GetPlayerCharacterNameFunc, "Blis.Client.ClientService", "GetPlayerCharacterName", 1 },
        { (void**)&GetFuncFindBoxSlotFunc, "Blis.Client.ItemBoxWindow", "FindBoxSlot", 1 },
        { (void**)&OnSlotLeftClickFunc, "Blis.Client.ItemBoxWindow", "OnSlotLeftClick", 1 },
        { (void**)&GetConsumableTypeFunc, "Blis.Common.ItemData", "GetConsumableType", 0 },
        { (void**)&GetSightRangeFunc, "Blis.Common.SummonData", "get_sightRange", 0 },
		{ (void**)&GetNameFunc, "UnityEngine.Object", "get_name", 0 },
    };

    for (const auto& sig : sigs) 
    {
        void* ptr = nullptr;
        if (sig.paramCount > 0)
            ptr = IL2CPP::Class::Utils::GetMethodPointer(sig.className, sig.methodName, sig.paramCount);
        else
            ptr = IL2CPP::Class::Utils::GetMethodPointer(sig.className, sig.methodName);

		if (!ptr) 
        {
            printf("Failed to find %s::%s with %d params\n", sig.className, sig.methodName, sig.paramCount);
            return false;
		}
        
        *sig.outFunc = ptr;
    }

    return true;
}

void Find_MethodInfo()
{
    // Get MethodInfo
    printf("===============  Find MethodInfo Start  =============== \n");
    Il2CppClass* klass = reinterpret_cast<Il2CppClass*>(IL2CPP::Class::FindRaw("UnityEngine.Sprite"));
    if (klass)
    {
        printf("Find Class Name : %s \n", klass->_1.name);
        GetInnerUVs_Injected_MethodInfo = reinterpret_cast<MethodInfo*>(IL2CPP::Class::Utils::GetMethodRaw(klass, "GetInnerUVs_Injected", 1));
    }

    if (klass)
    {
        printf("Find Class Name : %s \n", klass->_1.name);
        GetRect_MethodInfo = reinterpret_cast<MethodInfo*>(IL2CPP::Class::Utils::GetMethodRaw(klass, "get_rect"));
    }

    klass = reinterpret_cast<Il2CppClass*>(IL2CPP::Class::FindRaw("Blis.Client.UIMapBase"));
    if (klass)
    {
        printf("Find Class Name : %s \n", klass->_1.name);
        TryFindPlayerIcon_MethodInfo = reinterpret_cast<MethodInfo*>(IL2CPP::Class::Utils::GetMethodRaw(klass, "TryFindPlayerIcon", 2));
    }

    klass = reinterpret_cast<Il2CppClass*>(IL2CPP::Class::FindRaw("UnityEngine.Component"));
    if (klass)
    {
        printf("Find Class Name : %s \n", klass->_1.name);
        GetGameObject_MethodInfo = reinterpret_cast<MethodInfo*>(IL2CPP::Class::Utils::GetMethodRaw(klass, "get_gameObject"));
    }

    klass = reinterpret_cast<Il2CppClass*>(IL2CPP::Class::FindRaw("UnityEngine.GameObject"));
    if (klass)
    {
        printf("Find Class Name : %s \n", klass->_1.name);
        GetComponent_MethodInfo = reinterpret_cast<MethodInfo*>(IL2CPP::Class::Utils::GetMethodRaw(klass, "GetComponent", 1));
    }

    klass = reinterpret_cast<Il2CppClass*>(IL2CPP::Class::FindRaw("Blis.Client.MinimapUI"));
    if (klass)
    {
        printf("Find Class Name : %s \n", klass->_1.name);
        GetUIMap_MethodInfo = reinterpret_cast<MethodInfo*>(IL2CPP::Class::Utils::GetMethodRaw(klass, "get_UIMap"));
    }

    klass = reinterpret_cast<Il2CppClass*>(IL2CPP::Class::FindRaw("Blis.Client.ClientService"));
    if (klass)
    {
        printf("Find Class Name : %s \n", klass->_1.name);
        IsInSafeZone_MethodInfo = reinterpret_cast<MethodInfo*>(IL2CPP::Class::Utils::GetMethodRaw(klass, "IsInSafeZone", 2));
    }

    klass = reinterpret_cast<Il2CppClass*>(IL2CPP::Class::FindRaw("Blis.Client.LocalPlayerCharacter"));
    if (klass)
    {
        printf("Find Class Name : %s \n", klass->_1.name);
        GetMapIconPlayer_MethodInfo = reinterpret_cast<MethodInfo*>(IL2CPP::Class::Utils::GetMethodRaw(klass, "GetMapIcon"));
    }

    klass = reinterpret_cast<Il2CppClass*>(IL2CPP::Class::FindRaw("Blis.Client.LocalTouringObject"));
    if (klass)
    {
        printf("Find Class Name : %s \n", klass->_1.name);
        GetMapIconLumi_MethodInfo = reinterpret_cast<MethodInfo*>(IL2CPP::Class::Utils::GetMethodRaw(klass, "GetMapIcon"));
    }

    klass = reinterpret_cast<Il2CppClass*>(IL2CPP::Class::FindRaw("Blis.Client.ItemBoxWindow"));
    if (klass)
    {
        printf("Find Class Name : %s \n", klass->_1.name);
        FindBoxSlot_MethodInfo = reinterpret_cast<MethodInfo*>(IL2CPP::Class::Utils::GetMethodRaw(klass, "FindBoxSlot", 1));
        OnSlotLeftClick_MethodInfo = reinterpret_cast<MethodInfo*>(IL2CPP::Class::Utils::GetMethodRaw(klass, "OnSlotLeftClick", 1));
    }
}

// Hook activation
void EnableHooks() 
{
    printf("===============  Hooking Start  =============== \n");

    struct HookMethodInfo 
    {
        const char* ClassName;
        const char* MethodName;
        void* HookMethod;
        void** OrigMethod;
        int ParamCount = 0;
    };

    std::vector<HookMethodInfo> HookList = 
    {
        { "Blis.Client.LocalMonster", ".ctor", (void*)&Hooked_LocalMonster_Ctor, (void**)&orig_LocalMonsterctor },
        { "Blis.Client.LocalSpawnScratch", ".ctor", (void*)&Hooked_LocalSpawnScratch_Ctor, (void**)&orig_LocalSpawnScratchCtor },
        { "Blis.Client.LocalSpawnScratchGroup", ".ctor", (void*)&Hooked_LocalSpawnScratchGroup_Ctor, (void**)&orig_LocalSpawnScratchGroupCtor },
        { "Blis.Client.GameUI", ".ctor", (void*)&Hooked_GameUI_Ctor, (void**)&orig_GameUICtor },
        { "Blis.Client.UIMapDefault", ".ctor", (void*)&Hooked_UIMapDefault_Ctor, (void**)&orig_UIMapDefaultCtor },
        { "Blis.Client.UIMapIconPlayer", ".ctor", (void*)&Hooked_UIMapIconPlayer_Ctor, (void**)&orig_UIMapIconPlayerCtor },
        { "Blis.Client.MinimapUI", ".ctor", (void*)&Hooked_MinimapUI_Ctor, (void**)&orig_MinimapUICtor },
        { "Blis.Client.UI.ItemBoxStore", ".ctor", (void*)&Hooked_ItemBoxStoreCtor, (void**)&orig_ItemBoxStoreCtor },
        { "Blis.Client.ItemBoxWindow", ".ctor", (void*)&Hooked_ItemBoxWindowCtor, (void**)&orig_ItemBoxWindowCtor },
        { "Blis.Client.LocalTouringObject", ".ctor", (void*)&Hooked_LocalTouringObject_Ctor, (void**)&orig_LocalTouringObjectCtor },
        { "Blis.Client.LocalCastingAgent", "StartActionCasting", (void*)&Hooked_StartActionCasting, (void**)&orig_StartActionCasting, 6 },
        { "Blis.Client.LocalMovableCharacter", "IsViewableCharacter", (void*)&Hooked_IsViewableCharacter, (void**)&orig_IsViewableCharacter },
		{ "Blis.Client.LocalSummonCamera", ".ctor", (void*)&Hooked_LocalSummonCameraCtor, (void**)&orig_LocalSummonCameraCtor },
		{ "Blis.Client.LocalCharacter", "IsInSightCharacter", (void*)&Hooked_IsInSight, (void**)&orig_IsInSight },
		{ "Blis.Common.BaseSightAgent", ".ctor", (void*)&Hooked_BaseSightAgent_Ctor, (void**)&orig_BaseSightAgentCtor },
    };

    for (const auto& hook : HookList) 
    {
        Il2CppClass* klass = reinterpret_cast<Il2CppClass*>(IL2CPP::Class::FindRaw(hook.ClassName));
        if (klass) 
        {
            MethodInfo* mtInfo = nullptr;
            if (strcmp(hook.MethodName, ".ctor") == 0) 
                mtInfo = reinterpret_cast<MethodInfo*>(IL2CPP::Class::Utils::GetMethodRaw(klass, hook.MethodName));
            else if (hook.ParamCount > 0) 
                mtInfo = reinterpret_cast<MethodInfo*>(IL2CPP::Class::Utils::GetMethodRaw(klass, hook.MethodName, hook.ParamCount));
            else 
                mtInfo = reinterpret_cast<MethodInfo*>(IL2CPP::Class::Utils::GetMethodRaw(klass, hook.MethodName));
            
            if (mtInfo && InstallHook((void**)&mtInfo->methodPointer, hook.HookMethod, hook.OrigMethod)) 
                printf("%s %s Hook installed! \n", klass->_1.name, hook.MethodName);
        } 
        else 
            printf("Can't Find Class Name : %s \n", hook.ClassName);
    }
}

// ImGui initialization
void InitImGui() 
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(pDevice, pContext);
}

// Window procedure
LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return true;
    return CallWindowProcA(oWndProc, hWnd, uMsg, wParam, lParam);
}

inline void draw_health_circle(const ImVec2& position, float health, float max_health, float radius)
{
    if (max_health <= 0) return;

    float healthPercent = (health / max_health) + 1.0f;
    float v1 = health / max_health;
    float difference = v1 - 1.0f;

    g_ForegroundDrawList->PathArcTo(position, radius, PI, PI * healthPercent, 32);
    g_ForegroundDrawList->PathStroke(ImGui::ColorConvertFloat4ToU32({ fabs(difference), v1, 0.0f, 1.0f }), false, 4.0f);
}

void Looting_Thread()
{
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    void* cachedThread = nullptr;
    static Blis_Client_ClientService_o* m_CService = nullptr;

    while (true)
    {
        if (!vars::initil2cpp)
        {
            Sleep(500);
            continue;
        }

        if (!cachedThread)
        {
            cachedThread = IL2CPP::Thread::Attach(IL2CPP::Domain::Get());
        }

        if (!vars::SpeedLooting) continue;

        if (!ItemBoxStore)
        {
            if (!ItemBoxStores.empty())
                ItemBoxStore = reinterpret_cast<Blis_Client_UI_ItemBoxStore_o*>(ItemBoxStores[0]);

            if (!ItemBoxStore) continue;
        }

        if (!ItemBoxWindow)
        {
            if (ItemBoxWindows.size() > 1)
                ItemBoxWindow = reinterpret_cast<Blis_Client_ItemBoxWindow_o*>(ItemBoxWindows[1]);

            if (!ItemBoxWindow) continue;
        }

        if (vars::SpeedLooting && ItemBoxStore && ItemBoxWindow)
        {
            if (ItemBoxStore->fields._BoxId_k__BackingField && ItemBoxWindow->fields._isOpen)
            {
                if (ItemBoxStore->fields._BoxWindowType_k__BackingField != 3 && !vars::BoxParsingLimit)
                {
                    std::vector<Il2CppObject*> items;
                    if (ParseList(ItemBoxStore->fields.boxItems, items))
                    {
                        for (int i = 0; i < items.size(); i++)
                        {
                            auto* item = reinterpret_cast<Blis_Common_Item_o*>(items[i]);
                            if (item == nullptr) continue;
                            auto* itemData = item->fields._ItemData_k__BackingField;
                            if (itemData == nullptr) continue;
                            if (itemData->fields.itemType >= 3 && itemData->fields.itemGrade >= 3)
                            {
                                Blis_Client_BoxItemSlot_o* ItemSlot = nullptr;
                                if (itemData->fields.itemType == 5)
                                {
                                    int conType = GetConsumableTypeFunc(itemData);
                                    if (conType == 3 || conType == 6 || item->fields.itemCode == 301407)
                                    {
                                        Sleep(vars::SpeedLootDelay);
                                        while (1)
                                        {
                                            ItemSlot = GetFuncFindBoxSlotFunc(ItemBoxWindow, item->fields.id, FindBoxSlot_MethodInfo);
                                            if (ItemSlot != nullptr) break;
                                        }
                                        OnSlotLeftClickFunc(ItemBoxWindow, reinterpret_cast<Il2CppObject*>(ItemSlot), OnSlotLeftClick_MethodInfo);
                                    }
                                }
                                else
                                {
                                    Sleep(vars::SpeedLootDelay);
                                    while (1)
                                    {
                                        ItemSlot = GetFuncFindBoxSlotFunc(ItemBoxWindow, item->fields.id, FindBoxSlot_MethodInfo);
                                        if (ItemSlot != nullptr) break;
                                    }
                                    OnSlotLeftClickFunc(ItemBoxWindow, reinterpret_cast<Il2CppObject*>(ItemSlot), OnSlotLeftClick_MethodInfo);
                                }
                            }
                            vars::BoxParsingLimit = true;
                        }
                    }
                }
            }
            else
            {
                vars::BoxParsingLimit = false;
            }
        }
        std::this_thread::yield();
        //Sleep(1);
    }
}

// Player Cache optimization
void Player_Cache()
{
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    bool GameEndCheck = false;
    void* cachedThread = nullptr;

    while (true)
    {
        if (!vars::initil2cpp)
        {
            Sleep(500);
            continue;
        }

        auto start = std::chrono::high_resolution_clock::now();
        if (!cachedThread)
        {
            cachedThread = IL2CPP::Thread::Attach(IL2CPP::Domain::Get());
        }

        static Il2CppClass* cachedKlass = nullptr;
        if (GameEndCheck)
        {
            PlayerList.clear();
            MonsterList.clear();
            MonsterObjects.clear();
            SpawnScratchObjects.clear();
            SpawnScratchGroupObjects.clear();
            MonsterUIPos.clear();
            UIMap.clear();
            GameUI.clear();
            ItemBoxWindows.clear();
			TouringObjects.clear();
           
            DrawPlayerList.clear();
            DrawMonsterList.clear();
            DrawMonsterUIPos.clear();

            GameMap = nullptr;
            CService = nullptr;
            IALManager = nullptr;
            ItemBoxWindow = nullptr;
            Lumi = nullptr;
            
            vars::SpeedLooting = false;
            vars::PlayerImageCached = false;
            vars::MonsterSnaplines = false;
            vars::PlayerSnaplines = false;
            vars::ShowMap = false;
            vars::GameStarted = false;
            vars::GameEnded = false;

            GameEndCheck = false;
        }

        {
            if (!CService)
            {
                cachedKlass = reinterpret_cast<Il2CppClass*>(IL2CPP::Class::FindRaw("Blis.Client.ClientService"));
                if (!cachedKlass) goto continue_label;

                Il2CppClass* parentClass = cachedKlass->_1.parent;
                if (!parentClass) goto continue_label;

                auto* staticFields = reinterpret_cast<Blis_Common_Utils_MonoBehaviourInstance_ClientService__StaticFields*>(parentClass->static_fields);
                if (!staticFields) goto continue_label;

                CService = staticFields->_instance_k__BackingField;
                if (!CService) goto continue_label;
            }
            
            GameEndCheck = CService->fields.isGameEnd;
            vars::GameStarted = CService->fields.isGameStarted;
            if (GameEndCheck || !vars::GameStarted) goto continue_label;

            if (!IALManager)
            {
                auto* autoLootManagerKlass = reinterpret_cast<Il2CppClass*>(IL2CPP::Class::FindRaw("Blis.Client.ItemAutoLootingManager"));
                if (!autoLootManagerKlass) goto continue_label;

                Il2CppClass* parentClass = autoLootManagerKlass->_1.parent;
                if (!parentClass) goto continue_label;

                auto* staticFields2 = reinterpret_cast<Blis_Common_Utils_MonoBehaviourInstance_ItemAutoLootingManager__StaticFields*>(parentClass->static_fields);
                if (!staticFields2) goto continue_label;

                IALManager = staticFields2->_instance_k__BackingField;
                if(!IALManager) goto continue_label;
            }

            if (IALManager)
            {
                if (vars::SpeedLooting)
                {
                    IALManager->fields.START_CHECK_TIMER = 0.25f;
                    IALManager->fields.TAKE_ITEM_INTERVAL = 0.04f;
                }
                else
                {
                    IALManager->fields.START_CHECK_TIMER = 0.7f;
                    IALManager->fields.TAKE_ITEM_INTERVAL = 0.1f;
                }
            }

            // Player parsing
            std::vector<Il2CppObject*> players;
            if (ParseDictionary(CService->fields.playerMap, players))
            {
                PlayerList.resize(0);
                for (auto* playerObj : players)
                {
                    auto* playerCont = reinterpret_cast<Blis_Client_PlayerContext_o*>(playerObj);
                    if (!playerCont || !playerCont->fields.playerCharacter) continue;

                    auto* player = playerCont->fields.playerCharacter;
                    if (!player->fields.status || !player->fields.stat) continue;

                    PlayerInfoForDraw tempInfo;
                    tempInfo.PlayerObject = player;
                    tempInfo.TeamNumber = player->fields.teamNumber;
                    tempInfo.TeamSlot = player->fields.teamSlot;
                    tempInfo.CharCode = player->fields.characterCode; //GetCharCodeFunc(player);
                    tempInfo.CharName = ConvertToUTF8_Cached(PlayerCharacterNames[tempInfo.CharCode]);
                    tempInfo.IsAlive = player->fields.isAlive; //GetAliveFunc(player);
                    tempInfo.playerHP = GetHpFunc(player->fields.status);
                    tempInfo.playerMaxHP = GetMaxHpFunc(player->fields.stat);
                    tempInfo.isDying = player->fields.isDyingCondition;
                    //tempInfo.AreaCode = GetCurrentAreaCodeFunc(reinterpret_cast<Blis_Client_LocalObject_o*>(player));

                    PlayerList.emplace_back(std::move(tempInfo));
                }
            }
            
            // Monster parsing
            MonsterList.resize(0);
            for (auto* monObj : MonsterObjects)
            {
                if (!monObj) continue;

                auto* localMonster = reinterpret_cast<Blis_Client_LocalMonster_o*>(monObj);
                if (!localMonster || !localMonster->fields.monsterData) continue;

                int mtype = localMonster->fields.monsterData->fields.monster;

                MonsterInfoForDraw tempInfo;
                tempInfo.MonsterObject = localMonster;
                tempInfo.DrawColor = localMonster->fields.monsterData->fields.isMutant ? ImColor(128, 0, 128) : ImColor(255, 100, 100);
                tempInfo.IsAlive = localMonster->fields.isAlive; //GetAliveFunc(localMonster);
                tempInfo.isRaven = (mtype == 18);
                //tempInfo.areacode = GetAreaCodeFunc(localMonster);
                //tempInfo.isMutant = localMonster->fields.monsterData->fields.isMutant;

                MonsterList.emplace_back(std::move(tempInfo));
            }
            
            // MonsterUI parsing
            MonsterUIPos.resize(0);
            for (auto* scratchObj : SpawnScratchObjects)
            {
                if (!scratchObj) continue;

                auto* localScratch = reinterpret_cast<Blis_Client_LocalSpawnScratch_o*>(scratchObj);
                if (!localScratch || localScratch->fields.spawnTargetObjectType != 3) continue;

                MonsterUI asd;
                asd.ScratchObject = localScratch;
                asd.remainTime = GetRemainTimeFunc(localScratch);
                asd.isAlive = false;
				asd.areaCode = GetCurrentAreaCodeFunc(reinterpret_cast<Blis_Client_LocalObject_o*>(localScratch));

                if (auto* localMonster = reinterpret_cast<Blis_Client_LocalMonster_o*>(localScratch->fields.spawnCharacter))
                {
                    if (auto* monsterData = localMonster->fields.monsterData)
                    {
                        asd.monsterType = monsterData->fields.monster;
                        asd.isAlive = localMonster->fields.isAlive;
                        asd.DrawColor = monsterData->fields.isMutant ? ImColor(0.6f, 0.0f, 0.8f, 1.0f) : ImColor(1.0f, 1.0f, 1.0f, 1.0f);
                        //asd.isMutant = monsterData->fields.isMutant;
                    }
                }

                MonsterUIPos.emplace_back(std::move(asd));
            }

            for (auto* groupObj : SpawnScratchGroupObjects)
            {
                if (!groupObj) continue;

                auto* localGroup = reinterpret_cast<Blis_Client_LocalSpawnScratchGroup_o*>(groupObj);
                if (!localGroup || localGroup->fields.spawnTargetObjectType != 39) continue;

                std::vector<Il2CppObject*> spawnVec;
                if (!ParseList(localGroup->fields.spawnCharacters, spawnVec)) continue;

                MonsterUI asd;
                asd.ScratchObject = localGroup;
                asd.remainTime = GetRemainTimeFunc(localGroup);
                asd.isAlive = false;
                asd.areaCode = GetCurrentAreaCodeFunc(reinterpret_cast<Blis_Client_LocalObject_o*>(localGroup));

                if (!spawnVec.empty())
                {
                    if (auto* localMonster = reinterpret_cast<Blis_Client_LocalMonster_o*>(spawnVec[0]))
                    {
                        if (auto* monsterData = localMonster->fields.monsterData)
                        {
                            asd.monsterType = monsterData->fields.monster;
                            asd.isAlive = localMonster->fields.isAlive;
                            asd.DrawColor = monsterData->fields.isMutant ? ImColor(0.6f, 0.0f, 0.8f, 1.0f) : ImColor(1.0f, 1.0f, 1.0f, 1.0f);
                        }
                    }
                }

                MonsterUIPos.emplace_back(std::move(asd));
            }

            if (Lumi == nullptr)
            {
                for (auto* TouringObject : TouringObjects)
                {
                    if (!TouringObject) continue;

                    auto* touringObj = reinterpret_cast<Blis_Client_LocalTouringObject_o*>(TouringObject);
                    if (!touringObj) continue;
                    auto* data = touringObj->fields.touringObjectData;
                    if (!data) continue;
                    if (data->fields.touringObjectType == 3)
                    {
                        Lumi = TouringObject;
                        break;
                    }
                }
            }
            
            FastLock();
			// Camera parsing
            CameraList.clear();
            for(auto* camObj : SummonCameras)
            {
                if(!camObj) continue;
                auto* summonCam = reinterpret_cast<Blis_Client_LocalSummonCamera_o*>(camObj);
                if(!summonCam) continue;

				auto* summonData = summonCam->fields.summonData;
				if (!summonData) continue;

                //summonData->fields.sightRange

				CameraInfoForDraw tempInfo;
				tempInfo.CameraObject = summonCam;

				GetSightRangeFunc(summonData);
                CameraList.emplace_back();
			}
         

            DrawPlayerList.swap(PlayerList);
            DrawMonsterList.swap(MonsterList);
            DrawMonsterUIPos.swap(MonsterUIPos);
			DrawCameraList.swap(CameraList);
            FastUnlock();
        }

       

    continue_label:
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        int sleepTime = 1000 - static_cast<int>(elapsed);
        Sleep(sleepTime > 0 ? sleepTime : 1);
    }
}

void renderloop() 
{
    return;

    if (!vars::initil2cpp) 
    {
        Sleep(100);
        return;
    }
    
    if (!g_BackgroundDrawList) g_BackgroundDrawList = ImGui::GetBackgroundDrawList();
    if (!g_ForegroundDrawList) g_ForegroundDrawList = ImGui::GetForegroundDrawList();
    
    Unity::Vector3 myPlayerPosition;
    
    FastLock();
    
    if (DrawPlayerList.empty() || GameUI.empty() || !CService)
    {
        FastUnlock();
        return;
    }

    if(!GameMap)
        GameMap = reinterpret_cast<Blis_Client_UIMapDefault_o*>(reinterpret_cast<Blis_Client_GameUI_o*>(GameUI[0])->fields.uiMapBase);
        
    myPlayerPosition = GetPosition(DrawPlayerList[0].PlayerObject);
    Vector2 localPos; 
    Functions::worldtoscreen(myPlayerPosition, localPos);

    if (!MapSprite.empty() && vars::ShowMap)
    {
        const ImVec2 mapSize(MiniMapBG.width * vars::MapScale, MiniMapBG.height * vars::MapScale);
        g_BackgroundDrawList->AddImage(MiniMapBG.srv,
            CalcuratingCorrectionPosition(ImVec2((float)vars::MapPositionX, (float)vars::MapPositionY)),
            CalcuratingCorrectionPosition(ImVec2(mapSize.x + (float)vars::MapPositionX, mapSize.y + (float)vars::MapPositionY)),
            ImVec2(MiniMapBG.uv0, MiniMapBG.uv1), ImVec2(MiniMapBG.uv2, MiniMapBG.uv3), ImColor(1.0f, 1.0f, 1.0f, 0.4f));   
    }

    // Monster rendering
    if (vars::MonsterSnaplines && !DrawMonsterList.empty())
    {
        for (const auto& info : DrawMonsterList)
        {
            if (!info.IsAlive || !info.MonsterObject) continue;

            Unity::Vector3 root_pos = GetPosition(info.MonsterObject);
            float dist = root_pos.DistToSqr(myPlayerPosition);

            if (dist <= vars::CheckDistance)
            {
                Vector2 pos;
                if (Functions::worldtoscreen(root_pos, pos))
                {
                    g_BackgroundDrawList->AddLine(ImVec2(localPos.x, localPos.y), ImVec2(pos.x, pos.y), info.DrawColor, 1.0f);
                }
            }

            if (vars::ShowMap && GameMap)
            {
                OutputVector2 mapPos = GetWorldPosToUIPosFunc(GameMap, InputVector3(root_pos));
                mapPos.y = vars::screen_size.y - mapPos.y;

                g_BackgroundDrawList->AddCircleFilled(CalcuratingCorrectionPosition(ImVec2(mapPos.x, mapPos.y)), 3.0f, ImColor(0.9f, 0.0f, 0.0f, 1.0f));
            }
        }
    }
        
    // Map rendering
    if (vars::ShowMap)
    {
        for (const auto& temp : DrawMonsterUIPos)
        {
            if (!temp.ScratchObject) continue;

            Unity::Vector3 soPos = GetPosition(temp.ScratchObject);
			auto isSafe = IsInSafeZoneFunc(CService, InputVector3(soPos), temp.areaCode, IsInSafeZone_MethodInfo);

			//if (!isSafe) continue;

            OutputVector2 scratchMapPos = GetWorldPosToUIPosFunc(GameMap, InputVector3(soPos));

            ImVec2 monPos(scratchMapPos.x, vars::screen_size.y - scratchMapPos.y);

            if (temp.isAlive)
            {
                if ((temp.monsterType > 0 && temp.monsterType <= 6) || temp.monsterType == 11)
                {
                    ImVec2 calcMonPos = CalcuratingCorrectionPosition(
                        ImVec2(monPos.x - (MonsterSpriteSize[temp.monsterType].x * 0.5f),
                            monPos.y - (MonsterSpriteSize[temp.monsterType].y * 0.5f)));

                    ImVec2 maxPos(calcMonPos.x + (MonsterSpriteSize[temp.monsterType].x * vars::IconScale),
                        calcMonPos.y + (MonsterSpriteSize[temp.monsterType].y * vars::IconScale));

                    g_BackgroundDrawList->AddImage(MonsterSprites[temp.monsterType - 1], calcMonPos, maxPos, ImVec2(0, 0), ImVec2(1, 1), temp.DrawColor);
                }
            }
            
            if (temp.remainTime > 0)
            {
                char timeStr[8];
                snprintf(timeStr, sizeof(timeStr), "%d", static_cast<int>(temp.remainTime));
                g_BackgroundDrawList->AddText(gameFont, 12.0f,
                    CalcuratingCorrectionPosition(ImVec2(monPos.x - 1, monPos.y - 1)),
                    ImColor(1.0f, 1.0f, 1.0f, 1.0f), timeStr);
            }
        }
    }
    
    // Player rendering
    if (vars::PlayerSnaplines && !DrawPlayerList.empty() && !IconTest.empty())
    {
        const float checkDist = vars::CheckDistance;
        const float disableDist = vars::DisableDistance;
        const bool showMap = vars::ShowMap;

        int i = -1;
        for (const auto& info : DrawPlayerList) 
        {
            i++;
            if (!info.IsAlive || !info.PlayerObject) continue;
    
            if (GameMap)
            {
                Unity::Vector3 playerPos = GetPosition(info.PlayerObject);
                OutputVector2 mapPos = GetWorldPosToUIPosFunc(GameMap, InputVector3(playerPos));
                Vector2 playerMapPos(mapPos.x, vars::screen_size.y - mapPos.y);

                if (info.TeamSlot)
                {
                    if (showMap)
                        g_BackgroundDrawList->AddCircleFilled(CalcuratingCorrectionPosition(ImVec2(playerMapPos.x, playerMapPos.y)), 3.0f, ImColor(0.0f, 0.9f, 0.0f, 1.0f));

                    continue;
                }
                
                float dist = playerPos.DistToSqr(myPlayerPosition);
                
                if (showMap) 
                {
                    g_BackgroundDrawList->AddCircleFilled(CalcuratingCorrectionPosition(ImVec2(playerMapPos.x, playerMapPos.y)), 3.0f, ImColor(0.9f, 0.9f, 0.0f, 1.0f));
                    g_BackgroundDrawList->AddText(gameFont, 12.0f, CalcuratingCorrectionPosition(ImVec2(playerMapPos.x + 5, playerMapPos.y)), vars::PlayerSnaplineColor, info.CharName.c_str());
                }
                
                if (dist >= disableDist && dist <= checkDist) 
                {
                    Vector2 pos;
                    if (Functions::worldtoscreen(playerPos, pos)) 
                    {
                        float normal = normalizeClamped(dist, checkDist);
                        
                        g_BackgroundDrawList->AddLine(ImVec2(localPos.x, localPos.y), ImVec2(pos.x, pos.y), vars::PlayerSnaplineColor, 1.0f);
                        
                        Vector2 tempVec = Vector2::Lerp(localPos, pos, normal);
                        ImVec2 indicator(tempVec.x, tempVec.y);
                        
                        const IconEntry& ent = IconTest[i];
                        ImColor iconCol(1.f, 1.f, 1.f, 1.f);
                        if (info.isDying)
                            iconCol = ImColor(0.8f, 0.f, 0.f, 0.8f);

                        g_BackgroundDrawList->AddImage(ent.srv, indicator, 
                            ImVec2(indicator.x + ent.width, indicator.y + ent.height), ImVec2(ent.uv0 + vars::uv1, ent.uv1 + vars::uv2), ImVec2(ent.uv2 + vars::uv3, ent.uv3 + vars::uv4), iconCol);

                        g_BackgroundDrawList->AddText(gameFont, 13.0f, ImVec2(indicator.x, indicator.y + 37), vars::PlayerSnaplineColor, info.CharName.c_str());
                        
                        char tempText[32];
                        snprintf(tempText, sizeof(tempText), "%d M", static_cast<int>(dist));
                        g_BackgroundDrawList->AddText(gameFont, 13.0f, ImVec2(indicator.x, indicator.y + 47), vars::PlayerSnaplineColor, tempText);

                        snprintf(tempText, sizeof(tempText), "Team %d", static_cast<int>(info.TeamNumber));
                        g_BackgroundDrawList->AddText(gameFont, 13.0f, ImVec2(indicator.x, indicator.y + 57), vars::PlayerSnaplineColor, tempText);
                        
                        draw_health_circle(ImVec2(indicator.x + (ent.width / 2), indicator.y + (ent.height / 2)), static_cast<float>(info.playerHP), static_cast<float>(info.playerMaxHP), 16);
                    }
                }
            }
        }
    }
    
    FastUnlock();
}

HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) 
{
    if (!tls_ThreadHandle) {
        tls_ThreadHandle = IL2CPP::Thread::Attach(IL2CPP::Domain::Get());
    }
    
    if (!init) 
    {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice))) 
        {
            pDevice->GetImmediateContext(&pContext);
            DXGI_SWAP_CHAIN_DESC sd;
            pSwapChain->GetDesc(&sd);
            window = sd.OutputWindow;
            
            ID3D11Texture2D* pBackBuffer;
            pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
            pBackBuffer->Release();
            
            oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
            InitImGui();
            
            ImGuiIO& io = ImGui::GetIO();
            io.Fonts->AddFontDefault();
            ImFontConfig font_cfg;
            font_cfg.MergeMode = true;
            font_cfg.GlyphRanges = io.Fonts->GetGlyphRangesKorean();
            gameFont = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\malgunbd.ttf", 16.0f, &font_cfg);
            io.Fonts->Build();
            io.FontDefault = gameFont;
            
            LoadTexturesFromArchive(pDevice, L"Resource.arc", CharacterSprites, MonsterSprites, MapSprite);
            LoadINIFile();

            init = true;
        }
        else 
        {
            return oPresent(pSwapChain, SyncInterval, Flags);
        }
    }
    
    pContext->RSGetViewports(&vars::vps, &vars::viewport);
    vars::screen_size = { vars::viewport.Width, vars::viewport.Height };
    vars::screen_center = { vars::viewport.Width * 0.5f, vars::viewport.Height * 0.5f };
    
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    if (show_menu) 
    {
        DrawMenu();
    }

    // Player Icon & Name Parsing
    if (!vars::PlayerImageCached)
    {
        FastLock();
        if (!DrawPlayerList.empty() && GameMap)
        {
            IconTest.clear();
            IconTest.reserve(DrawPlayerList.size());
            for (PlayerInfoForDraw& PlayerInfo : DrawPlayerList)
            {
                //auto playerData = DrawPlayerList[i];
                Blis_Client_LocalPlayerCharacter_o* player = static_cast<Blis_Client_LocalPlayerCharacter_o*>(PlayerInfo.PlayerObject);

                UnityEngine_Sprite_o* sprite = GetMapIconPlayerFunc(player, GetMapIconPlayer_MethodInfo);
                UnityEngine_Texture2D_o* texture = GetTextureFunc(sprite);
                if (!texture) continue;

                intptr_t nativePtr = GetNativeTextureFunc(texture, GetNativeTexturePtr_MethodInfo);
                if (nativePtr == 0) { continue; }

                ID3D11ShaderResourceView* srv = nullptr;
                ID3D11Texture2D* tex2d = reinterpret_cast<ID3D11Texture2D*>(nativePtr);
                if (tex2d)
                {
                    tex2d->AddRef();
                    HRESULT hr = pDevice->CreateShaderResourceView(tex2d, nullptr, &srv);
                    if (FAILED(hr)) { tex2d->Release(); srv = nullptr; }
                    else { tex2d->Release(); }
                }
                if (!srv)
                {
                    ID3D11ShaderResourceView* maybeSrv = reinterpret_cast<ID3D11ShaderResourceView*>(nativePtr);
                    if (maybeSrv) { maybeSrv->AddRef(); srv = maybeSrv; }
                }

                if (!srv) continue;

                InputVector4* uvs = new InputVector4();
                GetInnerUVs_InjectedFunc(sprite, uvs, GetInnerUVs_Injected_MethodInfo);

                UnityEngine_Rect_o rect = GetRectFunc(sprite, GetRect_MethodInfo);

                IconEntry e;
                e.srv = reinterpret_cast<void*>(srv);
                e.uv0 = uvs->uv1;
                e.uv1 = uvs->uv4;
                e.uv2 = uvs->uv3;
                e.uv3 = uvs->uv2;

                e.uvMin = ImVec2(uvs->uv1, uvs->uv4);
                e.uvMax = ImVec2(uvs->uv3, uvs->uv2);

                e.width = rect.fields.m_Width;
                e.height = rect.fields.m_Height;

                IconTest.emplace_back(e);
                delete uvs;

                if (IconTest.size() > 3) vars::PlayerImageCached = true;
            }

            if(MiniMapUI.size() > 1 && MapSpriteTest.empty())
            {
                auto* sprite = GameMap->fields.baseMapOverlayImage->fields.m_Sprite;
                auto* texture = GetTextureFunc(sprite);
                if (texture)
                {
                    intptr_t nativePtr = GetNativeTextureFunc(texture, GetNativeTexturePtr_MethodInfo);
                    if (nativePtr)
                    {
                        ID3D11ShaderResourceView* srv = nullptr;
                        ID3D11Texture2D* tex2d = reinterpret_cast<ID3D11Texture2D*>(nativePtr);
                        if (tex2d)
                        {
                            tex2d->AddRef();
                            HRESULT hr = pDevice->CreateShaderResourceView(tex2d, nullptr, &srv);
                            if (FAILED(hr)) { tex2d->Release(); srv = nullptr; }
                            else { tex2d->Release(); }
                        }
                        if (!srv)
                        {
                            ID3D11ShaderResourceView* maybeSrv = reinterpret_cast<ID3D11ShaderResourceView*>(nativePtr);
                            if (maybeSrv) { maybeSrv->AddRef(); srv = maybeSrv; }
                        }

                        if (srv)
                        {
							auto* FileName = GetNameFunc(sprite);
                            printf("Sprite : %s \n", SystemStringToCharWindows(FileName));

							FileName = GetNameFunc(texture);
                            printf("Texture : %s \n", SystemStringToCharWindows(FileName));

                            MapSpriteTest.emplace_back(srv);
                            InputVector4* uvs = new InputVector4();
                            GetInnerUVs_InjectedFunc(sprite, uvs, GetInnerUVs_Injected_MethodInfo);

                            UnityEngine_Rect_o rect = GetRectFunc(sprite, GetRect_MethodInfo);

                            IconEntry e;
                            e.srv = reinterpret_cast<void*>(srv);
                            e.uv0 = uvs->uv1;
                            e.uv1 = uvs->uv4;
                            e.uv2 = uvs->uv3;
                            e.uv3 = uvs->uv2;

                            e.uvMin = ImVec2(uvs->uv1, uvs->uv4);
                            e.uvMax = ImVec2(uvs->uv3, uvs->uv2);

                            e.width = rect.fields.m_Width;
                            e.height = rect.fields.m_Height;

                            //Unity::Vector3 pos = GetPosition(instance);
                            //OutputVector2 mapPos = GetWorldPosToUIPosFunc(GameMap, InputVector3(pos));

                            //e.MapPos = ImVec2(mapPos.x, mapPos.y);

                            MiniMapBG = e;
                            delete uvs;
                        }
                    }
                }
			}
            

            if (Lumi != nullptr)
            {
                auto* instance = reinterpret_cast<Blis_Client_LocalTouringObject_o*>(Lumi);
                auto* sprite = GetMapIconLumiFunc(instance, GetMapIconPlayer_MethodInfo);
                auto* texture = GetTextureFunc(sprite);
                if (texture)
                {
                    intptr_t nativePtr = GetNativeTextureFunc(texture, GetNativeTexturePtr_MethodInfo);
                    if (nativePtr) 
                    {
                        ID3D11ShaderResourceView* srv = nullptr;
                        ID3D11Texture2D* tex2d = reinterpret_cast<ID3D11Texture2D*>(nativePtr);
                        if (tex2d)
                        {
                            tex2d->AddRef();
                            HRESULT hr = pDevice->CreateShaderResourceView(tex2d, nullptr, &srv);
                            if (FAILED(hr)) { tex2d->Release(); srv = nullptr; }
                            else { tex2d->Release(); }
                        }
                        if (!srv)
                        {
                            ID3D11ShaderResourceView* maybeSrv = reinterpret_cast<ID3D11ShaderResourceView*>(nativePtr);
                            if (maybeSrv) { maybeSrv->AddRef(); srv = maybeSrv; }
                        }

                        if (srv)
                        {
                            InputVector4* uvs = new InputVector4();
                            GetInnerUVs_InjectedFunc(sprite, uvs, GetInnerUVs_Injected_MethodInfo);

                            UnityEngine_Rect_o rect = GetRectFunc(sprite, GetRect_MethodInfo);

                            IconEntry e;
                            e.srv = reinterpret_cast<void*>(srv);
                            e.uv0 = uvs->uv1;
                            e.uv1 = uvs->uv4;
                            e.uv2 = uvs->uv3;
                            e.uv3 = uvs->uv2;

                            e.uvMin = ImVec2(uvs->uv1, uvs->uv4);
                            e.uvMax = ImVec2(uvs->uv3, uvs->uv2);

                            e.width = rect.fields.m_Width;
                            e.height = rect.fields.m_Height;

                            Unity::Vector3 pos = GetPosition(instance);
                            OutputVector2 mapPos = GetWorldPosToUIPosFunc(GameMap, InputVector3(pos));

                            e.MapPos = ImVec2(mapPos.x, mapPos.y);

                            LumiIcon = e;
                            delete uvs;
                        }
                    }
                }
            }
        }
        FastUnlock();
    }
    
    try 
    {
        renderloop();
    } 
    catch (...) {}
    
    ImGui::Render();

    if (vars::SaveSetting)
    {
        SaveINIFile();
    }
    
    static DWORD lastKeyCheck = 0;
    DWORD currentTime = GetTickCount64();
    if (currentTime - lastKeyCheck > 10) 
    {
        if (GetAsyncKeyState(VK_OEM_3) & 1) 
        {
            vars::ShowMap = !vars::ShowMap;
        }
        if (GetAsyncKeyState(VK_XBUTTON1) & 1)
        {
            vars::MonsterSnaplines = !vars::MonsterSnaplines;
        }
        if (GetAsyncKeyState(VK_XBUTTON2) & 1)
        {
            vars::PlayerSnaplines = !vars::PlayerSnaplines;
        }
        if (GetAsyncKeyState(VK_INSERT) & 1) 
        {
            show_menu = !show_menu;
            ImGui::GetIO().MouseDrawCursor = show_menu;
        }
        if (GetAsyncKeyState(VK_NEXT) & 1)
        {
            vars::SpeedLooting = !vars::SpeedLooting;
        }
        
        lastKeyCheck = currentTime;
    }
    
    pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    
    return oPresent(pSwapChain, SyncInterval, Flags);
}

void initchair() 
{
#ifdef DEBUG
    CreateConsole();
    system("cls");
#endif
    initvars();
    find_sigs();
    Find_MethodInfo();
    IL2CPP::Callback::Initialize();
    EnableHooks();
    kiero::bind(8, (void**)&oPresent, hkPresent);
    
    HANDLE hThread1 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Player_Cache, NULL, 0, NULL);
    HANDLE hThread2 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Looting_Thread, NULL, 0, NULL);
    SetThreadPriority(hThread1, THREAD_PRIORITY_ABOVE_NORMAL);
    SetThreadPriority(hThread2, THREAD_PRIORITY_ABOVE_NORMAL);
}

DWORD WINAPI MainThread(LPVOID lpReserved) 
{
    while (kiero::init(kiero::RenderType::D3D11) != kiero::Status::Success) 
    {
        Sleep(100);
    }
    initchair();
    vars::initil2cpp = true;
    return TRUE;
}

BOOL WINAPI DllMain(HMODULE mod, DWORD reason, LPVOID lpReserved) 
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(mod);
        CreateThread(nullptr, 0, MainThread, mod, 0, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        SaveINIFile();
        break;
    }
    
    return TRUE;
}

bool LoadTexturesFromArchive(ID3D11Device* device, const std::wstring& archivePath, std::vector<void*>& outSRVs_Char, std::vector<void*>& outSRVs_Mon, std::vector<void*>& outSRVs_Map) 
{
    std::ifstream in(archivePath, std::ios::binary);
    if (!in) return false;

    int count = 0;
    in.read(reinterpret_cast<char*>(&count), sizeof(count));
    outSRVs_Char.resize(vars::CharacterCount);

    if (count != vars::CharacterCount + vars::MonsterCount + vars::MapCount + 1) 
    {
        printf("Resource count not matched | %d, %d \n", count, vars::CharacterCount + vars::MonsterCount + vars::MapCount + 1);
        return false;
    }
    
    for (int i = 0; i < vars::CharacterCount; i++) 
    {
        DirectX::TexMetadata metadata;
        in.read(reinterpret_cast<char*>(&metadata), sizeof(metadata));

        size_t dataSize = 0;
        in.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));

        std::vector<uint8_t> buffer(dataSize);
        in.read(reinterpret_cast<char*>(buffer.data()), dataSize);

        DirectX::ScratchImage scratch;
        if (FAILED(scratch.Initialize2D(metadata.format, metadata.width, metadata.height, metadata.arraySize, metadata.mipLevels)))
            return false;

        memcpy(scratch.GetImages()->pixels, buffer.data(), dataSize);

        ID3D11ShaderResourceView* srv = nullptr;
        if (FAILED(CreateShaderResourceView(device, scratch.GetImages(), 1, metadata, &srv)))
            return false;

        outSRVs_Char[i] = srv;
    }

    outSRVs_Mon.resize(vars::MonsterCount + 6);
    for (int i = 0; i < vars::MonsterCount; i++) 
    {
        DirectX::TexMetadata metadata;
        in.read(reinterpret_cast<char*>(&metadata), sizeof(metadata));

        size_t dataSize = 0;
        in.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));

        std::vector<uint8_t> buffer(dataSize);
        in.read(reinterpret_cast<char*>(buffer.data()), dataSize);

        DirectX::ScratchImage scratch;
        if (FAILED(scratch.Initialize2D(metadata.format, metadata.width, metadata.height, metadata.arraySize, metadata.mipLevels)))
            return false;

        memcpy(scratch.GetImages()->pixels, buffer.data(), dataSize);

        ID3D11ShaderResourceView* srv = nullptr;
        if (FAILED(CreateShaderResourceView(device, scratch.GetImages(), 1, metadata, &srv)))
            return false;

        outSRVs_Mon[i] = srv;
    }

    // Drone Icon
    {
        DirectX::TexMetadata metadata;
        in.read(reinterpret_cast<char*>(&metadata), sizeof(metadata));

        size_t dataSize = 0;
        in.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));

        std::vector<uint8_t> buffer(dataSize);
        in.read(reinterpret_cast<char*>(buffer.data()), dataSize);

        DirectX::ScratchImage scratch;
        if (FAILED(scratch.Initialize2D(metadata.format, metadata.width, metadata.height, metadata.arraySize, metadata.mipLevels)))
            return false;

        memcpy(scratch.GetImages()->pixels, buffer.data(), dataSize);

        ID3D11ShaderResourceView* srv = nullptr;
        if (FAILED(CreateShaderResourceView(device, scratch.GetImages(), 1, metadata, &srv)))
            return false;

        outSRVs_Mon[10] = srv;
    }
    
    outSRVs_Map.resize(1);
    {
        DirectX::TexMetadata metadata;
        in.read(reinterpret_cast<char*>(&metadata), sizeof(metadata));

        size_t dataSize = 0;
        in.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));

        std::vector<uint8_t> buffer(dataSize);
        in.read(reinterpret_cast<char*>(buffer.data()), dataSize);

        DirectX::ScratchImage scratch;
        if (FAILED(scratch.Initialize2D(metadata.format, metadata.width, metadata.height, metadata.arraySize, metadata.mipLevels)))
            return false;

        memcpy(scratch.GetImages()->pixels, buffer.data(), dataSize);

        ID3D11ShaderResourceView* srv = nullptr;
        if (FAILED(CreateShaderResourceView(device, scratch.GetImages(), 1, metadata, &srv)))
            return false;

        outSRVs_Map[0] = srv;
    }

    return true;
}