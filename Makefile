CPPFILES:=$(wildcard *.cpp)
OBJFILES:=$(patsubst %.cpp,obj/%.o,$(CPPFILES))
NAME:=jetson-nano-random-cams

$(NAME):	setup $(OBJFILES)
	g++ -o "$(NAME)" -g -Wall -Werror $(OBJFILES) -L /usr/local/cuda-10.0/targets/aarch64-linux/lib -lfltk -lpthread -ljetson-inference -ljetson-utils -lcuda -lcudart -lcpr -lfltk
	@echo "you may want to 'make install' to install"

install:
	@if [ `whoami` = 'root' ]; then echo "\nPlease don't run install as root; it uses sudo when appropriate.\n"; exit 1; fi
	mkdir -p "$$HOME/.config/autostart"
	cp -fp "$(NAME).desktop" "$$HOME/.config/autostart/"
	sudo sh -c "cp -fp $(NAME).desktop /usr/share/applications/; cp -fp $(NAME) /usr/local/bin/; cp -fp $(NAME).png /usr/share/icons/hicolor/48x48/apps/"

clean:
	rm -rf obj "$(NAME)"

setup:
	[ -f /usr/lib/aarch64-linux-gnu/libfltk.so.1.3 ] || sudo apt install libfltk1.3-dev

obj/%.o:	%.cpp
	@mkdir -p obj
	g++ -MMD -c -std=c++14 -g -Wall -Werror -I /usr/local/cuda-10.0/targets/aarch64-linux/include -o $@ $<

-include $(patsubst %.o,%.d,$(OBJFILES))
