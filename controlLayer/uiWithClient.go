package controlLayer

import (
	"FuckRDP/common"
	"FuckRDP/dataLayer"
	"FuckRDP/dataLayer/controlStream"
	"FuckRDP/dataLayer/fileStream"
	"FuckRDP/grdp/glog"
	"FuckRDP/grdp/protocol/pdu"
	"FuckRDP/uiLayer"
	"image/jpeg"
	"net/http"
)

func BindUIClient(ui *uiLayer.AppUI, client *Client, flinger *common.SurfaceFlinger) {
	bindEvent(ui)
	ui.HandleFunc("/desktop.jpeg", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Cache-Control", "no-cache, no-store, must-revalidate")
		w.Header().Set("Pragma", "no-cache")
		w.Header().Set("Expires", "0")
		w.Header().Set("Content-Type", "image/jpeg")
		bitmap := dataLayer.GetBitmap()
		err := jpeg.Encode(w, bitmap, &jpeg.Options{Quality: 80})
		//err := png.Encode(w, dataLayer.GetBitmap())
		if err != nil {
			return
		}
	})
	ui.Bind("onStart", func() {
		glog.Info("onStart")
		login(client, flinger)
	})
	ui.Bind("mouseMove", func(x, y int) {
		event := pdu.NewAkuaMouseEvent(pdu.PTRFLAGS_MOVE, x, y)
		dataLayer.HighPriorityInput(event)
	})
	ui.Bind("mouseDown", func(button, x, y int) {
		var flags = pdu.PTRFLAGS_DOWN
		switch button {
		case 0:
			flags |= pdu.PTRFLAGS_BUTTON1
		case 2:
			flags |= pdu.PTRFLAGS_BUTTON2
		case 1:
			flags |= pdu.PTRFLAGS_BUTTON3
		default:
			flags |= pdu.PTRFLAGS_MOVE
		}
		event := pdu.NewAkuaMouseEvent(flags, x, y)
		dataLayer.HighPriorityInput(event)
	})
	ui.Bind("mouseUp", func(button, x, y int) {
		var flags = 0
		switch button {
		case 0:
			flags |= pdu.PTRFLAGS_BUTTON1
		case 2:
			flags |= pdu.PTRFLAGS_BUTTON2
		case 1:
			flags |= pdu.PTRFLAGS_BUTTON3
		default:
			flags |= pdu.PTRFLAGS_MOVE
		}
		event := pdu.NewAkuaMouseEvent(flags, x, y)
		dataLayer.HighPriorityInput(event)
	})
	ui.Bind("keyboard", func(button int, isPressed bool) {
		event := pdu.NewAkuaKeyboardEvent(button, isPressed)
		dataLayer.HighPriorityInput(event)
	})
	ui.Bind("uploadFile", func(destinationPath, sourceFilePath string) {
		glog.Debug("uploadFile: ", destinationPath, sourceFilePath)
		task, err := fileStream.NewFileTask(sourceFilePath, destinationPath)
		if err != nil {
			glog.Warnf("UploadFile error: %s %s %v", destinationPath, sourceFilePath, err)
		}
		task.StartTransfer()
	})
	ui.Bind("setPortMap", func(portMap string) {
		glog.Info("setPortMap: ", portMap)
	})
	ui.Bind("command", func(cmdline string) {
		glog.Info("command: ", cmdline)
	})
	ui.Bind("openControl", func() {
		controlStream.SendEnableControl()
	})
	ui.Bind("closeControl", func() {
		controlStream.SendDisableControl()
	})
}
