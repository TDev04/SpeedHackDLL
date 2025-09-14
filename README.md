# SpeedHackDLL (ONLY X86)

Time-Scaler is a DLL project that allows you to change the system time for a specific process. It works by hooking into core Windows timing functions like `QueryPerformanceCounter`, `GetTickCount`, and `GetTickCount64` and scaling their output. This can be useful for various purposes, such as speeding up or slowing down processes.

---

## Features

* **Adjustable Speed:** Scale the process's time from 0% to 10,000% of the normal rate.
* **Multiple Control Methods:** Adjust the speed using either a console input or dedicated hotkeys.
* **Non-intrusive:** Operates by injecting a DLL into the target process, leaving the system-wide time unaffected.
* **Continuous Time:** Maintains time continuity even when changing the speed, preventing sudden jumps or lags.

---

## How it Works

The project uses the **Detours** library to intercept and redirect calls to the following Windows API functions:

* `QueryPerformanceCounter()`: Provides a high-resolution performance counter.
* `GetTickCount()`: Returns the number of milliseconds that have elapsed since the system was started.
* `GetTickCount64()`: A 64-bit version of `GetTickCount` to prevent integer overflow.

When a hooked function is called, the DLL calculates the scaled time based on the current speed factor and returns the new value to the calling process.

---

## Usage

This project is intended to be used by injecting the compiled DLL into a target process. There are many tools available online for DLL injection.

Once the DLL is injected, a console window will appear. You can enter a new speed value as a number.

| Key combination  | Action        |
| ---------------- | ------------- |
| `Ctrl` + `Up`    | Increase speed by 0.1 |
| `Ctrl` + `Down`  | Decrease speed by 0.1 |

**Note:** If the console cannot be allocated for some reason, the hotkeys will still be active.

---

## Building the Project

### Prerequisites

* **Microsoft Visual Studio:** This project requires Visual Studio to compile.
* **Detours Library:** You must have the Microsoft Detours library set up. The project assumes the Detours headers and libraries are configured correctly in your build environment.

### Steps

1.  Clone this repository.
2.  Open the project file in Visual Studio.
3.  Configure your project to link against the Detours library.
4.  Build the project to generate the `SpeedHackDLL.dll` file.

---

## License

This project is open-source and available under the MIT License.
