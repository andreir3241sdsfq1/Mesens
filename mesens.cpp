#include <Windows.h>
#include <TlHelp32.h>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <float.h>
#include <sstream>
#include <iomanip>
#include <random>
#include <fstream>
#include <queue>
#include <array>
#include <functional>
#include <d3d9.h>
#include <filesystem>
#include <shlobj.h>

// MinHook
#pragma comment(lib, "MH/lib/libMinHook.x86.lib")
#include "MH/include/MinHook.h"

// ImGui
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "d3d9.lib")

// ================================
// === ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ===
// ================================

// Глобальный хэндл DLL
static HMODULE g_hModule = nullptr;

// ImGui
static bool g_ImGuiInitialized = false;
static bool g_ShowMenu = true;
static bool g_MenuCollapsed = false;
static HWND g_GameWindow = nullptr;
static LPDIRECT3DDEVICE9 g_D3D9Device = nullptr;
static WNDPROC g_OriginalWndProc = nullptr;

// DirectX хуки
typedef HRESULT(__stdcall* EndScene_t)(LPDIRECT3DDEVICE9);
typedef HRESULT(__stdcall* Reset_t)(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);
static EndScene_t g_OriginalEndScene = nullptr;
static Reset_t g_OriginalReset = nullptr;

// Game offsets (используем hazedumper)
#define dwLocalPlayer           0xDEF97C
#define dwEntityList            0x4E051DC
#define dwGlowObjectManager     0x535FCB8
#define dwClientState           0x59F19C
#define dwClientState_ViewAngles 0x4D90
#define dwViewMatrix            0x4DF6024
#define dwPlayerResource        0x3231380
#define dwGameRulesProxy        0x5334764
#define dwForceJump             0x52C0F50
#define dwForceAttack           0x3233024
#define dwForceAttack2          0x3233030
#define dwForceForward          0x323306C
#define dwForceBackward         0x3233078
#define dwForceLeft             0x3233084
#define dwForceRight            0x3233090
#define dwRadarBase             0x523BCB4
#define dwGlobalVars            0x59EE60
#define dwClientState_PlayerInfo 0x52C8
#define dwClientState_MaxPlayer 0x388
#define m_iPing2                 0x24
#define m_iScore                0x104
#define m_szPlayerName          0x225

// Netvars
#define m_iHealth               0x100
#define m_iTeamNum              0xF4
#define m_iGlowIndex            0x10488
#define m_bDormant              0xED
#define m_vecOrigin             0x138
#define m_vecViewOffset         0x108
#define m_dwBoneMatrix          0x26A8
#define m_bSpotted              0x93D
#define m_bIsScoped             0x9974
#define m_iDefaultFOV           0x333C
#define m_iFOV                  0x31F4
#define m_hViewModel            0x3308
#define m_iObserverMode         0x3388
#define m_hObserverTarget       0x339C
#define m_hActiveWeapon         0x2F08
#define m_iClip1                0x3274
#define m_iItemDefinitionIndex  0x2FBA
#define m_thirdPersonViewAngles 0x31E8
#define m_aimPunchAngle         0x303C
#define m_fFlags                0x104
#define m_iCrosshairId          0x11838
#define m_flFlashDuration       0x10470
#define m_lifeState             0x25F
#define m_vecVelocity           0x114
#define m_flSimulationTime      0x268
#define m_iShotsFired           0x103E0
#define m_clrRender             0x70
#define m_nModelIndex           0x258
#define m_nRenderMode           0x25B
#define m_ArmorValue            0x117CC
#define m_iFOVStart             0x31F8
#define m_angEyeAnglesX         0x117D0
#define m_angEyeAnglesY         0x117D4
#define m_viewPunchAngle        0x3030
#define m_aimPunchAngleVel      0x3048
#define m_bGunGameImmunity      0x9990
#define m_hMyWeapons            0x2E08
#define m_flNextAttack          0x2D80
#define m_bHasHelmet            0x117C0
#define m_bHasDefuser           0x117DC
#define m_flLowerBodyYawTarget  0x9ADC
#define m_iAccountID            0x2FD8
#define m_iCompetitiveWins      0x1B88
#define m_flThirdPersonAngle    0x31E8
#define m_flNextPrimaryAttack   0x3248
#define m_flC4Blow              0x29A0
#define m_bBombTicking          0x2990
#define m_nBombSite             0x2994
#define m_hBombDefuser          0x29C4
#define m_flTimerLength         0x29A4
#define m_flDefuseCountDown     0x29BC
#define m_szCustomName          0x303C

// Grenade offsets
#define m_bPinPulled            0x104A4
#define m_fThrowTime            0x104A8
#define m_vecThrowVelocity      0x104AC
#define m_nExplodeEffectTickBegin 0x29E4
#define m_flThrowStrength       0x104A0

// Weapon IDs
enum WeaponID {
    WEAPON_DEAGLE = 1,
    WEAPON_ELITE = 2,
    WEAPON_FIVESEVEN = 3,
    WEAPON_GLOCK = 4,
    WEAPON_AK47 = 7,
    WEAPON_AUG = 8,
    WEAPON_AWP = 9,
    WEAPON_FAMAS = 10,
    WEAPON_G3SG1 = 11,
    WEAPON_GALILAR = 13,
    WEAPON_M249 = 14,
    WEAPON_M4A1 = 16,
    WEAPON_MAC10 = 17,
    WEAPON_P90 = 19,
    WEAPON_UMP45 = 24,
    WEAPON_XM1014 = 25,
    WEAPON_BIZON = 26,
    WEAPON_MAG7 = 27,
    WEAPON_NEGEV = 28,
    WEAPON_SAWEDOFF = 29,
    WEAPON_TEC9 = 30,
    WEAPON_TASER = 31,
    WEAPON_HKP2000 = 32,
    WEAPON_MP7 = 33,
    WEAPON_MP9 = 34,
    WEAPON_NOVA = 35,
    WEAPON_P250 = 36,
    WEAPON_SCAR20 = 38,
    WEAPON_SG553 = 39,
    WEAPON_SSG08 = 40,
    WEAPON_KNIFE = 42,
    WEAPON_FLASHBANG = 43,
    WEAPON_HEGRENADE = 44,
    WEAPON_SMOKEGRENADE = 45,
    WEAPON_MOLOTOV = 46,
    WEAPON_DECOY = 47,
    WEAPON_INCGRENADE = 48,
    WEAPON_C4 = 49,
    WEAPON_KNIFE_T = 59,
    WEAPON_M4A1_SILENCER = 60,
    WEAPON_USP_SILENCER = 61,
    WEAPON_CZ75A = 63,
    WEAPON_REVOLVER = 64,
    WEAPON_KNIFE_BAYONET = 500,
    WEAPON_KNIFE_FLIP = 505,
    WEAPON_KNIFE_GUT = 506,
    WEAPON_KNIFE_KARAMBIT = 507,
    WEAPON_KNIFE_M9_BAYONET = 508,
    WEAPON_KNIFE_TACTICAL = 509,
    WEAPON_KNIFE_FALCHION = 512,
    WEAPON_KNIFE_SURVIVAL_BOWIE = 514,
    WEAPON_KNIFE_BUTTERFLY = 515,
    WEAPON_KNIFE_PUSH = 516
};

// Weapon names
std::map<int, std::string> weaponNames = {
    {WEAPON_DEAGLE, "Desert Eagle"},
    {WEAPON_ELITE, "Dual Berettas"},
    {WEAPON_FIVESEVEN, "Five-Seven"},
    {WEAPON_GLOCK, "Glock-18"},
    {WEAPON_AK47, "AK-47"},
    {WEAPON_AUG, "AUG"},
    {WEAPON_AWP, "AWP"},
    {WEAPON_FAMAS, "FAMAS"},
    {WEAPON_G3SG1, "G3SG1"},
    {WEAPON_GALILAR, "Galil AR"},
    {WEAPON_M249, "M249"},
    {WEAPON_M4A1, "M4A4"},
    {WEAPON_MAC10, "MAC-10"},
    {WEAPON_P90, "P90"},
    {WEAPON_UMP45, "UMP-45"},
    {WEAPON_XM1014, "XM1014"},
    {WEAPON_BIZON, "PP-Bizon"},
    {WEAPON_MAG7, "MAG-7"},
    {WEAPON_NEGEV, "Negev"},
    {WEAPON_SAWEDOFF, "Sawed-Off"},
    {WEAPON_TEC9, "Tec-9"},
    {WEAPON_TASER, "Zeus x27"},
    {WEAPON_HKP2000, "P2000"},
    {WEAPON_MP7, "MP7"},
    {WEAPON_MP9, "MP9"},
    {WEAPON_NOVA, "Nova"},
    {WEAPON_P250, "P250"},
    {WEAPON_SCAR20, "SCAR-20"},
    {WEAPON_SG553, "SG 553"},
    {WEAPON_SSG08, "SSG 08"},
    {WEAPON_KNIFE, "Knife"},
    {WEAPON_FLASHBANG, "Flashbang"},
    {WEAPON_HEGRENADE, "HE Grenade"},
    {WEAPON_SMOKEGRENADE, "Smoke Grenade"},
    {WEAPON_MOLOTOV, "Molotov"},
    {WEAPON_DECOY, "Decoy"},
    {WEAPON_INCGRENADE, "Incendiary"},
    {WEAPON_C4, "C4"},
    {WEAPON_M4A1_SILENCER, "M4A1-S"},
    {WEAPON_USP_SILENCER, "USP-S"},
    {WEAPON_CZ75A, "CZ75-Auto"},
    {WEAPON_REVOLVER, "R8 Revolver"}
};

// ================================
// === СТРУКТУРЫ ===
// ================================

struct Vector3 {
    float x, y, z;
    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    Vector3 operator-(const Vector3& o) const { return Vector3(x - o.x, y - o.y, z - o.z); }
    Vector3 operator+(const Vector3& o) const { return Vector3(x + o.x, y + o.y, z + o.z); }
    Vector3 operator*(float f) const { return Vector3(x * f, y * f, z * f); }
    Vector3 operator/(float f) const { return Vector3(x / f, y / f, z / f); }
    bool operator==(const Vector3& o) const { return x == o.x && y == o.y && z == o.z; }
    bool operator!=(const Vector3& o) const { return !(*this == o); }
    float Length() const { return sqrtf(x * x + y * y + z * z); }
    float Length2D() const { return sqrtf(x * x + y * y); }
    float LengthSqr() const { return x * x + y * y + z * z; }
    float Length2DSqr() const { return x * x + y * y; }
    Vector3 Normalized() const { float l = Length(); return l > 0 ? *this / l : Vector3(); }
    float Dot(const Vector3& o) const { return x * o.x + y * o.y + z * o.z; }
    Vector3 Cross(const Vector3& o) const {
        return Vector3(y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x);
    }
    float DistTo(const Vector3& o) const { return (*this - o).Length(); }
    float DistToSqr(const Vector3& o) const { return (*this - o).LengthSqr(); }
};

struct Vector2 {
    float x, y;
    Vector2() : x(0), y(0) {}
    Vector2(float x, float y) : x(x), y(y) {}

    operator ImVec2() const { return ImVec2(x, y); }
};

struct AimPoint {
    Vector3 pos;
    Vector3 center;
    int hitbox;
    int hitgroup;
    float damage;
    float hitchance;
    float no_pen_damage;
    int safety;
    float safety_size;
    float center_dist;
    bool has_safe_point;
    bool operator<(const AimPoint& other) const {
        return damage < other.damage;
    }
};

// Trace structure for penetration
struct Trace {
    Vector3 start;
    Vector3 end;
    Vector3 plane_normal;
    float fraction;
    int contents;
    unsigned short disp_flags;
    bool allsolid;
    bool startsolid;
    float fraction_left_solid;
    float fraction_world;
    int hitgroup;
    short physicsbone;
    unsigned short worldSurfaceIndex;
    void* entity;
    int hitbox;
};

// Grenade prediction structure
struct GrenadeInfo {
    Vector3 start;
    Vector3 velocity;
    Vector3 position;
    int type;
    float throwTime;
    bool predicted;
    std::vector<Vector3> trajectory;
    Vector3 endPoint;
    bool willExplode;
    float explodeTime;
};

// Player info structure
struct PlayerInfo {
    char name[32];
    char pad[96];
};

// Menu settings structure
struct MenuSettings {
    // Window settings
    float windowWidth;
    float windowHeight;
    ImVec4 windowBgColor;
    ImVec4 windowTitleColor;
    ImVec4 windowBorderColor;
    float windowRounding;
    float windowBorderSize;

    // Tab settings
    ImVec4 tabActiveColor;
    ImVec4 tabInactiveColor;
    ImVec4 tabHoveredColor;
    float tabRounding;

    // Button settings
    ImVec4 buttonColor;
    ImVec4 buttonHoveredColor;
    ImVec4 buttonActiveColor;
    float buttonRounding;

    // Checkbox settings
    ImVec4 checkboxColor;
    ImVec4 checkboxActiveColor;

    // Slider settings
    ImVec4 sliderGrabColor;
    ImVec4 sliderGrabActiveColor;

    // Text settings
    ImVec4 textColor;
    ImVec4 textDisabledColor;

    // Input settings
    ImVec4 inputBgColor;
    ImVec4 inputBorderColor;

    // Menu position
    float menuX;
    float menuY;

    // Font settings
    float fontSize;
    bool customFont;
    char fontPath[260];

    // Animation
    float animationSpeed;

    // Blur effect
    bool enableBlur;
    float blurStrength;

    // Menu key
    int menuKey;

    // Menu toggle type
    int toggleType; // 0 = toggle, 1 = hold

    MenuSettings() {
        // Default values
        windowWidth = 800.0f;
        windowHeight = 650.0f;
        windowBgColor = ImVec4(0.1f, 0.1f, 0.1f, 0.95f);
        windowTitleColor = ImVec4(0.2f, 0.6f, 1.0f, 1.0f);
        windowBorderColor = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        windowRounding = 5.0f;
        windowBorderSize = 1.0f;

        tabActiveColor = ImVec4(0.2f, 0.6f, 1.0f, 1.0f);
        tabInactiveColor = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        tabHoveredColor = ImVec4(0.4f, 0.7f, 1.0f, 1.0f);
        tabRounding = 3.0f;

        buttonColor = ImVec4(0.2f, 0.6f, 1.0f, 1.0f);
        buttonHoveredColor = ImVec4(0.3f, 0.7f, 1.0f, 1.0f);
        buttonActiveColor = ImVec4(0.1f, 0.5f, 0.9f, 1.0f);
        buttonRounding = 3.0f;

        checkboxColor = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        checkboxActiveColor = ImVec4(0.2f, 0.6f, 1.0f, 1.0f);

        sliderGrabColor = ImVec4(0.2f, 0.6f, 1.0f, 1.0f);
        sliderGrabActiveColor = ImVec4(0.3f, 0.7f, 1.0f, 1.0f);

        textColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        textDisabledColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

        inputBgColor = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
        inputBorderColor = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);

        menuX = 0.0f;
        menuY = 0.0f;

        fontSize = 13.0f;
        customFont = false;
        memset(fontPath, 0, sizeof(fontPath));

        animationSpeed = 0.2f;

        enableBlur = false;
        blurStrength = 0.5f;

        menuKey = VK_INSERT;
        toggleType = 0;
    }
};

// Config structure
struct CheatConfig {
    char name[64];

    // Aimbot
    bool aimbotEnabled;
    int aimbotMode;
    bool aimbotTeamCheck;
    bool aimbotVisibilityCheck;
    bool aimbotAutoShoot;
    bool aimbotAutoStop;
    bool aimbotAutoScope;
    bool aimbotPenetrationCheck;
    float aimbotFOV;
    float aimbotSmooth;
    float aimbotHitchance;
    int aimbotMinDamage;
    int aimbotBone;
    float aimbotRageSmooth;
    bool rageAutoFire; // Новое: автоогонь для rage
    bool legitTriggerbot; // Новое: триггербот для legit

