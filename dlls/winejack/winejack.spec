# Wine JACK driver exports

@ stdcall jack_activate(int64)
@ stdcall jack_client_close(int64)
@ stdcall jack_client_open(str long ptr)
@ stdcall jack_deactivate(int64)
@ stdcall jack_get_ports(int64 str str long)
@ stdcall jack_get_sample_rate(int64)
@ stdcall jack_on_shutdown(int64 ptr ptr)
@ stdcall jack_set_buffer_size_callback(int64 ptr ptr)
@ stdcall jack_set_process_callback(int64 ptr ptr)
@ stdcall jack_set_sample_rate_callback(int64 ptr ptr)

@ stdcall jack_connect(int64 str str)
@ stdcall jack_disconnect(int64 str str)
@ stdcall jack_port_get_buffer(int64 long)
@ stdcall jack_port_name(int64)
@ stdcall jack_port_register(int64 str str long long)

@ stdcall jack_free(ptr)
