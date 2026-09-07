CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter -Isrc -Ithird_party/stb
LDFLAGS = -l:libX11.so.6 -l:libGL.so.1 -lpthread

SRCS = src/main.cpp \
       src/ViewerApp.cpp \
       src/ImageLoader.cpp \
       src/FolderNavigator.cpp \
       src/HudRenderer.cpp \
       src/ThumbnailBar.cpp \
       src/CropToolbar.cpp \
       src/Localization.cpp \
       src/platform/D2DCompat.cpp \
       src/platform/PlatformLinux.cpp

OBJS = $(SRCS:.cpp=.o)
TARGET = GhostView

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