    // Triggerbot
    bool triggerbotEnabled;
    bool triggerbotTeamCheck;
    float triggerbotDelay;

    // BunnyHop
    bool bhopEnabled;
    int bhopPerfectJumps;

    // ESP
    bool espEnabled;
    bool espTeamCheck;
    bool radarEnabled;
    int espBoxType;
    bool espHealth;
    bool espHealthNumber;
    bool espHealthBar;
    bool espLine;
    bool espAmmo;
    bool espWeapon;
    bool espName;
    ImVec4 espColorEnemy;
    ImVec4 espColorTeam;
    float espBoxThickness;
    float espBoxAlpha;

    // ESP Positions
    int healthPosition;
    int ammoPosition;
    int weaponPosition;
    int namePosition;

    // Glow
    bool glowEnabled;
    bool glowTeam;
    ImVec4 glowColorEnemy;
    ImVec4 glowColorTeam;
    float glowAlpha;
    float glowStrength;

    // Grenade Prediction
    bool grenadePredictionEnabled;
    ImVec4 grenadePredictionColor;
    bool grenadePredictionShowTrajectory;
    bool grenadePredictionShowEndPoint;

    // Penetration Crosshair
    bool penetrationCrosshairEnabled;
    ImVec4 penetrationCrosshairColor;
    bool penetrationCrosshairShowDamage;

    // Misc
    bool thirdPersonEnabled;
    bool sniperCrosshairEnabled;
    bool fpsPingInfo;
    bool noFlashEnabled;
    int noFlashStrength;
    bool useCustomFOV;
    float customFOV;

    // Keys
    int keyMenu;
    int keyExit;
    int keyThirdPerson;
    int keyTrigger;
    int keyBhop;

    // Menu settings
    MenuSettings menuSettings;

    CheatConfig() {
        memset(name, 0, sizeof(name));

        // Default values
        aimbotEnabled = false;
        aimbotMode = 0;
        aimbotTeamCheck = true;
        aimbotVisibilityCheck = true;
        aimbotAutoShoot = false;
        aimbotAutoStop = false;
        aimbotAutoScope = false;
        aimbotPenetrationCheck = true;
        aimbotFOV = 5.0f;
        aimbotSmooth = 1.2f;
        aimbotHitchance = 50.0f;
        aimbotMinDamage = 20;
        aimbotBone = 8;
        aimbotRageSmooth = 1.0f;
        rageAutoFire = false;
        legitTriggerbot = false;

        triggerbotEnabled = false;
        triggerbotTeamCheck = true;
        triggerbotDelay = 0.1f;

        bhopEnabled = false;
        bhopPerfectJumps = 3;

        espEnabled = true;
        espTeamCheck = true;
        radarEnabled = false;
        espBoxType = 1;
        espHealth = true;
        espHealthNumber = false;
        espHealthBar = true;
        espLine = false;
        espAmmo = false;
        espWeapon = false;
        espName = false;
        espColorEnemy = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
        espColorTeam = ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
        espBoxThickness = 1.5f;
        espBoxAlpha = 1.0f;

        healthPosition = 0;
        ammoPosition = 0;
        weaponPosition = 2;
        namePosition = 2;

        glowEnabled = true;
        glowTeam = false;
        glowColorEnemy = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
        glowColorTeam = ImVec4(0.0f, 0.0f, 1.0f, 0.6f);
        glowAlpha = 0.8f;
        glowStrength = 1.0f;

        // Grenade Prediction
        grenadePredictionEnabled = false;
        grenadePredictionColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
        grenadePredictionShowTrajectory = true;
        grenadePredictionShowEndPoint = true;

        // Penetration Crosshair
        penetrationCrosshairEnabled = false;
        penetrationCrosshairColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
        penetrationCrosshairShowDamage = true;

        thirdPersonEnabled = false;
        sniperCrosshairEnabled = true;
        fpsPingInfo = false;
        noFlashEnabled = false;
        noFlashStrength = 100;
        useCustomFOV = false;
        customFOV = 90.0f;

        keyMenu = VK_INSERT;
        keyExit = VK_F10;
        keyThirdPerson = VK_F1;
        keyTrigger = VK_MENU;
        keyBhop = VK_SPACE;
    }
};

// ================================
// === ГЛОБАЛЬНЫЕ НАСТРОЙКИ ===
// ================================

// Game variables
static DWORD g_ClientModule = 0;
static DWORD g_EngineModule = 0;
static DWORD g_GamePID = 0;
static HANDLE g_GameHandle = 0;
static DWORD g_LocalPlayer = 0;
static DWORD g_ClientState = 0;
static DWORD g_PlayerResource = 0;

// Кэш сущностей
static DWORD g_CachedEntities[64] = { 0 };
static int g_CachedEntityCount = 0;
static DWORD g_LastCacheTime = 0;
static const int CACHE_INTERVAL = 50;

// Настройки аимбота
static bool g_AimbotEnabled = false;
static bool g_AimbotTeamCheck = true;
static bool g_AimbotVisibilityCheck = true;
static bool g_AimbotAutoShoot = false;
static bool g_AimbotAutoStop = false;
static bool g_AimbotAutoScope = false;
static bool g_AimbotPenetrationCheck = true;
static float g_AimbotFOV = 5.0f;
static float g_AimbotSmooth = 1.2f;
static float g_AimbotHitchance = 50.0f;
static int g_AimbotMinDamage = 20;
static int g_AimbotBone = 8; // Head
static int g_AimbotMode = 0; // 0 = Legit, 1 = Rage
static float g_AimbotRageSmooth = 1.0f;
static bool g_RageAutoFire = false;
static bool g_LegitTriggerbot = false;

// Настройки триггербота
static bool g_TriggerbotEnabled = false;
static bool g_TriggerbotTeamCheck = true;
static float g_TriggerbotDelay = 0.1f;
static int g_TriggerbotKey = VK_MENU;

// BunnyHop
static bool g_BunnyHopEnabled = false;
static int g_BunnyHopKey = VK_SPACE;
static int g_BunnyHopPerfectJumps = 3;
static int g_BunnyHopConsecutiveJumps = 0;

// ESP settings
enum ESPBoxType {
    ESPBOX_NONE = 0,
    ESPBOX_FULL = 1,
    ESPBOX_CORNER = 2,
    ESPBOX_FILLED = 3
};

// Grenade Prediction
static bool g_GrenadePredictionEnabled = false;
static ImVec4 g_GrenadePredictionColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
static bool g_GrenadePredictionShowTrajectory = true;
static bool g_GrenadePredictionShowEndPoint = true;
static GrenadeInfo g_CurrentGrenade;

// Penetration Crosshair
static bool g_PenetrationCrosshairEnabled = false;
static ImVec4 g_PenetrationCrosshairColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
static bool g_PenetrationCrosshairShowDamage = true;
static float g_PenetrationCrosshairDamage = 0.0f;

// Cheat features
static bool g_EspEnabled = true;
static bool g_RadarEnabled = false;
static bool g_EspTeamCheck = true;
static bool g_ThirdPersonEnabled = false;
static bool g_SniperCrosshairEnabled = true;
static int g_EspBoxType = ESPBOX_FULL;
static bool g_EspHealth = true;
static bool g_EspHealthNumber = false;
static bool g_EspHealthBar = true;
static bool g_EspLine = false;
static bool g_EspAmmo = false;
static bool g_EspWeapon = false;
static bool g_EspName = false;
static bool g_GlowEnabled = true;
static bool g_GlowTeam = false;
static float g_GlowStrength = 1.0f;
static bool g_FpsPingInfo = false;
static bool g_NoFlashEnabled = false;
static int g_NoFlashStrength = 100;

// Visual settings
static ImVec4 g_EspColorEnemy = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
static ImVec4 g_EspColorTeam = ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
static ImVec4 g_GlowColorEnemy = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
static ImVec4 g_GlowColorTeam = ImVec4(0.0f, 0.0f, 1.0f, 0.6f);
static float g_EspBoxThickness = 1.5f;
static float g_EspBoxAlpha = 1.0f;
static float g_GlowAlpha = 0.8f;
static float g_CustomFOV = 90.0f;
static bool g_UseCustomFOV = false;

// Layout positions
static int g_HealthPosition = 0;
static int g_AmmoPosition = 0;
static int g_WeaponPosition = 2;
static int g_NamePosition = 2;

// Key binds
static int g_KeyMenu = VK_INSERT;
static int g_KeyExit = VK_F10;
static int g_KeyThirdPerson = VK_F1;

// Reaction time
static int g_ReactionTime = 0;
static DWORD g_LastShotTime = 0;

// Config system
static std::string g_ConfigDirectory;
static std::vector<std::string> g_ConfigFiles;
static char g_NewConfigName[64] = "";
static int g_SelectedConfig = -1;

// Menu settings
static MenuSettings g_MenuSettings;

// Draw FOV
static bool g_DrawAimbotFOV = true;
static ImVec4 g_AimbotFOVColor = ImVec4(1.0f, 1.0f, 0.0f, 0.3f);

// Sniper crosshair
static bool g_SniperCrosshairActive = false;

// ================================
// === КОНФИГ ФУНКЦИИ ===
// ================================

void RefreshConfigList() {
    g_ConfigFiles.clear();

    CreateDirectoryA(g_ConfigDirectory.c_str(), NULL);

    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA((g_ConfigDirectory + "\\*.cfg").c_str(), &findFileData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                g_ConfigFiles.push_back(findFileData.cFileName);
            }
        } while (FindNextFileA(hFind, &findFileData));
        FindClose(hFind);
    }
}

void SaveConfigToCurrent() {
    if (g_SelectedConfig >= 0 && g_SelectedConfig < g_ConfigFiles.size()) {
        std::string configPath = g_ConfigDirectory + "\\" + g_ConfigFiles[g_SelectedConfig];

        CheatConfig config;
        strcpy_s(config.name, g_ConfigFiles[g_SelectedConfig].c_str());

        // Aimbot
        config.aimbotEnabled = g_AimbotEnabled;
        config.aimbotMode = g_AimbotMode;
        config.aimbotTeamCheck = g_AimbotTeamCheck;
        config.aimbotVisibilityCheck = g_AimbotVisibilityCheck;
        config.aimbotAutoShoot = g_AimbotAutoShoot;
        config.aimbotAutoStop = g_AimbotAutoStop;
        config.aimbotAutoScope = g_AimbotAutoScope;
        config.aimbotPenetrationCheck = g_AimbotPenetrationCheck;
        config.aimbotFOV = g_AimbotFOV;
        config.aimbotSmooth = g_AimbotSmooth;
        config.aimbotHitchance = g_AimbotHitchance;
        config.aimbotMinDamage = g_AimbotMinDamage;
        config.aimbotBone = g_AimbotBone;
        config.aimbotRageSmooth = g_AimbotRageSmooth;
        config.rageAutoFire = g_RageAutoFire;
        config.legitTriggerbot = g_LegitTriggerbot;

        // Triggerbot
        config.triggerbotEnabled = g_TriggerbotEnabled;
        config.triggerbotTeamCheck = g_TriggerbotTeamCheck;
        config.triggerbotDelay = g_TriggerbotDelay;

        // BunnyHop
        config.bhopEnabled = g_BunnyHopEnabled;
        config.bhopPerfectJumps = g_BunnyHopPerfectJumps;

        // ESP
        config.espEnabled = g_EspEnabled;
        config.espTeamCheck = g_EspTeamCheck;
        config.radarEnabled = g_RadarEnabled;
        config.espBoxType = g_EspBoxType;
        config.espHealth = g_EspHealth;
        config.espHealthNumber = g_EspHealthNumber;
        config.espHealthBar = g_EspHealthBar;
        config.espLine = g_EspLine;
        config.espAmmo = g_EspAmmo;
        config.espWeapon = g_EspWeapon;
        config.espName = g_EspName;
        config.espColorEnemy = g_EspColorEnemy;
        config.espColorTeam = g_EspColorTeam;
        config.espBoxThickness = g_EspBoxThickness;
        config.espBoxAlpha = g_EspBoxAlpha;

        // Positions
        config.healthPosition = g_HealthPosition;
        config.ammoPosition = g_AmmoPosition;
        config.weaponPosition = g_WeaponPosition;
        config.namePosition = g_NamePosition;

        // Glow
        config.glowEnabled = g_GlowEnabled;
        config.glowTeam = g_GlowTeam;
        config.glowColorEnemy = g_GlowColorEnemy;
        config.glowColorTeam = g_GlowColorTeam;
        config.glowAlpha = g_GlowAlpha;
        config.glowStrength = g_GlowStrength;

        // Grenade Prediction
        config.grenadePredictionEnabled = g_GrenadePredictionEnabled;
        config.grenadePredictionColor = g_GrenadePredictionColor;
        config.grenadePredictionShowTrajectory = g_GrenadePredictionShowTrajectory;
        config.grenadePredictionShowEndPoint = g_GrenadePredictionShowEndPoint;

        // Penetration Crosshair
        config.penetrationCrosshairEnabled = g_PenetrationCrosshairEnabled;
        config.penetrationCrosshairColor = g_PenetrationCrosshairColor;
        config.penetrationCrosshairShowDamage = g_PenetrationCrosshairShowDamage;

        // Misc
        config.thirdPersonEnabled = g_ThirdPersonEnabled;
        config.sniperCrosshairEnabled = g_SniperCrosshairEnabled;
        config.fpsPingInfo = g_FpsPingInfo;
        config.noFlashEnabled = g_NoFlashEnabled;
        config.noFlashStrength = g_NoFlashStrength;
        config.useCustomFOV = g_UseCustomFOV;
        config.customFOV = g_CustomFOV;

        // Keys
        config.keyMenu = g_KeyMenu;
        config.keyExit = g_KeyExit;
        config.keyThirdPerson = g_KeyThirdPerson;
        config.keyTrigger = g_TriggerbotKey;
        config.keyBhop = g_BunnyHopKey;

        // Menu settings
        config.menuSettings = g_MenuSettings;

        std::ofstream file(configPath, std::ios::binary);
        if (file.is_open()) {
            file.write((char*)&config, sizeof(CheatConfig));
            file.close();
        }
    }
}

