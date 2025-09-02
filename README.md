# VM Prevent Sleep

A lightweight, windowless Windows utility that prevents the host system from going to sleep while a VMware virtual machine is running.

## The Problem

My PCs and laptops are primarily for gaming, but I often need to run virtual machines for work or lab projects. Due to memory constraints, I sometimes have to spread these VMs across multiple machines. Manually toggling sleep settings on every device each time I start or stop a VM is tedious, and frankly, I'm too lazy for that.

I've tried other solutions like ```powercfg /requestsoverride``` and ```PowerToys Awake```, but they weren't the seamless, "set it and forget it" solution I wanted.

This tool is the simple answer. It doesn't care if the VMware Workstation GUI is open or if you've configured VMs to run in the background. As long as a virtual machine is actually running, the underlying vmware-vmx.exe process exists, and this tool will automatically prevent the system from sleeping. When you're done with your work, just shut down your VMs and walk away. The computer will go back to its normal sleep schedule without you having to do a thing.

## How It Works

The application periodically (every 30 seconds) scans the list of running processes for `vmware-vmx.exe`, which is the core process for a running VMware virtual machine.

* **If `vmware-vmx.exe` is found:** It calls the `SetThreadExecutionState` Windows API with flags that prevent the system from sleeping.
* **If `vmware-vmx.exe` is not found:** It calls the same API to restore the system's default power behavior, allowing it to sleep again.

The tool is efficient because it only changes the system state when the VM's status (running or not running) changes.

## Installation and Usage

### 1. Compilation

You will need a C compiler, such as the one included with **Visual Studio Build Tools**.

Open a command prompt with the compiler environment loaded (like the "Developer Command Prompt for VS") and run:

```bash
cl vmpreventsleep.c
```

This will produce ```vmpreventsleep.exe```.

### 2. Setup
Create a directory for the tool, for example: ```C:\Tools```.

Copy the compiled ```vmpreventsleep.exe``` into that directory.

Set up a Scheduled Task to run the program automatically at startup. Run this in administrative powershell_ise:

```
# Define the action: what program to run
$action = New-ScheduledTaskAction -Execute 'C:\Tools\vmpreventsleep.exe'

# Define the trigger: when to run the program
$trigger = New-ScheduledTaskTrigger -AtStartup

# Define the user account and privilege level for the task
$principal = New-ScheduledTaskPrincipal -UserId "NT AUTHORITY\SYSTEM" -RunLevel Highest

# Define the advanced settings for the task
$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -ExecutionTimeLimit 0

# Register (create) the scheduled task with all the defined properties
# The -Force switch will overwrite the task if it already exists
Register-ScheduledTask -TaskName "VM Prevent Sleep" -Action $action -Trigger $trigger -Principal $principal -Settings $settings -Force

Write-Host "Scheduled task 'VM Prevent Sleep' has been created successfully."
```

That's it! The tool will now start automatically with Windows and run silently in the background. You need to start it yourself once, or just reboot.

***


### Why Run as SYSTEM?

The scheduled task is configured to run as `NT AUTHORITY\SYSTEM` for a critical reason: **visibility**.

On a multi-user machine, a program running under a standard user account can only see processes belonging to that user. If another user logs in and starts a VMware machine, a user-level instance of this tool wouldn't see it.

By running as `SYSTEM`, the tool has a complete view of **all processes from all users**, ensuring it works reliably no matter who is logged in or who started the virtual machine. This approach is more robust and efficient than running separate instances for each user.


## Customization

You can modify the source code (`vmpreventsleep.c`) to change:
* `targetProcess`: Change the process name if you want to monitor a different application (e.g., VirtualBox).
* `Sleep(30000)`: Adjust the polling interval. 30000 milliseconds (30 seconds) is a reasonable default that balances responsiveness with low resource usage.


## License

This project is open source. Feel free to use, modify, and distribute it.