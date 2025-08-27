/**************************************************************************************************
 * vmpreventsleep.c
 *
 * Description:
 * A lightweight, windowless background utility for Windows that prevents the system from
 * entering sleep mode or turning off the display while a VMware virtual machine is running.
 * It periodically checks for the "vmware-vmx.exe" process and uses the Windows API call
 * SetThreadExecutionState to manage the system's power state accordingly.
 *
 * Compilation (using Visual C++ Compiler):
 * cl vmpreventsleep.c
 *
 **************************************************************************************************/

#include <windows.h>
#include <tlhelp32.h>
#include <string.h>

// Use WinMain as the entry point to create a windowless background application.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    
    // Tracks the last known state of the target processes to avoid redundant API calls.
    BOOL wasVmRunning = FALSE;

    // The main infinite loop to continuously monitor the system.
    while (TRUE) {
        // Create a snapshot of all running processes in the system.
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        
        // If the snapshot fails, wait for 30 seconds before trying again.
        if (hSnap == INVALID_HANDLE_VALUE) {
            Sleep(30000);
            continue;
        }

        // Initialize the PROCESSENTRY32 structure. Its size must be set before use.
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);

        // Retrieve information about the first process in the snapshot.
        // If it fails, clean up and retry after a delay.
        if (!Process32First(hSnap, &pe32)) {
            CloseHandle(hSnap);
            Sleep(30000);
            continue;
        }

        BOOL isVmRunning = FALSE;
        // Define the list of target processes to check for.
        const char* targetProcesses[] = {
            "vmware-vmx.exe", // VMware Workstation/Player
            "vmwp.exe",       // Hyper-V Worker Process
            "wsl.exe"         // Windows Subsystem for Linux
        };
        int numTargets = sizeof(targetProcesses) / sizeof(targetProcesses[0]);

        // Iterate through the system's process list.
        do {
            // For each system process, check if it matches any of our targets.
            for (int i = 0; i < numTargets; i++) {
                // _stricmp performs a case-insensitive comparison.
                if (_stricmp(pe32.szExeFile, targetProcesses[i]) == 0) {
                    isVmRunning = TRUE;
                    break; // Found a match, no need to check other targets for this process.
                }
            }
            if (isVmRunning) {
                break; // A target was found, no need to scan the rest of the system processes.
            }
        } while (Process32Next(hSnap, &pe32));

        // Always close the handle to the snapshot to free system resources.
        CloseHandle(hSnap);

        // Only update the system's execution state if the running status has changed.
        if (isVmRunning != wasVmRunning) {
            if (isVmRunning) {
                // A target process has started. Prevent the system from sleeping.
                SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);
            } else {
                // All target processes have stopped. Allow the system to sleep again.
                SetThreadExecutionState(ES_CONTINUOUS);
            }
            // Update the state tracker.
            wasVmRunning = isVmRunning;
        }
        
        // Pause for 30 seconds before the next check to minimize CPU usage.
        Sleep(30000);
    }

    return 0; // This line is never reached in this implementation.
}
