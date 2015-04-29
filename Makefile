#onvif ¹¤³ÌmakefileÄ¿Â¼
PROJDIR = .
OBJSDIR = $(PROJDIR)/objects
INCSDIR = $(PROJDIR)/include 

SRCS  += $(wildcard $(PROJDIR)/*.c)
OBJS  := $(addprefix $(OBJSDIR)/, $(addsuffix .o, $(basename $(notdir $(SRCS)))))
INCS  += -I$(INCSDIR)

CC = gcc -DDEBUG -DWITH_OPENSSL

#CFLAGS  += -g -O2
CFLAGS  += -O2 
CFLAGS  += -I$(INCSDIR)

#LDFLAGS += -ldl -lm -lpthread

EXENAME = onviftest 
DEPFILE = deps

.PHONY:all
all: $(DEPFILE)  $(EXENAME) 

$(DEPFILE): $(SRCS) Makefile
	@echo "make onvif test ...";
	@-rm -f $(DEPFILE)
	@for f in $(SRCS); do \
		OBJ=$(OBJSDIR)/`basename $$f|sed  -e 's/\.c/\.o/'`; \
		echo $$OBJ: $$f >> $(DEPFILE); \
		echo '	$(CC) $$(CFLAGS) -c -o $$@ $$^'>> $(DEPFILE); \
		done

-include $(DEPFILE)
$(EXENAME): $(OBJS)
	$(CC) $(CFLAGS)  $(OBJS) -o "$@"  $(LDFLAGS) -lcrypto -lssl


.PHONY:clean
clean:
	rm -fr $(OBJSDIR)/*.o
	rm -fr $(DEPFILE)
	rm -fr $(EXENAME)
