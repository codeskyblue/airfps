package main

// #cgo CFLAGS: -DCGO=1
// #cgo LDFLAGS: -lm -llog
// #include "inject.h"
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
	pid     = flag.Int("pid", 0, "pid of process to inject")
	pname   = flag.String("pname", "/system/bin/surfaceflinger", "process name(ignore if pid set)")
	libpath = flag.String("libpath", "", "libpath to inject")
	restore = flag.Bool("restore", false, "hook restore")
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

const LIBMYFPS = "libs/armeabi/libmyfps.so"

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
	fmt.Println("surfaceflinger pid=", sfpid)

	dir := filepath.Join(filepath.Dir(os.Args[0]), "_tmp")
	dir, _ = filepath.Abs(dir)
	var err error
	if err = RestoreAsset(dir, LIBMYFPS); err != nil {
		log.Fatal(err)
	}
	storetxt := filepath.Join(dir, "store.txt")
	hookfunc := "hook_entry"
	if *restore {
		hookfunc = "hook_restore"
	}
	C.inject_remote_process(C.pid_t(sfpid), C.CString(filepath.Join(dir, LIBMYFPS)),
		C.CString(hookfunc), unsafe.Pointer(&storetxt), C.size_t(len(storetxt)))
}
