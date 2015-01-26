steps
===============

* x86-64寄存器介绍: <http://www.searchtb.com/2013/03/x86-64_register_and_function_frame.html>
* ptrace经典教学: <http://www.linuxjournal.com/article/6100?page=0,2>
* ptrace参数介绍: <http://www.tutorialspoint.com/unix_system_calls/ptrace.htm>
* X86-64调用约定: <http://zh.wikipedia.org/wiki/X86%E8%B0%83%E7%94%A8%E7%BA%A6%E5%AE%9A>
* 入栈顺序：<http://lli_njupt.0fees.net/ar01s05.html>

### trap counter

ref: <http://blog.sina.com.cn/s/blog_5b9ea9840100d947.html>

	gcc trap_counter.c dump.c -o trap
	gcc counter.c -o counter

	# open a window
	./counter

	./trap <pid>
