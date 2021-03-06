﻿#include <iostream>
#include <string>
#include <windows.h>
#include <shlwapi.h>
#include <tlhelp32.h>
#include <wininet.h>
#include "winapi.h"

namespace winapi {
	win32_error::win32_error(int code) : code(code) {};

	std::wstring win32_error::message()
	{
		LPWSTR buffer;
		::FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER
			| FORMAT_MESSAGE_FROM_SYSTEM
			| FORMAT_MESSAGE_IGNORE_INSERTS
			| FORMAT_MESSAGE_MAX_WIDTH_MASK,
			NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&buffer, 0, NULL);
		std::wstring ret = buffer;
		::LocalFree(buffer);
		return ret;
	}
	win32_error_path_not_found::win32_error_path_not_found() : win32_error(ERROR_PATH_NOT_FOUND) { }
	win32_error_directory_not_found::win32_error_directory_not_found() : win32_error(ERROR_DIRECTORY) { }
	win32_error_invalid_function::win32_error_invalid_function() : win32_error(ERROR_INVALID_FUNCTION) { }
	win32_error_invalid_parameter::win32_error_invalid_parameter() : win32_error(ERROR_INVALID_PARAMETER) { }

	void execute(std::wstring exec, std::wstring parameters, show show_mode) {
		int show = 0;
		switch (show_mode) {
		case show::normal:   show = SW_SHOWNORMAL; break;
		case show::noactive: show = SW_SHOWNOACTIVATE; break;
		}

		parameters = L"\"" + parameters + L"\"";
		SHELLEXECUTEINFO sei = { 0 };
		sei.cbSize = sizeof(SHELLEXECUTEINFO);
		sei.lpVerb = L"open";
		sei.lpFile = exec.c_str();
		sei.lpParameters = parameters.c_str();
		sei.nShow = show;
		sei.fMask = NULL;
		::ShellExecuteEx(&sei);
	}

	void show_window(long handle)
	{
		auto hwnd = (HWND)LongToHandle(handle);
		::ShowWindow(hwnd, SW_SHOWNOACTIVATE);
		set_topmost_window(handle, true);
		set_topmost_window(handle, false);
	}

	void active_window(long handle)
	{
		auto hwnd = (HWND)LongToHandle(handle);
		::ShowWindow(hwnd, SW_RESTORE);
		::SetForegroundWindow(hwnd);
		set_topmost_window(handle, true);
		set_topmost_window(handle, false);
	}

	long find_console_window() {
		std::wstring title = get_console_title();
		int pid = get_current_process_id();
		std::wstring uniq_title = title + L" - " + std::to_wstring(pid);
		set_console_title(uniq_title);

		HWND handle = 0;
		for (int i = 0; i < 100; i++) {
			::Sleep(1);
			handle = ::FindWindow(NULL, uniq_title.c_str());
			if (handle != NULL) break;
		}
		set_console_title(title);
		return HandleToLong(handle);
	}

	BOOL CALLBACK enum_windows_proc(HWND hwnd, LPARAM lp) {
		auto handles = (std::vector<long>*)lp;
		handles->push_back(HandleToLong(hwnd));
		return true;
	}

	std::vector<long> enum_windows() {
		std::vector<long> handles;
		EnumWindows(enum_windows_proc, (LPARAM)&handles);
		return handles;
	}

	std::wstring get_window_text(long handle) {
		auto hwnd = (HWND)LongToHandle(handle);
		int len = GetWindowTextLength(hwnd);
		if (len == 0) return L"";
		std::vector<wchar_t> buffer((size_t)len + 1);
		::GetWindowText(hwnd, buffer.data(), len + 1);
		return std::basic_string<wchar_t>(buffer.begin(), buffer.end());
	}

	void set_topmost_window(long handle, bool topmost) {
		auto hwnd = (HWND)LongToHandle(handle);
		if (topmost) {
			UINT flags = SWP_NOMOVE | SWP_NOSIZE;
			::SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, flags);
		}
		else {
			UINT flags = SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE;
			::SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, flags);
		}
	}

	bool is_topmost_window(long handle)
	{
		int style = ::GetWindowLong((HWND)LongToHandle(handle), GWL_EXSTYLE);
		return (style & WS_EX_TOPMOST) == WS_EX_TOPMOST;
	}

	long get_main_window_handle(const long pid) {
		auto hwnd = ::GetTopWindow(NULL);

		do {
			if (::GetWindowLongPtr(hwnd, GWLP_HWNDPARENT) != 0) continue;
			if (!IsWindowVisible(hwnd)) continue;

			DWORD getPID;
			::GetWindowThreadProcessId(hwnd, &getPID);

			if (pid == getPID) return HandleToLong(hwnd);
		} while ((hwnd = ::GetNextWindow(hwnd, GW_HWNDNEXT)) != NULL);

		return NULL;
	}


	std::string wide2multi(std::wstring const& str, unsigned int codepage)
	{
		LPCSTR ch = "?";
		LPCWCH src = str.data();
		int size = ::WideCharToMultiByte(codepage, 0, src, -1, nullptr, 0, ch, nullptr);
		char* dest = new char[size];
		if (::WideCharToMultiByte(codepage, 0, src, -1, dest, size, ch, nullptr) == 0) {
			throw win32_error(::GetLastError());
		}
		std::string ret = dest;
		delete[] dest;
		return ret;
	}

	std::wstring multi2wide(std::string const& str, unsigned int codepage)
	{
		LPCSTR src = str.data();
		int const size = ::MultiByteToWideChar(codepage, 0, src, -1, nullptr, 0);
		wchar_t* dest = new wchar_t[size];
		if (::MultiByteToWideChar(codepage, 0, src, -1, dest, size) == 0) {
			throw win32_error(::GetLastError());
		}
		std::wstring ret = dest;
		delete[] dest;
		return ret;
	}

	std::wstring expand_environment_strings(std::wstring str)
	{
		int const size = ::ExpandEnvironmentStrings(str.c_str(), NULL, 0);
		wchar_t* dest = new wchar_t[size];
		if (::ExpandEnvironmentStrings(str.c_str(), dest, size) == 0) {
			throw win32_error(::GetLastError());
		}
		std::wstring ret = dest;
		delete[] dest;
		return ret;
	}

	std::wstring uri2path(std::wstring uri) {
		DWORD length = MAX_PATH;
		WCHAR path[MAX_PATH];
		if (::PathCreateFromUrl(uri.c_str(), path, &length, NULL) != S_OK) {
			return L"";
		}
		return path;
	}

	std::wstring path2uri(std::wstring uri) {
		DWORD length = INTERNET_MAX_URL_LENGTH;
		WCHAR path[INTERNET_MAX_URL_LENGTH];
		if (::UrlCreateFromPath(uri.c_str(), path, &length, NULL) != S_OK) {
			return L"";
		}
		return path;
	}

	std::wstring urlescape(std::wstring uri) {
		DWORD length = INTERNET_MAX_URL_LENGTH;
		WCHAR escaped[INTERNET_MAX_URL_LENGTH];
		DWORD flags = URL_ESCAPE_PERCENT | URL_ESCAPE_SEGMENT_ONLY;
		if (::UrlEscape(uri.c_str(), escaped, &length, flags) != S_OK) {
			return L"";
		}
		return escaped;
	}

	unsigned int get_console_output_codepage()
	{
		return ::GetConsoleOutputCP();
	}

	unsigned int set_console_output_codepage(unsigned int codepage)
	{
		UINT current_cp = ::GetConsoleOutputCP();
		if (codepage == 0 || codepage == current_cp) {
			return current_cp;
		}
		if (::SetConsoleOutputCP(codepage) == 0) {
			throw win32_error(GetLastError());
		}
		return current_cp;
	}

	std::wstring get_console_title()
	{
		wchar_t ch;
		int const size = ::GetConsoleTitle(&ch, 1) + 1;
		if (size == 1) return L"";
		wchar_t* dest = new wchar_t[size];
		if (::GetConsoleTitle(dest, size) == 0) {
			throw win32_error(::GetLastError());
		}
		std::wstring ret = dest;
		delete[] dest;
		return ret;
	}

	void set_console_title(std::wstring title) {
		::SetConsoleTitle(title.c_str());
	}

	int get_current_process_id() {
		return ::GetCurrentProcessId();
	}

	std::vector<process_entry> get_process_entries(std::wstring name) {
		std::vector<process_entry> entries;

		HANDLE handle = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS | TH32CS_SNAPMODULE, 0);
		if (!handle) return entries;

		::PROCESSENTRY32 process = { sizeof(::PROCESSENTRY32) };
		::Process32First(handle, &process);

		do {
			if (wcscmp(process.szExeFile, name.c_str()) != 0) continue;
			process_entry entry;
			entry.process_id = process.th32ProcessID;
			entry.window_handle = get_main_window_handle(process.th32ProcessID);
			entry.window_text_length = ::GetWindowTextLength((HWND)LongToHandle(entry.window_handle));
			entries.push_back(entry);
		} while (::Process32Next(handle, &process));

		::CloseHandle(handle);
		return entries;
	}

	long get_registry_value(std::wstring key, std::wstring name, long default_value)
	{
		HKEY hkey;
		if (::RegOpenKeyEx(HKEY_CURRENT_USER, key.c_str(), 0, KEY_READ, &hkey) != ERROR_SUCCESS) {
			return default_value;
		}

		DWORD dwSize, dwType, data;
		if (::RegQueryValueEx(hkey, name.c_str(), NULL, &dwType, NULL, &dwSize) != ERROR_SUCCESS) {
			::RegCloseKey(hkey);
			return default_value;
		}

		if (::RegQueryValueEx(hkey, name.c_str(), NULL, &dwType, (LPBYTE)&data, &dwSize) != ERROR_SUCCESS) {
			::RegCloseKey(hkey);
			return default_value;
		}

		::RegCloseKey(hkey);
		return data;
	}
}
