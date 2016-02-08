



shmem_am_1.out: shmem_am_1.c
	oshcc -DGASNET_PAR  $^ -I ../shmem_am/ -L /opt/berkeley/gasnet/openmpi/gnu/1.26.0/fast/lib/ -L/usr/lib64 -L/opt/SLURM/lib64/   -lgasnet-ibv-par -libverbs   -lpmi -I /opt/berkeley/gasnet/openmpi/gnu/1.26.0/fast/include/ -I /opt/berkeley/gasnet/openmpi/gnu/1.26.0/fast/include/ibv-conduit/ -L /usr/lib64/gcc/x86_64-suse-linux/4.8 -lgcc -lm -lrt -o ../bin/$@

clean:
	rm -rf  ../bin/*.out




