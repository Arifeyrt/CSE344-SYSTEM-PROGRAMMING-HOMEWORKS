all: PideShop HungryVeryMuch

PideShop: PideShop.c
	gcc -o PideShop PideShop.c -lpthread -lm

HungryVeryMuch: HungryVeryMuch.c
	gcc -pthread -o HungryVeryMuch HungryVeryMuch.c

clean:
	rm -f PideShop HungryVeryMuch
