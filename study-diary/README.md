steps
===============

* x86-64寄存器介绍: <http://www.searchtb.com/2013/03/x86-64_register_and_function_frame.html>
* ptrace经典教学: <http://www.linuxjournal.com/article/6100?page=0,2>
* ptrace参数介绍: <http://www.tutorialspoint.com/unix_system_calls/ptrace.htm>

### trap counter

ref: <http://blog.sina.com.cn/s/blog_5b9ea9840100d947.html>

	gcc trap_counter.c dump.c -o trap
	gcc counter.c -o counter

	# open a window
	./counter

	./trap <pid>
