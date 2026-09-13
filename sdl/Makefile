# SDL2 desktop build for Goom (macOS; same layout intended for
# Windows/Linux later).
#
#   make        # build build/<os>/goom-sdl
#   make run    # build, then run against the staged goom-data resources

TOP := ..
UNAME := $(shell uname -s)
BUILD := build/$(UNAME)
TARGET := $(BUILD)/goom-sdl

CC  ?= cc
CXX ?= c++

SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LIBS   := $(shell sdl2-config --libs)

INCLUDES := \
	-I. \
	-I$(TOP)/gametools \
	-I$(TOP)/iosfc \
	-I$(TOP)/iosfc/patools \
	-I$(TOP)/libs/soil

# GT_NO_FREETYPE: OpenGlDrawContext.cpp uses its inert font stub instead of
# libs/soil's freetype-backed GLFreeType.cpp (not compiled in below).
DEFINES := -DALIHOOD_STRICT_ASSETS=0 -DkIncludeEditor=0 -DGT_NO_FRIBIDI -DGT_NO_FREETYPE
ifeq ($(UNAME),Darwin)
DEFINES += -DMACOSX=1
PLATFORM_LIBS := -framework OpenGL -framework CoreFoundation -framework Carbon -lz
endif
ifeq ($(OS),Windows_NT)
DEFINES += -DWIN32=1
PLATFORM_LIBS := -lopengl32 -lz -lws2_32
endif

COMMON_FLAGS := -O2 -g -ffast-math -w $(INCLUDES) $(DEFINES) $(SDL_CFLAGS) \
	-include GoomPrefix.h -MMD -MP
# Goom core C files are ISO-8859-1; -Wno-invalid-source-encoding keeps clang
# happy (harmless under the file-wide -w).
CFLAGS   := $(COMMON_FLAGS) -Wno-invalid-source-encoding
CXXFLAGS := $(COMMON_FLAGS) -std=gnu++14

# ---- sdl binding -----------------------------------------------------------
SRC_SDL := \
	SdlMain.cpp \
	SdlGraphics.cpp \
	SdlEventManager.cpp \
	SdlAudioManager.cpp \
	SdlGtCloud.cpp \
	SdlSupport.cpp

# ---- iosfc -----------------------------------------------------------------
SRC_IOSFC := $(addprefix $(TOP)/iosfc/, \
	ios_exception.cpp ios_filepath.cpp ios_hash.cpp \
	ios_memory.cpp ios_stream.cpp ios_time.cpp \
	posix/ios_unixmuteximpl.cpp)


# ---- gametools -------------------------------------------------------------
SRC_GAMETOOLS := $(addprefix $(TOP)/gametools/, \
	AppDelegate.cpp BufferedDrawTarget.cpp \
	Commander.cpp CompositeDrawContext.cpp DataPathManager.cpp \
	FPDataPathManager.cpp FontPackage.cpp \
	GTAnisoSurface.cpp \
	GTAtlasLoader.cpp GTLog.cpp GTPak.cpp GTPlatform.cpp \
	GTPreferences.cpp GTRenderer.cpp \
	GTScreenOrientation.cpp GTTestGroup.cpp GTUI.cpp \
	LocalAchievementsManager.cpp LocalizedDictionary.cpp \
	MusicPackage.cpp NotifyCenter.cpp OpenGlDrawContext.cpp OverlayManager.cpp \
	audio.cpp audiomanager.cpp drawcontext.cpp \
	gameloop.cpp gameui.cpp graphicsmanager.cpp mt19937ar.cpp)

# ---- vendored libs ---------------------------------------------------------
SRC_SOIL := $(addprefix $(TOP)/libs/soil/, \
	SOIL.c image_DXT.c image_helper.c stb_image_aug.c)

SRC_GOOM := \
	$(TOP)/Goom/goom_core.c \
	$(TOP)/Goom/filters.c \
	$(TOP)/Goom/zoom_filter_neon.c \
	$(TOP)/Goom/ifs.c \
	$(TOP)/Goom/tentacle3d.c \
	$(TOP)/Goom/surf3d.c \
	$(TOP)/Goom/convolve_fx.c \
	$(TOP)/Goom/lines.c \
	$(TOP)/Goom/drawmethods.c \
	$(TOP)/Goom/v3d.c \
	$(TOP)/Goom/config_param.c \
	$(TOP)/Goom/flying_stars_fx.c \
	$(TOP)/Goom/gfontlib.c \
	$(TOP)/Goom/gfontrle.c \
	$(TOP)/Goom/plugin_info.c \
	$(TOP)/Goom/cpu_info.c \
	$(TOP)/Goom/sound_tester.c \
	$(TOP)/Goom/mathtools.c \
	$(TOP)/Goom/goom_tools.c \
	$(TOP)/Goom/graphic.c \
	$(TOP)/Goom/GoomWidget.cpp \
	$(TOP)/Goom/GoomAudioSourceHeartBeat.cpp \
	$(TOP)/sdl/GoomMain.cpp \
	$(TOP)/sdl/GoomSdlAppDelegate.cpp \
	$(TOP)/sdl/GoomAudioSourceSdlMic.cpp \
	$(TOP)/sdl/GoomAudioSourceWav.cpp \
	$(TOP)/sdl/GoomControlServer.cpp

SRC := $(SRC_SDL) $(SRC_IOSFC) $(SRC_GAMETOOLS) \
	$(SRC_SOIL) $(SRC_GOOM)

# Window/org strings for the shared SdlMain (guarded by #ifndef).
CPPFLAGS += -DGOOM_SDL_DESKTOP -I$(TOP)/Goom
CPPFLAGS += -DSDL_GAME_TITLE=\"Goom\" -DSDL_GAME_ORG=\"Fovea\" -DSDL_GAME_NAME=\"Goom\"
# Fold CPPFLAGS into CFLAGS/CXXFLAGS so they actually reach the sources.
CFLAGS += $(CPPFLAGS)
CXXFLAGS += $(CPPFLAGS)

.DEFAULT_GOAL := default

OBJ := $(patsubst %.cpp,$(BUILD)/%.o,$(patsubst %.c,$(BUILD)/%.o,$(subst $(TOP)/,,$(SRC))))

# Header dependency tracking: -MMD -MP above emits a .d next to each .o;
# include them so any header edit rebuilds the affected objects.
-include $(OBJ:.o=.d)

default: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CXX) -o $@ $(OBJ) $(SDL_LIBS) $(PLATFORM_LIBS)

$(BUILD)/%.o: $(TOP)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD)/%.o: $(TOP)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

goom-data:
	mkdir -p goom-data/data/goom.001/sfx
	cp $(TOP)/Goom/data/sfx/heartbeat.wav        goom-data/data/goom.001/sfx/

# Run against the staged goom-data resources.
# (Manual smoke testing only — opens a window; do NOT invoke from builds.)
# Extra args via ARGS, e.g.: make run ARGS="--no-mic --fullscreen"
run: default goom-data
	./$(TARGET) --resources goom-data $(ARGS)

clean:
	rm -rf build

.PHONY: default run goom-data clean
