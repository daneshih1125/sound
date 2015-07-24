SOUND_TEST := soundtest
SOUND_OBJ := sound.o
SOUND_LIB := libsound.so

CFLAGS += -Wall -DSOUND_DEBUG

all: $(SOUND_TEST) $(SOUND_LIB)

$(SOUND_TEST): $(SOUND_OBJ)
	$(CC) -Wall $(CFLAGS) -DSOUND_TEST sound.c -o $@ -lasound -lm

$(SOUND_LIB): $(SOUND_OBJ)
	$(CC) -Wall -fPIC $(CFLAGS) -o $@ sound.c -shared

clean:
	rm -f $(SOUND_TEST) $(SOUND_LIB) $(SOUND_OBJ)
