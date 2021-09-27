# name of your application
APPLICATION = ocaml-nrf52

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/..
BINDIR ?= $(CURDIR)/bin/nrf52840-mdk
export OCAMLDIR ?= $(CURDIR)/ocaml

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 0

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 0

BOARD=nrf52840-mdk

# USEMODULE += xtimer

APPDIR = $(CURDIR)/example

# CAMLRUN = /opt/homebrew/bin/ocamlrun

# LINKFLAGS = $(OCAMLDIR)/runtime/libcamlrun.a -lm

include $(RIOTBASE)/Makefile.include

CFLAGS := $(subst \",",$(CFLAGS))
CFLAGS := $(subst ',,$(CFLAGS))
CFLAGS := $(subst -Wall,,$(CFLAGS))
CFLAGS := $(subst -Wstrict-prototypes,,$(CFLAGS))
CFLAGS := $(subst -Wextra,,$(CFLAGS))
CFLAGS := $(subst -Wold-style-definition,,$(CFLAGS))
CFLAGS := $(subst -O3,,$(CFLAGS))
CFLAGS := $(subst -O2,,$(CFLAGS))
CFLAGS := $(subst -O1,,$(CFLAGS))
CFLAGS += -I$(CURDIR)/ocaml/runtime -Wno-format-nonliteral -Wno-strict-prototypes -s 

ASPPFLAGS := CFLAGS

all:
	$(RM) -f example/hello_caml.c
	ocamlc -output-obj -o example/hello_caml.c example/hello.ml
	$(MAKE) ocaml
	$(MAKE) riot
	cd bin/nrf52840-mdk ; \
		arm-none-eabi-objcopy --only-keep-debug $(APPLICATION).elf $(APPLICATION).debug ; \
		arm-none-eabi-strip --strip-debug $(APPLICATION).elf ; \
		arm-none-eabi-objcopy --add-gnu-debuglink=$(APPLICATION).debug $(APPLICATION).elf

riot:
	$(MAKE) -C $(RIOTBASE) -f Makefile.include link CFLAGS='$(CFLAGS)'

ocaml: Makeconf ocaml/Makefile.config ocaml/runtime/caml/version.h
	$(MAKE) -C ocaml/runtime libcamlrun.a CFLAGS='$(CFLAGS) --specs=nosys.specs'


Makeconf:
	CC="$(MAKECONF_CC)" \
	MAKECONF_CFLAGS="$(CFLAGS) $(OC_CFLAGS) $(GLOBAL_CFLAGS) --specs=nosys.specs" \
	AS="$(MAKECONF_AS)" \
	ASPP="$(MAKECONF_CC) $(OC_CFLAGS) -c" \
	ASPPFLAGS="$(ASPPFLAGS)" \
	CPPFLAGS="$(OC_CFLAGS)" \
	LIBS="$(OC_LIBS)"\
	LD="$(MAKECONF_LD)" \
	SYSTEM="eabi" \
	MODEL="none" \
	CAMLRUN="$(CAMLRUN)" \
	./configure.sh

ocaml/Makefile:
	cp -r `ocamlfind query ocaml-src` ./ocaml
	sed -i -e 's/runtime\/ocamlrun tools/$$(CAMLRUN) tools/g' ocaml/Makefile
	sed -i -e 's!ASPPFLAGS =!ASPPFLAGS = $(CFLAGS)!g' ocaml/runtime/Makefile
	sed -i -e 's/otherlibraries="dynlink"/otherlibraries=""/g' ocaml/configure
	sed -i -e 's/oc_cflags="/oc_cflags="$$OC_CFLAGS /g' ocaml/configure
	sed -i -e 's/ocamlc_cflags="/ocamlc_cflags="$$OCAMLC_CFLAGS /g' ocaml/configure
	cp stubs/sys.c ocaml/runtime/sys.c
	cp stubs/unix.c ocaml/runtime/unix.c
	cp stubs/interp.c ocaml/runtime/interp.c
	cp stubs/fail.c ocaml/runtime/fail.c
	cp stubs/config.h ocaml/runtime/caml/config.h
	cp stubs/io.h ocaml/runtime/caml/io.h
	mkdir -p bin/nrf52840-mdk/riotbuild
	cp stubs/riotbuild.h bin/nrf52840-mdk/riotbuild/riotbuild.h


ocaml/Makefile.config: ocaml/Makefile
	cd ocaml && \
		CC="$(MAKECONF_CC)" \
		CFLAGS="$(LOCAL_CFLAGS)" \
		AS="$(MAKECONF_AS)" \
		ASPP="$(MAKECONF_CC) $(OC_CFLAGS) -c" \
		ASPPFLAGS="$(ASPPFLAGS)" \
		CPPFLAGS="$(OC_CFLAGS)" \
		LIBS="$(OC_LIBS)"\
		LD="$(MAKECONF_LD)" \
		SYSTEM="eabi" \
		MODEL="none" \
		CAMLRUN="$(CAMLRUN)" \
	  ./configure \
		-host=$(MAKECONF_BUILD_ARCH)-none-eabi \
		-disable-shared\
		-disable-systhreads\
		-disable-unix-lib\
		-disable-instrumented-runtime
	echo "ARCH=$(MAKECONF_OCAML_BUILD_ARCH)" >> ocaml/Makefile.config
	echo '#undef HAS_SOCKETS' >> ocaml/runtime/caml/s.h
	echo '#undef OCAML_OS_TYPE' >> ocaml/runtime/caml/s.h
	echo '#define OCAML_OS_TYPE "None"' >> ocaml/runtime/caml/s.h

ocaml/runtime/caml/version.h: ocaml/Makefile.config
	ocaml/tools/make-version-header.sh > $@



# -----------------------------------------------------------------------------------------------------------------------------

.PHONY: clean

include Makeconf

# CFLAGS used to build nolibc / openlibm / ocaml runtime
LOCAL_CFLAGS=$(MAKECONF_CFLAGS) --specs=nosys.specs -I$(RIOTBASE)/core/include -I$(RIOTBASE)/drivers/include -I$(RIOTBASE)/sys/include -I$(RIOTBASE)/boards/nrf52840-mdk/include -I$(RIOTBASE)/boards/common/nrf52/include -I$(RIOTBASE)/cpu/nrf52/include -I$(RIOTBASE)/cpu/nrf5x_common/include -I$(RIOTBASE)/cpu/cortexm_common/include -I$(RIOTBASE)/cpu/cortexm_common/include/vendor
# CFLAGS used by the OCaml compiler to build C stubs
GLOBAL_CFLAGS=$(MAKECONF_CFLAGS) --specs=nosys.specs -I$(RIOTBASE)/core/include -I$(RIOTBASE)/drivers/include -I$(RIOTBASE)/sys/include -I$(RIOTBASE)/boards/nrf52840-mdk/include -I$(RIOTBASE)/boards/common/nrf52/include -I$(RIOTBASE)/cpu/nrf52/include -I$(RIOTBASE)/cpu/nrf5x_common/include -I$(RIOTBASE)/cpu/cortexm_common/include -I$(RIOTBASE)/cpu/cortexm_common/include/vendor

CAMLOPT:=$(shell which ocamlopt)
CAMLRUN:=$(shell which ocamlrun)
CAMLC:=$(shell which ocamlc)

clean:
	$(RM) -f example/hello_caml.c example/hello_caml.cds example/hello.cmo example/hello.cmi
	$(RM) -r ocaml/
	$(RM) Makeconf
	$(RM) -r bin/