void SaveConfig(const std::string& name) {
    std::string configPath = g_ConfigDirectory + "\\" + name + ".cfg";

    CheatConfig config;
    strcpy_s(config.name, name.c_str());

    // Aimbot
    config.aimbotEnabled = g_AimbotEnabled;
    config.aimbotMode = g_AimbotMode;
    config.aimbotTeamCheck = g_AimbotTeamCheck;
    config.aimbotVisibilityCheck = g_AimbotVisibilityCheck;
    config.aimbotAutoShoot = g_AimbotAutoShoot;
    config.aimbotAutoStop = g_AimbotAutoStop;
    config.aimbotAutoScope = g_AimbotAutoScope;
    config.aimbotPenetrationCheck = g_AimbotPenetrationCheck;
    config.aimbotFOV = g_AimbotFOV;
    config.aimbotSmooth = g_AimbotSmooth;
    config.aimbotHitchance = g_AimbotHitchance;
    config.aimbotMinDamage = g_AimbotMinDamage;
    config.aimbotBone = g_AimbotBone;
    config.aimbotRageSmooth = g_AimbotRageSmooth;
    config.rageAutoFire = g_RageAutoFire;
    config.legitTriggerbot = g_LegitTriggerbot;

    // Triggerbot
    config.triggerbotEnabled = g_TriggerbotEnabled;
    config.triggerbotTeamCheck = g_TriggerbotTeamCheck;
    config.triggerbotDelay = g_TriggerbotDelay;

    // BunnyHop
    config.bhopEnabled = g_BunnyHopEnabled;
    config.bhopPerfectJumps = g_BunnyHopPerfectJumps;

    // ESP
    config.espEnabled = g_EspEnabled;
    config.espTeamCheck = g_EspTeamCheck;
    config.radarEnabled = g_RadarEnabled;
    config.espBoxType = g_EspBoxType;
    config.espHealth = g_EspHealth;
    config.espHealthNumber = g_EspHealthNumber;
    config.espHealthBar = g_EspHealthBar;
    config.espLine = g_EspLine;
    config.espAmmo = g_EspAmmo;
    config.espWeapon = g_EspWeapon;
    config.espName = g_EspName;
    config.espColorEnemy = g_EspColorEnemy;
    config.espColorTeam = g_EspColorTeam;
    config.espBoxThickness = g_EspBoxThickness;
    config.espBoxAlpha = g_EspBoxAlpha;

    // Positions
    config.healthPosition = g_HealthPosition;
    config.ammoPosition = g_AmmoPosition;
    config.weaponPosition = g_WeaponPosition;
    config.namePosition = g_NamePosition;

    // Glow
    config.glowEnabled = g_GlowEnabled;
    config.glowTeam = g_GlowTeam;
    config.glowColorEnemy = g_GlowColorEnemy;
    config.glowColorTeam = g_GlowColorTeam;
    config.glowAlpha = g_GlowAlpha;
    config.glowStrength = g_GlowStrength;

    // Grenade Prediction
    config.grenadePredictionEnabled = g_GrenadePredictionEnabled;
    config.grenadePredictionColor = g_GrenadePredictionColor;
    config.grenadePredictionShowTrajectory = g_GrenadePredictionShowTrajectory;
    config.grenadePredictionShowEndPoint = g_GrenadePredictionShowEndPoint;

    // Penetration Crosshair
    config.penetrationCrosshairEnabled = g_PenetrationCrosshairEnabled;
    config.penetrationCrosshairColor = g_PenetrationCrosshairColor;
    config.penetrationCrosshairShowDamage = g_PenetrationCrosshairShowDamage;

    // Misc
    config.thirdPersonEnabled = g_ThirdPersonEnabled;
    config.sniperCrosshairEnabled = g_SniperCrosshairEnabled;
    config.fpsPingInfo = g_FpsPingInfo;
    config.noFlashEnabled = g_NoFlashEnabled;
    config.noFlashStrength = g_NoFlashStrength;
    config.useCustomFOV = g_UseCustomFOV;
    config.customFOV = g_CustomFOV;

    // Keys
    config.keyMenu = g_KeyMenu;
    config.keyExit = g_KeyExit;
    config.keyThirdPerson = g_KeyThirdPerson;
    config.keyTrigger = g_TriggerbotKey;
    config.keyBhop = g_BunnyHopKey;

    // Menu settings
    config.menuSettings = g_MenuSettings;

    std::ofstream file(configPath, std::ios::binary);
    if (file.is_open()) {
        file.write((char*)&config, sizeof(CheatConfig));
        file.close();
    }

    RefreshConfigList();
}

void LoadConfig(const std::string& name) {
    std::string configPath = g_ConfigDirectory + "\\" + name;

    CheatConfig config;
    std::ifstream file(configPath, std::ios::binary);

    if (file.is_open()) {
        file.read((char*)&config, sizeof(CheatConfig));
        file.close();

        // Aimbot
        g_AimbotEnabled = config.aimbotEnabled;
        g_AimbotMode = config.aimbotMode;
        g_AimbotTeamCheck = config.aimbotTeamCheck;
        g_AimbotVisibilityCheck = config.aimbotVisibilityCheck;
        g_AimbotAutoShoot = config.aimbotAutoShoot;
        g_AimbotAutoStop = config.aimbotAutoStop;
        g_AimbotAutoScope = config.aimbotAutoScope;
        g_AimbotPenetrationCheck = config.aimbotPenetrationCheck;
        g_AimbotFOV = config.aimbotFOV;
        g_AimbotSmooth = config.aimbotSmooth;
        g_AimbotHitchance = config.aimbotHitchance;
        g_AimbotMinDamage = config.aimbotMinDamage;
        g_AimbotBone = config.aimbotBone;
        g_AimbotRageSmooth = config.aimbotRageSmooth;
        g_RageAutoFire = config.rageAutoFire;
        g_LegitTriggerbot = config.legitTriggerbot;

        // Triggerbot
        g_TriggerbotEnabled = config.triggerbotEnabled;
        g_TriggerbotTeamCheck = config.triggerbotTeamCheck;
        g_TriggerbotDelay = config.triggerbotDelay;

        // BunnyHop
        g_BunnyHopEnabled = config.bhopEnabled;
        g_BunnyHopPerfectJumps = config.bhopPerfectJumps;

        // ESP
        g_EspEnabled = config.espEnabled;
        g_EspTeamCheck = config.espTeamCheck;
        g_RadarEnabled = config.radarEnabled;
        g_EspBoxType = config.espBoxType;
        g_EspHealth = config.espHealth;
        g_EspHealthNumber = config.espHealthNumber;
        g_EspHealthBar = config.espHealthBar;
        g_EspLine = config.espLine;
        g_EspAmmo = config.espAmmo;
        g_EspWeapon = config.espWeapon;
        g_EspName = config.espName;
        g_EspColorEnemy = config.espColorEnemy;
        g_EspColorTeam = config.espColorTeam;
        g_EspBoxThickness = config.espBoxThickness;
        g_EspBoxAlpha = config.espBoxAlpha;

        // Positions
        g_HealthPosition = config.healthPosition;
        g_AmmoPosition = config.ammoPosition;
        g_WeaponPosition = config.weaponPosition;
        g_NamePosition = config.namePosition;

        // Glow
        g_GlowEnabled = config.glowEnabled;
        g_GlowTeam = config.glowTeam;
        g_GlowColorEnemy = config.glowColorEnemy;
        g_GlowColorTeam = config.glowColorTeam;
        g_GlowAlpha = config.glowAlpha;
        g_GlowStrength = config.glowStrength;

        // Grenade Prediction
        g_GrenadePredictionEnabled = config.grenadePredictionEnabled;
        g_GrenadePredictionColor = config.grenadePredictionColor;
        g_GrenadePredictionShowTrajectory = config.grenadePredictionShowTrajectory;
        g_GrenadePredictionShowEndPoint = config.grenadePredictionShowEndPoint;

        // Penetration Crosshair
        g_PenetrationCrosshairEnabled = config.penetrationCrosshairEnabled;
        g_PenetrationCrosshairColor = config.penetrationCrosshairColor;
        g_PenetrationCrosshairShowDamage = config.penetrationCrosshairShowDamage;

        // Misc
        g_ThirdPersonEnabled = config.thirdPersonEnabled;
        g_SniperCrosshairEnabled = config.sniperCrosshairEnabled;
        g_FpsPingInfo = config.fpsPingInfo;
        g_NoFlashEnabled = config.noFlashEnabled;
        g_NoFlashStrength = config.noFlashStrength;
        g_UseCustomFOV = config.useCustomFOV;
        g_CustomFOV = config.customFOV;

        // Keys
        g_KeyMenu = config.keyMenu;
        g_KeyExit = config.keyExit;
        g_KeyThirdPerson = config.keyThirdPerson;
        g_TriggerbotKey = config.keyTrigger;
        g_BunnyHopKey = config.keyBhop;

        // Menu settings
        g_MenuSettings = config.menuSettings;
    }
}

void DeleteConfig(const std::string& name) {
    std::string configPath = g_ConfigDirectory + "\\" + name;
    DeleteFileA(configPath.c_str());
    RefreshConfigList();

    if (g_SelectedConfig >= (int)g_ConfigFiles.size()) {
        g_SelectedConfig = -1;
    }
}

// ================================
// === ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ===
// ================================

template<typename T>
T ReadMemory(DWORD addr) {
    T val;
    ReadProcessMemory(g_GameHandle, (LPCVOID)(void*)addr, &val, sizeof(T), NULL);
    return val;
}

template<typename T>
void WriteMemory(DWORD addr, T val) {
    WriteProcessMemory(g_GameHandle, (LPVOID)(void*)addr, &val, sizeof(T), NULL);
}

DWORD GetModuleBase(const char* name, DWORD pid) {
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    MODULEENTRY32 m = { sizeof(m) };
    DWORD res = 0;
    if (Module32First(h, &m)) do {
        if (_stricmp(m.szModule, name) == 0) {
            res = (DWORD)(uintptr_t)m.modBaseAddr;
            break;
        }
    } while (Module32Next(h, &m));
    CloseHandle(h);
    return res;
}

void NormalizeAngle(Vector3& angle) {
    while (angle.x > 180.0f) angle.x -= 360.0f;
    while (angle.x < -180.0f) angle.x += 360.0f;
    while (angle.y > 180.0f) angle.y -= 360.0f;
    while (angle.y < -180.0f) angle.y += 360.0f;
    angle.z = 0.0f;
}

void ClampAngle(Vector3& angle) {
    if (angle.x > 89.0f) angle.x = 89.0f;
    if (angle.x < -89.0f) angle.x = -89.0f;
    while (angle.y > 180.0f) angle.y -= 360.0f;
    while (angle.y < -180.0f) angle.y += 360.0f;
    angle.z = 0.0f;
}

Vector3 CalcAngle(const Vector3& source, const Vector3& destination) {
    Vector3 retAngle;
    Vector3 delta = destination - source;

    float hyp = sqrtf(delta.x * delta.x + delta.y * delta.y);
    retAngle.x = (float)(atan2(-delta.z, hyp) * (180.0f / 3.1415927f));
    retAngle.y = (float)(atan2(delta.y, delta.x) * (180.0f / 3.1415927f));
    retAngle.z = 0.0f;

    NormalizeAngle(retAngle);
    return retAngle;
}

bool WorldToScreen(Vector3 pos, Vector2& screen) {
    if (!g_ClientModule) return false;

    float viewMatrix[4][4];
    ReadProcessMemory(g_GameHandle, (LPCVOID)(void*)(g_ClientModule + dwViewMatrix),
        viewMatrix, sizeof(viewMatrix), NULL);

    float w = viewMatrix[3][0] * pos.x + viewMatrix[3][1] * pos.y + viewMatrix[3][2] * pos.z + viewMatrix[3][3];

    if (w < 0.01f)
        return false;

    float x = viewMatrix[0][0] * pos.x + viewMatrix[0][1] * pos.y + viewMatrix[0][2] * pos.z + viewMatrix[0][3];
    float y = viewMatrix[1][0] * pos.x + viewMatrix[1][1] * pos.y + viewMatrix[1][2] * pos.z + viewMatrix[1][3];

    int screenWidth, screenHeight;
    RECT desktop;
    GetWindowRect(GetDesktopWindow(), &desktop);
    screenWidth = desktop.right;
    screenHeight = desktop.bottom;

    screen.x = screenWidth * 0.5f * (1.0f + x / w);
    screen.y = screenHeight * 0.5f * (1.0f - y / w);

    return true;
}

Vector3 GetBonePos(DWORD ent, int bone) {
    Vector3 pos;
    DWORD boneMatrix = ReadMemory<DWORD>(ent + m_dwBoneMatrix);
    if (!boneMatrix) return pos;

    pos.x = ReadMemory<float>(boneMatrix + 0x30 * bone + 0x0C);
    pos.y = ReadMemory<float>(boneMatrix + 0x30 * bone + 0x1C);
    pos.z = ReadMemory<float>(boneMatrix + 0x30 * bone + 0x2C);
    return pos;
}

DWORD GetActiveWeapon(DWORD player) {
    if (!player) return 0;
    DWORD handle = ReadMemory<DWORD>(player + m_hActiveWeapon);
    if (!handle) return 0;
    return ReadMemory<DWORD>(g_ClientModule + dwEntityList + ((handle & 0xFFF) - 1) * 0x10);
}

void UpdateEntityCache() {
    DWORD currentTime = GetTickCount();
    if (currentTime - g_LastCacheTime < CACHE_INTERVAL)
        return;

    g_LastCacheTime = currentTime;
    g_CachedEntityCount = 0;

    if (!g_LocalPlayer) return;
    int localTeam = ReadMemory<int>(g_LocalPlayer + m_iTeamNum);

    for (int i = 1; i < 64; i++) {
        DWORD entity = ReadMemory<DWORD>(g_ClientModule + dwEntityList + i * 0x10);
        if (!entity) continue;

        if (ReadMemory<bool>(entity + m_bDormant)) continue;

        int entityHealth = ReadMemory<int>(entity + m_iHealth);
        if (entityHealth <= 0 || entityHealth > 100) continue;

        if (g_AimbotTeamCheck) {
            int entityTeam = ReadMemory<int>(entity + m_iTeamNum);
            if (entityTeam == localTeam) continue;
        }

        g_CachedEntities[g_CachedEntityCount++] = entity;
        if (g_CachedEntityCount >= 64) break;
    }
}

void UpdateLocalPlayer() {
    if (!g_ClientModule) return;
    g_LocalPlayer = ReadMemory<DWORD>(g_ClientModule + dwLocalPlayer);

    if (g_EngineModule) {
        g_ClientState = ReadMemory<DWORD>(g_EngineModule + dwClientState);
    }

    if (g_ClientModule && dwPlayerResource) {
        g_PlayerResource = ReadMemory<DWORD>(g_ClientModule + dwPlayerResource);
    }
}

Vector3 GetViewAngles() {
    if (!g_ClientState) return Vector3();
    return ReadMemory<Vector3>(g_ClientState + dwClientState_ViewAngles);
}

std::string GetPlayerName(DWORD player) {
    if (!g_ClientState || !player) return "Unknown";

    DWORD playerInfoManager = ReadMemory<DWORD>(g_ClientState + dwClientState_PlayerInfo);
    if (!playerInfoManager) return "Unknown";

    DWORD getPlayerInfo = ReadMemory<DWORD>(playerInfoManager + 0x40);
    if (!getPlayerInfo) return "Unknown";

    int playerIndex = ReadMemory<int>(player + 0x64);

    DWORD playerInfo = 0;
    __asm {
        push playerIndex
        mov ecx, playerInfoManager
        call getPlayerInfo
        mov playerInfo, eax
    }

    if (!playerInfo) return "Unknown";

    PlayerInfo info;
    ReadProcessMemory(g_GameHandle, (LPCVOID)playerInfo, &info, sizeof(PlayerInfo), NULL);

    return std::string(info.name);
}

// ================================
// === НОВЫЕ ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ===
// ================================

void UpdateThirdPerson() {
    if (!g_LocalPlayer) return;

    try {
        if (g_ThirdPersonEnabled) {
            // Включаем третье лицо
            WriteMemory<int>(g_LocalPlayer + m_iObserverMode, 1);
        }
        else {
            // Выключаем третье лицо
            WriteMemory<int>(g_LocalPlayer + m_iObserverMode, 0);
        }
    }
    catch (...) {
        // Игнорируем ошибки
    }
}

void UpdateSniperCrosshair() {
    if (!g_LocalPlayer) {
        g_SniperCrosshairActive = false;
        return;
    }

    DWORD weapon = GetActiveWeapon(g_LocalPlayer);
    if (!weapon) {
        g_SniperCrosshairActive = false;
        return;
    }

    try {
        int weaponID = ReadMemory<int>(weapon + m_iItemDefinitionIndex);
        bool isSniper = (weaponID == WEAPON_AWP || weaponID == WEAPON_SSG08 ||
            weaponID == WEAPON_SCAR20 || weaponID == WEAPON_G3SG1);

        g_SniperCrosshairActive = isSniper && g_SniperCrosshairEnabled;
    }
    catch (...) {
        g_SniperCrosshairActive = false;
    }
}

