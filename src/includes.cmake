

set(LINUX_HFILES sgidefs.h)

set(LINUX_SYS_HFILES syssgi.h)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

	include_directories(BEFORE windows/include)

	add_definitions(-DHAVE_ALLOCA_H=1)

endif()

set(INCLUDE_HFILES
	compact_reloc.h
	dwarf.h
	elf_abi.h
	elf_mips.h
	elfaccess.h
	fp_class.h
	isam.h
	libdwarf.h
	libXlate.h
	objlist.h
	obj_list.h
	obj_type.h
	rld_interface.h
	stamp.h
	svr4_math.h)


set(CMPLR_HFILES
	cif_c.h
	dwarf_addr_finder.h
	elf_interfaces.h
	f_errno.h
	events.h
	fb.h
	host.h
	leb128.h
	make_depend.h
	newinst.h
	rcodes.h
	xlate.h)

set(CMPLR_DIRS cmplrs)

set(SYS_HFILES elf_whirl.h inst.h unwind.h unwindP.h)

if(${BUILD_TARGET} MATCHES "IA64")
	set(SYS_DIRS ia64)
endif()

if(${BUILD_TARGET} MATCHES "MIPS")
	set(SYS_DIRS ia64)
endif()

if(${BUILD_TARGET} MATCHES "X8664")
	set(SYS_DIRS ia64)
endif()

set(LIBELF_HFILES elf_repl.h libelf.h nlist.h sys_elf.h)

set(LDIRT dwarf.h)

## To prevent repetitious submakes to this subdirectory, Makefile.gsetup
## files in other subdirectories invoke submake here only when the file
## main_defs.h is absent or out of date.  The dependencies for
## main_defs.h are provided in linux/make/gcommonrules for
## Makefile.gbase files in other build subdirectories.
#
#default: main_defs.h
#	@: Make directories in case they do not exist.
#	@for d in sys cmplrs libelf; do \
#	    if [ ! -d $$d ]; then \
#		mkdir $$d && echo Making $$d; \
#	    fi; \
#	done	A


set(PATHSCALE_INCLUDE_DIR ${PATHSCALE_BINARY_DIR}/include)

file(MAKE_DIRECTORY ${PATHSCALE_INCLUDE_DIR}/sys)
file(MAKE_DIRECTORY ${PATHSCALE_INCLUDE_DIR}/cmplrs)
file(MAKE_DIRECTORY ${PATHSCALE_INCLUDE_DIR}/libelf)

#	@for h in $(LINUX_HFILES); do \
#	    if ! test -e $$h; then \
#	      ln -sf $(BUILD_TOT)/linux/include/$$h $$h; \
#	    fi; \
#	done
foreach(H ${LINUX_HFILES})
	configure_file(linux/include/${H} ${PATHSCALE_INCLUDE_DIR}/${H})
endforeach()


#	@for h in $(LINUX_SYS_HFILES); do \
#	    if ! test -e sys/$$h; then \
#	      ln -sf $(BUILD_TOT)/../linux/include/sys/$$h sys/$$h; \
#	    fi; \
#	done
foreach(H ${LINUX_SYS_HFILES})
	configure_file(linux/include/sys/${H}
		${PATHSCALE_INCLUDE_DIR}/sys/${H})
endforeach()

#	@for h in $(WINDOWS_HFILES); do \
#	    if ! test -e $$h; then \
#	      ln -sf $(BUILD_TOT)/windows/include/$$h $$h; \
#	    fi; \
#	done
foreach(H ${WINDOWS_HFILES})
	configure_file(windows/include/${H} ${PATHSCALE_INCLUDE_DIR}/${H})
endforeach()

#	@for h in $(WINDOWS_SYS_HFILES); do \
#	    if ! test -e sys/$$h; then \
#	      ln -sf $(BUILD_TOT)/../windows/include/sys/$$h sys/$$h; \
#	    fi; \
#	done
foreach(H ${WINDOWS_SYS_HFILES})
	configure_file(windows/include/sys/${H}
		${PATHSCALE_INCLUDE_DIR}/sys/${H})
endforeach()


#	@for h in $(INCLUDE_HFILES); do \
#	    if ! test -e $$h; then \
#	      ln -sf $(BUILD_TOT)/include/$$h $$h; \
#	    fi; \
#	done
foreach(H ${INCLUDE_HFILES})
	configure_file(include/${H} ${PATHSCALE_INCLUDE_DIR}/${H})
endforeach()


#	@for h in $(CMPLR_DIRS); do \
#	    if ! test -e $$h; then \
#	      mkdir $$h; \
#	    fi; \
#	done
file(MAKE_DIRECTORY ${PATHSCALE_INCLUDE_DIR}/cmplrs)


#	@for h in $(CMPLR_HFILES); do \
#	    if ! test -e cmplrs/$$h; then \
#	      ln -sf $(BUILD_TOT)/../include/cmplrs/$$h cmplrs/$$h; \
#	    fi; \
#	done
foreach(H ${CMPLR_HFILES})
	configure_file(include/cmplrs/${H}
		${PATHSCALE_INCLUDE_DIR}/cmplrs/${H})
endforeach()


#	@for h in $(SYS_HFILES); do \
#	    if ! test -e sys/$$h; then \
#	      ln -sf $(BUILD_TOT)/../include/sys/$$h sys/$$h; \
#	    fi; \
#	done
foreach(H ${SYS_HFILES})
	configure_file(include/sys/${H} ${PATHSCALE_INCLUDE_DIR}/sys/${H})
endforeach()


#	@for h in $(SYS_DIRS); do \
#	    if ! test -e sys/$$h; then \
#	      ln -sf $(BUILD_TOT)/../include/sys/$$h sys/$$h; \
#	    fi; \
#	done
foreach(H ${SYS_DIRS})
	execute_process(COMMAND
		${SYMLINK_COMMAND}
		include/sys/${H}
		${PATHSCALE_INCLUDE_DIR}/sys/${H})
endforeach()


#
## If you change these prerequisites, also update main_defs.h in
## linux/make/gcommonrules.
#main_defs.h: $(BUILD_TOT)/include/main_defs.h
#	cp $(BUILD_TOT)/include/main_defs.h $@
#
configure_file(include/main_defs.h.in
	${PATHSCALE_INCLUDE_DIR}/main_defs.h)



foreach(H ${LIBELF_HFILES})
	configure_file(libelf/lib/${H} ${PATHSCALE_INCLUDE_DIR}/libelf/${H})
endforeach()


include_directories(BEFORE SYSTEM include ${PATHSCALE_INCLUDE_DIR})


install(FILES
	include/omp/omp_lib.h
	include/omp/omp_lib.f
	DESTINATION
	include/${PSC_FULL_VERSION})

#clean:
#
#clobber:
#	@rm -rf *.h *.sg $(CMPLR_DIRS) ia64 stl sys

