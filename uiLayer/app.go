package uiLayer

import (
	"FuckRDP/common"
	"FuckRDP/grdp/glog"
	"fmt"
	"github.com/zserge/lorca"
	"io/fs"
	"net"
	"net/http"
)

type AppUI struct {
	ui             lorca.UI
	webLn          net.Listener
	mux            *http.ServeMux
	surfaceFlinger *common.SurfaceFlinger
}

func NewUI(wwwFs fs.FS, surfaceFlinger *common.SurfaceFlinger, width, height int) (*AppUI, error) {
	ui, err := lorca.New("", "", width, height, "--remote-allow-origins=*")
	if err != nil {
		return nil, err
	}
	mux := http.NewServeMux()
	ln, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		_ = ui.Close()
		return nil, err
	}
	fileServer := http.FileServer(http.FS(wwwFs))
	mux.Handle("/www/", fileServer)

	return &AppUI{
		webLn:          ln,
		ui:             ui,
		mux:            mux,
		surfaceFlinger: surfaceFlinger,
	}, nil
}
func (app *AppUI) Bind(name string, f interface{}) {
	if err := app.ui.Bind(name, f); err != nil {
		glog.Warnf("Bind %s failed:%v", name, err)
	}
}
func (app *AppUI) Show() {
	go http.Serve(app.webLn, app.mux)
	go func(app *AppUI) {
		app.surfaceFlinger.Run()
		for range app.surfaceFlinger.VSync {
			app.Eval(`drawImageOnCanvas('/desktop.jpeg');`)
		}
	}(app)
	err := app.ui.Load(fmt.Sprintf("http://%s/www", app.webLn.Addr()))
	if err != nil {
		_ = app.ui.Close()
		glog.Errorf("ShowUI failed:%v", err)
	}
}
func (app *AppUI) Eval(js string) lorca.Value {
	return app.ui.Eval(js)
}
func (app *AppUI) Done() <-chan struct{} {
	return app.ui.Done()
}
func (app *AppUI) Close() {
	app.surfaceFlinger.Stop()
	_ = app.webLn.Close()
	_ = app.ui.Close()
}
func (app *AppUI) HandleFunc(pattern string, handler func(http.ResponseWriter, *http.Request)) {
	app.mux.HandleFunc(pattern, handler)
}
