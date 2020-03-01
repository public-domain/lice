CC      ?= clang
CFLAGS  += -Wall -Wextra -Wno-missing-field-initializers -O3 -std=c99 -MD -DLICE_TARGET_AMD64
LDFLAGS +=

LICESOURCES = ast.c parse.c lice.c gen.c gen_amd64.c lexer.c util.c conv.c decl.c init.c list.c opt.c
ARGSSOURCES = misc/argsgen.c util.c list.c
TESTSOURCES = test.c util.c list.c
LICEOBJECTS = $(LICESOURCES:.c=.o)
TESTOBJECTS = $(TESTSOURCES:.c=.o)
ARGSOBJECTS = $(ARGSSOURCES:.c=.o)
LICEDEPENDS = $(LICESOURCES:.c=.d)
TESTDEPENDS = $(TESTSOURCES:.c=.d)
ARGSDEPENDS = $(ARGSSOURCES:.c=.d)
LICEBIN     = lice
TESTBIN     = testsuite
ARGSBIN     = argsgenerator

all: $(LICEBIN) $(TESTBIN)

$(LICEBIN): $(LICEOBJECTS)
	$(CC) $(LDFLAGS) $(LICEOBJECTS) -o $@

$(TESTBIN): $(TESTOBJECTS)
	$(CC) $(LDFLAGS) $(TESTOBJECTS) -o $@

$(ARGSBIN): $(ARGSOBJECTS)
	$(CC) $(LDFLAGS) $(ARGSOBJECTS) -o $@

c.o:
	$(CC) -c $(CFLAGS) $< -o $@

args: $(ARGSBIN)
	@./$(ARGSBIN)

clean:
	rm -f $(LICEOBJECTS)
	rm -f $(TESTOBJECTS)
	rm -f $(LICEDEPENDS)
	rm -f $(TESTDEPENDS)
	rm -f $(ARGSDEPENDS)
	rm -f $(LICEBIN)
	rm -f $(TESTBIN)
	rm -f $(ARGSBIN)

test: $(LICEBIN) $(TESTBIN)
	@./$(TESTBIN)
