package main

// #cgo CFLAGS: -DGOLANG=1
// #cgo LDFLAGS: -lm -llog -landroid -lEGL -lGLESv2
// #include "inject.h"
// #include "utils.h"
import "C"
import (
	"flag"
	"fmt"
	"log"
	"os"
	"os/signal"
	"path/filepath"
	"unsafe"
)

var (
	pid      = flag.Int("p", 0, "pid of process to inject")
	pname    = flag.String("pname", "/system/bin/surfaceflinger", "process name(ignore if pid set)")
	libpath  = flag.String("l", "", "libpath to inject")
	hookfunc = flag.String("n", "hook_entry", "hook init function")

	repllib  = flag.String("rl", "/data/local/tmp/libfoo.so", "hooked library")
	replfunc = flag.String("rf", "eglSwapBuffers", "hooked function name")
)

func SurfacePid() int {
	if *pid > 0 {
		return *pid
	} else {
		sfpid := int(C.find_pid_of(C.CString(*pname)))
		if sfpid == -1 {
			log.Fatalf("No such process: %s", *pname)
		}
		return sfpid
	}
}

const (
	LIBMYFPS = "libs/armeabi/libmyfps.so"
	LIBSF    = "/system/lib/libsurfaceflinger.so"
)

func TrapSignal(sigs ...os.Signal) {
	ch := make(chan os.Signal, 1)
	for _, sig := range sigs {
		signal.Notify(ch, sig)
	}
	go func() {
		for sig := range ch {
			fmt.Println("Receive signal: ", sig)
		}
	}()
}

func main() {
	flag.Parse()
	sfpid := SurfacePid()
	fmt.Printf("[%s] pid: %d\n", *pname, sfpid)

	dir := filepath.Join(filepath.Dir(os.Args[0]), "_tmp")
	dir, _ = filepath.Abs(dir)
	var err error

	if *libpath == "" {
		if err = RestoreAsset(dir, LIBMYFPS); err != nil {
			log.Fatal(err)
		}
		*libpath = filepath.Join(dir, LIBMYFPS)
	} else {
		*libpath, _ = filepath.Abs(*libpath)
	}

	mbase := C.get_module_base(C.pid_t(sfpid), C.CString(*repllib))
	if mbase == C.long(0) {
		log.Fatal("maybe you need root previlage")
	}
	log.Printf("module base: 0x%x\n", mbase)

	func_addr := C.find_got_entry_address(C.pid_t(sfpid),
		C.CString(*repllib), C.CString(*replfunc))
	log.Printf("func address: 0x%x\n", func_addr)
	arg := fmt.Sprintf("0x%x\n", func_addr)
	//arg := "abcdefg"

	C.inject_remote_process(C.pid_t(sfpid), C.CString(*libpath),
		C.CString(*hookfunc), unsafe.Pointer(C.CString(arg)), C.size_t(len(arg)))
}
