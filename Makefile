1: sfs
	rm DISK3.img
	cp -a ~ksilab/oshw4/DISK1.img .
	./sfs <test_cpin> b
	mv DISK1.img DISK3.img

1m: mysfs
	cp -a ~ksilab/oshw4/DISK1.img .
	./mysfs <test_cpin> a
	diff a b -s
	cmp -l DISK1.img DISK3.img

2: sfs
	rm DISK3.img
	cp -a ~ksilab/oshw4/DISK2.img .
	./sfs <test_cpin_full> b
	mv DISK2.img DISK3.img

2m: mysfs
	cp -a ~ksilab/oshw4/DISK2.img .
	./mysfs <test_cpin_full> a
	diff a b -s
	cmp -l DISK2.img DISK3.img

3: sfs
	rm ok12sfs
	rm DISK3.img
	cp -a ~ksilab/oshw4/DISK1.img .
	./sfs <test_cpout> b
	mv DISK1.img DISK3.img

3m: mysfs
	rm ok12sfs
	cp -a ~ksilab/oshw4/DISK1.img .
	./mysfs <test_cpout> a
	diff a b -s
	cmp -l DISK1.img DISK3.img

4: sfs
	rm DISK3.img
	cp -a ~ksilab/oshw4/DISKFull.img .
	./sfs <test_full_cmds> b
	mv DISKFull.img DISK3.img

4m: mysfs
	cp -a ~ksilab/oshw4/DISKFull.img .
	./mysfs <test_full_cmds> a
	diff a b -s
	cmp -l DISKFull.img DISK3.img

5: sfs
	rm DISK3.img
	cp -a ~ksilab/oshw4/DISK1.img .
	./sfs <test_lscd> b
	mv DISK1.img DISK3.img

5m: mysfs
	cp -a ~ksilab/oshw4/DISK1.img .
	./mysfs <test_lscd> a
	diff a b -s
	cmp -l DISK1.img DISK3.img

6: sfs
	rm DISK3.img
	cp -a ~ksilab/oshw4/DISK1.img .
	./sfs <test_mkdir> b
	mv DISK1.img DISK3.img

6m: mysfs
	cp -a ~ksilab/oshw4/DISK1.img .
	./mysfs <test_mkdir> a
	diff a b -s
	cmp -l DISK1.img DISK3.img

7: sfs
	rm DISK3.img
	cp -a ~ksilab/oshw4/DISK1.img .
	./sfs <test_mv> b
	mv DISK1.img DISK3.img

7m: mysfs
	cp -a ~ksilab/oshw4/DISK1.img .
	./mysfs <test_mv> a
	diff a b -s
	cmp -l DISK1.img DISK3.img

8: sfs
	rm DISK3.img
	cp -a ~ksilab/oshw4/DISK1.img .
	./sfs <test_rm> b
	mv DISK1.img DISK3.img

8m: mysfs
	cp -a ~ksilab/oshw4/DISK1.img .
	./mysfs <test_rm> a
	diff a b -s
	cmp -l DISK1.img DISK3.img

9: sfs
	rm DISK3.img
	cp -a ~ksilab/oshw4/DISK1.img .
	./sfs <test_rmdir> b
	mv DISK1.img DISK3.img

9m: mysfs
	cp -a ~ksilab/oshw4/DISK1.img .
	./mysfs <test_rmdir> a
	diff a b -s
	cmp -l DISK1.img DISK3.img

aa: sfs
	rm DISK3.img
	cp -a ~ksilab/oshw4/DISK1.img .
	./sfs <test_stress_dir> b
	mv DISK1.img DISK3.img

aam: mysfs
	cp -a ~ksilab/oshw4/DISK1.img .
	./mysfs <test_stress_dir> a
	diff a b -s
	cmp -l DISK1.img DISK3.img

bb: sfs
	rm DISK3.img
	cp -a ~ksilab/oshw4/DISK1.img .
	./sfs <test_touch> b
	mv DISK1.img DISK3.img

bbm: mysfs
	cp -a ~ksilab/oshw4/DISK1.img .
	./mysfs <test_touch> a
	diff a b -s
	cmp -l DISK1.img DISK3.img
cc: sfs
	rm ex
	rm ex1
	rm exe
	rm exe1
	rm DISK3.img
	cp -a ~ksilab/oshw4/DISK1.img .
	./sfs <test_0> b
	mv DISK1.img DISK3.img
	mv ex ex1
	mv exe exe1

ccm: mysfs
	cp -a ~ksilab/oshw4/DISK1.img .
	./mysfs <test_0> a
	diff a b -s
	diff ex ex1 -s
	diff exe exe1 -s
	cmp -l ex ex1
	cmp -l exe exe1
	cmp -l DISK1.img DISK3.img
