set(target i686-pc-win32)
set(cpu_type i386)
set(tm_defines)
set(tm_file
	i386/i386.h
	i386/unix.h
	i386/bsd.h
	i386/gas.h
	dbxcoff.h
	i386/cygming.h
	i386/mingw32.h)
set(xm_defines POSIX)
set(xm_file i386/xm-mingw32.h)
set(host_xm_defines POSIX)
set(host_xm_file i386/xm-mingw32.h)
set(build_xm_defines POSIX)
set(build_xm_file i386/xm-mingw32.h)
set(tm_p_file i386/i386-protos.h)
set(extra_modes i386/i386-modes.def)
set(extra_objs winnt.o winnt-stubs.o)
set(extra_options i386/i386.opt i386/cygming.opt)
set(c_target_objs)
set(cxx_target_objs winnt-cxx.o)
set(target_cpu_default)
set(out_host_hook_obj host-mingw32.o)