void DrawFpsPingInfo() {
    if (!g_FpsPingInfo || !g_LocalPlayer) return;

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    int screenWidth, screenHeight;
    RECT desktop;
    GetWindowRect(GetDesktopWindow(), &desktop);
    screenWidth = desktop.right;
    screenHeight = desktop.bottom;

    // FPS расчет
    static int frameCount = 0;
    static float lastTime = 0;
    static float fps = 0;

    frameCount++;
    float currentTime = GetTickCount() * 0.001f;
    if (currentTime - lastTime >= 1.0f) {
        fps = frameCount;
        frameCount = 0;
        lastTime = currentTime;
    }

    // Ping расчет
    int ping = 0;
    if (g_PlayerResource) {
        try {
            int localIndex = ReadMemory<int>(g_LocalPlayer + 0x64);
            ping = ReadMemory<int>(g_PlayerResource + m_iPing2 + (localIndex * 4));
        }
        catch (...) {
            ping = 0;
        }
    }

    char infoText[64];
    sprintf_s(infoText, "FPS: %.0f | Ping: %d", fps, ping);

    drawList->AddText(ImVec2(10, 10), ImColor(255, 255, 255, 255), infoText);
}

void DrawFOVCircle() {
    if (!g_DrawAimbotFOV || !g_AimbotEnabled) return;

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    int screenWidth, screenHeight;
    RECT desktop;
    GetWindowRect(GetDesktopWindow(), &desktop);
    screenWidth = desktop.right;
    screenHeight = desktop.bottom;

    ImVec2 center(screenWidth / 2.0f, screenHeight / 2.0f);
    float radius = (g_AimbotFOV * screenHeight) / (90.0f * 2.0f);

    drawList->AddCircle(center, radius, ImColor(g_AimbotFOVColor), 32, 1.0f);
}

void RenderESPSettingsPreview() {
    // Функция для предварительного просмотра настроек ESP (заглушка)
    // Может быть реализована позже
}

// ================================
// === GRENADE PREDICTION ===
// ================================

