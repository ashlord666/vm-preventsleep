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

// Function to write a message to the log file in the user's temp directory.
void WriteToLog(const char* message) {
    char tempPath[MAX_PATH];
    char logFilePath[MAX_PATH];

    // Get the path to the user's temporary folder.
    if (GetTempPath(MAX_PATH, tempPath) == 0) {
        // Fallback or error handling if we can't get the temp path.
        // For simplicity, we'll just exit the function.
        return;
    }

    // Construct the full path for the log file.
    // e.g., "C:\Users\YourName\AppData\Local\Temp\vmpreventsleep.log"
    sprintf_s(logFilePath, MAX_PATH, "%svmpreventsleep.log", tempPath);

    FILE* logFile;
    // Use the dynamically created file path.
    if (fopen_s(&logFile, logFilePath, "a+") == 0 && logFile != NULL) {
        time_t now = time(NULL);
        struct tm localTime;
        
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
    WriteToLog("--------------------------------------------------");
    WriteToLog("vmpreventsleep utility started.");

    // Tracks the last known state of the target processes to avoid redundant API calls.
    BOOL wasVmRunning = FALSE;

    // The main infinite loop to continuously monitor the system.
    while (TRUE) {
        // NEW: Heartbeat log to show the loop is running.
        WriteToLog("Scanning for target processes...");

        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        
        if (hSnap == INVALID_HANDLE_VALUE) {
            WriteToLog("ERROR: Failed to create process snapshot.");
            Sleep(30000);
            continue;
        }

        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);

        if (!Process32First(hSnap, &pe32)) {
            WriteToLog("ERROR: Process32First failed.");
            CloseHandle(hSnap);
            Sleep(30000);
            continue;
        }

        BOOL isVmRunning = FALSE;
        char foundProcessName[MAX_PATH] = "";
        char logBuffer[512];

        const char* targetProcesses[] = {
            "vmware-vmx.exe", // VMware Workstation/Player
            "vmwp.exe",       // Hyper-V Worker Process
            "wsl.exe"         // Windows Subsystem for Linux
        };
        int numTargets = sizeof(targetProcesses) / sizeof(targetProcesses[0]);

        do {
            for (int i = 0; i < numTargets; i++) {
                if (_stricmp(pe32.szExeFile, targetProcesses[i]) == 0) {
                    isVmRunning = TRUE;
                    strcpy_s(foundProcessName, MAX_PATH, pe32.szExeFile);
                    goto process_scan_done; // Use goto to break out of nested loops cleanly.
                }
            }
        } while (Process32Next(hSnap, &pe32));

    process_scan_done:
        CloseHandle(hSnap);

        // NEW: Log the result of every scan.
        sprintf_s(logBuffer, sizeof(logBuffer), "Scan complete. Target process running: %s.", isVmRunning ? "Yes" : "No");
        WriteToLog(logBuffer);

        // Only update the system's execution state if the running status has changed.
        if (isVmRunning != wasVmRunning) {
            sprintf_s(logBuffer, sizeof(logBuffer), "State change detected. Previous state: %s, New state: %s.", wasVmRunning ? "Running" : "Not Running", isVmRunning ? "Running" : "Not Running");
            WriteToLog(logBuffer);
            
            if (isVmRunning) {
                WriteToLog("Attempting to ENABLE sleep prevention...");
                // NEW: Check the return value of the API call.
                EXECUTION_STATE prevState = SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
                if (prevState == NULL) {
                    WriteToLog("ERROR: SetThreadExecutionState call FAILED.");
                } else {
                    sprintf_s(logBuffer, sizeof(logBuffer), "SUCCESS: Sleep prevention enabled for process: %s.", foundProcessName);
                    WriteToLog(logBuffer);
                }
            } else {
                WriteToLog("Attempting to DISABLE sleep prevention...");
                // NEW: Check the return value of the API call.
                EXECUTION_STATE prevState = SetThreadExecutionState(ES_CONTINUOUS);
                if (prevState == NULL) {
                     WriteToLog("ERROR: SetThreadExecutionState call FAILED.");
                } else {
                     WriteToLog("SUCCESS: Sleep prevention disabled. No target processes running.");
                }
            }
            // Update the state tracker.
            wasVmRunning = isVmRunning;
        }
        
        WriteToLog("...sleeping for 30 seconds...");
        Sleep(30000);
    }

    return 0; // This line is never reached in this implementation.
}