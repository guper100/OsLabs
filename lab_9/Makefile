all:
	gcc dd_alt.c -o dd_alt
	./dd_alt /dev/null 5
	./dd_alt /dev/zero 5
	./dd_alt /dev/urandom 5
	./dd_alt dd_alt 1000