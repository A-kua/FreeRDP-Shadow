package main

import (
	"FuckRDP/common"
	"FuckRDP/controlLayer"
	"FuckRDP/dataLayer"
	"FuckRDP/grdp/glog"
	"FuckRDP/uiLayer"
	"embed"
	"log"
	"math/rand"
	"os"
	"os/signal"
	"time"
)

//go:embed www
var fs embed.FS

func init() {
	_logger := log.New(os.Stdout, "", 0)

	glog.SetLogger(_logger)
	glog.SetLevel(glog.INFO)
	rand.Seed(time.Now().UnixNano())
}

func main() {
	client := controlLayer.NewClient("192.168.232.5:3388", "", "", "")
	flinger := common.NewSurfaceFlinger(1*time.Second, 17*time.Millisecond)
	ui, err := uiLayer.NewUI(fs, flinger, 2000, 1100)
	if err != nil {
		glog.Errorf("NewUI: ", err)
		os.Exit(-1)
	}
	controlLayer.BindUIClient(ui, client, flinger)

	ui.Show()

	sign := make(chan os.Signal)
	signal.Notify(sign, os.Interrupt)
	select {
	case <-sign:
	case <-(*ui).Done():
	}
	dataLayer.LooperStop()
}
