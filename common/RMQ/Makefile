CXXFLAGS= -O3 -fomit-frame-pointer -funroll-loops

libRMQ_improved.a: RMQ_n_1_improved.o RMQ_nlogn_1.o
	$(AR) rcu -o $@ $^
