OBJ = src/main.c src/sensors.c src/helpers.c src/base.c src/structures.c

ALL: $(OBJ)
	mpicc -Wall -Iheaders $(OBJ) -o main_out -lm

run: ./main_out
	mpirun -np 21 -oversubscribe ./main_out 5 4 10000

run_caas: ./main_out
	mpirun -np 21 -oversubscribe ./main_out 5 4 10000 -caas

clean:
	/bin/rm -f main_out *.o
	/bin/rm -f rec_sentinel.txt