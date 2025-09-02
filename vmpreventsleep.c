/**************************************************************************************************
 * vmpreventsleep.c
 *
 * Description:
 * A lightweight, windowless background utility for Windows that prevents the system from
 * entering sleep mode or turning off the display while a VMware, Hyper-V, or WSL process
 * is running. It periodically checks for target processes and uses the Windows API call
 * SetThreadExecutionState to manage the system's power state, logging its actions.
 *
 * Compilation (using Visual C++ Compiler):
 * cl vmpreventsleep.c
 *
 **************************************************************************************************/

#include <windows.h>
#include <tlhelp32.h>
#include <string.h>
#include <stdio.h> // Required for file I/O (fopen_s, fprintf, fclose)
#include <time.h>  // Required for date/time functions

// Function to write a message to the log file.
void WriteToLog(const char* message) {
    FILE* logFile;
    // Use fopen_s for safer file opening. "a+" opens for appending; creates the file if it doesn't exist.
    if (fopen_s(&logFile, "C:\\windows\\temp\\vmpreventsleep.log", "a+") == 0 && logFile != NULL) {
        time_t now = time(NULL);
        struct tm localTime;
        // Use localtime_s for thread-safe time conversion.
        if (localtime_s(&localTime, &now) == 0) {
            char timeBuffer[20]; // Buffer for "YYYY-MM-DD HH:MM:SS"
            strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &localTime);
            fprintf(logFile, "[%s] %s\n", timeBuffer, message);
        }
        fclose(logFile);
    }
}

// Use WinMain as the entry point to create a windowless background application.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    
    // Initial log entry to confirm the utility has started.
    WriteToLog("vmpreventsleep utility started.");

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
        char foundProcessName[MAX_PATH] = ""; // Buffer to store the name of the found process.
        char logBuffer[512];                  // Buffer for formatting log messages.

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
                    // Store the name of the found process for logging.
                    strcpy_s(foundProcessName, MAX_PATH, pe32.szExeFile);
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
                // Format and write the log message.
                sprintf_s(logBuffer, sizeof(logBuffer), "Sleep prevention ENABLED. Process detected: %s", foundProcessName);
                WriteToLog(logBuffer);
            } else {
                // All target processes have stopped. Allow the system to sleep again.
                SetThreadExecutionState(ES_CONTINUOUS);
                WriteToLog("Sleep prevention DISABLED. No target processes running.");
            }
            // Update the state tracker.
            wasVmRunning = isVmRunning;
        }
        
        // Pause for 30 seconds before the next check to minimize CPU usage.
        Sleep(30000);
    }

    return 0; // This line is never reached in this implementation.
}