float DotProduct(const Vector3& a, const Vector3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

void SimulateGrenade(Vector3 pos, Vector3 vel, int type, std::vector<Vector3>& trajectory) {
    const float interval_per_tick = 0.015f;
    const float gravity = 800.0f;
    const float drag = 0.4f;

    trajectory.clear();
    trajectory.push_back(pos);

    for (int i = 0; i < 500; i++) {
        Vector3 lastPos = pos;

        // Apply gravity
        vel.z -= gravity * interval_per_tick;

        // Apply drag
        vel.x *= (1.0f - drag * interval_per_tick);
        vel.y *= (1.0f - drag * interval_per_tick);
        vel.z *= (1.0f - drag * interval_per_tick);

        // Move
        pos.x += vel.x * interval_per_tick;
        pos.y += vel.y * interval_per_tick;
        pos.z += vel.z * interval_per_tick;

        trajectory.push_back(pos);

        // Check for collision (simplified)
        if (pos.z < lastPos.z - 10.0f) {
            // Hit ground
            break;
        }
    }
}

void UpdateGrenadePrediction() {
    if (!g_GrenadePredictionEnabled || !g_LocalPlayer) {
        g_CurrentGrenade.predicted = false;
        return;
    }

    DWORD weapon = GetActiveWeapon(g_LocalPlayer);
    if (!weapon) {
        g_CurrentGrenade.predicted = false;
        return;
    }

    int weaponID = ReadMemory<int>(weapon + m_iItemDefinitionIndex);

    // Check if it's a grenade
    bool isGrenade = (weaponID == WEAPON_HEGRENADE || weaponID == WEAPON_FLASHBANG ||
        weaponID == WEAPON_SMOKEGRENADE || weaponID == WEAPON_MOLOTOV ||
        weaponID == WEAPON_DECOY || weaponID == WEAPON_INCGRENADE);

    if (!isGrenade) {
        g_CurrentGrenade.predicted = false;
        return;
    }

    bool pinPulled = false;
    try {
        pinPulled = ReadMemory<bool>(weapon + m_bPinPulled);
    }
    catch (...) {
        g_CurrentGrenade.predicted = false;
        return;
    }

    if (!pinPulled) {
        g_CurrentGrenade.predicted = false;
        return;
    }

    // Get throw parameters
    Vector3 eyePos;
    Vector3 viewAngles;

    try {
        eyePos = ReadMemory<Vector3>(g_LocalPlayer + m_vecOrigin) +
            ReadMemory<Vector3>(g_LocalPlayer + m_vecViewOffset);
        viewAngles = GetViewAngles();
    }
    catch (...) {
        g_CurrentGrenade.predicted = false;
        return;
    }

    // Convert angles to radians
    float pitch = viewAngles.x * 3.14159f / 180.0f;
    float yaw = viewAngles.y * 3.14159f / 180.0f;

    // Calculate throw direction
    Vector3 throwDir;
    throwDir.x = cos(pitch) * cos(yaw);
    throwDir.y = cos(pitch) * sin(yaw);
    throwDir.z = -sin(pitch);

    // Get throw strength
    float throwStrength = 1.0f;
    try {
        throwStrength = ReadMemory<float>(weapon + m_flThrowStrength);
    }
    catch (...) {
        throwStrength = 1.0f;
    }

    // Base velocity
    float baseVelocity = 750.0f;
    if (weaponID == WEAPON_HEGRENADE) baseVelocity = 750.0f;
    else if (weaponID == WEAPON_FLASHBANG) baseVelocity = 750.0f;
    else if (weaponID == WEAPON_SMOKEGRENADE) baseVelocity = 550.0f;
    else if (weaponID == WEAPON_MOLOTOV || weaponID == WEAPON_INCGRENADE) baseVelocity = 650.0f;
    else if (weaponID == WEAPON_DECOY) baseVelocity = 550.0f;

    // Calculate initial velocity
    Vector3 velocity = throwDir * (baseVelocity * throwStrength);

    // Add player velocity
    Vector3 playerVel;
    try {
        playerVel = ReadMemory<Vector3>(g_LocalPlayer + m_vecVelocity);
    }
    catch (...) {
        playerVel = Vector3();
    }

    velocity.x += playerVel.x * 0.25f;
    velocity.y += playerVel.y * 0.25f;
    velocity.z += playerVel.z * 0.25f;

    // Simulate grenade trajectory
    g_CurrentGrenade.start = eyePos;
    g_CurrentGrenade.velocity = velocity;
    g_CurrentGrenade.type = weaponID;
    g_CurrentGrenade.trajectory.clear();

    SimulateGrenade(eyePos, velocity, weaponID, g_CurrentGrenade.trajectory);

    if (g_CurrentGrenade.trajectory.size() > 1) {
        g_CurrentGrenade.endPoint = g_CurrentGrenade.trajectory.back();
        g_CurrentGrenade.predicted = true;
        g_CurrentGrenade.willExplode = true;
        g_CurrentGrenade.explodeTime = g_CurrentGrenade.trajectory.size() * 0.015f;
    }
}

void DrawGrenadePrediction() {
    if (!g_GrenadePredictionEnabled || !g_CurrentGrenade.predicted) return;

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    // Draw trajectory
    if (g_GrenadePredictionShowTrajectory && g_CurrentGrenade.trajectory.size() > 1) {
        for (size_t i = 1; i < g_CurrentGrenade.trajectory.size(); i++) {
            Vector2 screen1, screen2;
            if (WorldToScreen(g_CurrentGrenade.trajectory[i - 1], screen1) &&
                WorldToScreen(g_CurrentGrenade.trajectory[i], screen2)) {

                float alpha = 1.0f - (float)i / g_CurrentGrenade.trajectory.size();
                ImColor color = ImColor(
                    g_GrenadePredictionColor.x,
                    g_GrenadePredictionColor.y,
                    g_GrenadePredictionColor.z,
                    g_GrenadePredictionColor.w * alpha
                );

                drawList->AddLine(ImVec2(screen1.x, screen1.y),
                    ImVec2(screen2.x, screen2.y),
                    color, 2.0f);
            }
        }
    }

    // Draw end point
    if (g_GrenadePredictionShowEndPoint) {
        Vector2 screenPos;
        if (WorldToScreen(g_CurrentGrenade.endPoint, screenPos)) {
            // Draw circle at endpoint
            drawList->AddCircle(ImVec2(screenPos.x, screenPos.y),
                10.0f,
                ImColor(g_GrenadePredictionColor),
                12, 2.0f);

            // Draw cross
            drawList->AddLine(ImVec2(screenPos.x - 5, screenPos.y),
                ImVec2(screenPos.x + 5, screenPos.y),
                ImColor(g_GrenadePredictionColor), 2.0f);
            drawList->AddLine(ImVec2(screenPos.x, screenPos.y - 5),
                ImVec2(screenPos.x, screenPos.y + 5),
                ImColor(g_GrenadePredictionColor), 2.0f);

            // Draw grenade type indicator
            std::string grenadeType = "?";
            switch (g_CurrentGrenade.type) {
            case WEAPON_HEGRENADE: grenadeType = "HE"; break;
            case WEAPON_FLASHBANG: grenadeType = "FLASH"; break;
            case WEAPON_SMOKEGRENADE: grenadeType = "SMOKE"; break;
            case WEAPON_MOLOTOV: grenadeType = "MOLOTOV"; break;
            case WEAPON_INCGRENADE: grenadeType = "INCENDIARY"; break;
            case WEAPON_DECOY: grenadeType = "DECOY"; break;
            }

            drawList->AddText(ImVec2(screenPos.x + 15, screenPos.y - 10),
                ImColor(255, 255, 255, 255),
                grenadeType.c_str());
        }
    }
}

// ================================
// === PENETRATION CROSSHAIR ===
// ================================

float CalculateDamageThroughWall(Vector3 start, Vector3 end, DWORD weapon, int hitbox) {
    // Simplified damage calculation through walls
    // In reality, this would need proper penetration system

    if (!weapon) return 0.0f;

    int weaponID = ReadMemory<int>(weapon + m_iItemDefinitionIndex);

    // Base damages for different weapons
    float baseDamage = 0.0f;
    float penetrationPower = 0.0f;

    switch (weaponID) {
    case WEAPON_AK47: baseDamage = 36.0f; penetrationPower = 2.0f; break;
    case WEAPON_M4A1: baseDamage = 33.0f; penetrationPower = 2.0f; break;
    case WEAPON_M4A1_SILENCER: baseDamage = 33.0f; penetrationPower = 2.0f; break;
    case WEAPON_AWP: baseDamage = 115.0f; penetrationPower = 3.0f; break;
    case WEAPON_SSG08: baseDamage = 88.0f; penetrationPower = 2.5f; break;
    case WEAPON_SCAR20: baseDamage = 80.0f; penetrationPower = 2.5f; break;
    case WEAPON_G3SG1: baseDamage = 80.0f; penetrationPower = 2.5f; break;
    case WEAPON_AUG: baseDamage = 33.0f; penetrationPower = 2.0f; break;
    case WEAPON_SG553: baseDamage = 33.0f; penetrationPower = 2.0f; break;
    case WEAPON_FAMAS: baseDamage = 30.0f; penetrationPower = 1.8f; break;
    case WEAPON_GALILAR: baseDamage = 30.0f; penetrationPower = 1.8f; break;
    default: baseDamage = 30.0f; penetrationPower = 1.5f; break;
    }

    // Calculate distance
    float distance = start.DistTo(end);

    // Distance damage falloff
    float distanceModifier = 1.0f;
    if (distance > 500.0f) {
        distanceModifier = 500.0f / distance;
    }

    // Hitbox multiplier
    float hitboxMultiplier = 1.0f;
    if (hitbox == 8) hitboxMultiplier = 4.0f; // Head
    else if (hitbox == 6) hitboxMultiplier = 1.3f; // Chest
    else if (hitbox == 5) hitboxMultiplier = 1.0f; // Stomach
    else if (hitbox == 3 || hitbox == 4) hitboxMultiplier = 0.8f; // Arms

    // Wall penetration reduction
    float wallPenalty = 0.3f; // 70% damage reduction through walls

    return baseDamage * distanceModifier * hitboxMultiplier * wallPenalty * penetrationPower;
}

void UpdatePenetrationCrosshair() {
    if (!g_PenetrationCrosshairEnabled || !g_LocalPlayer) {
        g_PenetrationCrosshairDamage = 0.0f;
        return;
    }

    DWORD weapon = GetActiveWeapon(g_LocalPlayer);
    if (!weapon) {
        g_PenetrationCrosshairDamage = 0.0f;
        return;
    }

    int weaponID = ReadMemory<int>(weapon + m_iItemDefinitionIndex);

    // Only check for rifles and snipers
    bool isRifle = (weaponID == WEAPON_AK47 || weaponID == WEAPON_M4A1 ||
        weaponID == WEAPON_M4A1_SILENCER || weaponID == WEAPON_AUG ||
        weaponID == WEAPON_SG553 || weaponID == WEAPON_FAMAS ||
        weaponID == WEAPON_GALILAR);

    bool isSniper = (weaponID == WEAPON_AWP || weaponID == WEAPON_SSG08 ||
        weaponID == WEAPON_SCAR20 || weaponID == WEAPON_G3SG1);

    if (!isRifle && !isSniper) {
        g_PenetrationCrosshairDamage = 0.0f;
        return;
    }

    // Get view position and direction
    Vector3 eyePos;
    Vector3 viewAngles;

    try {
        eyePos = ReadMemory<Vector3>(g_LocalPlayer + m_vecOrigin) +
            ReadMemory<Vector3>(g_LocalPlayer + m_vecViewOffset);
        viewAngles = GetViewAngles();
    }
    catch (...) {
        g_PenetrationCrosshairDamage = 0.0f;
        return;
    }

    // Convert angles to direction vector
    float pitch = viewAngles.x * 3.14159f / 180.0f;
    float yaw = viewAngles.y * 3.14159f / 180.0f;

    Vector3 direction;
    direction.x = cos(pitch) * cos(yaw);
    direction.y = cos(pitch) * sin(yaw);
    direction.z = -sin(pitch);

    // Trace forward
    Vector3 endPoint = eyePos + direction * 8192.0f;

    // Find best target along the trace
    float bestDamage = 0.0f;

    for (int i = 1; i < 64; i++) {
        DWORD entity = ReadMemory<DWORD>(g_ClientModule + dwEntityList + i * 0x10);
        if (!entity) continue;

        if (entity == g_LocalPlayer) continue;

        if (ReadMemory<bool>(entity + m_bDormant)) continue;

        int entityHealth = ReadMemory<int>(entity + m_iHealth);
        if (entityHealth <= 0 || entityHealth > 100) continue;

        if (g_AimbotTeamCheck) {
            int entityTeam = ReadMemory<int>(entity + m_iTeamNum);
            int localTeam = ReadMemory<int>(g_LocalPlayer + m_iTeamNum);
            if (entityTeam == localTeam) continue;
        }

        // Get bone position (head)
        Vector3 bonePos = GetBonePos(entity, 8); // Head bone

        // Calculate if bone is in direction
        Vector3 toBone = bonePos - eyePos;
        Vector3 toBoneNormalized = toBone.Normalized();

        float dot = direction.Dot(toBoneNormalized);
        if (dot > 0.99f) { // Almost exactly in direction
            // Estimate wall thickness (simplified)
            float estimatedDistance = toBone.Length();
            float estimatedDamage = CalculateDamageThroughWall(eyePos, bonePos, weapon, 8);

            if (estimatedDamage > bestDamage) {
                bestDamage = estimatedDamage;
            }
        }
    }

    g_PenetrationCrosshairDamage = bestDamage;
}

void DrawPenetrationCrosshair() {
    if (!g_PenetrationCrosshairEnabled || g_PenetrationCrosshairDamage <= 0.0f) return;

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    int screenWidth, screenHeight;
    RECT desktop;
    GetWindowRect(GetDesktopWindow(), &desktop);
    screenWidth = desktop.right;
    screenHeight = desktop.bottom;

    ImVec2 center(screenWidth / 2.0f, screenHeight / 2.0f);

    // Draw penetration indicator around crosshair
    float indicatorSize = 20.0f;

    // Top line
    drawList->AddLine(
        ImVec2(center.x - indicatorSize, center.y - indicatorSize),
        ImVec2(center.x + indicatorSize, center.y - indicatorSize),
        ImColor(g_PenetrationCrosshairColor),
        2.0f
    );

    // Bottom line
    drawList->AddLine(
        ImVec2(center.x - indicatorSize, center.y + indicatorSize),
        ImVec2(center.x + indicatorSize, center.y + indicatorSize),
        ImColor(g_PenetrationCrosshairColor),
        2.0f
    );

    // Left line
    drawList->AddLine(
        ImVec2(center.x - indicatorSize, center.y - indicatorSize),
        ImVec2(center.x - indicatorSize, center.y + indicatorSize),
        ImColor(g_PenetrationCrosshairColor),
        2.0f
    );

    // Right line
    drawList->AddLine(
        ImVec2(center.x + indicatorSize, center.y - indicatorSize),
        ImVec2(center.x + indicatorSize, center.y + indicatorSize),
        ImColor(g_PenetrationCrosshairColor),
        2.0f
    );

    // Draw damage text
    if (g_PenetrationCrosshairShowDamage) {
        char damageText[32];
        sprintf(damageText, "%.0f", g_PenetrationCrosshairDamage);

        drawList->AddText(
            ImVec2(center.x + indicatorSize + 5, center.y - 10),
            ImColor(255, 255, 255, 255),
            damageText
        );
    }
}

// ================================
// === RENDER FUNCTIONS ===
// ================================

void RenderESPBox(ImDrawList* drawList, Vector2 top, Vector2 bottom, ImColor color, int boxType, float thickness, bool filled) {
    float height = bottom.y - top.y;
    float width = height * 0.4f;

    ImVec2 boxMin = ImVec2(bottom.x - width / 2, top.y);
    ImVec2 boxMax = ImVec2(bottom.x + width / 2, bottom.y);

    switch (boxType) {
    case ESPBOX_FULL:
        drawList->AddRect(boxMin, boxMax, color, 0.0f, 15, thickness);
        break;
    case ESPBOX_CORNER:
    {
        float cornerSize = width * 0.2f;
        drawList->AddLine(boxMin, ImVec2(boxMin.x + cornerSize, boxMin.y), color, thickness);
        drawList->AddLine(boxMin, ImVec2(boxMin.x, boxMin.y + cornerSize), color, thickness);
        drawList->AddLine(ImVec2(boxMax.x, boxMin.y), ImVec2(boxMax.x - cornerSize, boxMin.y), color, thickness);
        drawList->AddLine(ImVec2(boxMax.x, boxMin.y), ImVec2(boxMax.x, boxMin.y + cornerSize), color, thickness);
        drawList->AddLine(ImVec2(boxMin.x, boxMax.y), ImVec2(boxMin.x + cornerSize, boxMax.y), color, thickness);
        drawList->AddLine(ImVec2(boxMin.x, boxMax.y), ImVec2(boxMin.x, boxMax.y - cornerSize), color, thickness);
        drawList->AddLine(boxMax, ImVec2(boxMax.x - cornerSize, boxMax.y), color, thickness);
        drawList->AddLine(boxMax, ImVec2(boxMax.x, boxMax.y - cornerSize), color, thickness);
        break;
    }
    case ESPBOX_FILLED:
        drawList->AddRectFilled(boxMin, boxMax, ImColor(color.Value.x, color.Value.y, color.Value.z, g_EspBoxAlpha));
        break;
    }
}

void DrawHealthBar(ImDrawList* drawList, ImVec2 boxMin, ImVec2 boxMax, int health, ImColor color, int position, bool drawNumber = true) {
    float height = boxMax.y - boxMin.y;
    float width = boxMax.x - boxMin.x;

    float barWidth = 3.0f;
    float barHeight = height * (health / 100.0f);

    ImVec2 barStart, barEnd;

    switch (position) {
    case 0: // Left
        barStart = ImVec2(boxMin.x - barWidth - 10, boxMax.y - barHeight);
        barEnd = ImVec2(boxMin.x - 10, boxMax.y);
        break;
    case 1: // Right
        barStart = ImVec2(boxMax.x + 10, boxMax.y - barHeight);
        barEnd = ImVec2(boxMax.x + barWidth + 10, boxMax.y);
        break;
    case 2: // Top
        barStart = ImVec2(boxMin.x, boxMin.y - barWidth - 2);
        barEnd = ImVec2(boxMin.x + width * (health / 100.0f), boxMin.y - 2);
        break;
    case 3: // Bottom
        barStart = ImVec2(boxMin.x, boxMax.y + 2);
        barEnd = ImVec2(boxMin.x + width * (health / 100.0f), boxMax.y + barWidth + 2);
        break;
    }

    // Background
    ImVec2 bgStart, bgEnd;
    switch (position) {
    case 0: // Left
        bgStart = ImVec2(boxMin.x - barWidth - 10, boxMin.y);
        bgEnd = ImVec2(boxMin.x - 10, boxMax.y);
        break;
    case 1: // Right
        bgStart = ImVec2(boxMax.x + 10, boxMin.y);
        bgEnd = ImVec2(boxMax.x + barWidth + 10, boxMax.y);
        break;
    case 2: // Top
        bgStart = ImVec2(boxMin.x, boxMin.y - barWidth - 2);
        bgEnd = ImVec2(boxMax.x, boxMin.y - 2);
        break;
    case 3: // Bottom
        bgStart = ImVec2(boxMin.x, boxMax.y + 2);
        bgEnd = ImVec2(boxMax.x, boxMax.y + barWidth + 2);
        break;
    }

    drawList->AddRectFilled(bgStart, bgEnd, ImColor(0, 0, 0, 180));

    // Health fill
    drawList->AddRectFilled(barStart, barEnd, color);

    // Health number - рисуется только если включена опция И мы не рисуем отдельно
    if (drawNumber && g_EspHealthNumber && !g_EspHealthBar) {
        char healthText[8];
        sprintf(healthText, "%d", health);

        ImVec2 textPos;
        switch (position) {
        case 0: // Left
            textPos = ImVec2(barStart.x - 25, barStart.y - 8);
            break;
        case 1: // Right
            textPos = ImVec2(barEnd.x + 5, barStart.y - 8);
            break;
        case 2: // Top
            textPos = ImVec2(barStart.x + 5, barStart.y - 13);
            break;
        case 3: // Bottom
            textPos = ImVec2(barStart.x + 5, barEnd.y + 3);
            break;
        }

        drawList->AddText(textPos, ImColor(255, 255, 255, 255), healthText);
    }
}

void DrawHealthNumberOnly(ImDrawList* drawList, ImVec2 boxMin, ImVec2 boxMax, int health, ImColor color, int position) {
    if (!g_EspHealthNumber) return;

    char healthText[8];
    sprintf(healthText, "%d", health);

    float height = boxMax.y - boxMin.y;
    float width = boxMax.x - boxMin.x;
    float healthPercentage = health / 100.0f;

    ImVec2 textPos;
    switch (position) {
    case 0: // Left - исправлено: центрируем по вертикали
        textPos = ImVec2(boxMin.x - 30, boxMin.y + height / 2 - 8);
        break;
    case 1: // Right - исправлено: центрируем по вертикали
        textPos = ImVec2(boxMax.x + 15, boxMin.y + height / 2 - 8);
        break;
    case 2: // Top - исправлено: позиционируем над верхней границей
        textPos = ImVec2(boxMin.x + width / 2 - ImGui::CalcTextSize(healthText).x / 2, boxMin.y - 20);
        break;
    case 3: // Bottom - исправлено: позиционируем под нижней границей
        textPos = ImVec2(boxMin.x + width / 2 - ImGui::CalcTextSize(healthText).x / 2, boxMax.y + 5);
        break;
    }

    drawList->AddText(textPos, ImColor(255, 255, 255, 255), healthText);
}

void DrawPlayerInfo(ImDrawList* drawList, ImVec2 boxMin, ImVec2 boxMax, int health,
    const std::string& name, const std::string& weapon, int ammo,
    bool isEnemy, int healthPos, int ammoPos, int weaponPos, int namePos) {

    ImColor color = isEnemy ? ImColor(g_EspColorEnemy) : ImColor(g_EspColorTeam);
    float width = boxMax.x - boxMin.x;
    float height = boxMax.y - boxMin.y;

    // Draw name
    if (g_EspName && !name.empty()) {
        ImVec2 textPos;
        switch (namePos) {
        case 0: // Left
            textPos = ImVec2(boxMin.x - ImGui::CalcTextSize(name.c_str()).x - 5, boxMin.y);
            break;
        case 1: // Right
            textPos = ImVec2(boxMax.x + 5, boxMin.y);
            break;
        case 2: // Top
            textPos = ImVec2(boxMin.x + width / 2 - ImGui::CalcTextSize(name.c_str()).x / 2, boxMin.y - 15);
            break;
        case 3: // Bottom
            textPos = ImVec2(boxMin.x + width / 2 - ImGui::CalcTextSize(name.c_str()).x / 2, boxMax.y + 5);
            break;
        }
        drawList->AddText(textPos, color, name.c_str());
    }

    // Draw weapon
    if (g_EspWeapon && !weapon.empty()) {
        ImVec2 textPos;
        switch (weaponPos) {
        case 0: // Left
            textPos = ImVec2(boxMin.x - ImGui::CalcTextSize(weapon.c_str()).x - 5, boxMin.y + 15);
            break;
        case 1: // Right
            textPos = ImVec2(boxMax.x + 5, boxMin.y + 15);
            break;
        case 2: // Top
            textPos = ImVec2(boxMin.x + width / 2 - ImGui::CalcTextSize(weapon.c_str()).x / 2, boxMin.y - 30);
            break;
        case 3: // Bottom
            textPos = ImVec2(boxMin.x + width / 2 - ImGui::CalcTextSize(weapon.c_str()).x / 2, boxMax.y + 20);
            break;
        }
        drawList->AddText(textPos, color, weapon.c_str());
    }

    // Draw ammo - исправлено: проверяем корректность значения патронов
    if (g_EspAmmo && ammo >= 0 && ammo <= 100) {
        char ammoText[16];
        sprintf(ammoText, "%d", ammo);

        ImVec2 textPos;
        switch (ammoPos) {
        case 0: // Left
            textPos = ImVec2(boxMin.x - ImGui::CalcTextSize(ammoText).x - 5, boxMin.y + 30);
            break;
        case 1: // Right
            textPos = ImVec2(boxMax.x + 5, boxMin.y + 30);
            break;
        case 2: // Top
            textPos = ImVec2(boxMin.x + width / 2 - ImGui::CalcTextSize(ammoText).x / 2, boxMin.y - 45);
            break;
        case 3: // Bottom
            textPos = ImVec2(boxMin.x + width / 2 - ImGui::CalcTextSize(ammoText).x / 2, boxMax.y + 35);
            break;
        }
        drawList->AddText(textPos, color, ammoText);
    }
}

void DrawLineToPlayer(ImDrawList* drawList, Vector2 screenPos, ImColor color) {
    int screenWidth, screenHeight;
    RECT desktop;
    GetWindowRect(GetDesktopWindow(), &desktop);
    screenWidth = desktop.right;
    screenHeight = desktop.bottom;

    ImVec2 center(screenWidth / 2.0f, screenHeight / 2.0f);
    drawList->AddLine(center, ImVec2(screenPos.x, screenPos.y), color, 1.0f);
}

void RenderESP() {
    if (!g_EspEnabled || !g_LocalPlayer) return;

    int localTeam = ReadMemory<int>(g_LocalPlayer + m_iTeamNum);
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    for (int i = 1; i < 64; i++) {
        DWORD entity = 0;
        try {
            entity = ReadMemory<DWORD>(g_ClientModule + dwEntityList + i * 0x10);
        }
        catch (...) {
            continue;
        }

        if (!entity) continue;
        if (entity == g_LocalPlayer) continue;

        bool dormant = false;
        try {
            dormant = ReadMemory<bool>(entity + m_bDormant);
        }
        catch (...) {
            continue;
        }

        if (dormant) continue;

        int entityHealth = 0;
        try {
            entityHealth = ReadMemory<int>(entity + m_iHealth);
        }
        catch (...) {
            continue;
        }

        if (entityHealth <= 0 || entityHealth > 100) continue;

        int entityTeam = 0;
        try {
            entityTeam = ReadMemory<int>(entity + m_iTeamNum);
        }
        catch (...) {
            continue;
        }

        if (g_EspTeamCheck && entityTeam == localTeam) continue;

        Vector3 origin;
        try {
            origin = ReadMemory<Vector3>(entity + m_vecOrigin);
        }
        catch (...) {
            continue;
        }

        Vector3 head = origin + Vector3(0, 0, 72.0f);

        Vector2 screenBottom, screenTop;
        if (!WorldToScreen(origin, screenBottom) || !WorldToScreen(head, screenTop)) continue;

        ImColor color = (entityTeam != localTeam) ?
            ImColor(g_EspColorEnemy) : ImColor(g_EspColorTeam);

        // Calculate box dimensions
        float height = screenBottom.y - screenTop.y;
        float width = height * 0.4f;

        ImVec2 boxMin = ImVec2(screenBottom.x - width / 2, screenTop.y);
        ImVec2 boxMax = ImVec2(screenBottom.x + width / 2, screenBottom.y);

        // Box
        if (g_EspBoxType != ESPBOX_NONE) {
            RenderESPBox(drawList, screenTop, screenBottom, color, g_EspBoxType, g_EspBoxThickness, false);
        }

        // Health
        if (g_EspHealth) {
            if (g_EspHealthBar) {
                DrawHealthBar(drawList, boxMin, boxMax, entityHealth, color, g_HealthPosition, false);
            }

            // Цифры здоровья рисуются отдельно, только если включена опция
            if (g_EspHealthNumber) {
                DrawHealthNumberOnly(drawList, boxMin, boxMax, entityHealth, color, g_HealthPosition);
            }
        }

        // Line
        if (g_EspLine) {
            DrawLineToPlayer(drawList, screenBottom, color);
        }

        // Get weapon info
        DWORD weapon = GetActiveWeapon(entity);
        std::string weaponName = "Unknown";
        int ammo = 0;

        if (weapon) {
            try {
                int weaponID = ReadMemory<int>(weapon + m_iItemDefinitionIndex);
                if (weaponNames.find(weaponID) != weaponNames.end()) {
                    weaponName = weaponNames[weaponID];
                }

                // Исправлено: проверяем, что оружие использует патроны
                bool usesAmmo = (weaponID != WEAPON_KNIFE && weaponID != WEAPON_KNIFE_T &&
                    weaponID != WEAPON_KNIFE_BAYONET && weaponID != WEAPON_KNIFE_FLIP &&
                    weaponID != WEAPON_KNIFE_GUT && weaponID != WEAPON_KNIFE_KARAMBIT &&
                    weaponID != WEAPON_KNIFE_M9_BAYONET && weaponID != WEAPON_KNIFE_TACTICAL &&
                    weaponID != WEAPON_KNIFE_FALCHION && weaponID != WEAPON_KNIFE_SURVIVAL_BOWIE &&
                    weaponID != WEAPON_KNIFE_BUTTERFLY && weaponID != WEAPON_KNIFE_PUSH &&
                    weaponID != WEAPON_TASER && weaponID != WEAPON_C4);

                if (usesAmmo) {
                    ammo = ReadMemory<int>(weapon + m_iClip1);
                    // Исправлено: если патроны некорректные, не отображаем
                    if (ammo < 0 || ammo > 100) {
                        ammo = 0;
                    }
                }
            }
            catch (...) {
                // Игнорируем ошибки
            }
        }

        // Get player name
        std::string playerName = "Player";
        try {
            playerName = GetPlayerName(entity);
        }
        catch (...) {
            playerName = "Player";
        }

        // Draw info
        DrawPlayerInfo(drawList, boxMin, boxMax, entityHealth, playerName,
            weaponName, ammo, entityTeam != localTeam,
            g_HealthPosition, g_AmmoPosition, g_WeaponPosition, g_NamePosition);
    }
}

// ================================
// === GLOW FUNCTIONS ===
// ================================

void ApplyGlow() {
    if (!g_GlowEnabled || !g_LocalPlayer) return;

    DWORD glowManager = 0;
    try {
        glowManager = ReadMemory<DWORD>(g_ClientModule + dwGlowObjectManager);
    }
    catch (...) {
        return;
    }

    if (!glowManager) return;

    int localTeam = ReadMemory<int>(g_LocalPlayer + m_iTeamNum);

    for (int i = 1; i < 64; i++) {
        DWORD entity = 0;
        try {
            entity = ReadMemory<DWORD>(g_ClientModule + dwEntityList + i * 0x10);
        }
        catch (...) {
            continue;
        }

        if (!entity) continue;

        bool dormant = false;
        try {
            dormant = ReadMemory<bool>(entity + m_bDormant);
        }
        catch (...) {
            continue;
        }

        if (dormant) continue;

        int entityHealth = 0;
        try {
            entityHealth = ReadMemory<int>(entity + m_iHealth);
        }
        catch (...) {
            continue;
        }

        if (entityHealth <= 0 || entityHealth > 100) continue;

        int entityTeam = 0;
        try {
            entityTeam = ReadMemory<int>(entity + m_iTeamNum);
        }
        catch (...) {
            continue;
        }

        bool applyGlow = false;
        ImVec4 glowColor;

        if (entityTeam != localTeam) {
            applyGlow = true;
            glowColor = g_GlowColorEnemy;
        }
        else if (g_GlowTeam) {
            applyGlow = true;
            glowColor = g_GlowColorTeam;
        }

        if (applyGlow) {
            int glowIndex = 0;
            try {
                glowIndex = ReadMemory<int>(entity + m_iGlowIndex);
            }
            catch (...) {
                continue;
            }

            if (glowIndex >= 0 && glowIndex < 1024) {
                DWORD glowAddress = glowManager + (glowIndex * 0x38);

                try {
                    WriteMemory<float>(glowAddress + 0x8, glowColor.x * g_GlowStrength);
                    WriteMemory<float>(glowAddress + 0xC, glowColor.y * g_GlowStrength);
                    WriteMemory<float>(glowAddress + 0x10, glowColor.z * g_GlowStrength);
                    WriteMemory<float>(glowAddress + 0x14, g_GlowAlpha);
                    WriteMemory<bool>(glowAddress + 0x28, true);
                    WriteMemory<bool>(glowAddress + 0x24, true);
                }
                catch (...) {
                    // Игнорируем ошибки
                }
            }
        }
    }
}

// ================================
// === AIMBOT FUNCTIONS ===
// ================================

void RunLegitAimbot() {
    if (!g_AimbotEnabled || g_AimbotMode != 0 || !g_LocalPlayer || !g_ClientState) return;

    int localHealth = 0;
    try {
        localHealth = ReadMemory<int>(g_LocalPlayer + m_iHealth);
    }
    catch (...) {
        return;
    }

    if (localHealth <= 0) return;

    DWORD currentTime = GetTickCount();
    if (g_ReactionTime > 0 && (currentTime - g_LastShotTime) < g_ReactionTime)
        return;

    UpdateEntityCache();

    Vector3 localEyePos;
    try {
        localEyePos = ReadMemory<Vector3>(g_LocalPlayer + m_vecOrigin) +
            ReadMemory<Vector3>(g_LocalPlayer + m_vecViewOffset);
    }
    catch (...) {
        return;
    }

    Vector3 currentAngle = GetViewAngles();

    float bestFov = g_AimbotFOV;
    if (bestFov > 180.0f) bestFov = 180.0f;
    if (bestFov < 0.1f) bestFov = 0.1f;

    Vector3 bestAngle = currentAngle;
    DWORD bestTarget = 0;
    float bestDistance = FLT_MAX;
    Vector3 bestBonePos = { 0, 0, 0 };

    for (int i = 0; i < g_CachedEntityCount; i++) {
        DWORD entity = g_CachedEntities[i];
        if (!entity) continue;

        if (g_AimbotTeamCheck) {
            int entityTeam = 0;
            int localTeam = 0;
            try {
                entityTeam = ReadMemory<int>(entity + m_iTeamNum);
                localTeam = ReadMemory<int>(g_LocalPlayer + m_iTeamNum);
            }
            catch (...) {
                continue;
            }

            if (entityTeam == localTeam) continue;
        }

        if (g_AimbotVisibilityCheck) {
            bool spotted = false;
            try {
                spotted = ReadMemory<bool>(entity + m_bSpotted);
            }
            catch (...) {
                continue;
            }

            if (!spotted) continue;
        }

        Vector3 bonePos = GetBonePos(entity, g_AimbotBone);
        if (bonePos.x == 0 && bonePos.y == 0 && bonePos.z == 0) {
            int altBones[] = { 6, 5, 4, 3 };
            for (int j = 0; j < 4; j++) {
                bonePos = GetBonePos(entity, altBones[j]);
                if (bonePos.x != 0 || bonePos.y != 0 || bonePos.z != 0)
                    break;
            }
        }

        Vector2 screenPos;
        if (!WorldToScreen(bonePos, screenPos))
            continue;

        Vector3 angleToTarget = CalcAngle(localEyePos, bonePos);
        Vector3 deltaAngle = angleToTarget - currentAngle;
        NormalizeAngle(deltaAngle);

        float angleFOV = sqrtf(deltaAngle.x * deltaAngle.x + deltaAngle.y * deltaAngle.y);
        float distance = localEyePos.DistTo(bonePos);

        if (angleFOV < bestFov && distance < 8192.0f) {
            bestFov = angleFOV;
            bestAngle = angleToTarget;
            bestTarget = entity;
            bestDistance = distance;
            bestBonePos = bonePos;
        }
    }

    if (bestTarget && bestAngle.x != currentAngle.x) {
        Vector3 aimPunch;
        try {
            aimPunch = ReadMemory<Vector3>(g_LocalPlayer + m_aimPunchAngle);
        }
        catch (...) {
            return;
        }

        aimPunch = aimPunch * 2.0f;

        Vector3 finalAngle = bestAngle - aimPunch;

        if (g_AimbotSmooth > 1.0f) {
            Vector3 delta = finalAngle - currentAngle;
            NormalizeAngle(delta);

            finalAngle.x = currentAngle.x + delta.x / g_AimbotSmooth;
            finalAngle.y = currentAngle.y + delta.y / g_AimbotSmooth;
            finalAngle.z = 0;

            ClampAngle(finalAngle);
        }

        ClampAngle(finalAngle);

        try {
            WriteMemory<Vector3>(g_ClientState + dwClientState_ViewAngles, finalAngle);
        }
        catch (...) {
            return;
        }

        // Исправлено: Legit триггербот
        if ((g_AimbotAutoShoot || g_LegitTriggerbot) && bestFov < g_AimbotFOV * 0.5f) {
            try {
                WriteMemory<DWORD>(g_ClientModule + dwForceAttack, 6);
                Sleep(10);
                WriteMemory<DWORD>(g_ClientModule + dwForceAttack, 4);
            }
            catch (...) {
                // Игнорируем ошибки
            }
        }

        g_LastShotTime = currentTime;
    }
}

// Новое: Rage аимбот
void RunRageAimbot() {
    if (!g_AimbotEnabled || g_AimbotMode != 1 || !g_LocalPlayer || !g_ClientState) return;

    int localHealth = 0;
    try {
        localHealth = ReadMemory<int>(g_LocalPlayer + m_iHealth);
    }
    catch (...) {
        return;
    }

    if (localHealth <= 0) return;

    DWORD currentTime = GetTickCount();
    if (g_ReactionTime > 0 && (currentTime - g_LastShotTime) < g_ReactionTime)
        return;

    UpdateEntityCache();

    Vector3 localEyePos;
    try {
        localEyePos = ReadMemory<Vector3>(g_LocalPlayer + m_vecOrigin) +
            ReadMemory<Vector3>(g_LocalPlayer + m_vecViewOffset);
    }
    catch (...) {
        return;
    }

    Vector3 currentAngle = GetViewAngles();

    float bestFov = g_AimbotFOV;
    Vector3 bestAngle = currentAngle;
    DWORD bestTarget = 0;
    float bestDistance = FLT_MAX;

    for (int i = 0; i < g_CachedEntityCount; i++) {
        DWORD entity = g_CachedEntities[i];
        if (!entity) continue;

        if (g_AimbotTeamCheck) {
            int entityTeam = 0;
            int localTeam = 0;
            try {
                entityTeam = ReadMemory<int>(entity + m_iTeamNum);
                localTeam = ReadMemory<int>(g_LocalPlayer + m_iTeamNum);
            }
            catch (...) {
                continue;
            }

            if (entityTeam == localTeam) continue;
        }

        Vector3 bonePos = GetBonePos(entity, g_AimbotBone);
        if (bonePos.x == 0 && bonePos.y == 0 && bonePos.z == 0) continue;

        Vector3 angleToTarget = CalcAngle(localEyePos, bonePos);
        Vector3 deltaAngle = angleToTarget - currentAngle;
        NormalizeAngle(deltaAngle);

        float angleFOV = sqrtf(deltaAngle.x * deltaAngle.x + deltaAngle.y * deltaAngle.y);
        float distance = localEyePos.DistTo(bonePos);

        if (angleFOV < bestFov && distance < 8192.0f) {
            bestFov = angleFOV;
            bestAngle = angleToTarget;
            bestTarget = entity;
            bestDistance = distance;
        }
    }

    if (bestTarget) {
        Vector3 aimPunch = ReadMemory<Vector3>(g_LocalPlayer + m_aimPunchAngle);
        aimPunch = aimPunch * 2.0f;

        Vector3 finalAngle = bestAngle - aimPunch;

        if (g_AimbotRageSmooth > 1.0f) {
            Vector3 delta = finalAngle - currentAngle;
            NormalizeAngle(delta);

            finalAngle.x = currentAngle.x + delta.x / g_AimbotRageSmooth;
            finalAngle.y = currentAngle.y + delta.y / g_AimbotRageSmooth;
            finalAngle.z = 0;

            ClampAngle(finalAngle);
        }

        ClampAngle(finalAngle);

        try {
            WriteMemory<Vector3>(g_ClientState + dwClientState_ViewAngles, finalAngle);
        }
        catch (...) {
            return;
        }

        // Исправлено: Rage автоогонь
        if (g_RageAutoFire && bestFov < g_AimbotFOV * 0.5f) {
            try {
                WriteMemory<DWORD>(g_ClientModule + dwForceAttack, 6);
                Sleep(10);
                WriteMemory<DWORD>(g_ClientModule + dwForceAttack, 4);
            }
            catch (...) {
                // Игнорируем ошибки
            }
        }

        g_LastShotTime = currentTime;
    }
}

void RunTriggerBot() {
    if (!g_TriggerbotEnabled || !g_LocalPlayer) return;

    if (!(GetAsyncKeyState(g_TriggerbotKey) & 0x8000)) return;

    int crosshairId = 0;
    try {
        crosshairId = ReadMemory<int>(g_LocalPlayer + m_iCrosshairId);
    }
    catch (...) {
        return;
    }

    if (crosshairId <= 0 || crosshairId > 64) return;

    DWORD entity = 0;
    try {
        entity = ReadMemory<DWORD>(g_ClientModule + dwEntityList + (crosshairId - 1) * 0x10);
    }
    catch (...) {
        return;
    }

    if (!entity) return;

    int entityTeam = 0;
    int localTeam = 0;
    int entityHealth = 0;
    try {
        entityTeam = ReadMemory<int>(entity + m_iTeamNum);
        localTeam = ReadMemory<int>(g_LocalPlayer + m_iTeamNum);
        entityHealth = ReadMemory<int>(entity + m_iHealth);
    }
    catch (...) {
        return;
    }

    if (g_TriggerbotTeamCheck && entityTeam == localTeam) return;

    if (entityHealth > 0 && entityHealth <= 100) {
        if (g_TriggerbotDelay > 0) {
            Sleep((DWORD)(g_TriggerbotDelay * 10));
        }

        try {
            WriteMemory<DWORD>(g_ClientModule + dwForceAttack, 6);
            Sleep(10);
            WriteMemory<DWORD>(g_ClientModule + dwForceAttack, 4);
        }
        catch (...) {
            // Игнорируем ошибки
        }
    }
}

// ================================
// === IMGUI МЕНЮ ===
// ================================

void ApplyMenuSettings() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = g_MenuSettings.windowBgColor;
    style.Colors[ImGuiCol_TitleBg] = g_MenuSettings.windowTitleColor;
    style.Colors[ImGuiCol_TitleBgActive] = g_MenuSettings.windowTitleColor;
    style.Colors[ImGuiCol_Border] = g_MenuSettings.windowBorderColor;
    style.WindowRounding = g_MenuSettings.windowRounding;
    style.WindowBorderSize = g_MenuSettings.windowBorderSize;

    style.Colors[ImGuiCol_Tab] = g_MenuSettings.tabInactiveColor;
    style.Colors[ImGuiCol_TabActive] = g_MenuSettings.tabActiveColor;
    style.Colors[ImGuiCol_TabHovered] = g_MenuSettings.tabHoveredColor;
    style.TabRounding = g_MenuSettings.tabRounding;

    style.Colors[ImGuiCol_Button] = g_MenuSettings.buttonColor;
    style.Colors[ImGuiCol_ButtonHovered] = g_MenuSettings.buttonHoveredColor;
    style.Colors[ImGuiCol_ButtonActive] = g_MenuSettings.buttonActiveColor;
    style.FrameRounding = g_MenuSettings.buttonRounding;

    style.Colors[ImGuiCol_CheckMark] = g_MenuSettings.checkboxActiveColor;
    style.Colors[ImGuiCol_FrameBg] = g_MenuSettings.checkboxColor;

    style.Colors[ImGuiCol_SliderGrab] = g_MenuSettings.sliderGrabColor;
    style.Colors[ImGuiCol_SliderGrabActive] = g_MenuSettings.sliderGrabActiveColor;

    style.Colors[ImGuiCol_Text] = g_MenuSettings.textColor;
    style.Colors[ImGuiCol_TextDisabled] = g_MenuSettings.textDisabledColor;

    style.Colors[ImGuiCol_FrameBg] = g_MenuSettings.inputBgColor;
    style.Colors[ImGuiCol_Border] = g_MenuSettings.inputBorderColor;
}

