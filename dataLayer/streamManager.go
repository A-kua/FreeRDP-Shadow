package dataLayer

import "FuckRDP/grdp/glog"

type streamEvent struct {
	name  string
	data1 interface{}
	data2 interface{}
}

var eventQueue = make(chan streamEvent, 4)
var eventDispatcher = make(map[string]func(data1, data2 interface{}))

func PublishEvent(name string, data1, data2 interface{}) {
	eventQueue <- streamEvent{
		name:  name,
		data1: data1,
		data2: data2,
	}
}

func AddDispatcher(name string, fun func(data1, data2 interface{})) {
	eventDispatcher[name] = fun
}

func dispatchEvent(event streamEvent) {
	if fun, has := eventDispatcher[event.name]; has {
		fun(event.data1, event.data2)
	} else {
		glog.Warnf("streamEvent %s ignored.", event.name)
	}
}
