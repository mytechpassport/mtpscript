# MTPScript Makefile - Core Runtime Build
# After successful migration, this Makefile contains only working targets

CONFIG_SMALL=y

ifdef CONFIG_WIN32
  ifdef CONFIG_X86_32
    CROSS_PREFIX?=i686-w64-mingw32-
  else
    CROSS_PREFIX?=x86_64-w64-mingw32-
  endif
  EXE=.exe
else
  CROSS_PREFIX?=
  EXE=
endif

HOST_CC=gcc
CC=$(CROSS_PREFIX)gcc
MYSQL_CFLAGS=$(shell mysql_config --include)
MYSQL_LDFLAGS=$(shell mysql_config --libs)

# Core include paths for migrated structure
CFLAGS=-Wall -g -MMD -D_GNU_SOURCE -DMTPSCRIPT_DETERMINISTIC -fno-math-errno -fno-trapping-math -I/usr/local/opt/openssl@1.1/include $(MYSQL_CFLAGS) -I. -Icore/runtime -Icore/stdlib -Icore/crypto -Icore/db -Icore/http -Icore/effects -Icore/utils -Ibuild/generated
HOST_CFLAGS=-Wall -g -MMD -D_GNU_SOURCE -DMTPSCRIPT_DETERMINISTIC -fno-math-errno -fno-trapping-math $(MYSQL_CFLAGS) -Icore/runtime -Icore/stdlib -Icore/crypto -Icore/db -Icore/http -Icore/effects -Icore/utils -Ibuild/generated

ifdef CONFIG_SMALL
CFLAGS+=-Os
else
CFLAGS+=-O1
endif

HOST_CFLAGS+=-O3 -DHOST_BUILD
LDFLAGS=-g
HOST_LDFLAGS=-g

PROGS=mtpjs$(EXE) example$(EXE)
TEST_PROGS=dtoa_test libm_test

all: mtpjs_stdlib build/generated/mquickjs_atom.h $(PROGS)

# Core runtime object files (migrated structure)
# Core runtime object files (migrated structure)
MTPJS_OBJS=src/main/mtpjs.o src/main/readline_tty.o src/main/readline.o core/runtime/mquickjs.o core/crypto/mquickjs_crypto.o core/effects/mquickjs_effects.o core/db/mquickjs_db.o core/http/mquickjs_http.o core/effects/mquickjs_log.o core/stdlib/mquickjs_api.o core/runtime/mquickjs_errors.o core/utils/dtoa.o core/utils/libm.o core/utils/cutils.o
LIBS=-lm -L/usr/local/opt/openssl@1.1/lib -lcrypto $(MYSQL_LDFLAGS) -lcurl

mtpjs$(EXE): $(MTPJS_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

# Stdlib generation for runtime
mtpjs_stdlib: src/stdlib/mtpjs_stdlib.host.o src/stdlib/mquickjs_build.host.o
	$(HOST_CC) $(HOST_LDFLAGS) -o $@ $^

build/generated/mquickjs_atom.h: mtpjs_stdlib
	./mtpjs_stdlib -a > $@

build/generated/mtpjs_stdlib.h: mtpjs_stdlib
	./mtpjs_stdlib > $@

src/main/mtpjs.o: build/generated/mtpjs_stdlib.h

# Example program
example$(EXE): examples/example.o core/runtime/mquickjs.o core/crypto/mquickjs_crypto.o core/effects/mquickjs_effects.o core/db/mquickjs_db.o core/http/mquickjs_http.o core/effects/mquickjs_log.o core/stdlib/mquickjs_api.o core/runtime/mquickjs_errors.o core/utils/dtoa.o core/utils/libm.o core/utils/cutils.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

example_stdlib: examples/example_stdlib.host.o src/stdlib/mquickjs_build.host.o
	$(HOST_CC) $(HOST_LDFLAGS) -o $@ $^

build/generated/example_stdlib.h: example_stdlib
	./example_stdlib > $@

examples/example.o: build/generated/example_stdlib.h

# Build rules
%.host.o: %.c
	$(HOST_CC) $(HOST_CFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Test targets
test: mtpjs example
	./mtpjs tests/integration/test_closure.js
	./mtpjs tests/integration/test_language.js
	./mtpjs tests/integration/test_loop.js
	./mtpjs tests/integration/test_builtin.js
	./mtpjs -o test_builtin.bin tests/integration/test_builtin.js
	./mtpjs -b test_builtin.bin
	./example tests/integration/test_rect.js

microbench: mtpjs
	./mtpjs tests/microbench.js

octane: mtpjs
	./mtpjs --memory-limit 256M tests/octane/run.js

size: mtpjs
	size mtpjs mtpjs.o

# Unit tests
dtoa_test: tests/dtoa_test.o core/utils/dtoa.o core/utils/cutils.o tests/gay-fixed.o tests/gay-precision.o tests/gay-shortest.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

libm_test: tests/libm_test.o core/utils/libm.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

# Cleanup
clean:
	rm -f *.o *.d *~ tests/*.o tests/*.d tests/*~ test_builtin.bin
	rm -rf build/generated/* build/artifacts/*
	rm -f mtpjs_stdlib example_stdlib
	rm -f $(PROGS) $(TEST_PROGS)
	find . -name "*.o" -type f -delete
	find . -name "*.d" -type f -delete

-include $(wildcard *.d)
