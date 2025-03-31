package common

import (
	"FuckRDP/grdp/glog"
	"math/rand"
	"sync/atomic"
	"time"
)

type SurfaceFlinger struct {
	VSync    chan struct{}
	counter  int64
	stop     chan struct{}
	maxTimer *time.Ticker
	minTimer *time.Ticker
}

func NewSurfaceFlinger(maxFrameInterval, minFrameInterval time.Duration) *SurfaceFlinger {
	return &SurfaceFlinger{
		VSync:    make(chan struct{}),
		counter:  0,
		stop:     nil,
		maxTimer: time.NewTicker(maxFrameInterval),
		minTimer: time.NewTicker(minFrameInterval),
	}
}

func (f *SurfaceFlinger) TickOnce() {
	atomic.AddInt64(&f.counter, 1)
	glog.Debug("SurfaceFlinger tick")
}

func (f *SurfaceFlinger) Run() {
	if f.stop != nil {
		return
	}
	f.stop = make(chan struct{})

	go func() {
		var size int64 = 0
		for {
			select {
			case <-f.maxTimer.C:
				glog.Debug("SurfaceFlinger maxTimer")
				randomNum := rand.Intn(100)
				if randomNum < 20 {
					continue
				}
				f.VSync <- struct{}{}
			case <-f.minTimer.C:
				glog.Debug("SurfaceFlinger minTimer")
				size = atomic.LoadInt64(&f.counter)
				if size != 0 {
					f.VSync <- struct{}{}
					atomic.StoreInt64(&f.counter, 0)
				}
			case <-f.stop:
				glog.Debug("SurfaceFlinger stop")
				return
			}
		}
	}()
}

func (f *SurfaceFlinger) Stop() {
	f.stop <- struct{}{}
	close(f.stop)
	close(f.VSync)
	f.maxTimer.Stop()
	f.minTimer.Stop()
}
