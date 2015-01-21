## airfps
For mobile phone game to get FPS: frame per second.

## Still developing


## 开发者指南
介绍了如何搭建编译环境，如何编译代码

### 环境搭建
代码中用了Golang，Android-NDK. [Golang-mobile](https://github.com/golang/mobile)的官网推荐用docker

	docker pull golang/mobile
	docker run -v $GOPATH/src:/src golang/mobile /bin/bash -c 'cd /src/your/project && ./make.bash'

不过手工搭建个环境也不复杂，参考着[Dockerfile](https://github.com/golang/mobile/blob/master/Dockerfile)中的描述一点点的来就可以。
Golang需要1.4+的版本，直接下载个就好 <http://golang.org/dl/>

不要忘了把CGO打开，我有时就会忘了`export CGO_ENABLED=1`

## LICENSE
Under [GPL v2](LICENSE)
