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
    
    // Tracks the last known state of the VM process to avoid redundant API calls.
    // Initialized to FALSE, assuming no VM is running at launch.
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
        const char* targetProcess = "vmware-vmx.exe";

        // Iterate through the process list to find the target VM process.
        do {
            // _stricmp performs a case-insensitive comparison of the process name.
            if (_stricmp(pe32.szExeFile, targetProcess) == 0) {
                isVmRunning = TRUE;
                break; // Found the process, no need to check further.
            }
        } while (Process32Next(hSnap, &pe32));

        // Always close the handle to the snapshot to free system resources.
        CloseHandle(hSnap);

        // Only update the system's execution state if the VM's running status has changed.
        // This prevents making unnecessary API calls in every loop iteration.
        if (isVmRunning != wasVmRunning) {
            if (isVmRunning) {
                // VM has started. Tell Windows the system is in use and should not sleep.
                // ES_SYSTEM_REQUIRED: Prevents the system from sleeping.
                // ES_CONTINUOUS: Keeps the setting active until explicitly cleared.
                SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);
            } else {
                // VM has stopped. Revert to default behavior, allowing the system to sleep.
                // Calling with only ES_CONTINUOUS clears previously set flags.
                SetThreadExecutionState(ES_CONTINUOUS);
            }
            // Update the state tracker to reflect the new reality.
            wasVmRunning = isVmRunning;
        }
        
        // Pause for 30 seconds before the next check to minimize CPU usage.
        Sleep(30000);
    }

    return 0; // This line is never reached in this implementation.
}