void ResetMenuSettings() {
    g_MenuSettings = MenuSettings(); // Сброс к значениям по умолчанию
    ApplyMenuSettings();
}

void RenderMenu() {
    if (!g_ShowMenu) return;

    if (g_MenuCollapsed) {
        ImGui::SetNextWindowSize(ImVec2(200, 40), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(g_MenuSettings.menuX, g_MenuSettings.menuY), ImGuiCond_FirstUseEver);
        ImGui::Begin("Mesens", NULL,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

        if (ImGui::Button("Expand Menu", ImVec2(180, 30))) {
            g_MenuCollapsed = false;
        }

        ImGui::End();
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(g_MenuSettings.windowWidth, g_MenuSettings.windowHeight), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(g_MenuSettings.menuX, g_MenuSettings.menuY), ImGuiCond_FirstUseEver);
    ImGui::Begin("Mesens | Build 0303", NULL,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

    if (ImGui::BeginTabBar("MainTabs")) {
        // ESP Tab
        if (ImGui::BeginTabItem("ESP Settings")) {
            ImGui::Columns(2, "esp_settings");

            // Left column - Settings
            ImGui::SetColumnWidth(0, 400);
            ImGui::BeginChild("Settings", ImVec2(0, 0), true);

            ImGui::Checkbox("Enable ESP", &g_EspEnabled);
            ImGui::Checkbox("Team Check", &g_EspTeamCheck);
            ImGui::Checkbox("Radar Hack", &g_RadarEnabled);

            ImGui::Separator();

            // Box settings
            const char* boxTypes[] = { "None", "Full Box", "Corner Box", "Filled Box" };
            ImGui::Combo("Box Type", &g_EspBoxType, boxTypes, IM_ARRAYSIZE(boxTypes));

            if (g_EspBoxType != ESPBOX_NONE) {
                ImGui::SliderFloat("Box Thickness", &g_EspBoxThickness, 0.5f, 5.0f, "%.1f");
                ImGui::SliderFloat("Box Alpha", &g_EspBoxAlpha, 0.1f, 1.0f, "%.2f");
            }

            ImGui::Separator();

            // Health settings
            ImGui::Checkbox("Show Health", &g_EspHealth);
            if (g_EspHealth) {
                ImGui::Checkbox("Health Bar", &g_EspHealthBar);
                ImGui::Checkbox("Health Number", &g_EspHealthNumber);

                const char* healthPositions[] = { "Left", "Right", "Top", "Bottom" };
                ImGui::Combo("Health Position", &g_HealthPosition, healthPositions, IM_ARRAYSIZE(healthPositions));
            }

            ImGui::Separator();

            // Other ESP elements
            ImGui::Checkbox("Show Name", &g_EspName);
            if (g_EspName) {
                const char* namePositions[] = { "Left", "Right", "Top", "Bottom" };
                ImGui::Combo("Name Position", &g_NamePosition, namePositions, IM_ARRAYSIZE(namePositions));
            }

            ImGui::Checkbox("Show Weapon", &g_EspWeapon);
            if (g_EspWeapon) {
                const char* weaponPositions[] = { "Left", "Right", "Top", "Bottom" };
                ImGui::Combo("Weapon Position", &g_WeaponPosition, weaponPositions, IM_ARRAYSIZE(weaponPositions));
            }

            ImGui::Checkbox("Show Ammo", &g_EspAmmo);
            if (g_EspAmmo) {
                const char* ammoPositions[] = { "Left", "Right", "Top", "Bottom" };
                ImGui::Combo("Ammo Position", &g_AmmoPosition, ammoPositions, IM_ARRAYSIZE(ammoPositions));
            }

            ImGui::Checkbox("Show Line", &g_EspLine);

            ImGui::Separator();

            // Colors
            ImGui::ColorEdit3("Enemy Color", (float*)&g_EspColorEnemy);
            ImGui::ColorEdit3("Team Color", (float*)&g_EspColorTeam);

            ImGui::EndChild();

            // Right column - Preview
            ImGui::NextColumn();
            ImGui::BeginChild("Preview", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar);

            ImGui::Text("ESP Preview");
            ImGui::Separator();

            // Show preview
            // RenderESPSettingsPreview(); // Temporarily commented

            ImGui::Text("\nPreview shows:");
            ImGui::Text("- Box type: %s", boxTypes[g_EspBoxType]);
            ImGui::Text("- Health bar: %s", g_EspHealthBar ? "On" : "Off");
            ImGui::Text("- Health number: %s", g_EspHealthNumber ? "On" : "Off");
            ImGui::Text("- Health position: %s",
                g_HealthPosition == 0 ? "Left" :
                g_HealthPosition == 1 ? "Right" :
                g_HealthPosition == 2 ? "Top" : "Bottom");
            ImGui::Text("- Name position: %s",
                g_NamePosition == 0 ? "Left" :
                g_NamePosition == 1 ? "Right" :
                g_NamePosition == 2 ? "Top" : "Bottom");
            ImGui::Text("- Weapon position: %s",
                g_WeaponPosition == 0 ? "Left" :
                g_WeaponPosition == 1 ? "Right" :
                g_WeaponPosition == 2 ? "Top" : "Bottom");

            ImGui::EndChild();

            ImGui::Columns(1);
            ImGui::EndTabItem();
        }

        // Glow Tab
        if (ImGui::BeginTabItem("Glow")) {
            ImGui::Checkbox("Enable Glow", &g_GlowEnabled);
            ImGui::Checkbox("Show Team Glow", &g_GlowTeam);

            ImGui::Separator();

            ImGui::Text("Glow Settings:");
            ImGui::SliderFloat("Glow Alpha", &g_GlowAlpha, 0.1f, 1.0f, "%.2f");
            ImGui::SliderFloat("Glow Strength", &g_GlowStrength, 0.1f, 2.0f, "%.1f");

            ImGui::Separator();

            ImGui::Text("Glow Colors:");
            ImGui::ColorEdit3("Enemy Glow Color", (float*)&g_GlowColorEnemy);
            ImGui::ColorEdit3("Team Glow Color", (float*)&g_GlowColorTeam);

            ImGui::Separator();

            ImGui::Text("Note: Glow works through walls");
            ImGui::Text("Adjust strength for brightness control");

            ImGui::EndTabItem();
        }

        // Aimbot Tab
        if (ImGui::BeginTabItem("AimBot")) {
            ImGui::Checkbox("Enable AimBot", &g_AimbotEnabled);
            ImGui::SameLine(200);
            ImGui::Checkbox("Team Check", &g_AimbotTeamCheck);
            ImGui::SameLine(400);
            ImGui::Checkbox("Draw FOV Circle", &g_DrawAimbotFOV);

            if (g_DrawAimbotFOV) {
                ImGui::SameLine();
                ImGui::ColorEdit4("##FOVColor", (float*)&g_AimbotFOVColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
            }

            ImGui::Separator();

            ImGui::Text("AimBot Mode:");
            if (ImGui::RadioButton("Legit", g_AimbotMode == 0)) g_AimbotMode = 0;
            ImGui::SameLine();
            if (ImGui::RadioButton("Rage", g_AimbotMode == 1)) g_AimbotMode = 1;

            ImGui::Separator();

            if (g_AimbotMode == 0) {
                ImGui::Text("Legit AimBot Settings:");
                ImGui::SliderFloat("FOV", &g_AimbotFOV, 0.1f, 30.0f, "%.1f");
                ImGui::SliderFloat("Smooth", &g_AimbotSmooth, 1.0f, 20.0f, "%.1f");
                ImGui::Checkbox("Auto Shoot", &g_AimbotAutoShoot);
                ImGui::Checkbox("Legit Triggerbot", &g_LegitTriggerbot);
                ImGui::Checkbox("Visibility Check", &g_AimbotVisibilityCheck);
            }
            else {
                ImGui::Text("Rage AimBot Settings:");
                ImGui::SliderFloat("FOV", &g_AimbotFOV, 0.1f, 180.0f, "%.1f");
                ImGui::SliderFloat("Smooth", &g_AimbotRageSmooth, 1.0f, 20.0f, "%.1f");
                ImGui::SliderFloat("Hitchance %", &g_AimbotHitchance, 0.0f, 100.0f, "%.0f");
                ImGui::SliderInt("Min Damage", &g_AimbotMinDamage, 1, 100);
                ImGui::Checkbox("Auto Fire", &g_RageAutoFire);
            }

            ImGui::EndTabItem();
        }

        // Grenade Prediction Tab
        if (ImGui::BeginTabItem("Grenade Prediction")) {
            ImGui::Checkbox("Enable Grenade Prediction", &g_GrenadePredictionEnabled);

            ImGui::Separator();

            ImGui::Text("Prediction Settings:");
            ImGui::Checkbox("Show Trajectory", &g_GrenadePredictionShowTrajectory);
            ImGui::Checkbox("Show End Point", &g_GrenadePredictionShowEndPoint);

            ImGui::Separator();

            ImGui::Text("Visual Settings:");
            ImGui::ColorEdit3("Prediction Color", (float*)&g_GrenadePredictionColor);

            ImGui::Separator();

            ImGui::Text("Supported Grenades:");
            ImGui::BulletText("HE Grenade");
            ImGui::BulletText("Flashbang");
            ImGui::BulletText("Smoke Grenade");
            ImGui::BulletText("Molotov/Incendiary");
            ImGui::BulletText("Decoy");

            ImGui::Separator();

            ImGui::Text("How to use:");
            ImGui::BulletText("Equip a grenade");
            ImGui::BulletText("Pull the pin (hold left click)");
            ImGui::BulletText("See the predicted trajectory");

            ImGui::EndTabItem();
        }

        // Penetration Crosshair Tab
        if (ImGui::BeginTabItem("Penetration Crosshair")) {
            ImGui::Checkbox("Enable Penetration Crosshair", &g_PenetrationCrosshairEnabled);

            ImGui::Separator();

            ImGui::Text("Display Settings:");
            ImGui::Checkbox("Show Damage Value", &g_PenetrationCrosshairShowDamage);

            ImGui::Separator();

            ImGui::Text("Visual Settings:");
            ImGui::ColorEdit3("Crosshair Color", (float*)&g_PenetrationCrosshairColor);

            ImGui::Separator();

            ImGui::Text("Supported Weapons:");
            ImGui::BulletText("All rifles (AK-47, M4A4, etc.)");
            ImGui::BulletText("All snipers (AWP, SSG08, etc.)");
            ImGui::BulletText("Some SMGs (with penetration)");

            ImGui::Separator();

            ImGui::Text("How it works:");
            ImGui::BulletText("Shows when you can wallbang enemies");
            ImGui::BulletText("Displays estimated damage through walls");
            ImGui::BulletText("Works with all wallbangable surfaces");

            ImGui::EndTabItem();
        }

        // Config Tab
        if (ImGui::BeginTabItem("Config")) {
            ImGui::Text("Config Management");
            ImGui::Separator();

            // Save new config
            ImGui::InputText("Config Name", g_NewConfigName, IM_ARRAYSIZE(g_NewConfigName));
            if (ImGui::Button("Save New Config")) {
                if (strlen(g_NewConfigName) > 0) {
                    std::string configName = g_NewConfigName;
                    if (configName.find(".cfg") == std::string::npos) {
                        configName += ".cfg";
                    }
                    SaveConfig(configName);
                    memset(g_NewConfigName, 0, sizeof(g_NewConfigName));
                    RefreshConfigList();
                }
            }

            ImGui::Separator();
            ImGui::Text("Saved Configs:");

            // List configs
            ImGui::BeginChild("ConfigList", ImVec2(0, 300), true);
            for (int i = 0; i < g_ConfigFiles.size(); i++) {
                bool isSelected = (g_SelectedConfig == i);
                if (ImGui::Selectable(g_ConfigFiles[i].c_str(), isSelected)) {
                    g_SelectedConfig = i;
                }
            }
            ImGui::EndChild();

            // Кнопки Load и Delete внизу
            ImGui::Separator();
            if (g_SelectedConfig != -1 && g_SelectedConfig < g_ConfigFiles.size()) {
                ImGui::Text("Selected: %s", g_ConfigFiles[g_SelectedConfig].c_str());

                if (ImGui::Button("Load Selected Config")) {
                    LoadConfig(g_ConfigFiles[g_SelectedConfig]);
                }

                ImGui::SameLine();

                if (ImGui::Button("Delete Selected Config")) {
                    DeleteConfig(g_ConfigFiles[g_SelectedConfig]);
                }
            }
            else {
                ImGui::Text("No config selected");
                ImGui::BeginDisabled();
                ImGui::Button("Load Selected Config");
                ImGui::SameLine();
                ImGui::Button("Delete Selected Config");
                ImGui::EndDisabled();
            }

            ImGui::EndTabItem();
        }

        // Menu Tab
        if (ImGui::BeginTabItem("Menu")) {
            ImGui::Text("Menu Appearance Settings");
            ImGui::Separator();

            // Window settings
            ImGui::Text("Window Settings:");
            ImGui::SliderFloat("Width", &g_MenuSettings.windowWidth, 600.0f, 1200.0f, "%.0f");
            if (ImGui::IsItemEdited()) {
                ImGui::SetWindowSize(ImVec2(g_MenuSettings.windowWidth, g_MenuSettings.windowHeight));
            }

            ImGui::SliderFloat("Height", &g_MenuSettings.windowHeight, 400.0f, 900.0f, "%.0f");
            if (ImGui::IsItemEdited()) {
                ImGui::SetWindowSize(ImVec2(g_MenuSettings.windowWidth, g_MenuSettings.windowHeight));
            }

            ImGui::ColorEdit4("Background", (float*)&g_MenuSettings.windowBgColor);
            if (ImGui::IsItemEdited()) ApplyMenuSettings();

            ImGui::ColorEdit4("Title Bar", (float*)&g_MenuSettings.windowTitleColor);
            if (ImGui::IsItemEdited()) ApplyMenuSettings();

            ImGui::ColorEdit4("Border", (float*)&g_MenuSettings.windowBorderColor);
            if (ImGui::IsItemEdited()) ApplyMenuSettings();

            ImGui::SliderFloat("Rounding", &g_MenuSettings.windowRounding, 0.0f, 20.0f, "%.1f");
            if (ImGui::IsItemEdited()) ApplyMenuSettings();

            ImGui::SliderFloat("Border Size", &g_MenuSettings.windowBorderSize, 0.0f, 5.0f, "%.1f");
            if (ImGui::IsItemEdited()) ApplyMenuSettings();

            ImGui::Separator();

            // Tab settings
            ImGui::Text("Tab Settings:");
            ImGui::ColorEdit4("Active Tab", (float*)&g_MenuSettings.tabActiveColor);
            if (ImGui::IsItemEdited()) ApplyMenuSettings();

            ImGui::ColorEdit4("Inactive Tab", (float*)&g_MenuSettings.tabInactiveColor);
            if (ImGui::IsItemEdited()) ApplyMenuSettings();

            ImGui::ColorEdit4("Hovered Tab", (float*)&g_MenuSettings.tabHoveredColor);
            if (ImGui::IsItemEdited()) ApplyMenuSettings();

            ImGui::SliderFloat("Tab Rounding", &g_MenuSettings.tabRounding, 0.0f, 10.0f, "%.1f");
            if (ImGui::IsItemEdited()) ApplyMenuSettings();

            ImGui::Separator();

            // Button settings
            ImGui::Text("Button Settings:");
            ImGui::ColorEdit4("Button", (float*)&g_MenuSettings.buttonColor);
            if (ImGui::IsItemEdited()) ApplyMenuSettings();

            ImGui::ColorEdit4("Button Hovered", (float*)&g_MenuSettings.buttonHoveredColor);
            if (ImGui::IsItemEdited()) ApplyMenuSettings();

            ImGui::ColorEdit4("Button Active", (float*)&g_MenuSettings.buttonActiveColor);
            if (ImGui::IsItemEdited()) ApplyMenuSettings();

            ImGui::SliderFloat("Button Rounding", &g_MenuSettings.buttonRounding, 0.0f, 10.0f, "%.1f");
            if (ImGui::IsItemEdited()) ApplyMenuSettings();

            ImGui::Separator();

            // Text settings
            ImGui::Text("Text Settings:");
            ImGui::ColorEdit4("Text Color", (float*)&g_MenuSettings.textColor);
            if (ImGui::IsItemEdited()) ApplyMenuSettings();

            ImGui::ColorEdit4("Disabled Text", (float*)&g_MenuSettings.textDisabledColor);
            if (ImGui::IsItemEdited()) ApplyMenuSettings();

            ImGui::SliderFloat("Font Size", &g_MenuSettings.fontSize, 10.0f, 20.0f, "%.0f");

            // Menu position
            ImGui::Separator();
            ImGui::Text("Menu Position:");
            ImGui::SliderFloat("X Position", &g_MenuSettings.menuX, 0.0f, 1920.0f, "%.0f");
            ImGui::SliderFloat("Y Position", &g_MenuSettings.menuY, 0.0f, 1080.0f, "%.0f");

            if (ImGui::Button("Save Current Position")) {
                ImVec2 pos = ImGui::GetWindowPos();
                g_MenuSettings.menuX = pos.x;
                g_MenuSettings.menuY = pos.y;
            }

            // Menu key
            ImGui::Separator();
            ImGui::Text("Menu Key:");
            static const char* keyNames[] = {
                "INSERT", "DELETE", "HOME", "END", "PAGEUP", "PAGEDOWN",
                "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12"
            };
            static int keyValues[] = {
                VK_INSERT, VK_DELETE, VK_HOME, VK_END, VK_PRIOR, VK_NEXT,
                VK_F1, VK_F2, VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, VK_F10, VK_F11, VK_F12
            };

            int currentKeyIndex = 0;
            for (int i = 0; i < IM_ARRAYSIZE(keyValues); i++) {
                if (keyValues[i] == g_MenuSettings.menuKey) {
                    currentKeyIndex = i;
                    break;
                }
            }

            if (ImGui::Combo("Menu Toggle Key", &currentKeyIndex, keyNames, IM_ARRAYSIZE(keyNames))) {
                g_MenuSettings.menuKey = keyValues[currentKeyIndex];
                g_KeyMenu = g_MenuSettings.menuKey;
            }

            // Toggle type
            ImGui::Combo("Toggle Type", &g_MenuSettings.toggleType, "Toggle\0Hold\0");

            // Кнопка сброса
            ImGui::Separator();
            if (ImGui::Button("Reset Menu Settings")) {
                ResetMenuSettings();
            }
            ImGui::SameLine();
            ImGui::Text("(Сброс к значениям по умолчанию)");

            ImGui::EndTabItem();
        }

        // Misc Tab
        if (ImGui::BeginTabItem("Misc")) {
            ImGui::Checkbox("Third Person", &g_ThirdPersonEnabled);
            ImGui::SameLine(200);
            ImGui::Checkbox("Sniper Crosshair", &g_SniperCrosshairEnabled);
            ImGui::SameLine(400);
            ImGui::Checkbox("FPS/Ping Info", &g_FpsPingInfo);

            ImGui::Separator();
            ImGui::Checkbox("Bunny Hop", &g_BunnyHopEnabled);
            if (g_BunnyHopEnabled) {
                ImGui::SliderInt("Perfect Jumps", &g_BunnyHopPerfectJumps, 1, 10);
            }

            ImGui::Separator();
            ImGui::Checkbox("No Flash", &g_NoFlashEnabled);
            if (g_NoFlashEnabled) {
                ImGui::SliderInt("Strength %", &g_NoFlashStrength, 0, 100);
            }

            ImGui::Separator();
            ImGui::Checkbox("Custom FOV", &g_UseCustomFOV);
            if (g_UseCustomFOV) {
                ImGui::SliderFloat("FOV Value", &g_CustomFOV, 60.0f, 150.0f, "%.0f");
            }

            ImGui::Separator();
            ImGui::Checkbox("Triggerbot", &g_TriggerbotEnabled);
            if (g_TriggerbotEnabled) {
                ImGui::Checkbox("Triggerbot Team Check", &g_TriggerbotTeamCheck);
                ImGui::SliderFloat("Triggerbot Delay", &g_TriggerbotDelay, 0.0f, 1.0f, "%.2f");
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::Separator();
    ImGui::Text("Status: %s", g_LocalPlayer ? "In Game" : "Not In Game");

    if (g_LocalPlayer) {
        int health = 0;
        try {
            health = ReadMemory<int>(g_LocalPlayer + m_iHealth);
        }
        catch (...) {
            health = 0;
        }
        ImGui::Text("Local Player Health: %d", health);
    }

    ImGui::End();
}

// ================================
// === HOOK ФУНКЦИИ ===
// ================================

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT __stdcall Hooked_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return 1;

    if (uMsg == WM_KEYDOWN) {
        if (g_MenuSettings.toggleType == 0) {
            if (wParam == g_MenuSettings.menuKey) {
                g_ShowMenu = !g_ShowMenu;
                return 0;
            }
        }
        else if (g_MenuSettings.toggleType == 1) {
            if (wParam == g_MenuSettings.menuKey) {
                g_ShowMenu = true;
                return 0;
            }
        }

        if (wParam == g_KeyExit) {
            if (g_OriginalWndProc && g_GameWindow) {
                SetWindowLongPtr(g_GameWindow, GWLP_WNDPROC, (LONG_PTR)g_OriginalWndProc);
            }
            return 0;
        }
        else if (wParam == g_KeyThirdPerson) {
            g_ThirdPersonEnabled = !g_ThirdPersonEnabled;
            return 0;
        }
    }

    if (uMsg == WM_KEYUP) {
        if (g_MenuSettings.toggleType == 1) {
            if (wParam == g_MenuSettings.menuKey) {
                g_ShowMenu = false;
                return 0;
            }
        }
    }

    if (g_OriginalWndProc) {
        return CallWindowProc(g_OriginalWndProc, hWnd, uMsg, wParam, lParam);
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

HRESULT __stdcall Hooked_EndScene(LPDIRECT3DDEVICE9 pDevice) {
    static bool firstFrame = true;

    if (!g_ImGuiInitialized) {
        g_D3D9Device = pDevice;

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

        if (!g_GameWindow) {
            g_GameWindow = FindWindowA("Valve001", "Counter-Strike: Global Offensive - Direct3D 9");
            if (!g_GameWindow) {
                g_GameWindow = FindWindowA(NULL, "Counter-Strike: Global Offensive");
            }
            if (!g_GameWindow) {
                g_GameWindow = GetForegroundWindow();
            }
        }

        if (g_GameWindow && g_GameWindow != (HWND)0) {
            ImGui_ImplWin32_Init(g_GameWindow);
            ImGui_ImplDX9_Init(pDevice);

            g_OriginalWndProc = (WNDPROC)SetWindowLongPtr(g_GameWindow, GWLP_WNDPROC, (LONG_PTR)Hooked_WndProc);
            g_ImGuiInitialized = true;
            firstFrame = true;
        }
    }

    if (g_ImGuiInitialized) {
        if (firstFrame) {
            firstFrame = false;
            return g_OriginalEndScene(pDevice);
        }

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        try {
            UpdateLocalPlayer();
        }
        catch (...) {
            // Игнорируем ошибки
        }

        if (g_LocalPlayer) {
            // Update features
            UpdateThirdPerson();
            UpdateSniperCrosshair();

            // Update new features
            UpdateGrenadePrediction();
            UpdatePenetrationCrosshair();

            // Run cheats
            if (g_AimbotEnabled) {
                if (g_AimbotMode == 0) {
                    RunLegitAimbot();
                }
                else if (g_AimbotMode == 1) {
                    RunRageAimbot();
                }
            }

            if (g_TriggerbotEnabled) RunTriggerBot();

            if (g_RadarEnabled) {
                for (int i = 1; i < 64; i++) {
                    DWORD entity = 0;
                    try {
                        entity = ReadMemory<DWORD>(g_ClientModule + dwEntityList + i * 0x10);
                    }
                    catch (...) {
                        continue;
                    }
                    if (!entity) continue;
                    try {
                        WriteMemory<bool>(entity + m_bSpotted, true);
                    }
                    catch (...) {
                        // Игнорируем ошибки
                    }
                }
            }

            if (g_GlowEnabled) ApplyGlow();

            if (g_NoFlashEnabled && g_LocalPlayer) {
                float flashDuration = 0;
                try {
                    flashDuration = ReadMemory<float>(g_LocalPlayer + m_flFlashDuration);
                }
                catch (...) {
                    flashDuration = 0;
                }
                if (flashDuration > 0) {
                    float reduction = (100.0f - g_NoFlashStrength) / 100.0f;
                    try {
                        WriteMemory<float>(g_LocalPlayer + m_flFlashDuration, flashDuration * reduction);
                    }
                    catch (...) {
                        // Игнорируем ошибки
                    }
                }
            }

            if (g_UseCustomFOV && g_LocalPlayer) {
                try {
                    WriteMemory<int>(g_LocalPlayer + m_iFOV, (int)g_CustomFOV);
                }
                catch (...) {
                    // Игнорируем ошибки
                }
            }

            // Rendering
            if (g_EspEnabled) {
                try {
                    RenderESP();
                }
                catch (...) {
                    // Игнорируем ошибки ESP
                }
            }

            // FPS/Ping info
            if (g_FpsPingInfo) {
                try {
                    DrawFpsPingInfo();
                }
                catch (...) {
                    // Игнорируем ошибки FPS/Ping
                }
            }

            // Draw FOV circle
            if (g_DrawAimbotFOV && g_AimbotEnabled) {
                try {
                    DrawFOVCircle();
                }
                catch (...) {
                    // Игнорируем ошибки FOV circle
                }
            }

            // Draw new features
            if (g_GrenadePredictionEnabled) {
                try {
                    DrawGrenadePrediction();
                }
                catch (...) {
                    // Игнорируем ошибки Grenade Prediction
                }
            }

            if (g_PenetrationCrosshairEnabled) {
                try {
                    DrawPenetrationCrosshair();
                }
                catch (...) {
                    // Игнорируем ошибки Penetration Crosshair
                }
            }
        }

        // Всегда рендерим меню
        RenderMenu();

        ImGui::EndFrame();
        ImGui::Render();

        try {
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
        }
        catch (...) {
            // Игнорируем ошибки рендеринга
        }
    }

    return g_OriginalEndScene(pDevice);
}

HRESULT __stdcall Hooked_Reset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters) {
    if (g_ImGuiInitialized) {
        ImGui_ImplDX9_InvalidateDeviceObjects();
    }

    HRESULT result = g_OriginalReset(pDevice, pPresentationParameters);

    if (result == D3D_OK && g_ImGuiInitialized) {
        ImGui_ImplDX9_CreateDeviceObjects();
    }

    return result;
}

bool InstallDX9Hooks() {
    IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!pD3D) return false;

    D3DPRESENT_PARAMETERS d3dpp = { 0 };
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow = GetForegroundWindow();
    d3dpp.Windowed = TRUE;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    d3dpp.BackBufferCount = 1;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    IDirect3DDevice9* pDevice = nullptr;
    HRESULT hr = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
        d3dpp.hDeviceWindow,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
        &d3dpp, &pDevice);

    if (FAILED(hr) || !pDevice) {
        pD3D->Release();
        return false;
    }

    void** pVTable = *(void***)pDevice;

    g_OriginalEndScene = (EndScene_t)pVTable[42];
    g_OriginalReset = (Reset_t)pVTable[16];

    pDevice->Release();
    pD3D->Release();

    MH_STATUS status;

    status = MH_CreateHook((LPVOID)g_OriginalEndScene, Hooked_EndScene, (LPVOID*)&g_OriginalEndScene);
    if (status != MH_OK) {
        return false;
    }

    status = MH_CreateHook((LPVOID)g_OriginalReset, Hooked_Reset, (LPVOID*)&g_OriginalReset);
    if (status != MH_OK) {
        return false;
    }

    status = MH_EnableHook(MH_ALL_HOOKS);
    if (status != MH_OK) {
        return false;
    }

    return true;
}

// ================================
// === ИНИЦИАЛИЗАЦИЯ ===
// ================================

DWORD WINAPI InitializeCheat(LPVOID lpParam) {
    Sleep(5000);

    if (MH_Initialize() != MH_OK) {
        return 0;
    }

    int retryCount = 0;
    const int maxRetries = 10;

    while (retryCount < maxRetries) {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32 entry = { sizeof(entry) };

        if (Process32First(snapshot, &entry)) {
            do {
                if (_stricmp(entry.szExeFile, "csgo.exe") == 0) {
                    g_GamePID = entry.th32ProcessID;
                    break;
                }
            } while (Process32Next(snapshot, &entry));
        }

        CloseHandle(snapshot);

        if (g_GamePID) break;

        retryCount++;
        Sleep(1000);
    }

    if (!g_GamePID) {
        return 0;
    }

    g_GameHandle = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, g_GamePID);
    if (!g_GameHandle) {
        return 0;
    }

    Sleep(2000);

    retryCount = 0;
    while (retryCount < maxRetries) {
        g_ClientModule = GetModuleBase("client.dll", g_GamePID);
        g_EngineModule = GetModuleBase("engine.dll", g_GamePID);

        if (g_ClientModule && g_EngineModule) {
            break;
        }

        retryCount++;
        Sleep(1000);
    }

    if (!g_ClientModule || !g_EngineModule) {
        if (g_GameHandle) CloseHandle(g_GameHandle);
        return 0;
    }

    // Инициализация директории конфигов
    char dllPath[MAX_PATH];
    GetModuleFileNameA(g_hModule, dllPath, MAX_PATH);
    std::string dllDirectory = dllPath;
    size_t lastSlash = dllDirectory.find_last_of("\\/");
    g_ConfigDirectory = dllDirectory.substr(0, lastSlash) + "\\MesensConfigs";
    CreateDirectoryA(g_ConfigDirectory.c_str(), NULL);

    // Загрузка списка конфигов
    RefreshConfigList();

    if (!InstallDX9Hooks()) {
        if (g_GameHandle) CloseHandle(g_GameHandle);
        return 0;
    }

    // Основной цикл
    while (true) {
        if (GetAsyncKeyState(VK_END) & 1) break;

        DWORD exitCode = 0;
        if (!g_GameHandle || !GetExitCodeProcess(g_GameHandle, &exitCode) || exitCode != STILL_ACTIVE) {
            break;
        }

        try {
            UpdateLocalPlayer();
        }
        catch (...) {
            // Игнорируем ошибки
        }

        if (g_LocalPlayer) {
            // BunnyHop
            if (g_BunnyHopEnabled) {
                int flags = 0;
                try {
                    flags = ReadMemory<int>(g_LocalPlayer + m_fFlags);
                }
                catch (...) {
                    flags = 0;
                }
                bool onGround = (flags & 1);
                if (GetAsyncKeyState(g_BunnyHopKey) & 0x8000) {
                    if (onGround) {
                        try {
                            WriteMemory<DWORD>(g_ClientModule + dwForceJump, 6);
                            Sleep(10);
                            WriteMemory<DWORD>(g_ClientModule + dwForceJump, 4);
                        }
                        catch (...) {
                            // Игнорируем ошибки
                        }
                    }
                }
            }
        }

        Sleep(10);
    }

    // Cleanup
    if (g_OriginalWndProc && g_GameWindow && IsWindow(g_GameWindow)) {
        SetWindowLongPtr(g_GameWindow, GWLP_WNDPROC, (LONG_PTR)g_OriginalWndProc);
    }

    if (g_ImGuiInitialized) {
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();

    if (g_GameHandle) {
        CloseHandle(g_GameHandle);
        g_GameHandle = nullptr;
    }

    FreeLibraryAndExitThread(g_hModule, 0);
    return 0;
}

// ================================
// === DLL MAIN ===
// ================================

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);

        HANDLE hThread = CreateThread(nullptr, 0, InitializeCheat, nullptr, CREATE_SUSPENDED, nullptr);
        if (hThread) {
            SetThreadPriority(hThread, THREAD_PRIORITY_NORMAL);
            ResumeThread(hThread);
            CloseHandle(hThread);
        }
    }
    else if (reason == DLL_PROCESS_DETACH) {
        if (g_OriginalWndProc && g_GameWindow && IsWindow(g_GameWindow)) {
            SetWindowLongPtr(g_GameWindow, GWLP_WNDPROC, (LONG_PTR)g_OriginalWndProc);
        }
    }

    return TRUE;
}