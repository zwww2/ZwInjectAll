#include <Windows.h>
#include <stdio.h>
#include "ntdll.h"

#pragma comment(lib, "ntdll.lib")


BOOLEAN Log = TRUE;


typedef NTSTATUS(NTAPI* NtOpenProcess__)(PHANDLE a, ACCESS_MASK b, OBJECT_ATTRIBUTES* c, CLIENT_ID* d);
typedef NTSTATUS(NTAPI* NtAllocateVirtualMemory__)(HANDLE a, PVOID b, ULONG c, PULONG d, ULONG e, ULONG f);
typedef NTSTATUS(NTAPI* NtWriteVirtualMemory__)(HANDLE a, PVOID b, PVOID c, ULONG d, PULONG e);
typedef NTSTATUS(NTAPI* RtlCreateUserThread__)(HANDLE a, PSECURITY_DESCRIPTOR b, BOOLEAN c, ULONG d, PULONG e, PULONG f, PVOID g, PVOID h, PHANDLE i, CLIENT_ID* j);

NtOpenProcess__ NtOpenProcess_;
NtAllocateVirtualMemory__ NtAllocateVirtualMemory_;
NtWriteVirtualMemory__ NtWriteVirtualMemory_;
RtlCreateUserThread__ RtlCreateUserThread_;

BOOLEAN InjectDll(HANDLE ProcessId, const char* path)
{
	HANDLE hProcess;
	OBJECT_ATTRIBUTES oProcess;
	CLIENT_ID cProcess;
	InitializeObjectAttributes(&oProcess, 0, OBJ_INHERIT, 0, 0);
	cProcess.UniqueProcess = ProcessId;
	cProcess.UniqueThread = (HANDLE)0;

	NTSTATUS status = NtOpenProcess_(&hProcess, PROCESS_ALL_ACCESS, &oProcess, &cProcess);

	if (!NT_SUCCESS(status))
	{
		if (Log == TRUE)
		{
			printf("Failed open process 0x%X", status);
		}
		return FALSE;
	}


	PVOID data;
	ULONG size = 8192;

	status = NtAllocateVirtualMemory_(hProcess, &data, 0, &size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	
	if (!NT_SUCCESS(status))
	{

		CloseHandle(hProcess);
		if (Log == TRUE)
		{
			printf("Failed allocate process memory 0x%X", status);
		}
		return FALSE;
	}

	
	status = NtWriteVirtualMemory_(hProcess, data, (LPSTR)path, strlen(path) + 1, 0);

	if (!NT_SUCCESS(status))
	{
		CloseHandle(hProcess);
		if (Log == TRUE)
		{
			printf("Failed write process memory 0x%X", status);
		}
		return FALSE;
	}

	HANDLE hThread;
	CLIENT_ID cThread;

	status = RtlCreateUserThread_(hProcess, 0, FALSE, 0, 0, 0, (PVOID)LoadLibraryA, data, &hThread, &cThread);

	if (!NT_SUCCESS(status))
	{
		CloseHandle(hProcess);
		if (Log == TRUE)
		{
			

			printf("Failed create thread 0x%X", status);
		}
		return FALSE;
	}

	CloseHandle(hProcess);
	CloseHandle(hThread);


	return TRUE;

}

void InjectAll()
{
	PVOID buffer = malloc(2048 * 512 * 8);
	NtQuerySystemInformation(5, buffer, 2048 * 512 * 8, 0);

	PSYSTEM_PROCESS_INFORMATION processInfo = (PSYSTEM_PROCESS_INFORMATION)buffer;



	processInfo = (PSYSTEM_PROCESS_INFORMATION)((PUCHAR)processInfo + processInfo->NextEntryOffset);


	while (processInfo->NextEntryOffset != 0)
	{
		InjectDll(processInfo->UniqueProcessId, "D:\\Games\\12\\ZwHook\\x64\\Release\\ZwHook.dll");

		processInfo = (PSYSTEM_PROCESS_INFORMATION)((PUCHAR)processInfo + processInfo->NextEntryOffset);
	}
	free(buffer);
}


int main()
{
	NtOpenProcess_ = (NtOpenProcess__)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtOpenProcess");
	NtAllocateVirtualMemory_ = (NtAllocateVirtualMemory__)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtAllocateVirtualMemory");
	NtWriteVirtualMemory_ = (NtWriteVirtualMemory__)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtWriteVirtualMemory");
	RtlCreateUserThread_ = (RtlCreateUserThread__)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlCreateUserThread");
	
	InjectAll();

	Sleep(-1);
}


