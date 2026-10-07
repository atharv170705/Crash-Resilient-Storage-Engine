CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude

TARGET = minidb

SOURCES = main.cpp \
          src/database.cpp \
          src/storage_engine.cpp \
          src/wal_record.cpp \
          src/crc32.cpp \
          src/wal_serializer.cpp \
          src/wal_manager.cpp

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)