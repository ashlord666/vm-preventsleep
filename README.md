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
Register-ScheduledTask -TaskName "VM Prevent Sleep (User)" -Action (New-ScheduledTaskAction -Execute 'C:\Tools\vmpreventsleep.exe') -Trigger (New-ScheduledTaskTrigger -AtLogon) -Settings (New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -ExecutionTimeLimit 0) -Force
```

That's it! The tool will now start automatically with Windows and run silently in the background. You need to start it yourself once, or just reboot.

***


## Customization

You can modify the source code (`vmpreventsleep.c`) to change:
* `targetProcess`: Change the process name if you want to monitor a different application (e.g., VirtualBox).
* `Sleep(30000)`: Adjust the polling interval. 30000 milliseconds (30 seconds) is a reasonable default that balances responsiveness with low resource usage.


## License

This project is open source. Feel free to use, modify, and distribute it.