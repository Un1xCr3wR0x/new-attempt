#include <Windows.h>
#include <iostream>

template <typename T>
T* GetInterface(const char* name, const char* library) {
	const auto handle = GetModuleHandle(library);

	if (!handle) {
		return nullptr;
	}

	const auto functionAddress = GetProcAddress(handle, "CreateInterface");

	if (!functionAddress)
		return nullptr;


	using Fn = T * (*)(const char*, int*);

	const auto CreateInterface = reinterpret_cast<Fn>(functionAddress);

	if (!CreateInterface) {
		return nullptr;
	}

	return CreateInterface(name, nullptr);
};

class CEntity {
public:
	const int& GetHealth() const noexcept {
		return *reinterpret_cast<int*>(std::uintptr_t(this) + 0x320);
	}
};

class IClientEntityList
{
public:
	// Get IClientNetworkable interface for specified entity
	virtual void* GetClientNetworkable(int entnum) = 0;
	virtual void* GetClientNetworkableFromHandle(unsigned long hEnt) = 0;
	virtual void* GetClientUnknownFromHandle(unsigned long hEnt) = 0;

	// NOTE: This function is only a convenience wrapper.
	// It returns GetClientNetworkable( entnum )->GetIClientEntity().
	virtual CEntity* GetClientEntity(int entnum) = 0;
	virtual CEntity* GetClientEntityFromHandle(unsigned long hEnt) = 0;

	// Returns number of entities currently in use
	virtual int					NumberOfEntities(bool bIncludeNonNetworkable) = 0;

	// Returns highest index actually used
	virtual int					GetHighestEntityIndex(void) = 0;

	// Sizes entity list to specified size
	virtual void				SetMaxEntities(int maxents) = 0;
	virtual int					GetMaxEntities() = 0;
};

void HackThread(HMODULE instance) {


	AllocConsole();
	FILE* file;
	freopen_s(&file, "CONOUT$", "w", stdout);


	while (!GetAsyncKeyState(VK_END)) {
		if (GetAsyncKeyState(VK_INSERT) & 1) {
			auto GameResourceServiceClient =
				((uintptr_t(*)(const char*, void*))GetProcAddress(GetModuleHandleA("engine2.dll"),
					"CreateInterface"))("GameResourceServiceClientV001", nullptr);
			void* CGameEntitySystem = *(void**)(GameResourceServiceClient + 0x58);

			// Function to get an entity by its index
			const auto _GetEntityByIndex = [CGameEntitySystem](std::size_t index) -> void*
				{
					constexpr auto ENTITY_SYSTEM_LIST_SIZE = 512;
					const auto entitysystem_lists = (const void**)((const std::uint8_t*)CGameEntitySystem + 0x10);
					const auto list = entitysystem_lists[index / ENTITY_SYSTEM_LIST_SIZE];
					if (list)
					{
						const auto entry_index = index % ENTITY_SYSTEM_LIST_SIZE;
						constexpr auto sizeof_CEntityIdentity = 0x78;
						const auto identity = (const std::uint8_t*)list + entry_index * sizeof_CEntityIdentity;
						if (identity)
						{
							return *(void**)identity;
						}
					}
					return nullptr;
				};

			// Loop through entities and process them
			for (std::size_t index = 1; index <= 64; ++index)
			{
				void* Entity = _GetEntityByIndex(index);
				if (Entity)
				{
					constexpr auto offset_m_iszPlayerName = 0x628;
					constexpr auto offset_m_hAssignedHero = 0x7e4;
					constexpr int offset_m_iTaggedAsVisibleByTeam = 0xc2c;
					constexpr auto offset_m_hInventory = 0xf30;
					constexpr auto offset_m_hAbilities = 0xae0;
					constexpr auto offset_m_fCooldown = 0x580;
					constexpr auto high_17_bits = 0b11111111111111111000000000000000;
					constexpr auto low_15_bits = 0b00000000000000000111111111111111;

					struct CHandle
					{
						int value;
						int serial() const noexcept
						{
							return value & high_17_bits;
						}
						int index() const noexcept
						{
							return value & low_15_bits;
						}
						bool is_valid() const noexcept
						{
							return value != -1;
						}
					};

					void* EntityHero = _GetEntityByIndex(((*(CHandle*)((uintptr_t)Entity + offset_m_hAssignedHero)).index()));
					{
						for (int i = 0; i < 35; i++)
						{
							void* EntityHeroAbility = _GetEntityByIndex(((*(CHandle*)((uintptr_t)EntityHero + offset_m_hAbilities + i * 0x4)).index()));
							if(EntityHeroAbility) 							{
								std::cout << "EntityHeroAbility: " << EntityHeroAbility << std::endl;
								std::cout << "EntityHeroAbility Cooldown: " << *(float*)((uintptr_t)EntityHeroAbility + offset_m_fCooldown) << std::endl;
							}
						}
						
					}
					int EntityHeroTaggedAsVisibleByTeam = (int)((uintptr_t)EntityHero + offset_m_iTaggedAsVisibleByTeam);
					std::cout << "Entity: " << Entity << std::endl;
					std::cout << "Entity Name : " << (char*)((uintptr_t)Entity + offset_m_iszPlayerName) << std::endl;
					std::cout << "Entity Assigned Hero : " << ((*(CHandle*)((uintptr_t)Entity + offset_m_hAssignedHero)).index()) << std::endl;
					std::cout << "EntityHero: " << EntityHero << std::endl;
					std::cout << "is visible: " << EntityHeroTaggedAsVisibleByTeam << std::endl;

					OutputDebugStringA(std::format("player {}({}) controls hero indexed {}\n",
						(char*)((uintptr_t)Entity + offset_m_iszPlayerName),
						Entity, ((*(CHandle*)((uintptr_t)Entity + offset_m_hAssignedHero)).index())).data());
				}
			}
		}
		Sleep(200);
	}
	if (file)
		fclose(file);

	FreeConsole();
	FreeLibraryAndExitThread((HMODULE)HackThread, 0);
}

BOOL WINAPI DllMain(HMODULE instance, DWORD reason, LPVOID reserved) {
	if (reason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(instance);

		const auto thread = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(HackThread), instance, 0, nullptr);

		if (thread) {
			CloseHandle(thread);
		}
	}
	return TRUE;
}