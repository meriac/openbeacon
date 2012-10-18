all: clean
	sox alphabet.wav -r 15625 -c1 -b 8 -u alphabet.raw silence 1 0.2 2% 1 0.2 2% : newfile : restart
	php create.php

clean:
	rm -f alphabe0*.raw [A-Z].png database.raw