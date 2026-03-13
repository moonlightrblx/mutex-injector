#include <windows.h>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>

template<typename T>
T read(uintptr_t addr) {
	return *reinterpret_cast<T*>(addr);
}

template<typename T>
void write(uintptr_t addr, T val) {
	*reinterpret_cast<T*>(addr) = val;
}

class mem_protect {
public:
	mem_protect(void* addr, size_t size, DWORD prot) {
		this->addr = addr;
		this->size = size;
		VirtualProtect(addr, size, prot, &old);
	}

	~mem_protect() {
		VirtualProtect(addr, size, old, &old);
	}

private:
	void* addr;
	size_t size;
	DWORD old;
};

void do_patches(const std::wstring& path) {
	std::ifstream f(path);
	if (!f.is_open()) return;

	std::string l;
	std::getline(f, l); // skip first line idk why the format does this but yeah it does >:3

	while (std::getline(f, l)) {
		if (l.empty()) continue;

		size_t c = l.find(':');
		size_t a = l.find("->");
		if (c == std::string::npos || a == std::string::npos) continue;

		try {
			uint64_t rva = std::stoull(l.substr(0, c), nullptr, 16); // uh yeah this is NOT aids i swear ;)
			BYTE orig = (BYTE)std::stoul(l.substr(c + 1, a - c - 1), nullptr, 16); // OwO
			BYTE rep = (BYTE)std::stoul(l.substr(a + 2), nullptr, 16); // >.<

			uintptr_t base = (uintptr_t)GetModuleHandle(0);
			uintptr_t addr = base + rva;

			if (read<BYTE>(addr) == orig) {
				mem_protect prot((void*)addr, 1, PAGE_EXECUTE_READWRITE);
				write<BYTE>(addr, rep);
			}
		}
		catch (...) {}
	}
}

DWORD WINAPI patch_thread(LPVOID) {
	std::this_thread::sleep_for(std::chrono::seconds(2));

	wchar_t exe_path[MAX_PATH];
	GetModuleFileNameW(0, exe_path, MAX_PATH);

	std::wstring dir = exe_path;
	dir = dir.substr(0, dir.find_last_of(L"\\/") + 1);
	std::wstring patch_path = dir + L"patch.1337";

	do_patches(patch_path);

	return 0;
}

bool is_injected_by_our_injector() {
	HANDLE hMutex = OpenMutexA(SYNCHRONIZE, FALSE, "Global\\ShxdowIsMyDaddyAndHeOwnsMe");
	if (hMutex) {
		CloseHandle(hMutex);
		return true;
	}
	return false;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
	if (reason == DLL_PROCESS_ATTACH) {
		if (!is_injected_by_our_injector() ){
			return FALSE; // invalid injector lol
		}

		DisableThreadLibraryCalls(hModule);
		CreateThread(0, 0, patch_thread, 0, 0, 0);
	}
	return TRUE;
}