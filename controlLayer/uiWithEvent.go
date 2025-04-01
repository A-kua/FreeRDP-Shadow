package controlLayer

import (
	"FuckRDP/dataLayer"
	"FuckRDP/dataLayer/cmdStream"
	"FuckRDP/dataLayer/controlStream"
	"FuckRDP/dataLayer/fileStream"
	"FuckRDP/uiLayer"
	"bytes"
	"fmt"
	"html"
)

var convert = func(input string) string {
	step1 := html.EscapeString(input)
	var buf bytes.Buffer
	for _, char := range step1 {
		if char == '\\' {
			buf.WriteString("\\\\")
		} else {
			buf.WriteRune(char)
		}
	}
	return buf.String()
}

var addNotificationFun = func(ui *uiLayer.AppUI, info string) {
	ui.Eval(fmt.Sprintf(`addNotification("%s");`, convert(info)))
}

func bindEvent(ui *uiLayer.AppUI) {
	dataLayer.AddDispatcher(fileStream.EVENT_FILETASK_JOIN, func(data1, data2 interface{}) {
		addNotificationFun(ui, fmt.Sprintf("%s-%s", data1, data2))
	})
	dataLayer.AddDispatcher(fileStream.EVENT_FILETASK_REMOVE, func(data1, data2 interface{}) {
		addNotificationFun(ui, fmt.Sprintf("%s-%s", data1, data2))
	})
	dataLayer.AddDispatcher(fileStream.EVENT_FILETASK_VERIFY, func(data1, data2 interface{}) {
		addNotificationFun(ui, fmt.Sprintf("%s-%s", data1, data2))
	})
	dataLayer.AddDispatcher(controlStream.EVENT_CONTROL_STATECHANGE, func(data1, data2 interface{}) {
		addNotificationFun(ui, fmt.Sprintf("%s-%s", data1, data2))
	})
	dataLayer.AddDispatcher(cmdStream.EVENT_CMD_RESET, func(data1, data2 interface{}) {
		addNotificationFun(ui, fmt.Sprintf("%s-%s", data1, data2))
	})
	dataLayer.AddDispatcher(cmdStream.EVENT_CMD_INIT, func(data1, data2 interface{}) {
		addNotificationFun(ui, fmt.Sprintf("%s-%s", data1, data2))
	})
}
