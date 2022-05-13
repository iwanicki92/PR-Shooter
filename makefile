CXX=g++
CC=gcc
override CFLAGS+=-pthread
override CXXFLAGS+=-pthread -std=c++17
EXTENDED_FLAGS=FALSE
DEBUG=TRUE
# PRINT_DEBUG=0 - no printing, 1 mutex locks, 2 array locks, 3 array functions(push/get...)
PRINT_DEBUG=-1
DEBUG_DEFINES=
extended_cflags=-pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Wredundant-decls -Wshadow -Wsign-conversion -Wstrict-overflow=5 -Wswitch-default -Wundef -Werror -Wno-unused
extended_cxxflags=-Wctor-dtor-privacy -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wsign-promo -Wstrict-null-sentinel
bin_dir=./bin
obj_dir=./obj
host_dir=./Host
host_sources=$(wildcard $(host_dir)/*.cpp)
server_dir=./Server
server_sources=$(wildcard $(server_dir)/*.c)
client_dir=./Client
client_sources=$(wildcard $(client_dir)/*.cpp)

# change every server_dir/*.c text to obj_dir/*.o
server_objs=$(server_sources:$(server_dir)/%.c=$(obj_dir)/%.o)
host_objs=$(host_sources:$(host_dir)/%.cpp=$(obj_dir)/%.o)
client_objs=$(client_sources:$(client_dir)/%.cpp=$(obj_dir)/%.o)
test_objs=$(obj_dir)/test_host.o $(obj_dir)/test_client.o
dependencies=$(server_objs:%.o=%.d) $(host_objs:%.o=%.d) $(client_objs:%.o=%.d)

# add debug preprocesor defines and flags
ifeq ($(DEBUG), TRUE)
override DEBUG_DEFINES+=PRINT_DEBUG=$(PRINT_DEBUG)
override CPPFLAGS+=$(foreach DEFINE,$(DEBUG_DEFINES),-D $(DEFINE))
override CFLAGS+=-fsanitize=address -fno-omit-frame-pointer -O0
override CXXFLAGS+=-fsanitize=address -fno-omit-frame-pointer -O0
endif

# if EXTENDED_FLAGS=TRUE then add warning flags to CFLAGS(C) and CXXFLAGS(C++)
ifeq ($(EXTENDED_FLAGS), TRUE)
override CFLAGS+=$(extended_cflags)
override CXXFLAGS+=$(extended_cxxflags)
endif

run: host
	$(bin_dir)/host

rebuild: clean
	$(MAKE) all

all: host client build_test
	@:

host: $(bin_dir)/host
	@:

client: $(bin_dir)/client
	@:

server: $(server_objs)
	@:

test: build_test
	./$(bin_dir)/test

build_test: $(bin_dir)/test_host $(bin_dir)/test_client $(bin_dir)/test
	@:

.PHONY: clean
clean:
	$(RM) $(obj_dir)/* $(bin_dir)/*

# $^ - expanded prerequisites, $@ - target: ./bin/host
$(bin_dir)/host: $(host_objs) $(server_objs) | $(bin_dir) $(obj_dir)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(bin_dir)/client: $(client_objs) | $(bin_dir) $(obj_dir)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(bin_dir)/test_host: $(obj_dir)/test_host.o $(server_objs) | $(bin_dir) $(obj_dir)
	$(CXX) $(CXXFLAGS) $^ -o $@
	
$(bin_dir)/test_client: $(obj_dir)/test_client.o | $(bin_dir) $(obj_dir)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(bin_dir)/test: $(obj_dir)/test.o | $(bin_dir) $(obj_dir)
	$(CXX) $(CXXFLAGS) $^ -o $@

-include $(dependencies)

# server_obj that is in format obj_dir/%.o requires server_dir/%.c source file
$(server_objs): $(obj_dir)/%.o: $(server_dir)/%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -MMD -c $< -o $@

$(host_objs): $(obj_dir)/%.o: $(host_dir)/%.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MMD -c $< -o $@

$(client_objs): $(obj_dir)/%.o: $(client_dir)/%.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MMD -c $< -o $@

$(obj_dir)/test_host.o $(obj_dir)/test_client.o $(obj_dir)/test.o: $(obj_dir)/%.o: ./Test/%.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MMD -c $< -o $@
    
$(bin_dir) $(obj_dir):
	mkdir -p $@
	