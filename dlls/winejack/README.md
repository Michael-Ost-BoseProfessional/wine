# winejack
This library provides a mechanism for Windows PE EXEs and DLLs to interface with a JACK server that is running in a Linux ELF system. 

Such programs could possibly work with a Windows version of JACK that is supported by Wine. But this would not allow them to connect with or share hardware with other Linux JACK applications.

## Thunking
winejack uses the Wine unixlib mechanism to translate between Windows-style PE code and Linux-style ELF code. The PE code is called by the Windows EXE or DLLs. The Linux code is then able to interact with the Linux JACK library and server.

As an example, the `jack_activate` method is declared in winejack.h and defined in winejack_pe.c. The function builds a parameter block and thunks through to winejack_linux via the WINE_UNIX_CALL macro. This is picked up on the Linux side with `linux_jack_activate` which is free to call into the JACK library's own `jack_activate` function.

## Callbacks
JACK makes use of callbacks to implement several features including audio processing. The challenge is that the functions are called with the ELF ABI but need to be handled in PE code. To bridge this divide we thunk the callbacks.

As an example, when a client calls `jack_set_process_callback` with a PE function, the PE function is passed through to `linux_jack_set_process_callback`. This function attaches an ELF proxy callback `linux_process_callback` to JACK. When JACK calls the process callback `linux_process_callback` knows to thunk a call to the original PE process callback that was passed in.

The mechanism for thunking callbacks is KeUserDispatchCallback. It converts between the ELF and PE ABIs. 

## Threading
For Windows functions to work in Wine they must be called from a thread created by Wine. They rely on Windows-style thread local data like the TEB.

Linux JACK creates threads in its normal operation so we need those threads to be Wine-friendly. JACK has a mechanism which allows threads to be created by a callback: jack_set_thread_creator. We use that to provide Wine-friendly threads to JACK.

The API is a little tricky to support. Here is the sequence of events to provide a thread to JACK:

- `winejack_pe` registers the `pe_create_thread_callback` with winejack_linux at attach time
- `winejack_linux` connects its `linux_create_thread_callback` to JACK with `jack_set_thread_creator`
- when JACK wants a thread it invokes `linux_create_thread_callback`. This thunks a call to `pe_create_thread_callback` in winejack_pe
- `linux_create_thread_callback` then waits on a condition variable for the thread to start
- `pe_create_thread_callback` calls CreateThread with `pe_thread_entry` as its thread function
- `pe_thread_entry` is unable to call the Linux-side ELF thread function that JACK wants. So it makes a thunked call to the linux side to run the pthread function with run_pthread_id
- `linux_run_pthread` picks up this call, captures the pthread_t of the newly created thread, signals the condition variable, and runs the requested pthread function. This is all happening in the context of the new thread
- the signal frees `linux_create_thread_callback` to continue and return the created thread to JACK

Shew!

## Using Winejack
Windows applications cross compiled for Linux can include the included `winejack.h` file to provide the basic JACK API. 

Wine built with this patch will create the required winejack.dll (PE side) and winejack.so (Linux side).

Winejack.dll is automatically copied into `c:\windows\system32` when Wine builds its wine directory. If you update it during development you must copy your new version over the old one. 

The 'winejack' TRACE channel is available for debugging.
