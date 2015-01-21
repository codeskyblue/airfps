## airfps
Get FPS(frame per second) on android. Root privileges required.

## Still developing
Not working for now.


## 编译指南
代码中用了Golang，Android-NDK. [Golang-mobile](https://github.com/golang/mobile)的官网推荐用docker

	docker pull golang/mobile
	docker run -v $GOPATH/src:/src golang/mobile /bin/bash -c 'cd /src/your/project && ./make.bash'

不过手工搭建个环境也不复杂，参考着[Dockerfile](https://github.com/golang/mobile/blob/master/Dockerfile)中的描述一点点的来就可以。
Golang需要1.4+的版本，直接下载个就好 <http://golang.org/dl/>

不要忘了把CGO打开，我有时就会忘了`export CGO_ENABLED=1`

## 感谢
* [未完成的项目realfps](https://github.com/cuitteam/RealFPS)
* [fpsmeter反编译分析](http://blog.csdn.net/freshui/article/details/9245511#comments)
* [jinzhuojun的CSDN-如何写出一个显示fps的东西](http://blog.csdn.net/jinzhuojun/article/details/10428435)
* [hook的原理](http://bbs.pediy.com/showthread.php?t=157419)

## LICENSE
Under [GPL v2](LICENSE)
