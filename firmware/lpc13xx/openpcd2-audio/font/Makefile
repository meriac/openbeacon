all: clean
	sox alphabet.wav -r 15625 -c1 -b 8 -u alphabet.raw silence 1 0.2 2% 1 0.2 2% : newfile : restart
	php create.php

store: all
	sudo dd_rescue -D database.raw /run/media/$(USER)/OPENBEACON/DATA1234.BIN

clean:
	rm -f alphabet0*.raw [A-Z].png database.raw
