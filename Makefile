CPPFILES:=$(wildcard *.cpp)
OBJFILES:=$(patsubst %.cpp,obj/%.o,$(CPPFILES))
NAME:=jetson-nano-random-cams

$(NAME):	setup $(OBJFILES)
	g++ -o "$(NAME)" -g -Wall -Werror $(OBJFILES) -lfltk -lpthread
	@echo "you may want to 'make install' to install"

install:
	@if [ `whoami` = 'root' ]; then echo "\nPlease don't run install as root; it uses sudo when appropriate.\n"; exit 1; fi
	mkdir -p "$$HOME/.config/autostart"
	cp -fp "$(NAME).desktop" "$$HOME/.config/autostart/"
	sudo cp -fp "$(NAME)" /usr/local/bin/

clean:
	rm -rf obj "$(NAME)"

setup:
	[ -f /usr/lib/aarch64-linux-gnu/libfltk.so.1.3 ] || sudo apt install libfltk1.3-dev

obj/%.o:	%.cpp
	@mkdir -p obj
	g++ -MMD -c -std=c++14 -g -Wall -Werror -o $@ $<

-include $(patsubst %.o,%.d,$(OBJFILES))
